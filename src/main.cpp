#define NOMINMAX

#include "glh/classes/OpenGLApplication.h"

#include <iostream>
#include <algorithm>
#include <limits>

int x = 0;
int y = 0;
int width = 200;
int height = 200;
ImVec4 selectorWindowColor = ImVec4(0.4f, 0.7f, 1.0f, 0.5f);
bool showSelectorWindow = true;

void setGLFWWindowColorAndOpacity(GLFWwindow* window, const ImVec4& color) {
    GLFWwindow* returnContext = glfwGetCurrentContext();
    glfwMakeContextCurrent(returnContext);

    glfwMakeContextCurrent(window);
    glClearColor(color.x, color.y, color.z, color.w);
    glClear(GL_COLOR_BUFFER_BIT);
    glfwSetWindowOpacity(window, color.w);
    glfwSwapBuffers(window);

    glfwMakeContextCurrent(returnContext);
}

void render(GLFWwindow* window) {
    // Get the list of monitors
    int monitorCount;
    GLFWmonitor** monitors = glfwGetMonitors(&monitorCount);
    if (!monitors) {
        std::cerr << "Failed to get monitors!" << std::endl;
        glfwTerminate();
        return;
    }

    // Variables to calculate the bounding box of the virtual screen space
    int minX = std::numeric_limits<int>::max(), minY = std::numeric_limits<int>::max();
    int maxX = std::numeric_limits<int>::min(), maxY = std::numeric_limits<int>::min();

    for (int i = 0; i < monitorCount; ++i) {
        // Get the monitor position and video mode
        int x, y;
        glfwGetMonitorPos(monitors[i], &x, &y);
        const GLFWvidmode* videoMode = glfwGetVideoMode(monitors[i]);

        if (videoMode) {
            // Update the bounding box
            minX = std::min(minX, x);
            minY = std::min(minY, y);
            maxX = std::max(maxX, x + videoMode->width);
            maxY = std::max(maxY, y + videoMode->height);
        }
    }

    // Calculate the maximum width and height
    int maxWidth = maxX - minX;
    int maxHeight = maxY - minY;

    // Print the results
    std::cout << "Maximum virtual screen width: " << maxWidth << std::endl;
    std::cout << "Maximum virtual screen height: " << maxHeight << std::endl;

    glfwWindowHint(GLFW_DECORATED, GLFW_FALSE);
    GLFWwindow* selectorWindow = glfwCreateWindow(width, height, "selector window", nullptr, nullptr);
    if (selectorWindow == nullptr) {
        throw std::runtime_error("ERROR OpenGLApplication::initGLFW() Could not create window!");
    }
    setGLFWWindowColorAndOpacity(selectorWindow, selectorWindowColor);

    glfwMakeContextCurrent(window);
    ImGuiIO& io = ImGui::GetIO();

    while (!glfwWindowShouldClose(window)) {
        glfwMakeContextCurrent(window);

        // Start the Dear ImGui frame
        ImGui_ImplOpenGL3_NewFrame();
        ImGui_ImplGlfw_NewFrame();
        ImGui::NewFrame();

        // Create window which fills viewport
        ImGui::SetNextWindowPos(ImVec2(0, 0));
        ImGui::SetNextWindowSize(io.DisplaySize);
        ImGui::Begin("Imgui Window", nullptr, ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoMove);

        if (ImGui::SliderInt("X", &x, 0, maxWidth) | ImGui::SliderInt("Y", &y, 0, maxHeight)) {
            glfwSetWindowPos(selectorWindow, x, y);
        }
        if (ImGui::SliderInt("Width", &width, 0, maxWidth) | ImGui::SliderInt("Height", &height, 0, maxHeight)) {
            glfwSetWindowSize(selectorWindow, width, height);
        }

        if (ImGui::ColorEdit4("Selector Window Color", (float*)&selectorWindowColor)) {
            setGLFWWindowColorAndOpacity(selectorWindow, selectorWindowColor);
        }

        if (ImGui::Checkbox("Toggle Selector Overlay", &showSelectorWindow)) {
            if (showSelectorWindow) {
                glfwShowWindow(selectorWindow);
                glfwFocusWindow(window);
            }
            else {
                glfwHideWindow(selectorWindow);
            }
        }

        ImGui::End();

        // Rendering
        ImGui::Render();
        int display_w, display_h;
        glfwGetFramebufferSize(window, &display_w, &display_h);
        glViewport(0, 0, display_w, display_h);
        glClearColor(0.45f, 0.55f, 0.60f, 1.00f);
        glClear(GL_COLOR_BUFFER_BIT);
        ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());
        glfwSwapBuffers(window);
    }
}

int main()
{
    OpenGLApplication::ApplicationConfig config;
    config.windowName = "WindowTouchRemapper Configurator";
    config.windowInitWidth = 800;
    config.windowInitHeight = 800;
    config.windowPosX = 100;
    config.windowPosY = 100;
    config.windowBorderless = false;
    config.windowResizeEnable = false;
    config.windowDarkmode = true;
    config.windowRounded = true;
    config.windowAlwaysOnTop = true;
    config.vsyncEnable = true;
    config.transparentFramebuffer = false;
    config.glVersionMajor = 4;
    config.glVersionMinor = 6;
    config.glslVersionString = "#version 460"; // Used for DearImgui, leave default unless you know what to put here
    config.customDrawFunc = render;
    config.customKeyCallback = nullptr; // std::function<void(GLFWwindow* window, int key, int scancode, int action, int mods)>
    config.customErrorCallback = nullptr; // std::function<void(int error_code, const char* description)>
    config.customDropCallback = nullptr; // std::function<void(GLFWwindow* window, int count, const char** paths)>

    try {
        OpenGLApplication application(config);
    }
    catch (const std::exception& e) {
        std::cout << e.what() << std::endl;
    }

    return EXIT_SUCCESS;
}