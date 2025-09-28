#include "ui_manager.hpp"
#include "../../utils/logging.hpp"
#include <thread>
#include <iostream>

namespace trading::ui {

UIManager::UIManager()
    : config_({}), is_running_(false), is_initialized_(false), should_close_(false),
      connection_status_(false), show_demo_window_(false), show_metrics_window_(false),
      dockspace_id_(0) {}

UIManager::UIManager(const UIManagerConfig& config)
    : config_(config), is_running_(false), is_initialized_(false), should_close_(false),
      connection_status_(false), show_demo_window_(false), show_metrics_window_(false),
      dockspace_id_(0) {}

UIManager::~UIManager() {
    shutdown();
}

bool UIManager::initialize() {
    try {
        // Initialize OpenGL context
        gl_context_ = std::make_unique<OpenGLContext>(config_.window_config, config_.imgui_config);
        if (!gl_context_->initialize()) {
            TRADING_LOG_ERROR("Failed to initialize OpenGL context");
            return false;
        }

        // Initialize UI panels
        market_data_panel_ = std::make_unique<MarketDataPanel>();
        order_entry_panel_ = std::make_unique<OrderEntryPanel>();
        positions_panel_ = std::make_unique<PositionsPanel>();
        trades_panel_ = std::make_unique<TradesPanel>();
        status_panel_ = std::make_unique<StatusPanel>();

        // Setup panel callbacks
        setup_callbacks();

        is_initialized_ = true;
        TRADING_LOG_INFO("UI Manager initialized successfully");
        return true;
    } catch (const std::exception& e) {
        TRADING_LOG_ERROR("Failed to initialize UI Manager: {}", e.what());
        return false;
    }
}

void UIManager::run() {
    if (!gl_context_ || !is_initialized_) {
        TRADING_LOG_ERROR("Cannot run UI: OpenGL context not initialized");
        return;
    }

    is_running_ = true;
    TRADING_LOG_INFO("Starting UI main loop");

    // Main render loop
    while (is_running_ && !should_close_) {
        // Poll events
        gl_context_->poll_events();

        // Start ImGui frame
        gl_context_->begin_frame();

        // Render UI panels
        render_panels();

        // End frame and swap buffers
        gl_context_->end_frame();
    }

    TRADING_LOG_INFO("UI main loop ended");
}

void UIManager::shutdown() {
    if (is_running_) {
        is_running_ = false;
        should_close_ = true;
        TRADING_LOG_INFO("Shutting down UI Manager");
    }

    // Cleanup panels
    market_data_panel_.reset();
    order_entry_panel_.reset();
    positions_panel_.reset();
    trades_panel_.reset();
    status_panel_.reset();

    // Cleanup OpenGL context
    if (gl_context_) {
        gl_context_->shutdown();
        gl_context_.reset();
    }

    is_initialized_ = false;
}

void UIManager::show_market_data_window(bool show) {
    market_data_window_open_ = show;
}

void UIManager::show_order_entry_window(bool show) {
    order_entry_window_open_ = show;
}

void UIManager::show_positions_window(bool show) {
    positions_window_open_ = show;
}

void UIManager::show_trades_window(bool show) {
    trades_window_open_ = show;
}

void UIManager::show_status_window(bool show) {
    status_window_open_ = show;
}

void UIManager::update_market_data(const std::vector<MarketDataRow>& data) {
    std::lock_guard<std::mutex> lock(data_mutex_);
    market_data_cache_ = data;

    if (market_data_panel_) {
        market_data_panel_->update_data(data);
    }
}

void UIManager::update_orders(const std::vector<OrderRow>& orders) {
    std::lock_guard<std::mutex> lock(data_mutex_);
    orders_cache_ = orders;
    // Orders are typically displayed in multiple panels, so no direct update here
}

void UIManager::update_positions(const std::vector<PositionRow>& positions) {
    std::lock_guard<std::mutex> lock(data_mutex_);
    positions_cache_ = positions;

    if (positions_panel_) {
        positions_panel_->update_data(positions);
    }
}

void UIManager::update_trades(const std::vector<TradeRow>& trades) {
    std::lock_guard<std::mutex> lock(data_mutex_);
    trades_cache_ = trades;

    if (trades_panel_) {
        trades_panel_->update_data(trades);
    }
}

void UIManager::update_connection_status(bool connected, const std::string& status) {
    std::lock_guard<std::mutex> lock(data_mutex_);
    connection_status_ = connected;
    connection_status_text_ = status;

    if (status_panel_) {
        status_panel_->update_connection_status(connected, status);
    }
}

void UIManager::set_order_submit_callback(std::function<void(const OrderFormData&)> callback) {
    order_submit_callback_ = callback;
}

void UIManager::set_order_cancel_callback(std::function<void(const std::string&)> callback) {
    order_cancel_callback_ = callback;
}

void UIManager::set_symbol_subscribe_callback(std::function<void(const std::string&)> callback) {
    symbol_subscribe_callback_ = callback;
}

void UIManager::set_symbol_unsubscribe_callback(std::function<void(const std::string&)> callback) {
    symbol_unsubscribe_callback_ = callback;
}

void UIManager::render_panels() {
    // Create main dock space
    render_dockspace();

    // Render individual panels if they're open
    if (market_data_window_open_ && market_data_panel_) {
        if (ImGui::Begin("Market Data", &market_data_window_open_)) {
            market_data_panel_->render();
        }
        ImGui::End();
    }

    if (order_entry_window_open_ && order_entry_panel_) {
        if (ImGui::Begin("Order Entry", &order_entry_window_open_)) {
            order_entry_panel_->render();
        }
        ImGui::End();
    }

    if (positions_window_open_ && positions_panel_) {
        if (ImGui::Begin("Positions", &positions_window_open_)) {
            positions_panel_->render();
        }
        ImGui::End();
    }

    if (trades_window_open_ && trades_panel_) {
        if (ImGui::Begin("Trades", &trades_window_open_)) {
            trades_panel_->render();
        }
        ImGui::End();
    }

    if (status_window_open_ && status_panel_) {
        if (ImGui::Begin("Status", &status_window_open_)) {
            status_panel_->render();
        }
        ImGui::End();
    }
}

void UIManager::render_dockspace() {
    // Simple menu bar without docking
    if (ImGui::BeginMainMenuBar()) {
        if (ImGui::BeginMenu("View")) {
            ImGui::MenuItem("Market Data", nullptr, &market_data_window_open_);
            ImGui::MenuItem("Order Entry", nullptr, &order_entry_window_open_);
            ImGui::MenuItem("Positions", nullptr, &positions_window_open_);
            ImGui::MenuItem("Trades", nullptr, &trades_window_open_);
            ImGui::MenuItem("Status", nullptr, &status_window_open_);
            ImGui::EndMenu();
        }
        if (ImGui::BeginMenu("Help")) {
            ImGui::MenuItem("About", nullptr, nullptr);
            ImGui::EndMenu();
        }
        ImGui::EndMainMenuBar();
    }
}

void UIManager::setup_callbacks() {
    // Order Entry Panel callbacks
    if (order_entry_panel_) {
        order_entry_panel_->set_submit_callback([this](const OrderFormData& form_data) {
            if (order_submit_callback_) {
                order_submit_callback_(form_data);
            }
        });
    }

    // Market Data Panel callbacks
    if (market_data_panel_) {
        market_data_panel_->set_subscribe_callback([this](const std::string& symbol) {
            if (symbol_subscribe_callback_) {
                symbol_subscribe_callback_(symbol);
            }
        });
    }

    // Positions Panel callbacks
    if (positions_panel_) {
        positions_panel_->set_close_position_callback([this](const std::string& symbol) {
            // Create a market sell order to close the position
            OrderFormData close_order;
            close_order.symbol = symbol;
            close_order.side = "SELL";
            close_order.type = "MARKET";

            // Get position quantity (would need access to position data)
            std::lock_guard<std::mutex> lock(data_mutex_);
            for (const auto& position : positions_cache_) {
                if (position.symbol == symbol) {
                    close_order.quantity = std::abs(position.quantity);
                    break;
                }
            }

            if (order_submit_callback_ && close_order.quantity > 0) {
                order_submit_callback_(close_order);
            }
        });
    }
}

// Additional public interface methods
void UIManager::set_window_title(const std::string& title) {
    if (gl_context_) {
        gl_context_->set_window_title(title);
    }
}

void UIManager::load_layout(const std::string& layout_file) {
    // TODO: Implement layout loading
    (void)layout_file; // Suppress unused parameter warning
}

void UIManager::save_layout(const std::string& layout_file) {
    // TODO: Implement layout saving
    (void)layout_file; // Suppress unused parameter warning
}

void UIManager::set_config(const UIManagerConfig& config) {
    config_ = config;
}

const UIManager::UIManagerConfig& UIManager::get_config() const {
    return config_;
}

bool UIManager::is_running() const {
    return is_running_;
}

bool UIManager::is_initialized() const {
    return is_initialized_;
}

OpenGLContext::PerformanceStats UIManager::get_performance_stats() const {
    if (gl_context_) {
        return gl_context_->get_performance_stats();
    }
    return {};
}

} // namespace trading::ui