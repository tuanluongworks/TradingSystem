#pragma once

#include <string>
#include <chrono>
#include <imgui.h>

// Include the contract from the include directory
#include "contracts/ui_interface.hpp"

namespace trading::ui {

class StatusPanel {
private:
    StatusInfo status_info_;

    // Connection states
    bool market_data_connected_ = false;
    bool database_available_ = true;
    std::string connection_status_text_ = "Disconnected";

    // Performance metrics
    double ui_fps_ = 0.0;
    double cpu_usage_ = 0.0;
    double memory_usage_mb_ = 0.0;

    // Last update tracking
    std::chrono::system_clock::time_point last_heartbeat_;

    // Helper methods
    void render_connection_status();
    void render_trading_status();
    void render_performance_metrics();
    void render_system_time();
    ImU32 get_connection_color(bool connected) const;
    std::string format_time(const std::chrono::system_clock::time_point& time) const;
    std::string format_currency(double value) const;
    void update_performance_metrics();

public:
    StatusPanel() = default;
    ~StatusPanel() = default;

    // Main interface
    void render();
    void update_status(const StatusInfo& status);

    // Connection status updates
    void set_market_data_connected(bool connected);
    void set_database_available(bool available);
    void set_connection_status(const std::string& status);
    void update_connection_status(bool connected, const std::string& status);

    // Performance updates
    void set_ui_fps(double fps);
    void update_heartbeat();
};

} // namespace trading::ui