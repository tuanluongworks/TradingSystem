#include "trading_engine.hpp"
#include "../models/order.hpp"
#include "../models/trade.hpp"
#include "../models/position.hpp"
#include "../models/instrument.hpp"
#include "../../utils/logging.hpp"
#include "../../utils/exceptions.hpp"

#include <sstream>
#include <algorithm>
#include <iomanip>
#include <random>

namespace trading {

TradingEngine::TradingEngine(
    std::shared_ptr<RiskManager> risk_manager,
    std::shared_ptr<SQLiteService> persistence_service
) : risk_manager_(std::move(risk_manager)),
    persistence_service_(std::move(persistence_service)),
    is_running_(false),
    should_stop_(false),
    order_sequence_(0),
    trade_sequence_(0) {

    if (!risk_manager_) {
        throw TradingException("Risk manager is required");
    }

    log_engine_event("Trading engine initialized");
}

TradingEngine::~TradingEngine() {
    shutdown();
}

bool TradingEngine::initialize() {
    std::lock_guard<std::mutex> lock(engine_mutex_);

    if (is_running_.load()) {
        log_engine_event("Engine already running");
        return true;
    }

    try {
        // Load existing positions from persistence if available
        if (persistence_service_) {
            auto saved_positions = persistence_service_->load_all_positions();
            for (const auto& position : saved_positions) {
                positions_[position->instrument_symbol] = position;
            }
            log_engine_event("Loaded " + std::to_string(saved_positions.size()) + " positions from persistence");
        }

        // Start order processing thread
        should_stop_.store(false);
        order_processing_thread_ = std::thread(&TradingEngine::process_orders, this);

        is_running_.store(true);
        log_engine_event("Trading engine started successfully");
        return true;

    } catch (const std::exception& e) {
        log_engine_event("Failed to initialize trading engine: " + std::string(e.what()));
        return false;
    }
}

void TradingEngine::shutdown() {
    if (!is_running_.load()) {
        return;
    }

    log_engine_event("Shutting down trading engine");

    should_stop_.store(true);

    // Signal order processing thread to stop
    order_processing_queue_.push([]{});  // Wake up the thread

    if (order_processing_thread_.joinable()) {
        order_processing_thread_.join();
    }

    is_running_.store(false);
    log_engine_event("Trading engine shutdown complete");
}

std::string TradingEngine::submit_order(const OrderRequest& request) {
    if (!is_running_.load()) {
        throw TradingException("Trading engine is not running");
    }

    if (!validate_order_request(request)) {
        throw TradingException("Invalid order request");
    }

    // Create order
    auto order = create_order(request);
    if (!order) {
        throw TradingException("Failed to create order");
    }

    // Risk validation
    if (!risk_manager_->validate_order(request)) {
        std::string reason = risk_manager_->get_rejection_reason(request);
        reject_order(order, reason);
        return order->order_id;
    }

    // Accept order and add to processing queue
    accept_order(order);

    // Queue order for processing
    std::string order_id = order->order_id;
    order_processing_queue_.push([this, order_id]() {
        auto order_iter = orders_.find(order_id);
        if (order_iter != orders_.end()) {
            auto order = order_iter->second;
            if (order->type == OrderType::MARKET) {
                execute_market_order(order);
            } else {
                execute_limit_order(order);
            }
        }
    });

    return order->order_id;
}

bool TradingEngine::cancel_order(const std::string& order_id) {
    std::lock_guard<std::mutex> lock(engine_mutex_);

    auto it = orders_.find(order_id);
    if (it == orders_.end()) {
        return false;
    }

    auto order = it->second;

    // Check if order can be canceled
    if (order->status == OrderStatus::FILLED ||
        order->status == OrderStatus::CANCELED ||
        order->status == OrderStatus::REJECTED) {
        return false;
    }

    // Update order status
    OrderStatus old_status = order->status;
    order->status = OrderStatus::CANCELED;
    order->last_modified = std::chrono::system_clock::now();

    persist_order(order);
    notify_order_update(order, old_status);

    log_order_event("Order canceled", order);
    return true;
}

bool TradingEngine::modify_order(const std::string& order_id, double new_quantity, double new_price) {
    std::lock_guard<std::mutex> lock(engine_mutex_);

    auto it = orders_.find(order_id);
    if (it == orders_.end()) {
        return false;
    }

    auto order = it->second;

    // Check if order can be modified
    if (order->status != OrderStatus::ACCEPTED && order->status != OrderStatus::PARTIALLY_FILLED) {
        return false;
    }

    // Validate new parameters
    if (new_quantity <= 0 || (order->type == OrderType::LIMIT && new_price <= 0)) {
        return false;
    }

    // Update order
    OrderStatus old_status = order->status;
    order->quantity = new_quantity;
    order->remaining_quantity = new_quantity - order->filled_quantity;

    if (order->type == OrderType::LIMIT) {
        order->price = new_price;
    }

    order->last_modified = std::chrono::system_clock::now();

    persist_order(order);
    notify_order_update(order, old_status);

    log_order_event("Order modified", order);
    return true;
}

std::shared_ptr<Order> TradingEngine::get_order(const std::string& order_id) const {
    std::lock_guard<std::mutex> lock(engine_mutex_);
    auto it = orders_.find(order_id);
    return (it != orders_.end()) ? it->second : nullptr;
}

std::vector<std::shared_ptr<Order>> TradingEngine::get_working_orders() const {
    std::lock_guard<std::mutex> lock(engine_mutex_);
    std::vector<std::shared_ptr<Order>> working_orders;

    for (const auto& [order_id, order] : orders_) {
        if (OrderManager::is_working_status(order->status)) {
            working_orders.push_back(order);
        }
    }

    return working_orders;
}

std::vector<std::shared_ptr<Order>> TradingEngine::get_orders_by_symbol(const std::string& symbol) const {
    std::lock_guard<std::mutex> lock(engine_mutex_);
    std::vector<std::shared_ptr<Order>> symbol_orders;

    auto it = orders_by_symbol_.find(symbol);
    if (it != orders_by_symbol_.end()) {
        for (const auto& order_id : it->second) {
            auto order_it = orders_.find(order_id);
            if (order_it != orders_.end()) {
                symbol_orders.push_back(order_it->second);
            }
        }
    }

    return symbol_orders;
}

std::shared_ptr<Position> TradingEngine::get_position(const std::string& symbol) const {
    std::lock_guard<std::mutex> lock(engine_mutex_);
    auto it = positions_.find(symbol);
    return (it != positions_.end()) ? it->second : nullptr;
}

std::vector<std::shared_ptr<Position>> TradingEngine::get_all_positions() const {
    std::lock_guard<std::mutex> lock(engine_mutex_);
    std::vector<std::shared_ptr<Position>> all_positions;

    for (const auto& [symbol, position] : positions_) {
        if (!position->is_flat()) {
            all_positions.push_back(position);
        }
    }

    return all_positions;
}

std::vector<std::shared_ptr<Trade>> TradingEngine::get_trades_by_order(const std::string& order_id) const {
    std::lock_guard<std::mutex> lock(engine_mutex_);
    auto it = trades_by_order_.find(order_id);
    return (it != trades_by_order_.end()) ? it->second : std::vector<std::shared_ptr<Trade>>();
}

std::vector<std::shared_ptr<Trade>> TradingEngine::get_trades_by_symbol(const std::string& symbol) const {
    std::lock_guard<std::mutex> lock(engine_mutex_);
    auto it = trades_by_symbol_.find(symbol);
    return (it != trades_by_symbol_.end()) ? it->second : std::vector<std::shared_ptr<Trade>>();
}

std::vector<std::shared_ptr<Trade>> TradingEngine::get_daily_trades() const {
    std::lock_guard<std::mutex> lock(engine_mutex_);

    auto now = std::chrono::system_clock::now();
    auto today_start = std::chrono::time_point_cast<std::chrono::days>(now);

    std::vector<std::shared_ptr<Trade>> daily_trades;
    for (const auto& trade : trades_) {
        if (trade->execution_time >= today_start) {
            daily_trades.push_back(trade);
        }
    }

    return daily_trades;
}

void TradingEngine::set_order_update_callback(std::function<void(const ExecutionReport&)> callback) {
    order_update_callback_ = std::move(callback);
}

void TradingEngine::set_trade_callback(std::function<void(const Trade&)> callback) {
    trade_callback_ = std::move(callback);
}

void TradingEngine::set_position_update_callback(std::function<void(const Position&)> callback) {
    position_update_callback_ = std::move(callback);
}

void TradingEngine::set_market_data_provider(std::shared_ptr<IMarketDataProvider> provider) {
    market_data_provider_ = std::move(provider);
}

// Helper methods implementation

std::string TradingEngine::generate_order_id() {
    size_t seq = order_sequence_.fetch_add(1);
    auto now = std::chrono::system_clock::now();
    auto timestamp = std::chrono::duration_cast<std::chrono::milliseconds>(now.time_since_epoch()).count();

    std::ostringstream oss;
    oss << "ORD" << std::setfill('0') << std::setw(8) << seq << "_" << timestamp;
    return oss.str();
}

std::string TradingEngine::generate_trade_id() {
    size_t seq = trade_sequence_.fetch_add(1);
    auto now = std::chrono::system_clock::now();
    auto timestamp = std::chrono::duration_cast<std::chrono::milliseconds>(now.time_since_epoch()).count();

    std::ostringstream oss;
    oss << "TRD" << std::setfill('0') << std::setw(8) << seq << "_" << timestamp;
    return oss.str();
}

bool TradingEngine::validate_order_request(const OrderRequest& request) const {
    return request.is_valid() &&
           request.quantity > 0 &&
           (request.type == OrderType::MARKET || request.price > 0) &&
           !request.instrument_symbol.empty();
}

std::shared_ptr<Order> TradingEngine::create_order(const OrderRequest& request) {
    auto order = std::make_shared<Order>();

    order->order_id = generate_order_id();
    order->instrument_symbol = request.instrument_symbol;
    order->side = request.side;
    order->type = request.type;
    order->quantity = request.quantity;
    order->price = request.price;
    order->status = OrderStatus::NEW;
    order->filled_quantity = 0.0;
    order->remaining_quantity = request.quantity;
    order->created_time = std::chrono::system_clock::now();
    order->last_modified = order->created_time;

    return order;
}

bool TradingEngine::accept_order(std::shared_ptr<Order> order) {
    std::lock_guard<std::mutex> lock(engine_mutex_);

    OrderStatus old_status = order->status;
    order->status = OrderStatus::ACCEPTED;
    order->last_modified = std::chrono::system_clock::now();

    // Store order
    orders_[order->order_id] = order;
    add_order_to_symbol_index(order->instrument_symbol, order->order_id);

    persist_order(order);
    notify_order_update(order, old_status);

    log_order_event("Order accepted", order);
    return true;
}

bool TradingEngine::reject_order(std::shared_ptr<Order> order, const std::string& reason) {
    std::lock_guard<std::mutex> lock(engine_mutex_);

    OrderStatus old_status = order->status;
    order->status = OrderStatus::REJECTED;
    order->rejection_reason = reason;
    order->last_modified = std::chrono::system_clock::now();

    orders_[order->order_id] = order;

    persist_order(order);
    notify_order_update(order, old_status);

    log_order_event("Order rejected: " + reason, order);
    return true;
}

void TradingEngine::execute_market_order(std::shared_ptr<Order> order) {
    double market_price = get_market_price(order->instrument_symbol, order->type);
    if (market_price <= 0) {
        reject_order(order, "No market price available");
        return;
    }

    // For simulation, execute immediately at market price
    execute_order(order->order_id, order->remaining_quantity, market_price);
}

void TradingEngine::execute_limit_order(std::shared_ptr<Order> order) {
    double market_price = get_market_price(order->instrument_symbol, order->type);
    if (market_price <= 0) {
        return; // Keep order working, wait for market data
    }

    if (can_execute_order(order, market_price)) {
        // Execute at limit price for favorable execution
        double execution_price = order->price;
        execute_order(order->order_id, order->remaining_quantity, execution_price);
    }
}

bool TradingEngine::can_execute_order(std::shared_ptr<Order> order, double market_price) const {
    if (order->side == OrderSide::BUY) {
        return market_price <= order->price; // Buy when market is at or below limit
    } else {
        return market_price >= order->price; // Sell when market is at or above limit
    }
}

bool TradingEngine::execute_order(const std::string& order_id, double quantity, double price) {
    std::lock_guard<std::mutex> lock(engine_mutex_);

    auto it = orders_.find(order_id);
    if (it == orders_.end()) {
        return false;
    }

    auto order = it->second;

    if (quantity <= 0 || quantity > order->remaining_quantity) {
        return false;
    }

    // Determine trade type
    TradeType trade_type = (quantity == order->remaining_quantity) ?
        TradeType::FULL_FILL : TradeType::PARTIAL_FILL;

    // Create trade
    auto trade = create_trade(order, quantity, price, trade_type);

    // Update order status
    OrderStatus old_status = order->status;
    order->filled_quantity += quantity;
    order->remaining_quantity -= quantity;

    if (order->remaining_quantity == 0) {
        order->status = OrderStatus::FILLED;
    } else {
        order->status = OrderStatus::PARTIALLY_FILLED;
    }

    order->last_modified = std::chrono::system_clock::now();

    // Process the trade
    process_trade(trade);

    // Notify about order update
    notify_order_update(order, old_status);

    log_trade_event("Order executed", trade);
    return true;
}

std::shared_ptr<Trade> TradingEngine::create_trade(
    std::shared_ptr<Order> order,
    double quantity,
    double price,
    TradeType type
) {
    auto trade = std::make_shared<Trade>();

    trade->trade_id = generate_trade_id();
    trade->order_id = order->order_id;
    trade->instrument_symbol = order->instrument_symbol;
    trade->side = order->side;
    trade->quantity = quantity;
    trade->price = price;
    trade->execution_time = std::chrono::system_clock::now();
    trade->type = type;

    return trade;
}

void TradingEngine::process_trade(std::shared_ptr<Trade> trade) {
    // Store trade
    trades_.push_back(trade);
    trades_by_order_[trade->order_id].push_back(trade);
    trades_by_symbol_[trade->instrument_symbol].push_back(trade);

    // Update position
    update_position(trade);

    // Persist
    persist_trade(trade);

    // Notify
    notify_trade(*trade);
}

void TradingEngine::update_position(std::shared_ptr<Trade> trade) {
    auto position = get_or_create_position(trade->instrument_symbol);
    PositionCalculator::update_position_with_trade(*position, *trade);

    persist_position(position);
    notify_position_update(position);
}

std::shared_ptr<Position> TradingEngine::get_or_create_position(const std::string& symbol) {
    auto it = positions_.find(symbol);
    if (it != positions_.end()) {
        return it->second;
    }

    // Create new position
    auto position = std::make_shared<Position>();
    position->instrument_symbol = symbol;
    position->quantity = 0.0;
    position->average_price = 0.0;
    position->realized_pnl = 0.0;
    position->unrealized_pnl = 0.0;
    position->last_updated = std::chrono::system_clock::now();

    positions_[symbol] = position;
    return position;
}

double TradingEngine::get_market_price(const std::string& symbol, OrderType order_type) const {
    if (!market_data_provider_) {
        // For simulation, return a random price around 100
        static std::random_device rd;
        static std::mt19937 gen(rd());
        static std::uniform_real_distribution<> dis(95.0, 105.0);
        return dis(gen);
    }

    auto tick = market_data_provider_->get_latest_tick(symbol);
    if (!tick) {
        return 0.0;
    }

    // Return appropriate price based on order type
    if (order_type == OrderType::MARKET) {
        return tick->get_mid_price();
    } else {
        return tick->last_price;
    }
}

void TradingEngine::process_orders() {
    while (!should_stop_.load()) {
        try {
            std::function<void()> task;
            if (order_processing_queue_.try_pop(task, std::chrono::milliseconds(100))) {
                if (task) {
                    task();
                }
            }
        } catch (const std::exception& e) {
            log_engine_event("Error in order processing: " + std::string(e.what()));
        }
    }
}

// Notification methods
void TradingEngine::notify_order_update(std::shared_ptr<Order> order, OrderStatus old_status) {
    if (order_update_callback_) {
        ExecutionReport report;
        report.order_id = order->order_id;
        report.old_status = old_status;
        report.new_status = order->status;
        report.filled_quantity = order->filled_quantity;
        report.remaining_quantity = order->remaining_quantity;
        report.timestamp = order->last_modified;
        report.rejection_reason = order->rejection_reason;

        order_update_callback_(report);
    }
}

void TradingEngine::notify_trade(std::shared_ptr<Trade> trade) {
    if (trade_callback_) {
        trade_callback_(*trade);
    }
}

void TradingEngine::notify_position_update(std::shared_ptr<Position> position) {
    if (position_update_callback_) {
        position_update_callback_(*position);
    }
}

// Persistence methods
void TradingEngine::persist_order(std::shared_ptr<Order> order) {
    if (persistence_service_) {
        try {
            persistence_service_->save_order(*order);
        } catch (const std::exception& e) {
            log_engine_event("Failed to persist order: " + std::string(e.what()));
        }
    }
}

void TradingEngine::persist_trade(std::shared_ptr<Trade> trade) {
    if (persistence_service_) {
        try {
            persistence_service_->save_trade(*trade);
        } catch (const std::exception& e) {
            log_engine_event("Failed to persist trade: " + std::string(e.what()));
        }
    }
}

void TradingEngine::persist_position(std::shared_ptr<Position> position) {
    if (persistence_service_) {
        try {
            persistence_service_->update_position(*position);
        } catch (const std::exception& e) {
            log_engine_event("Failed to persist position: " + std::string(e.what()));
        }
    }
}

// Utility methods
void TradingEngine::add_order_to_symbol_index(const std::string& symbol, const std::string& order_id) {
    orders_by_symbol_[symbol].push_back(order_id);
}

void TradingEngine::remove_order_from_symbol_index(const std::string& symbol, const std::string& order_id) {
    auto& order_list = orders_by_symbol_[symbol];
    order_list.erase(std::remove(order_list.begin(), order_list.end(), order_id), order_list.end());
}

// Logging methods
void TradingEngine::log_order_event(const std::string& event, std::shared_ptr<Order> order) const {
    Logger::info("TradingEngine", event + " - Order ID: " + order->order_id +
                 ", Symbol: " + order->instrument_symbol +
                 ", Status: " + order->get_status_string());
}

void TradingEngine::log_trade_event(const std::string& event, std::shared_ptr<Trade> trade) const {
    Logger::info("TradingEngine", event + " - Trade ID: " + trade->trade_id +
                 ", Order ID: " + trade->order_id +
                 ", Symbol: " + trade->instrument_symbol +
                 ", Quantity: " + std::to_string(trade->quantity) +
                 ", Price: " + std::to_string(trade->price));
}

void TradingEngine::log_engine_event(const std::string& event) const {
    Logger::info("TradingEngine", event);
}

// Statistics methods
size_t TradingEngine::get_order_count() const {
    std::lock_guard<std::mutex> lock(engine_mutex_);
    return orders_.size();
}

size_t TradingEngine::get_trade_count() const {
    std::lock_guard<std::mutex> lock(engine_mutex_);
    return trades_.size();
}

size_t TradingEngine::get_position_count() const {
    std::lock_guard<std::mutex> lock(engine_mutex_);
    return positions_.size();
}

bool TradingEngine::is_running() const {
    return is_running_.load();
}

std::string TradingEngine::get_engine_status() const {
    std::ostringstream oss;
    oss << "Running: " << (is_running_.load() ? "Yes" : "No")
        << ", Orders: " << get_order_count()
        << ", Trades: " << get_trade_count()
        << ", Positions: " << get_position_count();
    return oss.str();
}

// OrderManager implementation
bool OrderManager::is_valid_status_transition(OrderStatus from, OrderStatus to) {
    switch (from) {
        case OrderStatus::NEW:
            return (to == OrderStatus::ACCEPTED || to == OrderStatus::REJECTED);
        case OrderStatus::ACCEPTED:
            return (to == OrderStatus::PARTIALLY_FILLED || to == OrderStatus::FILLED || to == OrderStatus::CANCELED);
        case OrderStatus::PARTIALLY_FILLED:
            return (to == OrderStatus::FILLED || to == OrderStatus::CANCELED);
        case OrderStatus::FILLED:
        case OrderStatus::CANCELED:
        case OrderStatus::REJECTED:
            return false; // Terminal states
        default:
            return false;
    }
}

std::string OrderManager::get_transition_error(OrderStatus from, OrderStatus to) {
    if (is_valid_status_transition(from, to)) {
        return "";
    }
    return "Invalid status transition from " + std::to_string(static_cast<int>(from)) +
           " to " + std::to_string(static_cast<int>(to));
}

bool OrderManager::is_working_status(OrderStatus status) {
    return (status == OrderStatus::ACCEPTED || status == OrderStatus::PARTIALLY_FILLED);
}

bool OrderManager::is_terminal_status(OrderStatus status) {
    return (status == OrderStatus::FILLED || status == OrderStatus::CANCELED || status == OrderStatus::REJECTED);
}

// PositionCalculator implementation
void PositionCalculator::update_position_with_trade(Position& position, const Trade& trade) {
    double trade_quantity = (trade.side == OrderSide::BUY) ? trade.quantity : -trade.quantity;

    if (position.quantity == 0) {
        // First trade - establish position
        position.quantity = trade_quantity;
        position.average_price = trade.price;
    } else if ((position.quantity > 0 && trade_quantity > 0) ||
               (position.quantity < 0 && trade_quantity < 0)) {
        // Adding to existing position
        double old_notional = position.quantity * position.average_price;
        double new_notional = trade_quantity * trade.price;
        double new_quantity = position.quantity + trade_quantity;

        if (new_quantity != 0) {
            position.average_price = (old_notional + new_notional) / new_quantity;
        }
        position.quantity = new_quantity;
    } else {
        // Reducing or reversing position
        double closing_quantity = std::min(std::abs(trade_quantity), std::abs(position.quantity));
        double realized_pnl = calculate_realized_pnl(position, trade);
        position.realized_pnl += realized_pnl;

        position.quantity += trade_quantity;

        if (position.quantity != 0 && std::signbit(position.quantity) != std::signbit(position.quantity - trade_quantity)) {
            // Position reversed - new average price
            position.average_price = trade.price;
        }
    }

    position.last_updated = trade.execution_time;
}

double PositionCalculator::calculate_unrealized_pnl(const Position& position, double current_price) {
    if (position.quantity == 0 || current_price <= 0) {
        return 0.0;
    }

    return position.quantity * (current_price - position.average_price);
}

double PositionCalculator::calculate_realized_pnl(const Position& position, const Trade& closing_trade) {
    if (position.quantity == 0) {
        return 0.0;
    }

    double closing_quantity = std::min(std::abs(closing_trade.quantity), std::abs(position.quantity));
    double pnl_per_share = (closing_trade.side == OrderSide::SELL) ?
        (closing_trade.price - position.average_price) :
        (position.average_price - closing_trade.price);

    return closing_quantity * pnl_per_share;
}

} // namespace trading