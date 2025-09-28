#pragma once

#include <GLFW/glfw3.h>
#include <imgui.h>
#include <imgui_impl_glfw.h>
#include <imgui_impl_opengl3.h>

#include <string>
#include <memory>
#include <functional>

namespace trading::ui {

/**
 * OpenGL Context Manager
 * Manages OpenGL context initialization, Dear ImGui setup, and window management
 */
class OpenGLContext {
public:
    struct WindowConfig {
        int width = 1920;
        int height = 1080;
        std::string title = "Trading System";
        bool fullscreen = false;
        bool vsync = true;
        bool resizable = true;
        bool maximized = false;
        bool decorated = true;

        // Multi-sampling
        int samples = 4;

        // OpenGL version
        int gl_version_major = 3;
        int gl_version_minor = 3;
    };

    struct ImGuiConfig {
        std::string ini_filename = "imgui.ini";
        bool enable_keyboard_nav = true;
        bool enable_gamepad_nav = false;
        bool enable_docking = true;
        bool enable_viewports = false;  // Multi-viewport support
        float font_size = 16.0f;
        std::string font_path;  // Optional custom font

        // Style settings
        bool dark_theme = true;
        float alpha = 1.0f;
        float rounding = 4.0f;
    };

    explicit OpenGLContext(const WindowConfig& window_config, const ImGuiConfig& imgui_config);
    virtual ~OpenGLContext();

    // Initialization and cleanup
    bool initialize();
    void shutdown();

    // Main loop control
    bool should_close() const;
    void poll_events();
    void swap_buffers();

    // Frame lifecycle
    void begin_frame();
    void end_frame();

    // Window management
    GLFWwindow* get_window() const;
    void set_window_title(const std::string& title);
    void set_window_size(int width, int height);
    void get_window_size(int& width, int& height) const;
    void get_framebuffer_size(int& width, int& height) const;

    // Window state
    bool is_window_minimized() const;
    bool is_window_focused() const;
    void maximize_window();
    void restore_window();
    void set_window_pos(int x, int y);

    // Input handling
    void set_cursor_pos_callback(std::function<void(double, double)> callback);
    void set_mouse_button_callback(std::function<void(int, int, int)> callback);
    void set_scroll_callback(std::function<void(double, double)> callback);
    void set_key_callback(std::function<void(int, int, int, int)> callback);
    void set_char_callback(std::function<void(unsigned int)> callback);
    void set_window_size_callback(std::function<void(int, int)> callback);

    // ImGui utilities
    void load_imgui_style();
    void set_imgui_style_colors();
    bool load_font(const std::string& font_path, float size);
    void reset_imgui_style();

    // Rendering utilities
    void clear_screen(float r = 0.0f, float g = 0.0f, float b = 0.0f, float a = 1.0f);
    void set_viewport(int x, int y, int width, int height);

    // OpenGL state
    void enable_depth_test(bool enable = true);
    void enable_blending(bool enable = true);
    void set_blend_mode(GLenum src, GLenum dst);

    // Error handling
    std::string get_last_error() const;
    static void setup_debug_callback();

    // Configuration
    void update_window_config(const WindowConfig& config);
    void update_imgui_config(const ImGuiConfig& config);

    const WindowConfig& get_window_config() const;
    const ImGuiConfig& get_imgui_config() const;

    // Performance monitoring
    struct PerformanceStats {
        float fps = 0.0f;
        float frame_time_ms = 0.0f;
        float cpu_time_ms = 0.0f;
        float gpu_time_ms = 0.0f;
        int draw_calls = 0;
        size_t memory_usage_mb = 0;
    };

    PerformanceStats get_performance_stats() const;
    void reset_performance_stats();

private:
    // Configuration
    WindowConfig window_config_;
    ImGuiConfig imgui_config_;

    // GLFW and OpenGL state
    GLFWwindow* window_;
    bool initialized_;
    bool imgui_initialized_;

    // Error handling
    mutable std::string last_error_;

    // Callbacks
    std::function<void(double, double)> cursor_pos_callback_;
    std::function<void(int, int, int)> mouse_button_callback_;
    std::function<void(double, double)> scroll_callback_;
    std::function<void(int, int, int, int)> key_callback_;
    std::function<void(unsigned int)> char_callback_;
    std::function<void(int, int)> window_size_callback_;

    // Performance monitoring
    mutable PerformanceStats perf_stats_;
    mutable std::chrono::high_resolution_clock::time_point last_frame_time_;
    mutable std::chrono::high_resolution_clock::time_point frame_start_time_;

    // Internal methods
    bool initialize_glfw();
    bool create_window();
    bool initialize_opengl();
    bool initialize_imgui();

    void setup_imgui_style();
    void cleanup_glfw();
    void cleanup_imgui();

    // Error handling
    void set_error(const std::string& error);
    static void glfw_error_callback(int error, const char* description);
    static void APIENTRY opengl_debug_callback(GLenum source, GLenum type, GLuint id,
                                              GLenum severity, GLsizei length,
                                              const GLchar* message, const void* userParam);

    // GLFW callbacks (static functions that delegate to instance methods)
    static void cursor_pos_callback_impl(GLFWwindow* window, double xpos, double ypos);
    static void mouse_button_callback_impl(GLFWwindow* window, int button, int action, int mods);
    static void scroll_callback_impl(GLFWwindow* window, double xoffset, double yoffset);
    static void key_callback_impl(GLFWwindow* window, int key, int scancode, int action, int mods);
    static void char_callback_impl(GLFWwindow* window, unsigned int codepoint);
    static void window_size_callback_impl(GLFWwindow* window, int width, int height);

    // Performance monitoring
    void update_performance_stats() const;
};

/**
 * RAII Frame Guard
 * Automatically calls begin_frame() and end_frame()
 */
class FrameGuard {
public:
    explicit FrameGuard(OpenGLContext& context);
    ~FrameGuard();

    // Non-copyable, non-movable
    FrameGuard(const FrameGuard&) = delete;
    FrameGuard& operator=(const FrameGuard&) = delete;
    FrameGuard(FrameGuard&&) = delete;
    FrameGuard& operator=(FrameGuard&&) = delete;

private:
    OpenGLContext& context_;
};

/**
 * ImGui Utilities
 * Helper functions for common ImGui operations
 */
class ImGuiUtils {
public:
    // Colors
    static ImVec4 get_profit_color();
    static ImVec4 get_loss_color();
    static ImVec4 get_neutral_color();
    static ImVec4 get_warning_color();
    static ImVec4 get_error_color();
    static ImVec4 get_success_color();

    // Formatting
    static std::string format_currency(double value, int precision = 2);
    static std::string format_percentage(double value, int precision = 2);
    static std::string format_volume(double value);
    static std::string format_time(const std::chrono::system_clock::time_point& time);

    // Widgets
    static bool colored_button(const char* label, const ImVec4& color, const ImVec2& size = ImVec2(0, 0));
    static void profit_loss_text(double value, const char* format = "%.2f");
    static void status_indicator(bool status, const char* label);
    static void progress_bar_with_text(float fraction, const ImVec2& size, const char* overlay);

    // Tables
    static void setup_trading_table_columns();
    static void table_cell_right_aligned(const char* text);
    static void table_cell_colored(const char* text, const ImVec4& color);

    // Input validation
    static bool validate_price_input(const char* input, double& value);
    static bool validate_quantity_input(const char* input, double& value);

    // Tooltips and help
    static void help_marker(const char* desc);
    static void show_tooltip(const char* text);

    // Layout helpers
    static void center_next_window(float width, float height);
    static bool begin_popup_modal_centered(const char* name, bool* p_open, float width, float height);
};

/**
 * OpenGL Resource Manager
 * Manages OpenGL resources like textures, buffers, etc.
 */
class OpenGLResourceManager {
public:
    OpenGLResourceManager();
    ~OpenGLResourceManager();

    // Texture management
    struct Texture {
        unsigned int id;
        int width;
        int height;
        int channels;
    };

    bool load_texture(const std::string& name, const std::string& path);
    bool create_texture(const std::string& name, int width, int height, const void* data);
    Texture* get_texture(const std::string& name);
    void delete_texture(const std::string& name);

    // Buffer management
    struct Buffer {
        unsigned int vbo;
        unsigned int vao;
        unsigned int ebo;
        size_t vertex_count;
        size_t index_count;
    };

    bool create_buffer(const std::string& name, const float* vertices, size_t vertex_count,
                      const unsigned int* indices = nullptr, size_t index_count = 0);
    Buffer* get_buffer(const std::string& name);
    void delete_buffer(const std::string& name);

    // Cleanup
    void cleanup_all();

private:
    std::unordered_map<std::string, Texture> textures_;
    std::unordered_map<std::string, Buffer> buffers_;
};

} // namespace trading::ui