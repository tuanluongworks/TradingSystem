#pragma once

#include "../../contracts/ui_interface.hpp"
#include "../rendering/opengl_context.hpp"
#include "../components/market_data_panel.hpp"
#include "../components/order_entry_panel.hpp"
#include "../components/positions_panel.hpp"
#include "../components/trades_panel.hpp"
#include "../components/status_panel.hpp"

#include <memory>
#include <vector>
#include <mutex>
#include <atomic>
#include <chrono>

namespace trading::ui {

// Forward declarations
class MarketDataPanel;
class OrderEntryPanel;
class PositionsPanel;
class TradesPanel;
class StatusPanel;

/**
 * UI Manager Implementation
 * Manages the main application UI, window lifecycle, and component coordination
 */
class UIManager : public IUIManager {
public:
    struct UIManagerConfig {
        // Window configuration
        OpenGLContext::WindowConfig window_config;
        OpenGLContext::ImGuiConfig imgui_config;

        // Layout configuration
        bool enable_docking = true;
        bool show_demo_window = false;
        bool show_metrics_window = false;

        // Update intervals
        int ui_refresh_rate_ms = 16;  // ~60 FPS
        int data_update_rate_ms = 100; // 10 Hz for data updates

        // Panel visibility
        bool show_market_data_panel = true;
        bool show_order_entry_panel = true;
        bool show_positions_panel = true;
        bool show_trades_panel = true;
        bool show_status_panel = true;

        // Menu bar
        bool show_menu_bar = true;
        bool show_toolbar = true;
    };

    explicit UIManager(const UIManagerConfig& config = {});
    virtual ~UIManager();

    // IUIManager implementation
    bool initialize() override;
    void run() override;
    void shutdown() override;

    // Window management
    void show_market_data_window(bool show = true) override;
    void show_order_entry_window(bool show = true) override;
    void show_positions_window(bool show = true) override;
    void show_trades_window(bool show = true) override;
    void show_status_window(bool show = true) override;

    // Data updates (thread-safe)
    void update_market_data(const std::vector<MarketDataRow>& data) override;
    void update_orders(const std::vector<OrderRow>& orders) override;
    void update_positions(const std::vector<PositionRow>& positions) override;
    void update_trades(const std::vector<TradeRow>& trades) override;
    void update_connection_status(bool connected, const std::string& status) override;

    // Event callbacks
    void set_order_submit_callback(std::function<void(const OrderFormData&)> callback) override;
    void set_order_cancel_callback(std::function<void(const std::string&)> callback) override;
    void set_symbol_subscribe_callback(std::function<void(const std::string&)> callback) override;
    void set_symbol_unsubscribe_callback(std::function<void(const std::string&)> callback) override;

    // Additional functionality
    void set_window_title(const std::string& title);
    void load_layout(const std::string& layout_file);
    void save_layout(const std::string& layout_file);

    // Configuration
    void set_config(const UIManagerConfig& config);
    const UIManagerConfig& get_config() const;

    // State queries
    bool is_running() const;
    bool is_initialized() const;

    // Performance
    OpenGLContext::PerformanceStats get_performance_stats() const;

private:
    // Configuration
    UIManagerConfig config_;

    // Core components
    std::unique_ptr<OpenGLContext> gl_context_;

    // UI Panels
    std::unique_ptr<MarketDataPanel> market_data_panel_;
    std::unique_ptr<OrderEntryPanel> order_entry_panel_;
    std::unique_ptr<PositionsPanel> positions_panel_;
    std::unique_ptr<TradesPanel> trades_panel_;
    std::unique_ptr<StatusPanel> status_panel_;

    // State management
    std::atomic<bool> is_running_;
    std::atomic<bool> is_initialized_;
    std::atomic<bool> should_close_;

    // Data synchronization
    mutable std::mutex data_mutex_;
    std::vector<MarketDataRow> market_data_cache_;
    std::vector<OrderRow> orders_cache_;
    std::vector<PositionRow> positions_cache_;
    std::vector<TradeRow> trades_cache_;
    bool connection_status_;
    std::string connection_status_text_;

    // Callbacks
    std::function<void(const OrderFormData&)> order_submit_callback_;
    std::function<void(const std::string&)> order_cancel_callback_;
    std::function<void(const std::string&)> symbol_subscribe_callback_;
    std::function<void(const std::string&)> symbol_unsubscribe_callback_;

    // Timing
    std::chrono::high_resolution_clock::time_point last_update_time_;
    std::chrono::high_resolution_clock::time_point last_data_update_time_;

    // UI state
    bool show_demo_window_;
    bool show_metrics_window_;
    ImGuiID dockspace_id_;

    // Internal methods
    bool initialize_opengl();
    bool initialize_panels();
    void setup_docking();
    void setup_callbacks();

    // Main loop
    void render_frame();
    void render_menu_bar();
    void render_toolbar();
    void render_dockspace();
    void render_panels();
    void render_debug_windows();

    // Data updates
    void update_panel_data();
    bool should_update_data() const;

    // Event handling
    void handle_window_events();
    void handle_keyboard_shortcuts();

    // Layout management
    void setup_default_layout();
    void reset_layout();

    // Error handling
    void handle_error(const std::string& error);

    // Utility methods
    void log_ui_event(const std::string& event) const;
};

/**
 * UI Theme Manager
 * Manages application themes and styling
 */
class UIThemeManager {
public:
    enum class Theme {
        DARK,
        LIGHT,
        TRADING_DARK,
        TRADING_LIGHT,
        CUSTOM
    };

    static void apply_theme(Theme theme);
    static void apply_trading_colors();
    static void load_custom_theme(const std::string& theme_file);
    static void save_current_theme(const std::string& theme_file);

    // Color utilities
    static ImVec4 get_profit_color();
    static ImVec4 get_loss_color();
    static ImVec4 get_neutral_color();
    static ImVec4 get_buy_color();
    static ImVec4 get_sell_color();
    static ImVec4 get_warning_color();
    static ImVec4 get_error_color();
    static ImVec4 get_success_color();

private:
    static void apply_dark_theme();
    static void apply_light_theme();
    static void apply_trading_dark_theme();
    static void apply_trading_light_theme();
};

/**
 * UI Layout Manager
 * Manages window layouts and panel arrangements
 */
class UILayoutManager {
public:
    struct LayoutConfig {
        std::string name;
        bool market_data_visible = true;
        bool order_entry_visible = true;
        bool positions_visible = true;
        bool trades_visible = true;
        bool status_visible = true;

        // Panel positions and sizes (normalized 0-1)
        struct PanelLayout {
            float x, y, width, height;
            bool is_docked = true;
            ImGuiID dock_id = 0;
        };

        PanelLayout market_data_layout;
        PanelLayout order_entry_layout;
        PanelLayout positions_layout;
        PanelLayout trades_layout;
        PanelLayout status_layout;
    };

    static void save_layout(const std::string& name, const LayoutConfig& config);
    static bool load_layout(const std::string& name, LayoutConfig& config);
    static std::vector<std::string> get_available_layouts();
    static void delete_layout(const std::string& name);

    static void apply_default_trading_layout();
    static void apply_minimal_layout();
    static void apply_analysis_layout();

private:
    static std::string get_layouts_directory();
    static bool ensure_layouts_directory_exists();
};

/**
 * UI Performance Monitor
 * Monitors and reports UI performance metrics
 */
class UIPerformanceMonitor {
public:
    struct UIMetrics {
        float fps = 0.0f;
        float frame_time_ms = 0.0f;
        float render_time_ms = 0.0f;
        int draw_calls = 0;
        size_t vertices_count = 0;
        size_t indices_count = 0;
        size_t memory_usage_kb = 0;

        // Panel-specific metrics
        float market_data_render_time_ms = 0.0f;
        float order_entry_render_time_ms = 0.0f;
        float positions_render_time_ms = 0.0f;
        float trades_render_time_ms = 0.0f;
    };

    UIPerformanceMonitor();

    void begin_frame();
    void end_frame();

    void begin_panel_timing(const std::string& panel_name);
    void end_panel_timing(const std::string& panel_name);

    UIMetrics get_metrics() const;
    void reset_metrics();

    // Performance alerts
    void set_fps_threshold(float min_fps);
    void set_frame_time_threshold(float max_frame_time_ms);
    bool is_performance_acceptable() const;

private:
    UIMetrics current_metrics_;
    UIMetrics averaged_metrics_;

    std::chrono::high_resolution_clock::time_point frame_start_time_;
    std::chrono::high_resolution_clock::time_point last_frame_time_;

    std::unordered_map<std::string, std::chrono::high_resolution_clock::time_point> panel_timing_start_;
    std::unordered_map<std::string, float> panel_timing_totals_;

    float fps_threshold_ = 30.0f;
    float frame_time_threshold_ = 33.33f; // 30 FPS

    size_t frame_count_ = 0;
    static constexpr size_t AVERAGING_WINDOW = 60; // Average over 60 frames
};

} // namespace trading::ui