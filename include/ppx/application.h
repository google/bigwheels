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
#include "ppx/input.h"
#include "ppx/knob.h"
#include "ppx/math_config.h"
#include "ppx/metrics.h"
#include "ppx/timer.h"
#include "ppx/window.h"
#include "ppx/xr_component.h"

#include <deque>
#include <filesystem>
#include <cinttypes>
#include <vector>

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

// -------------------------------------------------------------------------------------------------
// StandardOptions
// -------------------------------------------------------------------------------------------------
struct StandardOptions
{
    // Flags
    std::shared_ptr<KnobFlag<bool>> pListGpus;
    std::shared_ptr<KnobFlag<bool>> pUseSoftwareRenderer;
    std::shared_ptr<KnobFlag<bool>> pHeadless;
    std::shared_ptr<KnobFlag<bool>> pDeterministic;
    std::shared_ptr<KnobFlag<bool>> pEnableMetrics;
    std::shared_ptr<KnobFlag<bool>> pOverwriteMetricsFile;

    // Options
    std::shared_ptr<KnobFlag<uint32_t>> pGpuIndex;
    std::shared_ptr<KnobFlag<uint64_t>> pFrameCount;
    std::shared_ptr<KnobFlag<uint32_t>> pRunTimeMs;
    std::shared_ptr<KnobFlag<int>>      pStatsFrameWindow;
    std::shared_ptr<KnobFlag<int>>      pScreenshotFrameNumber;

    std::shared_ptr<KnobFlag<std::string>> pScreenshotPath;
    std::shared_ptr<KnobFlag<std::string>> pMetricsFilename;

    std::shared_ptr<KnobFlag<std::pair<int, int>>> pResolution;
#if defined(PPX_BUILD_XR)
    std::shared_ptr<KnobFlag<std::pair<int, int>>>      pXrUiResolution;
    std::shared_ptr<KnobFlag<std::vector<std::string>>> pXrRequiredExtensions;
#endif

    std::shared_ptr<KnobFlag<std::vector<std::string>>> pAssetsPaths;
    std::shared_ptr<KnobFlag<std::vector<std::string>>> pConfigJsonPaths;

    std::shared_ptr<KnobFlag<std::string>> pShadingRateMode;
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

#if defined(PPX_ANDROID)
    bool emulateMouseAndroid = true;
#endif

    struct
    {
        bool enable = false;

        // Multiview will create one swapchain with layers per view
        // One Application::Render then should use multiview shaders
        // to render to both layers, as opposed to non multiview
        // where there is one swapchain per view, each with a ::Render
        bool enableMultiView = false;
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
        grfx::Api api = grfx::API_UNDEFINED;

#if !defined(NDEBUG) && !defined(PPX_ANDROID)
        // Enable debug for debug builds, unless it is an android build.
        // Validation on android requires additional setup, so it's not
        // turned on by default.
        bool enableDebug = true;
#else
        bool enableDebug = false;
#endif

        uint32_t numFramesInFlight = 1;
        uint32_t pacedFrameRate    = 60;

        struct
        {
            uint32_t gpuIndex           = 0;
            uint32_t graphicsQueueCount = 1;
            uint32_t computeQueueCount  = 0;
            uint32_t transferQueueCount = 0;

            // Enable support for this shading rate mode on the device.
            // The application must not use FDM or VRS without setting this to
            // the corresponding shading rate mode.
            grfx::ShadingRateMode supportShadingRateMode = grfx::SHADING_RATE_NONE;
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

        // imGuiDynamicRendering controls whether ImGui window is
        // drawn within a dynamic render pass. Dynamic render pass
        // must have begun with a single color attachment (no depth
        // stencil attachment).
        bool enableImGuiDynamicRendering = false;
    } grfx;

    // Default values for standard knobs
    struct StandardKnobsDefaultValue
    {
        std::vector<std::string> assetsPaths           = {};
        std::vector<std::string> configJsonPaths       = {};
        bool                     deterministic         = false;
        bool                     enableMetrics         = false;
        uint64_t                 frameCount            = 0;
        uint32_t                 gpuIndex              = 0;
        bool                     headless              = false;
        bool                     listGpus              = false;
        std::string              metricsFilename       = "report_@.json";
        bool                     overwriteMetricsFile  = false;
        std::pair<int, int>      resolution            = std::make_pair(0, 0);
        uint32_t                 runTimeMs             = 0;
        int                      screenshotFrameNumber = -1;
        std::string              screenshotPath        = "screenshot_frame_#.ppm";
        int                      statsFrameWindow      = -1;
        bool                     useSoftwareRenderer   = false;
#if defined(PPX_BUILD_XR)
        std::pair<int, int>      xrUiResolution       = std::make_pair(0, 0);
        std::vector<std::string> xrRequiredExtensions = {};
#endif
    } standardKnobsDefaultValue;
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
    virtual void DispatchUpdateMetrics();
    virtual void DrawGui(){}; // Draw additional project-related information to ImGui.

    // Override these methods in a derived class to change the default behavior of metrics.
    // Virtual for unit testing purposes.
    virtual void SetupMetrics();
    // Virtual for unit testing purposes.
    virtual void ShutdownMetrics();

    // NOTE: This function can be used for BOTH displayed AND recorded metrics.
    // Thus it should always be called once per frame. Virtual for unit testing purposes.
    virtual void UpdateMetrics() {}

    virtual metrics::GaugeBasicStatistics GetGaugeBasicStatistics(metrics::MetricID id) const;

    void TakeScreenshot();
    void SaveImage(grfx::ImagePtr image, const std::string& filepath, grfx::ResourceState resourceState) const;

    void DrawImGui(grfx::CommandBuffer* pCommandBuffer);
    void DrawDebugInfo();
    void DrawProfilerGrfxApiFunctions();

    KnobManager& GetKnobManager() { return mKnobManager; }

public:
    int  Run(int argc, char** argv);
    void Quit();

    std::vector<const char*> GetCommandLineArgs() const;
    const CliOptions&        GetExtraOptions() const;

    const ApplicationSettings* GetSettings() const { return &mSettings; }
    const StandardOptions&     GetStandardOptions() const { return mStandardOpts; }
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

    // Starts a new metric run and returns it. Only one run may be active at the same time.
    // This function wraps the metrics manager to add default metrics to the run:
    // framerate, cpu_frame_time and frame_count. Additional ones may be added by calling
    // the other wrapper functions below.
    // The run is automatically exported and saved to disk when the application shuts down.
    void StartMetricsRun(const std::string& name);

    // Stops the currently active run, invaliding all existing MetricIDs.
    // See StartMetricsRun for why this wrapper is necessary.
    void StopMetricsRun();

    // Returns true when a run is active, otherwise returns false.
    // See StartMetricsRun for why this wrapper is necessary.
    virtual bool HasActiveMetricsRun() const;

    // Adds a metric to the current run. If no run is active, returns metrics::kInvalidMetricID.
    // See StartMetricsRun for why this wrapper is necessary.
    virtual metrics::MetricID AddMetric(const metrics::MetricMetadata& metadata);

    // Record data for the given metric ID. Metrics for completed runs will be discarded.
    // See StartMetricsRun for why this wrapper is necessary.
    virtual bool RecordMetricData(metrics::MetricID id, const metrics::MetricData& data);

#if defined(PPX_BUILD_XR)
    // virtual is used for testing
    virtual XrComponent& GetXrComponent()
    {
        return mXrComponent;
    }

    const XrComponent& GetXrComponent() const
    {
        return mXrComponent;
    }

    grfx::SwapchainPtr GetUISwapchain() const
    {
        return GetSwapchain(mUISwapchainIndex);
    }

    ppx::float4x4 GetViewProjectionMatrix(uint32_t viewIndex, float nearZ, float farZ)
    {
        ppx::float4x4 viewProjectionMatrix(1);
        viewProjectionMatrix = mXrComponent.GetViewProjectionMatrix(viewIndex, nearZ, farZ);
        return viewProjectionMatrix;
    }

    uint32_t GetCurrentViewIndex() const
    {
        uint32_t current_view_index = 0;
        current_view_index          = (IsXrEnabled() ? mXrComponent.GetCurrentViewIndex() : 0);

        return current_view_index;
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
    Result InitializeWindow();
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

    // Config() allows derived classes to update settings
    // Updates the standard application settings to reflect the knob values.
    void UpdateStandardSettings();

    // Add the asset directories
    void AddAssetDirs();

    // Updates the shared, app-level metrics.
    void UpdateAppMetrics();
    // Saves the metrics data to a file on disk.
    void SaveMetricsReportToDisk();

    // Initializes standard knobs
    void InitStandardKnobs();

    // List gpus
    void ListGPUs() const;

    // Process events which could change the Running status of the application
    void ProcessEvents();

    // Render the frame, handles both XR and non-XR cases
    void RenderFrame();

    void MainLoop();

#if defined(PPX_BUILD_XR)
    void InitializeXRComponentBeforeGrfxDeviceInit();
    void InitializeXRComponentAndUpdateSettingsAfterGrfxDeviceInit();
    void DestroyXRComponent();
#endif

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
    StandardOptions                 mStandardOpts;
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
        metrics::Manager  manager;
        metrics::MetricID cpuFrameTimeId = metrics::kInvalidMetricID;
        metrics::MetricID framerateId    = metrics::kInvalidMetricID;
        metrics::MetricID frameCountId   = metrics::kInvalidMetricID;

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
    uint32_t    mUISwapchainIndex           = 0;
    uint32_t    mStereoscopicSwapchainIndex = 0;
    ImVec2      lastImGuiWindowSize         = {};
#endif
};

} // namespace ppx

#endif // ppx_application_h
