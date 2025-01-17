#define NOMINMAX

#include "glh/classes/OpenGLApplication.h"

#include "glh/utils.h"

#include <iostream>
#include <algorithm>
#include <limits>


int x = 0;
int y = 0;
int width = 200;
int height = 200;
int touchMonitorWidth = 0;
int touchMonitorHeight = 0;
ImVec4 selectorWindowColor = ImVec4(0.4f, 0.7f, 1.0f, 0.5f);
bool showSelectorWindow = true;
HHOOK mouseHook = NULL;

float squareX = 0;
float squareY = 0;
int framesToDrawSquare = 0;

float scale(float oldLower, float oldUpper, float newLower, float newUpper, float input) {
    input -= oldLower;               // Move old lower bound to 0
    input /= (oldUpper - oldLower);  // Normalize value between 0 and 1
    input *= (newUpper - newLower);  // Scale normalized value to new bound size
    input += newLower;               // Move scale to new lower bound
    return input;
}

LRESULT CALLBACK MouseProc(int nCode, WPARAM wParam, LPARAM lParam) {
    if (nCode >= 0) {
        if (wParam == WM_LBUTTONDOWN) {
            MSLLHOOKSTRUCT* mouseInfo = (MSLLHOOKSTRUCT*)lParam;
            squareX = mouseInfo->pt.x;
            squareY = mouseInfo->pt.y;
            framesToDrawSquare = 120;
        }
    }
    return CallNextHookEx(mouseHook, nCode, wParam, lParam);
}

void setGLFWWindowColorAndOpacity(GLFWwindow* window, const ImVec4& color) {
    GLFWwindow* returnContext = glfwGetCurrentContext();
    glfwMakeContextCurrent(returnContext);

    glfwMakeContextCurrent(window);
    glClearColor(color.x, color.y, color.z, color.w);
    glClear(GL_COLOR_BUFFER_BIT);
    glfwSetWindowOpacity(window, color.w);
    //glfwSwapBuffers(window);

    glfwMakeContextCurrent(returnContext);
}

void render(GLFWwindow* window) {
    GLFWmonitor* primaryMonitor = glfwGetPrimaryMonitor();
    if (primaryMonitor != nullptr) {
        const GLFWvidmode* primaryMonitorVidMode = glfwGetVideoMode(primaryMonitor);
        if (primaryMonitorVidMode != nullptr) {
            touchMonitorWidth = primaryMonitorVidMode->width;
            touchMonitorHeight = primaryMonitorVidMode->height;
        }
    }

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
        glhErrorCheck("Start of render");

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

        if (framesToDrawSquare > 0) {
            glfwMakeContextCurrent(selectorWindow);

            glViewport(0, 0, width, height);

            //double centerX = squareX / (double)touchMonitorWidth;
            //double centerY = squareY / (double)touchMonitorHeight;

            double squareSize = 50.0;

            double leftPx = squareX - squareSize;
            double rightPx = squareX + squareSize;
            double bottomPx = squareY - squareSize;
            double topPx = squareY + squareSize;

            float leftNdc = scale(0.0f, 1.0f, -1.0f, 1.0f, leftPx / (double)touchMonitorWidth);
            float rightNdc = scale(0.0f, 1.0f, -1.0f, 1.0f, rightPx / (double)touchMonitorWidth);
            float bottomNdc = -1.0f * scale(0.0f, 1.0f, -1.0f, 1.0f, bottomPx / (double)touchMonitorHeight);
            float topNdc = -1.0f * scale(0.0f, 1.0f, -1.0f, 1.0f, topPx / (double)touchMonitorHeight);

            glBegin(GL_QUADS);

            glColor4f(1.0f, 0.0f, 0.0f, 1.0f);
            glVertex2f(leftNdc, bottomNdc);
            glColor4f(1.0f, 0.0f, 0.0f, 1.0f);
            glVertex2f(leftNdc, topNdc);
            glColor4f(1.0f, 0.0f, 0.0f, 1.0f);
            glVertex2f(rightNdc, topNdc);
            glColor4f(1.0f, 0.0f, 0.0f, 1.0f);
            glVertex2f(rightNdc, bottomNdc);

            glEnd();

            glfwMakeContextCurrent(window);
            framesToDrawSquare--;
        }
        else if (framesToDrawSquare == 0) {
            glfwMakeContextCurrent(selectorWindow);
            glClearColor(selectorWindowColor.x, selectorWindowColor.y, selectorWindowColor.z, selectorWindowColor.w);
            glClear(GL_COLOR_BUFFER_BIT);
            glfwMakeContextCurrent(window);
            framesToDrawSquare--;
        }
        glfwSwapBuffers(window);
        glfwSwapBuffers(selectorWindow);
        glhErrorCheck("End of render");

    }
}

int main()
{
    mouseHook = SetWindowsHookEx(WH_MOUSE_LL, MouseProc, NULL, 0);
    if (!mouseHook) {
        printf("Failed to set hook\n");
        return -1;
    }

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
    config.glslVersionString = "#version 460";
    config.customDrawFunc = render;
    config.customKeyCallback = nullptr;
    config.customErrorCallback = nullptr;
    config.customDropCallback = nullptr;

    try {
        OpenGLApplication application(config);
    }
    catch (const std::exception& e) {
        std::cout << e.what() << std::endl;
    }

    return EXIT_SUCCESS;
}