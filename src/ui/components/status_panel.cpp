#include "status_panel.hpp"
#include <iomanip>
#include <sstream>
#include <cstdio>

#ifdef _WIN32
#include <windows.h>
#include <psapi.h>
#elif __APPLE__
#include <mach/mach.h>
#include <sys/sysctl.h>
#elif __linux__
#include <sys/sysinfo.h>
#include <unistd.h>
#endif

namespace trading::ui {

void StatusPanel::render() {
    // Status bar at bottom of window
    ImGuiWindowFlags window_flags = ImGuiWindowFlags_NoScrollbar | ImGuiWindowFlags_NoSavedSettings |
                                   ImGuiWindowFlags_MenuBar | ImGuiWindowFlags_NoTitleBar |
                                   ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoMove |
                                   ImGuiWindowFlags_NoCollapse;

    const ImGuiViewport* viewport = ImGui::GetMainViewport();
    ImGui::SetNextWindowPos(ImVec2(viewport->WorkPos.x, viewport->WorkPos.y + viewport->WorkSize.y - 30));
    ImGui::SetNextWindowSize(ImVec2(viewport->WorkSize.x, 30));

    if (ImGui::Begin("##StatusBar", nullptr, window_flags)) {
        // Connection status
        render_connection_status();

        ImGui::SameLine();
        ImGui::SeparatorEx(ImGuiSeparatorFlags_Vertical);
        ImGui::SameLine();

        // Trading status
        render_trading_status();

        ImGui::SameLine();
        ImGui::SeparatorEx(ImGuiSeparatorFlags_Vertical);
        ImGui::SameLine();

        // Performance metrics
        render_performance_metrics();

        ImGui::SameLine();
        ImGui::SeparatorEx(ImGuiSeparatorFlags_Vertical);
        ImGui::SameLine();

        // System time
        render_system_time();
    }
    ImGui::End();
}

void StatusPanel::update_status(const StatusInfo& status) {
    status_info_ = status;
    market_data_connected_ = status.market_data_connected;
    database_available_ = status.database_available;
}

void StatusPanel::set_market_data_connected(bool connected) {
    market_data_connected_ = connected;
    status_info_.market_data_connected = connected;
    connection_status_text_ = connected ? "Connected" : "Disconnected";
}

void StatusPanel::set_database_available(bool available) {
    database_available_ = available;
    status_info_.database_available = available;
}

void StatusPanel::set_connection_status(const std::string& status) {
    connection_status_text_ = status;
}

void StatusPanel::set_ui_fps(double fps) {
    ui_fps_ = fps;
}

void StatusPanel::update_heartbeat() {
    last_heartbeat_ = std::chrono::system_clock::now();
    update_performance_metrics();
}

void StatusPanel::render_connection_status() {
    // Market Data connection
    ImU32 md_color = get_connection_color(market_data_connected_);
    ImGui::TextColored(ImGui::ColorConvertU32ToFloat4(md_color), "●");
    if (ImGui::IsItemHovered()) {
        ImGui::SetTooltip("Market Data: %s", market_data_connected_ ? "Connected" : "Disconnected");
    }

    ImGui::SameLine();
    ImGui::Text("MD");

    // Database status
    ImGui::SameLine();
    ImU32 db_color = get_connection_color(database_available_);
    ImGui::TextColored(ImGui::ColorConvertU32ToFloat4(db_color), "●");
    if (ImGui::IsItemHovered()) {
        ImGui::SetTooltip("Database: %s", database_available_ ? "Available" : "Unavailable");
    }

    ImGui::SameLine();
    ImGui::Text("DB");

    // Connection status text
    ImGui::SameLine();
    ImGui::Text("| %s", connection_status_text_.c_str());
}

void StatusPanel::render_trading_status() {
    // Active orders
    ImGui::Text("Orders: %d", status_info_.active_orders);

    // Open positions
    ImGui::SameLine();
    ImGui::Text("| Positions: %d", status_info_.open_positions);

    // Daily P&L
    ImGui::SameLine();
    double daily_pnl = status_info_.daily_pnl;
    ImU32 pnl_color;
    if (daily_pnl > 0.0) {
        pnl_color = IM_COL32(0, 255, 0, 255); // Green
    } else if (daily_pnl < 0.0) {
        pnl_color = IM_COL32(255, 0, 0, 255); // Red
    } else {
        pnl_color = IM_COL32(255, 255, 255, 255); // White
    }

    ImGui::SameLine();
    ImGui::Text("| P&L:");
    ImGui::SameLine();
    ImGui::TextColored(ImGui::ColorConvertU32ToFloat4(pnl_color),
                      "%s", format_currency(daily_pnl).c_str());
}

void StatusPanel::render_performance_metrics() {
    // FPS
    ImGui::Text("FPS: %.1f", ui_fps_);

    // CPU usage
    ImGui::SameLine();
    ImU32 cpu_color = IM_COL32(255, 255, 255, 255);
    if (cpu_usage_ > 80.0) {
        cpu_color = IM_COL32(255, 0, 0, 255); // Red for high CPU
    } else if (cpu_usage_ > 60.0) {
        cpu_color = IM_COL32(255, 255, 0, 255); // Yellow for medium CPU
    }

    ImGui::SameLine();
    ImGui::TextColored(ImGui::ColorConvertU32ToFloat4(cpu_color),
                      "| CPU: %.1f%%", cpu_usage_);

    // Memory usage
    ImGui::SameLine();
    ImU32 mem_color = IM_COL32(255, 255, 255, 255);
    if (memory_usage_mb_ > 500.0) {
        mem_color = IM_COL32(255, 255, 0, 255); // Yellow for high memory
    }

    ImGui::SameLine();
    ImGui::TextColored(ImGui::ColorConvertU32ToFloat4(mem_color),
                      "| MEM: %.0f MB", memory_usage_mb_);
}

void StatusPanel::render_system_time() {
    auto now = std::chrono::system_clock::now();
    ImGui::Text("%s", format_time(now).c_str());

    // Show last update age if we have heartbeat data
    if (last_heartbeat_.time_since_epoch().count() > 0) {
        auto age = std::chrono::duration_cast<std::chrono::seconds>(now - last_heartbeat_).count();
        if (age > 5) { // Show warning if no heartbeat for 5+ seconds
            ImGui::SameLine();
            ImGui::TextColored(ImVec4(1.0f, 1.0f, 0.0f, 1.0f), "| Stale: %lds", age);
        }
    }
}

ImU32 StatusPanel::get_connection_color(bool connected) const {
    return connected ? IM_COL32(0, 255, 0, 255) : IM_COL32(255, 0, 0, 255);
}

std::string StatusPanel::format_time(const std::chrono::system_clock::time_point& time) const {
    auto time_t = std::chrono::system_clock::to_time_t(time);
    std::stringstream ss;
    ss << std::put_time(std::localtime(&time_t), "%H:%M:%S");
    return ss.str();
}

std::string StatusPanel::format_currency(double value) const {
    std::ostringstream oss;
    oss << std::fixed << std::setprecision(2);

    if (value >= 0) {
        oss << "$" << value;
    } else {
        oss << "-$" << std::abs(value);
    }

    return oss.str();
}

void StatusPanel::update_performance_metrics() {
    // Update FPS (handled externally via set_ui_fps)

    // Update CPU usage (simplified cross-platform approach)
#ifdef _WIN32
    // Windows implementation
    FILETIME idle, kernel, user;
    if (GetSystemTimes(&idle, &kernel, &user)) {
        static ULARGE_INTEGER prev_idle, prev_kernel, prev_user;
        static bool first_call = true;

        ULARGE_INTEGER curr_idle, curr_kernel, curr_user;
        curr_idle.LowPart = idle.dwLowDateTime;
        curr_idle.HighPart = idle.dwHighDateTime;
        curr_kernel.LowPart = kernel.dwLowDateTime;
        curr_kernel.HighPart = kernel.dwHighDateTime;
        curr_user.LowPart = user.dwLowDateTime;
        curr_user.HighPart = user.dwHighDateTime;

        if (!first_call) {
            ULONGLONG idle_diff = curr_idle.QuadPart - prev_idle.QuadPart;
            ULONGLONG total_diff = (curr_kernel.QuadPart - prev_kernel.QuadPart) +
                                 (curr_user.QuadPart - prev_user.QuadPart);

            if (total_diff > 0) {
                cpu_usage_ = 100.0 * (1.0 - (double)idle_diff / total_diff);
            }
        }

        prev_idle = curr_idle;
        prev_kernel = curr_kernel;
        prev_user = curr_user;
        first_call = false;
    }

    // Memory usage
    PROCESS_MEMORY_COUNTERS pmc;
    if (GetProcessMemoryInfo(GetCurrentProcess(), &pmc, sizeof(pmc))) {
        memory_usage_mb_ = pmc.WorkingSetSize / (1024.0 * 1024.0);
    }

#elif __APPLE__
    // macOS implementation
    mach_task_basic_info_data_t info;
    mach_msg_type_number_t info_count = MACH_TASK_BASIC_INFO_COUNT;

    if (task_info(mach_task_self(), MACH_TASK_BASIC_INFO,
                  (task_info_t)&info, &info_count) == KERN_SUCCESS) {
        memory_usage_mb_ = info.resident_size / (1024.0 * 1024.0);
    }

    // Simplified CPU usage - just use a placeholder for now
    cpu_usage_ = 0.0; // Would need more complex implementation

#elif __linux__
    // Linux implementation
    struct sysinfo sys_info;
    if (sysinfo(&sys_info) == 0) {
        // Simplified approach - would need /proc/stat parsing for real CPU usage
        cpu_usage_ = 0.0;
    }

    // Memory usage from /proc/self/status
    FILE* file = fopen("/proc/self/status", "r");
    if (file) {
        char line[128];
        while (fgets(line, sizeof(line), file)) {
            if (strncmp(line, "VmRSS:", 6) == 0) {
                int mem_kb;
                if (sscanf(line, "VmRSS: %d kB", &mem_kb) == 1) {
                    memory_usage_mb_ = mem_kb / 1024.0;
                }
                break;
            }
        }
        fclose(file);
    }

#else
    // Fallback for other platforms
    cpu_usage_ = 0.0;
    memory_usage_mb_ = 0.0;
#endif
}

} // namespace trading::ui