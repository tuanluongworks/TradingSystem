#define GL_SILENCE_DEPRECATION  // Silence OpenGL deprecation warnings on macOS
#include "opengl_context.hpp"
#include "../../utils/logging.hpp"

#include <GLFW/glfw3.h>
#include <iostream>
#include <sstream>
#include <iomanip>
#include <unordered_map>

namespace trading::ui {

// Static callback implementations
namespace {
    std::unordered_map<GLFWwindow*, OpenGLContext*> window_context_map;
}

// OpenGLContext implementation

OpenGLContext::OpenGLContext(const WindowConfig& window_config, const ImGuiConfig& imgui_config)
    : window_config_(window_config),
      imgui_config_(imgui_config),
      window_(nullptr),
      initialized_(false),
      imgui_initialized_(false),
      last_frame_time_(std::chrono::high_resolution_clock::now()) {
}

OpenGLContext::~OpenGLContext() {
    shutdown();
}

bool OpenGLContext::initialize() {
    if (initialized_) {
        return true;
    }

    try {
        if (!initialize_glfw()) {
            set_error("Failed to initialize GLFW");
            return false;
        }

        if (!create_window()) {
            set_error("Failed to create window");
            cleanup_glfw();
            return false;
        }

        if (!initialize_opengl()) {
            set_error("Failed to initialize OpenGL");
            cleanup_glfw();
            return false;
        }

        if (!initialize_imgui()) {
            set_error("Failed to initialize ImGui");
            cleanup_glfw();
            return false;
        }

        // Register this context with the window
        window_context_map[window_] = this;

        initialized_ = true;
        Logger::info("OpenGLContext: OpenGL context initialized successfully");
        return true;

    } catch (const std::exception& e) {
        set_error("Exception during initialization: " + std::string(e.what()));
        cleanup_glfw();
        return false;
    }
}

void OpenGLContext::shutdown() {
    if (!initialized_) {
        return;
    }

    // Remove from context map
    if (window_) {
        window_context_map.erase(window_);
    }

    cleanup_imgui();
    cleanup_glfw();

    initialized_ = false;
    imgui_initialized_ = false;
    Logger::info("OpenGLContext: OpenGL context shutdown complete");
}

bool OpenGLContext::should_close() const {
    return window_ ? glfwWindowShouldClose(window_) : true;
}

void OpenGLContext::poll_events() {
    glfwPollEvents();
}

void OpenGLContext::swap_buffers() {
    if (window_) {
        glfwSwapBuffers(window_);
    }
}

void OpenGLContext::begin_frame() {
    frame_start_time_ = std::chrono::high_resolution_clock::now();

    // Clear screen
    clear_screen(0.15f, 0.15f, 0.15f, 1.0f);

    // Start ImGui frame
    if (imgui_initialized_) {
        ImGui_ImplOpenGL3_NewFrame();
        ImGui_ImplGlfw_NewFrame();
        ImGui::NewFrame();
    }
}

void OpenGLContext::end_frame() {
    if (imgui_initialized_) {
        ImGui::Render();
        ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());

        // Handle multi-viewport if enabled
        if (imgui_config_.enable_viewports) {
            GLFWwindow* backup_current_context = glfwGetCurrentContext();
            // Multi-viewport rendering - may not be available in this ImGui version
            // ImGui::UpdatePlatformWindows();
            // ImGui::RenderPlatformWindowsDefault();
            glfwMakeContextCurrent(backup_current_context);
        }
    }

    update_performance_stats();
}

GLFWwindow* OpenGLContext::get_window() const {
    return window_;
}

void OpenGLContext::set_window_title(const std::string& title) {
    if (window_) {
        glfwSetWindowTitle(window_, title.c_str());
        window_config_.title = title;
    }
}

void OpenGLContext::set_window_size(int width, int height) {
    if (window_) {
        glfwSetWindowSize(window_, width, height);
        window_config_.width = width;
        window_config_.height = height;
    }
}

void OpenGLContext::get_window_size(int& width, int& height) const {
    if (window_) {
        glfwGetWindowSize(window_, &width, &height);
    } else {
        width = window_config_.width;
        height = window_config_.height;
    }
}

void OpenGLContext::get_framebuffer_size(int& width, int& height) const {
    if (window_) {
        glfwGetFramebufferSize(window_, &width, &height);
    } else {
        width = window_config_.width;
        height = window_config_.height;
    }
}

bool OpenGLContext::is_window_minimized() const {
    return window_ ? glfwGetWindowAttrib(window_, GLFW_ICONIFIED) : false;
}

bool OpenGLContext::is_window_focused() const {
    return window_ ? glfwGetWindowAttrib(window_, GLFW_FOCUSED) : false;
}

void OpenGLContext::maximize_window() {
    if (window_) {
        glfwMaximizeWindow(window_);
    }
}

void OpenGLContext::restore_window() {
    if (window_) {
        glfwRestoreWindow(window_);
    }
}

void OpenGLContext::set_window_pos(int x, int y) {
    if (window_) {
        glfwSetWindowPos(window_, x, y);
    }
}

void OpenGLContext::clear_screen(float r, float g, float b, float a) {
    glClearColor(r, g, b, a);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
}

void OpenGLContext::set_viewport(int x, int y, int width, int height) {
    glViewport(x, y, width, height);
}

void OpenGLContext::enable_depth_test(bool enable) {
    if (enable) {
        glEnable(GL_DEPTH_TEST);
    } else {
        glDisable(GL_DEPTH_TEST);
    }
}

void OpenGLContext::enable_blending(bool enable) {
    if (enable) {
        glEnable(GL_BLEND);
        glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    } else {
        glDisable(GL_BLEND);
    }
}

void OpenGLContext::set_blend_mode(GLenum src, GLenum dst) {
    glBlendFunc(src, dst);
}

std::string OpenGLContext::get_last_error() const {
    return last_error_;
}

const OpenGLContext::WindowConfig& OpenGLContext::get_window_config() const {
    return window_config_;
}

const OpenGLContext::ImGuiConfig& OpenGLContext::get_imgui_config() const {
    return imgui_config_;
}

OpenGLContext::PerformanceStats OpenGLContext::get_performance_stats() const {
    return perf_stats_;
}

void OpenGLContext::reset_performance_stats() {
    perf_stats_ = PerformanceStats{};
}

// Private methods

bool OpenGLContext::initialize_glfw() {
    // Set error callback
    glfwSetErrorCallback(glfw_error_callback);

    if (!glfwInit()) {
        return false;
    }

    // Set OpenGL version and profile
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, window_config_.gl_version_major);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, window_config_.gl_version_minor);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);

#ifdef __APPLE__
    glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GL_TRUE);
#endif

    // Window hints
    glfwWindowHint(GLFW_RESIZABLE, window_config_.resizable ? GLFW_TRUE : GLFW_FALSE);
    glfwWindowHint(GLFW_DECORATED, window_config_.decorated ? GLFW_TRUE : GLFW_FALSE);
    glfwWindowHint(GLFW_MAXIMIZED, window_config_.maximized ? GLFW_TRUE : GLFW_FALSE);

    if (window_config_.samples > 0) {
        glfwWindowHint(GLFW_SAMPLES, window_config_.samples);
    }

    return true;
}

bool OpenGLContext::create_window() {
    GLFWmonitor* monitor = window_config_.fullscreen ? glfwGetPrimaryMonitor() : nullptr;

    window_ = glfwCreateWindow(
        window_config_.width,
        window_config_.height,
        window_config_.title.c_str(),
        monitor,
        nullptr
    );

    if (!window_) {
        return false;
    }

    glfwMakeContextCurrent(window_);

    // Set VSync
    glfwSwapInterval(window_config_.vsync ? 1 : 0);

    // Set callbacks
    glfwSetCursorPosCallback(window_, cursor_pos_callback_impl);
    glfwSetMouseButtonCallback(window_, mouse_button_callback_impl);
    glfwSetScrollCallback(window_, scroll_callback_impl);
    glfwSetKeyCallback(window_, key_callback_impl);
    glfwSetCharCallback(window_, char_callback_impl);
    glfwSetWindowSizeCallback(window_, window_size_callback_impl);

    return true;
}

bool OpenGLContext::initialize_opengl() {
    // OpenGL loader is handled by ImGui - no separate initialization needed
    // Just verify that we have a valid context

    // Check OpenGL version
    const char* version = reinterpret_cast<const char*>(glGetString(GL_VERSION));
    if (!version) {
        return false;
    }

    Logger::info("OpenGLContext: OpenGL Version: " + std::string(version));

    // Enable debug output if available
    setup_debug_callback();

    // Set up default OpenGL state
    enable_blending(true);
    enable_depth_test(true);
    glDepthFunc(GL_LESS);

    return true;
}

bool OpenGLContext::initialize_imgui() {
    // Setup Dear ImGui context
    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    ImGuiIO& io = ImGui::GetIO();

    // Configuration flags
    io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;
    if (imgui_config_.enable_docking) {
        // io.ConfigFlags |= ImGuiConfigFlags_DockingEnable;  // May not be available in this ImGui version
    }
    if (imgui_config_.enable_viewports) {
        // io.ConfigFlags |= ImGuiConfigFlags_ViewportsEnable;  // May not be available in this ImGui version
    }

    // Setup ImGui style
    setup_imgui_style();

    // Setup platform/renderer backends
    if (!ImGui_ImplGlfw_InitForOpenGL(window_, true)) {
        return false;
    }

    const char* glsl_version = "#version 330";
    if (!ImGui_ImplOpenGL3_Init(glsl_version)) {
        ImGui_ImplGlfw_Shutdown();
        return false;
    }

    // Load font if specified
    if (!imgui_config_.font_path.empty()) {
        load_font(imgui_config_.font_path, imgui_config_.font_size);
    }

    imgui_initialized_ = true;
    return true;
}

void OpenGLContext::setup_imgui_style() {
    ImGuiStyle& style = ImGui::GetStyle();

    if (imgui_config_.dark_theme) {
        ImGui::StyleColorsDark();
    } else {
        ImGui::StyleColorsLight();
    }

    // Customize style
    style.Alpha = imgui_config_.alpha;
    style.WindowRounding = imgui_config_.rounding;
    style.FrameRounding = imgui_config_.rounding;
    style.GrabRounding = imgui_config_.rounding;
    style.TabRounding = imgui_config_.rounding;

    // Trading-specific color customizations
    ImVec4* colors = style.Colors;

    // Professional dark theme colors
    if (imgui_config_.dark_theme) {
        colors[ImGuiCol_WindowBg] = ImVec4(0.13f, 0.13f, 0.13f, 1.00f);
        colors[ImGuiCol_ChildBg] = ImVec4(0.16f, 0.16f, 0.16f, 1.00f);
        colors[ImGuiCol_PopupBg] = ImVec4(0.19f, 0.19f, 0.19f, 0.92f);
        colors[ImGuiCol_Border] = ImVec4(0.44f, 0.44f, 0.44f, 0.29f);
        colors[ImGuiCol_FrameBg] = ImVec4(0.20f, 0.20f, 0.20f, 1.00f);
        colors[ImGuiCol_FrameBgHovered] = ImVec4(0.26f, 0.26f, 0.26f, 1.00f);
        colors[ImGuiCol_FrameBgActive] = ImVec4(0.28f, 0.28f, 0.28f, 1.00f);
        colors[ImGuiCol_TitleBg] = ImVec4(0.15f, 0.15f, 0.15f, 1.00f);
        colors[ImGuiCol_TitleBgActive] = ImVec4(0.15f, 0.15f, 0.15f, 1.00f);
        colors[ImGuiCol_MenuBarBg] = ImVec4(0.19f, 0.19f, 0.19f, 1.00f);
        colors[ImGuiCol_Header] = ImVec4(0.26f, 0.59f, 0.98f, 0.31f);
        colors[ImGuiCol_HeaderHovered] = ImVec4(0.26f, 0.59f, 0.98f, 0.80f);
        colors[ImGuiCol_HeaderActive] = ImVec4(0.26f, 0.59f, 0.98f, 1.00f);
    }
}

void OpenGLContext::cleanup_glfw() {
    if (window_) {
        glfwDestroyWindow(window_);
        window_ = nullptr;
    }
    glfwTerminate();
}

void OpenGLContext::cleanup_imgui() {
    if (imgui_initialized_) {
        ImGui_ImplOpenGL3_Shutdown();
        ImGui_ImplGlfw_Shutdown();
        ImGui::DestroyContext();
        imgui_initialized_ = false;
    }
}

void OpenGLContext::set_error(const std::string& error) {
    last_error_ = error;
    Logger::error("OpenGLContext: " + error);
}

void OpenGLContext::update_performance_stats() const {
    auto now = std::chrono::high_resolution_clock::now();
    auto frame_duration = std::chrono::duration_cast<std::chrono::microseconds>(now - last_frame_time_);
    auto cpu_duration = std::chrono::duration_cast<std::chrono::microseconds>(now - frame_start_time_);

    last_frame_time_ = now;

    perf_stats_.frame_time_ms = frame_duration.count() / 1000.0f;
    perf_stats_.cpu_time_ms = cpu_duration.count() / 1000.0f;
    perf_stats_.fps = (frame_duration.count() > 0) ? 1000000.0f / frame_duration.count() : 0.0f;
}

bool OpenGLContext::load_font(const std::string& font_path, float size) {
    ImGuiIO& io = ImGui::GetIO();
    ImFont* font = io.Fonts->AddFontFromFileTTF(font_path.c_str(), size);
    if (font) {
        io.FontDefault = font;
        return true;
    }
    return false;
}

void OpenGLContext::setup_debug_callback() {
    // OpenGL debug functionality requires GL extensions not available with basic headers
    // This can be implemented later with proper GL loader (gl3w, GLEW, etc.)
    // For now, debug functionality is disabled
}

// Static callback implementations

void OpenGLContext::glfw_error_callback(int error, const char* description) {
    Logger::error("GLFW: Error " + std::to_string(error) + ": " + description);
}

void APIENTRY OpenGLContext::opengl_debug_callback(GLenum source, GLenum type, GLuint id,
                                                   GLenum severity, GLsizei length,
                                                   const GLchar* message, const void* userParam) {
    // Suppress unused parameter warnings
    (void)source; (void)type; (void)id; (void)severity; (void)length; (void)message; (void)userParam;

    // OpenGL debug callback implementation disabled - requires GL extensions
    // This can be implemented later with proper GL loader
}

void OpenGLContext::cursor_pos_callback_impl(GLFWwindow* window, double xpos, double ypos) {
    auto it = window_context_map.find(window);
    if (it != window_context_map.end() && it->second->cursor_pos_callback_) {
        it->second->cursor_pos_callback_(xpos, ypos);
    }
}

void OpenGLContext::mouse_button_callback_impl(GLFWwindow* window, int button, int action, int mods) {
    auto it = window_context_map.find(window);
    if (it != window_context_map.end() && it->second->mouse_button_callback_) {
        it->second->mouse_button_callback_(button, action, mods);
    }
}

void OpenGLContext::scroll_callback_impl(GLFWwindow* window, double xoffset, double yoffset) {
    auto it = window_context_map.find(window);
    if (it != window_context_map.end() && it->second->scroll_callback_) {
        it->second->scroll_callback_(xoffset, yoffset);
    }
}

void OpenGLContext::key_callback_impl(GLFWwindow* window, int key, int scancode, int action, int mods) {
    auto it = window_context_map.find(window);
    if (it != window_context_map.end() && it->second->key_callback_) {
        it->second->key_callback_(key, scancode, action, mods);
    }
}

void OpenGLContext::char_callback_impl(GLFWwindow* window, unsigned int codepoint) {
    auto it = window_context_map.find(window);
    if (it != window_context_map.end() && it->second->char_callback_) {
        it->second->char_callback_(codepoint);
    }
}

void OpenGLContext::window_size_callback_impl(GLFWwindow* window, int width, int height) {
    auto it = window_context_map.find(window);
    if (it != window_context_map.end()) {
        it->second->window_config_.width = width;
        it->second->window_config_.height = height;
        if (it->second->window_size_callback_) {
            it->second->window_size_callback_(width, height);
        }
    }
}

// FrameGuard implementation

FrameGuard::FrameGuard(OpenGLContext& context) : context_(context) {
    context_.begin_frame();
}

FrameGuard::~FrameGuard() {
    context_.end_frame();
}

// ImGuiUtils implementation

ImVec4 ImGuiUtils::get_profit_color() {
    return ImVec4(0.2f, 0.8f, 0.2f, 1.0f); // Green
}

ImVec4 ImGuiUtils::get_loss_color() {
    return ImVec4(0.8f, 0.2f, 0.2f, 1.0f); // Red
}

ImVec4 ImGuiUtils::get_neutral_color() {
    return ImVec4(0.7f, 0.7f, 0.7f, 1.0f); // Gray
}

ImVec4 ImGuiUtils::get_warning_color() {
    return ImVec4(1.0f, 0.7f, 0.0f, 1.0f); // Orange
}

ImVec4 ImGuiUtils::get_error_color() {
    return ImVec4(0.8f, 0.1f, 0.1f, 1.0f); // Dark Red
}

ImVec4 ImGuiUtils::get_success_color() {
    return ImVec4(0.1f, 0.7f, 0.1f, 1.0f); // Dark Green
}

std::string ImGuiUtils::format_currency(double value, int precision) {
    std::ostringstream oss;
    oss << std::fixed << std::setprecision(precision) << "$" << value;
    return oss.str();
}

std::string ImGuiUtils::format_percentage(double value, int precision) {
    std::ostringstream oss;
    oss << std::fixed << std::setprecision(precision) << (value * 100.0) << "%";
    return oss.str();
}

std::string ImGuiUtils::format_volume(double value) {
    if (value >= 1000000) {
        return std::to_string(static_cast<int>(value / 1000000)) + "M";
    } else if (value >= 1000) {
        return std::to_string(static_cast<int>(value / 1000)) + "K";
    } else {
        return std::to_string(static_cast<int>(value));
    }
}

std::string ImGuiUtils::format_time(const std::chrono::system_clock::time_point& time) {
    auto time_t = std::chrono::system_clock::to_time_t(time);
    auto tm = *std::localtime(&time_t);

    std::ostringstream oss;
    oss << std::put_time(&tm, "%H:%M:%S");
    return oss.str();
}

bool ImGuiUtils::colored_button(const char* label, const ImVec4& color, const ImVec2& size) {
    ImGui::PushStyleColor(ImGuiCol_Button, color);
    ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4(color.x * 1.1f, color.y * 1.1f, color.z * 1.1f, color.w));
    ImGui::PushStyleColor(ImGuiCol_ButtonActive, ImVec4(color.x * 0.9f, color.y * 0.9f, color.z * 0.9f, color.w));

    bool result = ImGui::Button(label, size);

    ImGui::PopStyleColor(3);
    return result;
}

void ImGuiUtils::profit_loss_text(double value, const char* format) {
    ImVec4 color = (value >= 0) ? get_profit_color() : get_loss_color();
    ImGui::TextColored(color, format, value);
}

void ImGuiUtils::status_indicator(bool status, const char* label) {
    ImVec4 color = status ? get_success_color() : get_error_color();
    const char* symbol = status ? "●" : "●";
    ImGui::TextColored(color, "%s %s", symbol, label);
}

void ImGuiUtils::help_marker(const char* desc) {
    ImGui::TextDisabled("(?)");
    if (ImGui::IsItemHovered()) {
        ImGui::BeginTooltip();
        ImGui::PushTextWrapPos(ImGui::GetFontSize() * 35.0f);
        ImGui::TextUnformatted(desc);
        ImGui::PopTextWrapPos();
        ImGui::EndTooltip();
    }
}

bool ImGuiUtils::validate_price_input(const char* input, double& value) {
    try {
        value = std::stod(input);
        return value > 0.0;
    } catch (...) {
        return false;
    }
}

bool ImGuiUtils::validate_quantity_input(const char* input, double& value) {
    try {
        value = std::stod(input);
        return value > 0.0;
    } catch (...) {
        return false;
    }
}

// OpenGLResourceManager implementation (basic skeleton)

OpenGLResourceManager::OpenGLResourceManager() = default;

OpenGLResourceManager::~OpenGLResourceManager() {
    cleanup_all();
}

void OpenGLResourceManager::cleanup_all() {
    // Clean up textures
    for (auto& [name, texture] : textures_) {
        if (texture.id != 0) {
            glDeleteTextures(1, &texture.id);
        }
    }
    textures_.clear();

    // Clean up buffers
    for (auto& [name, buffer] : buffers_) {
        // if (buffer.vao != 0) glDeleteVertexArrays(1, &buffer.vao);  // Requires GL extensions
        if (buffer.vbo != 0) glDeleteBuffers(1, &buffer.vbo);
        if (buffer.ebo != 0) glDeleteBuffers(1, &buffer.ebo);
    }
    buffers_.clear();
}

} // namespace trading::ui