// Copyright 2022 Google LLC
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
//     https://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.

#ifndef ppx_application_h
#define ppx_application_h

#include "ppx/base_application.h"
#include "ppx/command_line_parser.h"
#include "ppx/fs.h"
#include "ppx/imgui_impl.h"
#include "ppx/knob.h"
#include "ppx/math_config.h"
#include "ppx/metrics.h"
#include "ppx/timer.h"
#include "ppx/window.h"
#include "ppx/xr_component.h"

#include <deque>
#include <filesystem>
#include <cinttypes>

// clang-format off
#if !defined(PPX_ANDROID)
#if ! defined(GLFW_INCLUDE_NONE)
#   define GLFW_INCLUDE_NONE
#endif
#include <GLFW/glfw3.h>
#endif

#if defined(PPX_ANDROID)
    #define SETUP_APPLICATION(AppType)                                   \
        bool RunApp(android_app* pAndroidContext, int argc, char** argv) \
        {                                                                \
            AppType app;                                                 \
            app.SetAndroidContext(pAndroidContext);                      \
            int res = app.Run(argc, argv);                               \
            return res;                                                  \
        }
#else
    #define SETUP_APPLICATION(AppType)                  \
        int main(int argc, char** argv)                 \
        {                                               \
            AppType app;                                \
            int res = app.Run(argc, argv);              \
            return res;                                 \
        }
#endif
// clang-format on

namespace ppx {

/** @enum MouseButton
 *
 */
enum MouseButton
{
    MOUSE_BUTTON_LEFT   = 0x00000001,
    MOUSE_BUTTON_RIGHT  = 0x00000002,
    MOUSE_BUTTON_MIDDLE = 0x00000004,
};

/* @enum CursorMode
 *
 */
enum CursorMode
{
    CURSOR_MODE_VISIBLE = 0,
    CURSOR_MODE_HIDDEN,
    CURSOR_MODE_CAPTURED,
};

//! @enum Keyboard
//!
//!
enum KeyCode
{
    KEY_UNDEFINED   = 0,
    KEY_RANGE_FIRST = 32,
    KEY_SPACE       = KEY_RANGE_FIRST,
    KEY_APOSTROPHE,
    KEY_COMMA,
    KEY_MINUS,
    KEY_PERIOD,
    KEY_SLASH,
    KEY_0,
    KEY_1,
    KEY_2,
    KEY_3,
    KEY_4,
    KEY_5,
    KEY_6,
    KEY_7,
    KEY_8,
    KEY_9,
    KEY_SEMICOLON,
    KEY_EQUAL,
    KEY_A,
    KEY_B,
    KEY_C,
    KEY_D,
    KEY_E,
    KEY_F,
    KEY_G,
    KEY_H,
    KEY_I,
    KEY_J,
    KEY_K,
    KEY_L,
    KEY_M,
    KEY_N,
    KEY_O,
    KEY_P,
    KEY_Q,
    KEY_R,
    KEY_S,
    KEY_T,
    KEY_U,
    KEY_V,
    KEY_W,
    KEY_X,
    KEY_Y,
    KEY_Z,
    KEY_LEFT_BRACKET,
    KEY_BACKSLASH,
    KEY_RIGHT_BRACKET,
    KEY_GRAVE_ACCENT,
    KEY_WORLD_1,
    KEY_WORLD_2,
    KEY_ESCAPE,
    KEY_ENTER,
    KEY_TAB,
    KEY_BACKSPACE,
    KEY_INSERT,
    KEY_DELETE,
    KEY_RIGHT,
    KEY_LEFT,
    KEY_DOWN,
    KEY_UP,
    KEY_PAGE_UP,
    KEY_PAGE_DOWN,
    KEY_HOME,
    KEY_END,
    KEY_CAPS_LOCK,
    KEY_SCROLL_LOCK,
    KEY_NUM_LOCK,
    KEY_PRINT_SCREEN,
    KEY_PAUSE,
    KEY_F1,
    KEY_F2,
    KEY_F3,
    KEY_F4,
    KEY_F5,
    KEY_F6,
    KEY_F7,
    KEY_F8,
    KEY_F9,
    KEY_F10,
    KEY_F11,
    KEY_F12,
    KEY_F13,
    KEY_F14,
    KEY_F15,
    KEY_F16,
    KEY_F17,
    KEY_F18,
    KEY_F19,
    KEY_F20,
    KEY_F21,
    KEY_F22,
    KEY_F23,
    KEY_F24,
    KEY_F25,
    KEY_KEY_PAD_0,
    KEY_KEY_PAD_1,
    KEY_KEY_PAD_2,
    KEY_KEY_PAD_3,
    KEY_KEY_PAD_4,
    KEY_KEY_PAD_5,
    KEY_KEY_PAD_6,
    KEY_KEY_PAD_7,
    KEY_KEY_PAD_8,
    KEY_KEY_PAD_9,
    KEY_KEY_PAD_DECIMAL,
    KEY_KEY_PAD_DIVIDE,
    KEY_KEY_PAD_MULTIPLY,
    KEY_KEY_PAD_SUBTRACT,
    KEY_KEY_PAD_ADD,
    KEY_KEY_PAD_ENTER,
    KEY_KEY_PAD_EQUAL,
    KEY_LEFT_SHIFT,
    KEY_LEFT_CONTROL,
    KEY_LEFT_ALT,
    KEY_LEFT_SUPER,
    KEY_RIGHT_SHIFT,
    KEY_RIGHT_CONTROL,
    KEY_RIGHT_ALT,
    KEY_RIGHT_SUPER,
    KEY_MENU,
    KEY_RANGE_LAST = KEY_MENU,
    TOTAL_KEY_COUNT,
};

//! @struct KeyState
//!
//!
struct KeyState
{
    bool  down     = false;
    float timeDown = FLT_MAX;
};

// -------------------------------------------------------------------------------------------------
// Application
// -------------------------------------------------------------------------------------------------

//! @class ApplicationSettings
//!
//!
struct ApplicationSettings
{
    std::string appName               = "";
    bool        headless              = false;
    bool        enableImGui           = false;
    bool        allowThirdPartyAssets = false;
    // Set to true to opt-in for metrics.
    bool        enableMetrics        = false;
    bool        overwriteMetricsFile = false;
    std::string reportPath           = "";

    struct
    {
        bool enable             = false;
        bool enableDebugCapture = false;

        // Whether to create depth swapchains in addition to color swapchains,
        // and submit the depth info to the runtime as an additional layer.
        bool     enableDepthSwapchain = false;
        uint32_t uiWidth              = 0;
        uint32_t uiHeight             = 0;
    } xr;

    struct
    {
        uint32_t    width     = 0;
        uint32_t    height    = 0;
        std::string title     = "";
        bool        resizable = false;
    } window;

    struct
    {
        grfx::Api api               = grfx::API_UNDEFINED;
        bool      enableDebug       = false;
        uint32_t  numFramesInFlight = 1;
        uint32_t  pacedFrameRate    = 60;

        struct
        {
            uint32_t gpuIndex           = 0;
            uint32_t graphicsQueueCount = 1;
            uint32_t computeQueueCount  = 0;
            uint32_t transferQueueCount = 0;
        } device;

        struct
        {
            // NVIDIA only supports B8G8R8A8, ANDROID only supports R8G8B8A8, and
            // AMD supports both. So the default has to special-case either NVIDIA
            // or ANDROID :(
#if defined(PPX_ANDROID)
            grfx::Format colorFormat = grfx::FORMAT_R8G8B8A8_UNORM;
#else
            grfx::Format colorFormat = grfx::FORMAT_B8G8R8A8_UNORM;
#endif
            grfx::Format depthFormat = grfx::FORMAT_UNDEFINED;
            uint32_t     imageCount  = 2;
        } swapchain;
    } grfx;
};

//! @class Application
//!
//!
class Application
    : public BaseApplication
{
public:
    Application();
    Application(uint32_t windowWidth, uint32_t windowHeight, const char* windowTitle);
    virtual ~Application();

    static Application* Get();

    virtual void Config(ppx::ApplicationSettings& settings) {}
    virtual void Setup() {}
    virtual void Shutdown() {}
    virtual void Move(int32_t x, int32_t y) {}                                                // Window move event
    virtual void Resize(uint32_t width, uint32_t height) {}                                   // Window resize event
    virtual void WindowIconify(bool iconified) {}                                             // Window iconify event
    virtual void WindowMaximize(bool maximized) {}                                            // Window maximize event
    virtual void KeyDown(KeyCode key) {}                                                      // Key down event
    virtual void KeyUp(KeyCode key) {}                                                        // Key up event
    virtual void MouseMove(int32_t x, int32_t y, int32_t dx, int32_t dy, uint32_t buttons) {} // Mouse move event
    virtual void MouseDown(int32_t x, int32_t y, uint32_t buttons) {}                         // Mouse down event
    virtual void MouseUp(int32_t x, int32_t y, uint32_t buttons) {}                           // Mouse up event
    virtual void Scroll(float dx, float dy) {}                                                // Mouse wheel or touchpad scroll event
    virtual void Render() {}
    // Init knobs (adjustable parameters in the GUI that can be set at startup with commandline flags)
    virtual void InitKnobs() {}

protected:
    virtual void DispatchConfig();
    virtual void DispatchSetup();
    virtual void DispatchShutdown();
    virtual void DispatchMove(int32_t x, int32_t y);
    virtual void DispatchResize(uint32_t width, uint32_t height);
    virtual void DispatchWindowIconify(bool iconified);
    virtual void DispatchWindowMaximize(bool maximized);
    virtual void DispatchKeyDown(KeyCode key);
    virtual void DispatchKeyUp(KeyCode key);
    virtual void DispatchMouseMove(int32_t x, int32_t y, int32_t dx, int32_t dy, uint32_t buttons);
    virtual void DispatchMouseDown(int32_t x, int32_t y, uint32_t buttons);
    virtual void DispatchMouseUp(int32_t x, int32_t y, uint32_t buttons);
    virtual void DispatchScroll(float dx, float dy);
    virtual void DispatchRender();
    virtual void DispatchInitKnobs();
    virtual void DrawGui(){}; // Draw additional project-related information to ImGui.

    void TakeScreenshot();

    void DrawImGui(grfx::CommandBuffer* pCommandBuffer);
    void DrawDebugInfo();
    void DrawProfilerGrfxApiFunctions();

    KnobManager& GetKnobManager() { return mKnobManager; }

public:
    int  Run(int argc, char** argv);
    void Quit();

    std::vector<const char*> GetCommandLineArgs() const;
    const StandardOptions    GetStandardOptions() const;
    const CliOptions&        GetExtraOptions() const;

    const ApplicationSettings* GetSettings() const { return &mSettings; }
    uint32_t                   GetWindowWidth() const { return mSettings.window.width; }
    uint32_t                   GetWindowHeight() const { return mSettings.window.height; }
    bool                       IsWindowIconified() const;
    bool                       IsWindowMaximized() const;
    uint32_t                   GetUIWidth() const;
    uint32_t                   GetUIHeight() const;
    float                      GetWindowAspect() const { return static_cast<float>(mSettings.window.width) / static_cast<float>(mSettings.window.height); }
    grfx::Rect                 GetScissor() const;
    grfx::Viewport             GetViewport(float minDepth = 0.0f, float maxDepth = 1.0f) const;

    // Loads a DXIL or SPV shader from baseDir.
    //
    // 'baseDir' is the path to the directory that contains dxil, and spv subdirectories.
    // 'baseName' is the filename WITHOUT the dxil, and spv extension.
    //
    // Example(s):
    //   LoadShader("shaders", "Texture.vs")
    //     - loads shader file: shaders/dxil/Texture.vs.dxil for API_DX_12_0, API_DX_12_1
    //     - loads shader file: shaders/spv/Texture.vs.spv   for API_VK_1_1, API_VK_1_2
    //
    //   LoadShader("some/path/shaders", "Texture.vs")
    //     - loads shader file: some/path/shaders/dxil/Texture.vs.dxil for API_DX_12_0, API_DX_12_1
    //     - loads shader file: some/path/shaders/spv/Texture.vs.spv   for API_VK_1_1, API_VK_1_2
    //
    std::vector<char> LoadShader(const std::filesystem::path& baseDir, const std::filesystem::path& baseName) const;
    Result            CreateShader(const std::filesystem::path& baseDir, const std::filesystem::path& baseName, grfx::ShaderModule** ppShaderModule) const;

    Window*           GetWindow() const { return mWindow.get(); }
    grfx::InstancePtr GetInstance() const { return mInstance; }
    grfx::DevicePtr   GetDevice() const { return mDevice; }
    grfx::QueuePtr    GetGraphicsQueue(uint32_t index = 0) const { return GetDevice()->GetGraphicsQueue(index); }
    grfx::QueuePtr    GetComputeQueue(uint32_t index = 0) const { return GetDevice()->GetComputeQueue(index); }
    grfx::QueuePtr    GetTransferQueue(uint32_t index = 0) const { return GetDevice()->GetTransferQueue(index); }

    // "index" here is for XR applications to fetch the swapchain of different views.
    // For non-XR applications, "index" should be always 0.
    grfx::SwapchainPtr GetSwapchain(uint32_t index = 0) const;
    Result             Present(
                    const grfx::SwapchainPtr&     swapchain,
                    uint32_t                      imageIndex,
                    uint32_t                      waitSemaphoreCount,
                    const grfx::Semaphore* const* ppWaitSemaphores);

    float    GetElapsedSeconds() const;
    float    GetPrevFrameTime() const { return mPreviousFrameTime; }
    uint64_t GetFrameCount() const { return mFrameCount; }
    float    GetAverageFPS() const { return mAverageFPS; }
    float    GetAverageFrameTime() const { return mAverageFrameTime; }
    uint32_t GetNumFramesInFlight() const { return mSettings.grfx.numFramesInFlight; }
    uint32_t GetInFlightFrameIndex() const { return static_cast<uint32_t>(mFrameCount % mSettings.grfx.numFramesInFlight); }
    uint32_t GetPreviousInFlightFrameIndex() const { return static_cast<uint32_t>((mFrameCount - 1) % mSettings.grfx.numFramesInFlight); }

    const KeyState& GetKeyState(KeyCode code) const;
    float2          GetNormalizedDeviceCoordinates(int32_t x, int32_t y) const;

    bool IsXrEnabled() const { return mSettings.xr.enable; }

    // Starts a new metric run and returns it.
    // Only one run is ever active at the same time.
    // Default metrics are automatically added to the run: framerate, cpu_frame_time and frame_count.
    // Additional ones may be added by the caller.
    // The run is automatically exported and saved to disk when the application shuts down.
    metrics::Run* StartMetricsRun(const std::string& name);

    // Stops the currently active run.
    // The caller must not use the run, or any associated metrics, after calling this function.
    void StopMetricsRun();

    // Returns true when a run is active, otherwise returns false.
    bool HasActiveMetricsRun() const;

#if defined(PPX_BUILD_XR)
    XrComponent& GetXrComponent()
    {
        return mXrComponent;
    }

    grfx::SwapchainPtr GetDebugCaptureSwapchain() const
    {
        return GetSwapchain(mDebugCaptureSwapchainIndex);
    }

    grfx::SwapchainPtr GetUISwapchain() const
    {
        return GetSwapchain(mUISwapchainIndex);
    }
#else
    // Alias for UI component in non-XR contexts.
    grfx::SwapchainPtr GetUISwapchain() const
    {
        return GetSwapchain();
    }
#endif
private:
    void   InternalCtor();
    void   InitializeAssetDirs();
    Result InitializePlatform();
    Result InitializeGrfxDevice();
    Result InitializeGrfxSurface();
    Result CreateSwapchains();
    void   DestroySwapchains();
    Result InitializeImGui();
    void   ShutdownImGui();
    void   StopGrfx();
    void   ShutdownGrfx();
    Result CreatePlatformWindow();
    void   DestroyPlatformWindow();
    bool   IsRunning() const;

    // Updates the metrics.
    void UpdateMetrics();
    // Saves the metrics data to a file on disk.
    void SaveMetricsReportToDisk();

private:
    friend struct WindowEvents;
    //
    // These functions exists so that applications can override the
    // corresponding Dispatch* methods without interfering with the
    // internal bookkeeping the app needs to do for these events.
    //
    void MoveCallback(int32_t x, int32_t y);
    void ResizeCallback(uint32_t width, uint32_t height);
    void WindowIconifyCallback(bool iconified);
    void WindowMaximizeCallback(bool maximized);
    void KeyDownCallback(KeyCode key);
    void KeyUpCallback(KeyCode key);
    void MouseMoveCallback(int32_t x, int32_t y, uint32_t buttons);
    void MouseDownCallback(int32_t x, int32_t y, uint32_t buttons);
    void MouseUpCallback(int32_t x, int32_t y, uint32_t buttons);
    void ScrollCallback(float dx, float dy);

private:
    CommandLineParser               mCommandLineParser;
    StandardOptions                 mStandardOptions;
    uint64_t                        mMaxFrames;
    float                           mRunTimeSeconds;
    ApplicationSettings             mSettings = {};
    std::string                     mDecoratedApiName;
    Timer                           mTimer;
    std::unique_ptr<Window>         mWindow                     = nullptr; // Requires enableDisplay
    bool                            mWindowSurfaceInvalid       = false;
    KeyState                        mKeyStates[TOTAL_KEY_COUNT] = {false, 0.0f};
    int32_t                         mPreviousMouseX             = INT32_MAX;
    int32_t                         mPreviousMouseY             = INT32_MAX;
    grfx::InstancePtr               mInstance                   = nullptr;
    grfx::DevicePtr                 mDevice                     = nullptr;
    grfx::SurfacePtr                mSurface                    = nullptr; // Requires enableDisplay
    std::vector<grfx::SwapchainPtr> mSwapchains;                           // Requires enableDisplay
    std::unique_ptr<ImGuiImpl>      mImGui;
    KnobManager                     mKnobManager;

    uint64_t          mFrameCount        = 0;
    uint32_t          mSwapchainIndex    = 0;
    float             mAverageFPS        = 0;
    float             mFrameStartTime    = 0;
    float             mFrameEndTime      = 0;
    float             mPreviousFrameTime = 0;
    float             mAverageFrameTime  = 0;
    double            mFirstFrameTime    = 0;
    std::deque<float> mFrameTimesMs;

    // Metrics
    struct
    {
        metrics::Manager        manager;
        metrics::Run*           pCurrentRun   = nullptr;
        metrics::MetricGauge*   pCpuFrameTime = nullptr;
        metrics::MetricGauge*   pFramerate    = nullptr;
        metrics::MetricCounter* pFrameCount   = nullptr;

        double   framerateRecordTimer   = 0.0;
        uint64_t framerateFrameCount    = 0;
        bool     resetFramerateTracking = true;
    } mMetrics;

#if defined(PPX_MSW)
    //
    // D3D12 requires forced invalidation of client area
    // window is resized to render contents correctly. See
    // Application::Run() loop for more details.
    //
    bool mForceInvalidateClientArea = false;
#endif

#if defined(PPX_BUILD_XR)
    XrComponent mXrComponent;
    uint32_t    mDebugCaptureSwapchainIndex = 0;
    uint32_t    mUISwapchainIndex           = 0;
    uint32_t    mStereoscopicSwapchainIndex = 0;
    ImVec2      lastImGuiWindowSize         = {};
#endif
};

const char* GetKeyCodeString(KeyCode code);

} // namespace ppx

#endif // ppx_application_h
