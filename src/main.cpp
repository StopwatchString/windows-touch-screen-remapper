#define NOMINMAX

#include "glh/classes/OpenGLApplication.h"

#include "glh/utils.h"

#include <iostream>
#include <algorithm>
#include <limits>

GLFWwindow* configWindow = nullptr;
GLFWwindow* selectorWindow = nullptr;

int sourceScreenWidth = 0;
int sourceScreenHeight = 0;

int maxVirtualDisplayWidth = 0;
int maxVirtualDisplayHeight = 0;
HHOOK mouseHook = NULL;

// Polling Thread
bool mouseCallbackActive = true;
bool mouseCallbackHasChanged = true;

// Selector Window
int selectorWindowX = 0;
int selectorWindowY = 0;
int selectorWindowWidth = 400;
int selectorWindowHeight = 400;
bool showSelectorWindow = true;
ImVec4 selectorWindowColor = ImVec4(0.4f, 0.7f, 1.0f, 0.5f);
bool selectorWindowSizeChanged = true;
bool selectorWindowPositionChanged = true;
bool selectorWindowColorChanged = true;
bool selectorWindowVisibilityChanged = true;

// Remap demo box
int boxX = 0;
int boxY = 0;
int boxWidth = 0;
int boxHeight = 0;
int boxFramesToLive = 0;


//--------------------------------------------------------
// scale()
//--------------------------------------------------------
float scale(float oldLower, float oldUpper, float newLower, float newUpper, float input)
{
    input -= oldLower;               // Move old lower bound to 0
    input /= (oldUpper - oldLower);  // Normalize value between 0 and 1
    input *= (newUpper - newLower);  // Scale normalized value to new bound size
    input += newLower;               // Move scale to new lower bound
    return input;
}

//--------------------------------------------------------
// MouseProc()
//--------------------------------------------------------
LRESULT CALLBACK MouseProc(int nCode, WPARAM wParam, LPARAM lParam)
{
    if (nCode >= 0) {
        if (wParam == WM_LBUTTONDOWN) {
            MSLLHOOKSTRUCT* mouseInfo = (MSLLHOOKSTRUCT*)lParam;
            boxX = mouseInfo->pt.x;
            boxY = sourceScreenHeight - mouseInfo->pt.y - 1;
            boxFramesToLive = 120;
            std::cout << "mouse callbac" << std::endl;
        }
    }
    return CallNextHookEx(mouseHook, nCode, wParam, lParam);
}

//--------------------------------------------------------
// registerMouseCallback()
//--------------------------------------------------------
void registerMouseCallback()
{
    mouseHook = SetWindowsHookEx(WH_MOUSE_LL, MouseProc, NULL, 0);
    if (mouseHook == NULL) {
        std::cout << "Failed to register Mouse callback!" << std::endl;
    }
}

//--------------------------------------------------------
// unregisterMouseCallback()
//--------------------------------------------------------
void unregisterMouseCallback()
{
    if (!UnhookWindowsHookEx(mouseHook)) {
        std::cout << "Failed to unregister Mouse callback!" << std::endl;
    }
    mouseHook = NULL;
}

//--------------------------------------------------------
// calculateVirtualWindowExtents()
//--------------------------------------------------------
void calculateVirtualWindowExtents()
{
    GLFWmonitor* primaryMonitor = glfwGetPrimaryMonitor();
    if (primaryMonitor != nullptr) {
        const GLFWvidmode* primaryMonitorVidMode = glfwGetVideoMode(primaryMonitor);
        if (primaryMonitorVidMode != nullptr) {
            sourceScreenWidth = primaryMonitorVidMode->width;
            sourceScreenHeight = primaryMonitorVidMode->height;
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

    maxVirtualDisplayWidth = maxX - minX;
    maxVirtualDisplayHeight = maxY - minY;
}

//--------------------------------------------------------
// renderConfigWindow()
//--------------------------------------------------------
void renderConfigWindow()
{
    glhErrorCheck("Start renderConfigWindow()");

    glfwMakeContextCurrent(configWindow);

    // Start the Dear ImGui frame
    ImGui_ImplOpenGL3_NewFrame();
    ImGui_ImplGlfw_NewFrame();
    ImGui::NewFrame();

    // Create window which fills viewport
    ImGui::SetNextWindowPos(ImVec2(0, 0));
    ImGui::SetNextWindowSize(ImGui::GetIO().DisplaySize);
    ImGui::Begin("Imgui Window", nullptr, ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoMove);

    selectorWindowPositionChanged |= ImGui::SliderInt("X", &selectorWindowX, 0, maxVirtualDisplayWidth);
    selectorWindowPositionChanged |= ImGui::SliderInt("Y", &selectorWindowY, 0, maxVirtualDisplayHeight);

    selectorWindowSizeChanged |= ImGui::SliderInt("Width", &selectorWindowWidth, 0, maxVirtualDisplayWidth);
    selectorWindowSizeChanged |= ImGui::SliderInt("Height", &selectorWindowHeight, 0, maxVirtualDisplayHeight);

    selectorWindowColorChanged = ImGui::ColorEdit4("Selector Window Color", (float*)&selectorWindowColor);

    selectorWindowVisibilityChanged = ImGui::Checkbox("Toggle Selector Overlay", &showSelectorWindow);

    mouseCallbackHasChanged = ImGui::Checkbox("Toggle Mouse Callback", &mouseCallbackActive);

    ImGui::End();

    // Rendering
    ImGui::Render();
    int display_w, display_h;
    glfwGetFramebufferSize(configWindow, &display_w, &display_h);
    glViewport(0, 0, display_w, display_h);
    glClearColor(0.45f, 0.55f, 0.60f, 1.00f);
    glClear(GL_COLOR_BUFFER_BIT);
    ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());

    glfwSwapBuffers(configWindow);
    glhErrorCheck("End renderConfigWindow()");
}

//--------------------------------------------------------
// renderSelectorWindow()
//--------------------------------------------------------
void renderSelectorWindow()
{
    glhErrorCheck("Start renderSelectorWindow()");
    glfwMakeContextCurrent(selectorWindow);

    if (selectorWindowPositionChanged) {
        glfwSetWindowPos(selectorWindow, selectorWindowX, selectorWindowY);
        selectorWindowPositionChanged = false;
    }

    if (selectorWindowSizeChanged) {
        glfwSetWindowSize(selectorWindow, selectorWindowWidth, selectorWindowHeight);
        selectorWindowSizeChanged = false;
    }

    if (selectorWindowColorChanged) {
        glClearColor(selectorWindowColor.x, selectorWindowColor.y, selectorWindowColor.z, selectorWindowColor.w);
        glfwSetWindowOpacity(selectorWindow, selectorWindowColor.w);
        selectorWindowColorChanged = false;
    }

    if (selectorWindowVisibilityChanged) {
        if (showSelectorWindow) {
            glfwShowWindow(selectorWindow);
            // Focus config window so that gui remains responsive
            glfwFocusWindow(configWindow);
        }
        else {
            glfwHideWindow(selectorWindow);
        }
        selectorWindowVisibilityChanged = false;
    }

    glClear(GL_COLOR_BUFFER_BIT);

    // Box render
    if (boxFramesToLive > 0) {
        float u = static_cast<float>(boxX) / static_cast<float>(sourceScreenWidth);
        float v = static_cast<float>(boxY) / static_cast<float>(sourceScreenHeight);



        boxFramesToLive--;
    }

    glfwSwapBuffers(selectorWindow);

    glhErrorCheck("End renderSelectorWindow()");
}

//--------------------------------------------------------
// forceUpdateSelectorWindow()
//--------------------------------------------------------
void forceUpdateSelectorWindow()
{
    selectorWindowSizeChanged = true;
    selectorWindowPositionChanged = true;
    selectorWindowColorChanged = true;
    selectorWindowVisibilityChanged = true;

    renderSelectorWindow();
}

//--------------------------------------------------------
// render()
//--------------------------------------------------------
void render(GLFWwindow* window)
{
    configWindow = window;

    calculateVirtualWindowExtents();
    std::cout << "Maximum virtual window extents: " << maxVirtualDisplayWidth << "x" << maxVirtualDisplayHeight << std::endl;

    // Create selector window
    glfwWindowHint(GLFW_DECORATED, GLFW_FALSE);
    selectorWindow = glfwCreateWindow(selectorWindowWidth, selectorWindowHeight, "Selector Window", nullptr, nullptr);
    if (selectorWindow == nullptr) {
        throw std::runtime_error("ERROR OpenGLApplication::initGLFW() Could not create window!");
    }
    forceUpdateSelectorWindow();
    forceUpdateSelectorWindow();

    while (!glfwWindowShouldClose(configWindow)) {
        renderConfigWindow();

        renderSelectorWindow();

        glhErrorCheck("End of render");
    }
}

//--------------------------------------------------------
// polling()
//--------------------------------------------------------
void polling()
{
    if (mouseCallbackHasChanged) {
        if (mouseCallbackActive) {
            registerMouseCallback();
        }
        else {
            unregisterMouseCallback();
        }
        mouseCallbackHasChanged = false;
    }
}

//--------------------------------------------------------
// main()
//--------------------------------------------------------
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
    config.glslVersionString = "#version 460";
    config.customDrawFunc = render;
    config.customKeyCallback = nullptr;
    config.customErrorCallback = nullptr;
    config.customDropCallback = nullptr;
    config.customPollingFunc = polling;

    try {
        OpenGLApplication application(config);
    }
    catch (const std::exception& e) {
        std::cout << e.what() << std::endl;
    }

    return EXIT_SUCCESS;
}