#include "ui_manager.hpp"
#include "../../utils/logging.hpp"
#include <thread>
#include <iostream>

namespace trading::ui {

UIManager::UIManager()
    : running_(false), market_data_window_open_(true), order_entry_window_open_(true),
      positions_window_open_(true), trades_window_open_(true), status_window_open_(true) {}

UIManager::~UIManager() {
    shutdown();
}

bool UIManager::initialize() {
    try {
        // Initialize OpenGL context
        if (!opengl_context_.initialize(1920, 1080, "Trading System")) {
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
        setup_panel_callbacks();

        TRADING_LOG_INFO("UI Manager initialized successfully");
        return true;
    } catch (const std::exception& e) {
        TRADING_LOG_ERROR("Failed to initialize UI Manager: {}", e.what());
        return false;
    }
}

void UIManager::run() {
    if (!opengl_context_.is_valid()) {
        TRADING_LOG_ERROR("Cannot run UI: OpenGL context not initialized");
        return;
    }

    running_ = true;
    TRADING_LOG_INFO("Starting UI main loop");

    // Main render loop
    while (running_ && !opengl_context_.should_close()) {
        // Poll events
        opengl_context_.poll_events();

        // Start ImGui frame
        opengl_context_.new_frame();

        // Render UI panels
        render_panels();

        // End frame and swap buffers
        opengl_context_.render();
    }

    TRADING_LOG_INFO("UI main loop ended");
}

void UIManager::shutdown() {
    if (running_) {
        running_ = false;
        TRADING_LOG_INFO("Shutting down UI Manager");
    }

    // Cleanup panels
    market_data_panel_.reset();
    order_entry_panel_.reset();
    positions_panel_.reset();
    trades_panel_.reset();
    status_panel_.reset();

    // Cleanup OpenGL context
    opengl_context_.cleanup();
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
    connection_status_.market_data_connected = connected;

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
    create_dock_space();

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

void UIManager::create_dock_space() {
    // Create a full-screen window for docking
    ImGuiWindowFlags window_flags = ImGuiWindowFlags_MenuBar | ImGuiWindowFlags_NoDocking;
    const ImGuiViewport* viewport = ImGui::GetMainViewport();
    ImGui::SetNextWindowPos(viewport->Pos);
    ImGui::SetNextWindowSize(viewport->Size);
    ImGui::SetNextWindowViewport(viewport->ID);

    window_flags |= ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoCollapse;
    window_flags |= ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoMove;
    window_flags |= ImGuiWindowFlags_NoBringToFrontOnFocus | ImGuiWindowFlags_NoNavFocus;

    ImGui::PushStyleVar(ImGuiStyleVar_WindowRounding, 0.0f);
    ImGui::PushStyleVar(ImGuiStyleVar_WindowBorderSize, 0.0f);
    ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(0.0f, 0.0f));

    ImGui::Begin("DockSpace", nullptr, window_flags);
    ImGui::PopStyleVar(3);

    // Create dock space
    ImGuiID dockspace_id = ImGui::GetID("MainDockSpace");
    ImGui::DockSpace(dockspace_id, ImVec2(0.0f, 0.0f), ImGuiDockNodeFlags_None);

    // Menu bar
    if (ImGui::BeginMenuBar()) {
        if (ImGui::BeginMenu("View")) {
            ImGui::MenuItem("Market Data", nullptr, &market_data_window_open_);
            ImGui::MenuItem("Order Entry", nullptr, &order_entry_window_open_);
            ImGui::MenuItem("Positions", nullptr, &positions_window_open_);
            ImGui::MenuItem("Trades", nullptr, &trades_window_open_);
            ImGui::MenuItem("Status", nullptr, &status_window_open_);
            ImGui::EndMenu();
        }
        ImGui::EndMenuBar();
    }

    ImGui::End();
}

void UIManager::setup_panel_callbacks() {
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

} // namespace trading::ui