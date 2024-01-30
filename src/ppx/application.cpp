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

#include "ppx/application.h"
#include "ppx/fs.h"
#include "ppx/ppm_export.h"
#include "ppx/profiler.h"

#include <chrono>
#include <filesystem>
#include <fstream>
#include <limits>
#include <map>
#include <numeric>
#include <optional>
#include <sstream>
#include <unordered_map>

namespace ppx {

const char*    kDefaultAppName      = "PPX Application";
const uint32_t kDefaultWindowWidth  = 1280;
const uint32_t kDefaultWindowHeight = 720;
const uint32_t kImGuiMinWidth       = 400;
const uint32_t kImGuiMinHeight      = 300;

static Application* sApplicationInstance = nullptr;

// -------------------------------------------------------------------------------------------------
// Application
// -------------------------------------------------------------------------------------------------
Application::Application()
{
    InternalCtor();

    mSettings.appName      = kDefaultAppName;
    mSettings.window.title = kDefaultAppName;
}

Application::Application(uint32_t windowWidth, uint32_t windowHeight, const char* windowTitle)
{
    InternalCtor();

    mSettings.appName      = windowTitle;
    mSettings.window.title = windowTitle;
}

Application::~Application()
{
    if (sApplicationInstance == this) {
        sApplicationInstance = nullptr;
    }
}
void Application::InternalCtor()
{
    if (IsNull(sApplicationInstance)) {
        sApplicationInstance = this;
    }

    // Initialize timer static data
    ppx::TimerResult tmres = ppx::Timer::InitializeStaticData();
    if (tmres != ppx::TIMER_RESULT_SUCCESS) {
        PPX_ASSERT_MSG(false, "failed to initialize timer's statick data");
    }
}

Application* Application::Get()
{
    return sApplicationInstance;
}

Result Application::InitializePlatform()
{
    // CPU Info
    // clang-format off
    PPX_LOG_INFO("CPU info for " << Platform::GetCpuInfo().GetBrandString());
    PPX_LOG_INFO("   " << "vendor             : " << Platform::GetCpuInfo().GetVendorString());
    PPX_LOG_INFO("   " << "microarchitecture  : " << Platform::GetCpuInfo().GetMicroarchitectureString());
    PPX_LOG_INFO("   " << "L1 cache size      : " << Platform::GetCpuInfo().GetL1CacheSize());     // Intel only atm
    PPX_LOG_INFO("   " << "L2 cache size      : " << Platform::GetCpuInfo().GetL2CacheSize());     // Intel only atm
    PPX_LOG_INFO("   " << "L3 cache size      : " << Platform::GetCpuInfo().GetL3CacheSize());     // Intel only atm
    PPX_LOG_INFO("   " << "L1 cache line size : " << Platform::GetCpuInfo().GetL1CacheLineSize()); // Intel only atm
    PPX_LOG_INFO("   " << "L2 cache line size : " << Platform::GetCpuInfo().GetL2CacheLineSize()); // Intel only atm
    PPX_LOG_INFO("   " << "L3 cache line size : " << Platform::GetCpuInfo().GetL3CacheLineSize()); // Intel only atm
    PPX_LOG_INFO("   " << "SSE                : " << Platform::GetCpuInfo().GetFeatures().sse);
    PPX_LOG_INFO("   " << "SSE2               : " << Platform::GetCpuInfo().GetFeatures().sse2);
    PPX_LOG_INFO("   " << "SSE3               : " << Platform::GetCpuInfo().GetFeatures().sse3);
    PPX_LOG_INFO("   " << "SSSE3              : " << Platform::GetCpuInfo().GetFeatures().ssse3);
    PPX_LOG_INFO("   " << "SSE4_1             : " << Platform::GetCpuInfo().GetFeatures().sse4_1);
    PPX_LOG_INFO("   " << "SSE4_2             : " << Platform::GetCpuInfo().GetFeatures().sse4_2);
    PPX_LOG_INFO("   " << "SSE4A              : " << Platform::GetCpuInfo().GetFeatures().sse);
    PPX_LOG_INFO("   " << "AVX                : " << Platform::GetCpuInfo().GetFeatures().avx);
    PPX_LOG_INFO("   " << "AVX2               : " << Platform::GetCpuInfo().GetFeatures().avx2);
    // clang-format on

    return ppx::SUCCESS;
}

Result Application::InitializeGrfxDevice()
{
    // Instance
    {
        if (mInstance) {
            return ppx::ERROR_SINGLE_INIT_ONLY;
        }

        grfx::InstanceCreateInfo ci = {};
        ci.api                      = mSettings.grfx.api;
        ci.createDevices            = false;
        ci.enableDebug              = mSettings.grfx.enableDebug;
        ci.enableSwapchain          = true;
        ci.applicationName          = mSettings.appName;
        ci.engineName               = mSettings.appName;
        ci.useSoftwareRenderer      = mStandardOpts.pUseSoftwareRenderer->GetValue();
#if defined(PPX_BUILD_XR)
        ci.pXrComponent = mSettings.xr.enable ? &mXrComponent : nullptr;
        // Disable original swapchain when XR is enabled as the XR swapchain will be coming from OpenXR.
        // Enable creating swapchain when enabling debug capture, as the swapchain can help tools to do capture (dummy present calls).
        if (mSettings.xr.enable) {
            ci.enableSwapchain = false;
            if (mSettings.xr.enableDebugCapture) {
                ci.enableSwapchain = true;
            }
        }
#endif

        Result ppxres = grfx::CreateInstance(&ci, &mInstance);
        if (Failed(ppxres)) {
            PPX_ASSERT_MSG(false, "grfx::CreateInstance failed: " << ToString(ppxres));
            return ppxres;
        }
    }

    // Device
    {
        if (mDevice) {
            return ppx::ERROR_SINGLE_INIT_ONLY;
        }

        grfx::GpuPtr gpu;
        Result       ppxres = mInstance->GetGpu(mStandardOpts.pGpuIndex->GetValue(), &gpu);
        if (Failed(ppxres)) {
            PPX_ASSERT_MSG(false, "grfx::Instance::GetGpu failed");
            return ppxres;
        }

        grfx::DeviceCreateInfo ci = {};
        ci.pGpu                   = gpu;
        ci.graphicsQueueCount     = mSettings.grfx.device.graphicsQueueCount;
        ci.computeQueueCount      = mSettings.grfx.device.computeQueueCount;
        ci.transferQueueCount     = mSettings.grfx.device.transferQueueCount;
        ci.vulkanExtensions       = {};
        ci.pVulkanDeviceFeatures  = nullptr;
        ci.supportShadingRateMode = mSettings.grfx.device.supportShadingRateMode;
#if defined(PPX_BUILD_XR)
        ci.pXrComponent = mSettings.xr.enable ? &mXrComponent : nullptr;
#endif

        PPX_LOG_INFO("Creating application graphics device using " << gpu->GetDeviceName());
        PPX_LOG_INFO("   requested graphics queue count : " << mSettings.grfx.device.graphicsQueueCount);
        PPX_LOG_INFO("   requested compute  queue count : " << mSettings.grfx.device.computeQueueCount);
        PPX_LOG_INFO("   requested transfer queue count : " << mSettings.grfx.device.transferQueueCount);

        ppxres = mInstance->CreateDevice(&ci, &mDevice);
        if (Failed(ppxres)) {
            PPX_ASSERT_MSG(false, "grfx::Instance::CreateDevice failed");
            return ppxres;
        }
    }

    return ppx::SUCCESS;
}

Result Application::InitializeGrfxSurface()
{
    if (mSettings.headless) {
        PPX_LOG_INFO("Headless mode: skipping creation of surface");
        return ppx::SUCCESS;
    }

#if defined(PPX_BUILD_XR)
    // No need to create the surface when XR is enabled.
    // The swapchain will be created from the OpenXR functions directly.
    if (!mSettings.xr.enable
        // Surface is required for debug capture.
        || (mSettings.xr.enable && mSettings.xr.enableDebugCapture))
#endif
    // Surface
    {
        grfx::SurfaceCreateInfo ci = {};
        ci.pGpu                    = mDevice->GetGpu();
        mWindow->FillSurfaceInfo(&ci);

        Result ppxres = mInstance->CreateSurface(&ci, &mSurface);
        if (Failed(ppxres)) {
            PPX_ASSERT_MSG(false, "grfx::Instance::CreateSurface failed");
            return ppxres;
        }
    }

    return ppx::SUCCESS;
}

Result Application::CreateSwapchains()
{
#if defined(PPX_BUILD_XR)
    if (mSettings.xr.enable) {
        const size_t viewCount = mXrComponent.GetViewCount();
        PPX_ASSERT_MSG(viewCount != 0, "The config views should be already created at this point!");

        grfx::SwapchainCreateInfo ci = {};
        ci.pQueue                    = mDevice->GetGraphicsQueue();
        ci.pSurface                  = nullptr;
        ci.width                     = mWindow->Size().width;
        ci.height                    = mWindow->Size().height;
        ci.colorFormat               = mXrComponent.GetColorFormat();
        ci.depthFormat               = mXrComponent.GetDepthFormat();
        ci.imageCount                = 0;                            // This will be derived from XrSwapchain.
        ci.presentMode               = grfx::PRESENT_MODE_UNDEFINED; // No present for XR.
        ci.pXrComponent              = &mXrComponent;

        // We have one swapchain for each view, and one swapchain for the UI.
        mSwapchains.resize(viewCount + 1);
        mStereoscopicSwapchainIndex = 0;
        for (size_t k = 0; k < viewCount; ++k) {
            Result ppxres = mDevice->CreateSwapchain(&ci, &mSwapchains[k]);
            if (Failed(ppxres)) {
                PPX_ASSERT_MSG(false, "grfx::Device::CreateSwapchain failed");
                return ppxres;
            }
        }

        mUISwapchainIndex = static_cast<uint32_t>(viewCount);
        ci.width          = GetUIWidth();
        ci.height         = GetUIHeight();
        Result ppxres     = mDevice->CreateSwapchain(&ci, &mSwapchains[mUISwapchainIndex]);
        if (Failed(ppxres)) {
            PPX_ASSERT_MSG(false, "grfx::Device::CreateSwapchain failed");
            return ppxres;
        }

        // Image count is from xrEnumerateSwapchainImages
        mSettings.grfx.swapchain.imageCount = mSwapchains[0]->GetImageCount();
    }

    if (!mSettings.xr.enable
        // Extra swapchain for XR debug capture.
        || (mSettings.xr.enable && mSettings.xr.enableDebugCapture))
#endif
    {
        std::pair<uint32_t, uint32_t> swapchainSize = {mWindow->Size().width, mWindow->Size().height};
        if (mSurface) {
            const uint32_t surfaceMinImageCount = mSurface->GetMinImageCount();
            if (mSettings.grfx.swapchain.imageCount < surfaceMinImageCount) {
                PPX_LOG_WARN("readjusting swapchain's image count from " << mSettings.grfx.swapchain.imageCount << " to " << surfaceMinImageCount << " to match surface requirements");
                mSettings.grfx.swapchain.imageCount = surfaceMinImageCount;
            }

            //
            // Cap the image width/height to what the surface caps are.
            // The reason for this is that on Windows the TaskBar
            // affect the maximum size of the window if it has borders.
            // For example an application can request a 1920x1080 window
            // but because of the task bar, the window may get created
            // at 1920x1061. This limits the swapchain's max image extents
            // to 1920x1061.
            //
            const uint32_t surfaceMaxImageWidth  = mSurface->GetMaxImageWidth();
            const uint32_t surfaceMaxImageHeight = mSurface->GetMaxImageHeight();
            if ((swapchainSize.first > surfaceMaxImageWidth) || (swapchainSize.second > surfaceMaxImageHeight)) {
                PPX_LOG_WARN("readjusting swapchain/window size from " << swapchainSize.first << "x" << swapchainSize.second << " to " << surfaceMaxImageWidth << "x" << surfaceMaxImageHeight << " to match surface requirements");
                swapchainSize = {
                    std::min(swapchainSize.first, surfaceMaxImageWidth),
                    std::min(swapchainSize.second, surfaceMaxImageHeight)};
            }

            const uint32_t surfaceCurrentImageWidth  = mSurface->GetCurrentImageWidth();
            const uint32_t surfaceCurrentImageHeight = mSurface->GetCurrentImageHeight();
            if ((surfaceCurrentImageWidth != grfx::Surface::kInvalidExtent) &&
                (surfaceCurrentImageHeight != grfx::Surface::kInvalidExtent)) {
                if ((swapchainSize.first != surfaceCurrentImageWidth) ||
                    (swapchainSize.second != surfaceCurrentImageHeight)) {
                    PPX_LOG_WARN("window size " << swapchainSize.first << "x" << swapchainSize.second << " does not match current surface extent " << surfaceCurrentImageWidth << "x" << surfaceCurrentImageHeight);
                }
                PPX_LOG_WARN("surface current extent " << surfaceCurrentImageWidth << "x" << surfaceCurrentImageHeight);
            }
        }

        PPX_LOG_INFO("Creating application swapchain");
        PPX_LOG_INFO("   resolution  : " << swapchainSize.first << "x" << swapchainSize.second);
        PPX_LOG_INFO("   image count : " << mSettings.grfx.swapchain.imageCount);

        grfx::SwapchainCreateInfo ci = {};
        ci.pQueue                    = mDevice->GetGraphicsQueue();
        ci.pSurface                  = mSurface;
        ci.width                     = swapchainSize.first;
        ci.height                    = swapchainSize.second;
        ci.colorFormat               = mSettings.grfx.swapchain.colorFormat;
        ci.depthFormat               = mSettings.grfx.swapchain.depthFormat;
        ci.imageCount                = mSettings.grfx.swapchain.imageCount;
        ci.presentMode               = grfx::PRESENT_MODE_IMMEDIATE;

        grfx::SwapchainPtr swapchain;
        Result             ppxres = mDevice->CreateSwapchain(&ci, &swapchain);
        if (Failed(ppxres)) {
            PPX_ASSERT_MSG(false, "grfx::Device::CreateSwapchain failed");
            return ppxres;
        }
#if defined(PPX_BUILD_XR)
        if (mSettings.xr.enable && mSettings.xr.enableDebugCapture) {
            mDebugCaptureSwapchainIndex = static_cast<uint32_t>(mSwapchains.size());
        }
#endif
        mSwapchains.push_back(swapchain);
    }

    return ppx::SUCCESS;
}

void Application::DestroySwapchains()
{
    // Make sure the device is idle so that nothing is accessing the swapchain images
    mDevice->WaitIdle();

    // Destroy all swapchains
    for (auto& sc : mSwapchains) {
        mDevice->DestroySwapchain(sc);
        sc.Reset();
    }
    mSwapchains.clear();
}

Result Application::InitializeImGui()
{
    switch (mSettings.grfx.api) {
        default: {
            PPX_ASSERT_MSG(false, "[imgui] unknown graphics API");
            return ppx::ERROR_UNSUPPORTED_API;
        } break;

#if defined(PPX_D3D12)
        case grfx::API_DX_12_0:
        case grfx::API_DX_12_1: {
            mImGui = std::unique_ptr<ImGuiImpl>(new ImGuiImplDx12());
        } break;
#endif // defined(PPX_D3D12)

#if defined(PPX_VULKAN)
        case grfx::API_VK_1_1:
        case grfx::API_VK_1_2: {
            mImGui = std::unique_ptr<ImGuiImpl>(new ImGuiImplVk());
        } break;
#endif // defined(PPX_VULKAN)
    }

    if (mImGui) {
        Result ppxres = mImGui->Init(this);
        if (Failed(ppxres)) {
            return ppxres;
        }
    }

    return ppx::SUCCESS;
}

void Application::ShutdownImGui()
{
    if (mImGui) {
        mImGui->Shutdown(this);
        mImGui.reset();
    }
}

void Application::StopGrfx()
{
    if (mDevice) {
        mDevice->WaitIdle();
    }
}

void Application::ShutdownGrfx()
{
    if (mInstance) {
        DestroySwapchains();

        if (mDevice) {
            mInstance->DestroyDevice(mDevice);
            mDevice.Reset();
        }

        if (mSurface) {
            mInstance->DestroySurface(mSurface);
            mSurface.Reset();
        }

        grfx::DestroyInstance(mInstance);
        mInstance.Reset();
    }
}

Result Application::CreatePlatformWindow()
{
    // Decorated window title
    std::stringstream windowTitle;
    windowTitle << mSettings.window.title << " | " << ToString(mSettings.grfx.api)
                << " | " << mDevice->GetDeviceName()
                << " | " << Platform::GetPlatformString();

    return mWindow->Create(windowTitle.str().c_str());
}

void Application::DestroyPlatformWindow()
{
    mWindow->Destroy();
}

void Application::DispatchInitKnobs()
{
    InitStandardKnobs();
    InitKnobs();
}

void Application::DispatchSetup()
{
    SetupMetrics();
    Setup();
}

void Application::DispatchShutdown()
{
    Shutdown();

    ShutdownMetrics();
    SaveMetricsReportToDisk();

    PPX_LOG_INFO("Number of frames drawn: " << GetFrameCount());
    PPX_LOG_INFO("Average frame time:     " << GetAverageFrameTime() << " ms");
    PPX_LOG_INFO("Average FPS:            " << GetAverageFPS());
}

void Application::DispatchMove(int32_t x, int32_t y)
{
    Move(x, y);
}

void Application::DispatchResize(uint32_t width, uint32_t height)
{
    Resize(width, height);
}

void Application::DispatchWindowIconify(bool iconified)
{
    WindowIconify(iconified);
}

void Application::DispatchWindowMaximize(bool maximized)
{
    WindowMaximize(maximized);
}

void Application::DispatchKeyDown(KeyCode key)
{
    KeyDown(key);
}

void Application::DispatchKeyUp(KeyCode key)
{
    KeyUp(key);
}

void Application::DispatchMouseMove(int32_t x, int32_t y, int32_t dx, int32_t dy, uint32_t buttons)
{
    MouseMove(x, y, dx, dy, buttons);
}

void Application::DispatchMouseDown(int32_t x, int32_t y, uint32_t buttons)
{
    MouseDown(x, y, buttons);
}

void Application::DispatchMouseUp(int32_t x, int32_t y, uint32_t buttons)
{
    MouseUp(x, y, buttons);
}

void Application::DispatchScroll(float dx, float dy)
{
    Scroll(dx, dy);
}

void Application::DispatchRender()
{
    Render();
}

void Application::DispatchUpdateMetrics()
{
    // NOTE: This function is dispatched once per frame for both recorded AND displayed
    // metrics, thus it should always be called regardless of whether there's an active
    // recorded metrics run. Called functions with functionality tied to recorded
    // metrics must use HasActiveMetricsRun to gate their operations appropriately.

    // Update shared, app-level metrics first.
    UpdateAppMetrics();

    // Then update custom app metrics.
    UpdateMetrics();
}

void Application::SetupMetrics()
{
    if (!mStandardOpts.pEnableMetrics->GetValue()) {
        return;
    }

    // Default behavior for this function is to start a single run at setup, and stop it at shutdown.
    // This enables all applications to get a minimum of functionality from enabling metrics.
    StartMetricsRun("Default Run");
}

void Application::ShutdownMetrics()
{
    if (!mStandardOpts.pEnableMetrics->GetValue()) {
        return;
    }

    StopMetricsRun();
}

metrics::GaugeBasicStatistics Application::GetGaugeBasicStatistics(metrics::MetricID id) const
{
    if (!mStandardOpts.pEnableMetrics->GetValue()) {
        PPX_ASSERT_MSG(false, "Metrics is not enabled.");
        return metrics::GaugeBasicStatistics();
    }
    return mMetrics.manager.GetGaugeBasicStatistics(id);
}

void Application::SaveMetricsReportToDisk()
{
    // Ensure the base metrics knob was initialized by the KnobManager.
    PPX_ASSERT_MSG(mStandardOpts.pEnableMetrics != nullptr, "The --enable-metrics knob was not initialized.");
    if (!mStandardOpts.pEnableMetrics->GetValue()) {
        return;
    }

    // Ensure the needed knobs were initialized by the KnobManager.
    PPX_ASSERT_MSG(mStandardOpts.pMetricsFilename != nullptr, "The --metrics-filename knob was not initialized.");
    PPX_ASSERT_MSG(mStandardOpts.pOverwriteMetricsFile != nullptr, "The --overwrite-metrics-file knob was not initialized.");

    // Export the report from the metrics manager to the disk.
    auto report = mMetrics.manager.CreateReport(mStandardOpts.pMetricsFilename->GetValue());
    report.WriteToDisk(mStandardOpts.pOverwriteMetricsFile->GetValue());
}

void Application::InitStandardKnobs()
{
    // Flag names in alphabetical order
    GetKnobManager().InitKnob(&mStandardOpts.pAssetsPaths, "extra-assets-path", mSettings.standardKnobsDefaultValue.assetsPaths);
    mStandardOpts.pAssetsPaths->SetFlagDescription(
        "Add a path before the default assets folder in the search list.");
    mStandardOpts.pAssetsPaths->SetFlagParameters("<path>");

    GetKnobManager().InitKnob(&mStandardOpts.pConfigJsonPaths, mCommandLineParser.GetJsonConfigFlagName(), mSettings.standardKnobsDefaultValue.configJsonPaths);
    mStandardOpts.pConfigJsonPaths->SetFlagDescription(
        "Additional commandline flags specified in a JSON file. Values specified in JSON files are "
        "always overwritten by those specified on the command line. Between different files, the "
        "later ones take priority.");
    mStandardOpts.pConfigJsonPaths->SetFlagParameters("<path>");

    GetKnobManager().InitKnob(&mStandardOpts.pDeterministic, "deterministic", mSettings.standardKnobsDefaultValue.deterministic);
    mStandardOpts.pDeterministic->SetFlagDescription(
        "Disable non-deterministic behaviors, like clocks and ImGui.");

    GetKnobManager().InitKnob(&mStandardOpts.pEnableMetrics, "enable-metrics", mSettings.standardKnobsDefaultValue.enableMetrics);
    mStandardOpts.pEnableMetrics->SetFlagDescription(
        "Enable metrics report output. See also: `--metrics-filename` and `--overwrite-metrics-file`.");

    GetKnobManager().InitKnob(&mStandardOpts.pFrameCount, "frame-count", mSettings.standardKnobsDefaultValue.frameCount, 0, UINT64_MAX);
    mStandardOpts.pFrameCount->SetFlagDescription(
        "Shutdown the application after successfully rendering N frames. "
        "If 0, this is disabled.");

    GetKnobManager().InitKnob(&mStandardOpts.pGpuIndex, "gpu", mSettings.standardKnobsDefaultValue.gpuIndex, 0, UINT_MAX);
    mStandardOpts.pGpuIndex->SetFlagDescription(
        "Select the gpu with the given index. To determine the set of valid "
        "indices use `--list-gpus`.");

    GetKnobManager().InitKnob(&mStandardOpts.pHeadless, "headless", mSettings.standardKnobsDefaultValue.headless);
    mStandardOpts.pHeadless->SetFlagDescription(
        "Run the sample without creating windows.");

    GetKnobManager().InitKnob(&mStandardOpts.pListGpus, "list-gpus", mSettings.standardKnobsDefaultValue.listGpus);
    mStandardOpts.pListGpus->SetFlagDescription(
        "Prints a list of the available GPUs on the current system with their "
        "index and exits. See also `--gpu`.");

    GetKnobManager().InitKnob(&mStandardOpts.pMetricsFilename, "metrics-filename", mSettings.standardKnobsDefaultValue.metricsFilename);
    mStandardOpts.pMetricsFilename->SetFlagDescription(
        "If metrics are enabled, save the metrics report to the "
        "provided path. If used, any `@` symbols in the filename "
        "(not the path) will be replaced with the current timestamp. "
        "If not a full path, will be defined relative to the default "
        "output directory. See also `--enable-metrics` and `--overwrite-metrics-file`.");

    GetKnobManager().InitKnob(&mStandardOpts.pOverwriteMetricsFile, "overwrite-metrics-file", mSettings.standardKnobsDefaultValue.overwriteMetricsFile);
    mStandardOpts.pOverwriteMetricsFile->SetFlagDescription(
        "Only applies if metrics are enabled with `--enable-metrics`. "
        "If an existing file at the path set with `--metrics-filename` is found, it will be overwritten. "
        "See also: `--enable-metrics` and `--metrics-filename`.");

    GetKnobManager().InitKnob(&mStandardOpts.pResolution, "resolution", mSettings.standardKnobsDefaultValue.resolution);
    mStandardOpts.pResolution->SetFlagDescription(
        "Specify the main window resolution in pixels. Width and Height must be "
        "two positive integers greater or equal to 1. If 0, window dimensions will be used.");
#if defined(PPX_BUILD_XR)
    mStandardOpts.pResolution->SetFlagDescription(
        "Specify the per-eye resolution in pixels. Width and Height must be two "
        "positive integers greater or equal to 1. If 0, window dimensions will be used.");
#endif
    mStandardOpts.pResolution->SetFlagParameters("<width>x<height>");
    mStandardOpts.pResolution->SetValidator([](std::pair<int, int> res) {
        return res.first >= 0 && res.second >= 0;
    });

    GetKnobManager().InitKnob(&mStandardOpts.pRunTimeMs, "run-time-ms", mSettings.standardKnobsDefaultValue.runTimeMs, 0, UINT_MAX);
    mStandardOpts.pRunTimeMs->SetFlagDescription(
        "Shutdown the application after N milliseconds. If 0, this is disabled.");

    GetKnobManager().InitKnob(&mStandardOpts.pScreenshotFrameNumber, "screenshot-frame-number", mSettings.standardKnobsDefaultValue.screenshotFrameNumber, -1, INT_MAX);
    mStandardOpts.pScreenshotFrameNumber->SetFlagDescription(
        "Take a screenshot of frame number N and save it in PPM format. See also "
        "`--screenshot-path`.");

    GetKnobManager().InitKnob(&mStandardOpts.pScreenshotPath, "screenshot-path", mSettings.standardKnobsDefaultValue.screenshotPath);
    mStandardOpts.pScreenshotPath->SetFlagDescription(
        "Save the screenshot to this path. If used, any `#` symbols in the filename "
        "(not the path) will be replaced with the number of the screenshotted frame. "
        "If not a full path, will be defined relative to the default output directory. "
        "See also `--screenshot-frame-number`");
    mStandardOpts.pScreenshotPath->SetFlagParameters("<path>");

    GetKnobManager().InitKnob(&mStandardOpts.pStatsFrameWindow, "stats-frame-window", mSettings.standardKnobsDefaultValue.statsFrameWindow, -1, INT_MAX);
    mStandardOpts.pStatsFrameWindow->SetFlagDescription(
        "Calculate frame statistics over the last N frames only. If 0, "
        "all frames since the beginning of the application will be used.");

    GetKnobManager().InitKnob(&mStandardOpts.pUseSoftwareRenderer, "use-software-renderer", mSettings.standardKnobsDefaultValue.useSoftwareRenderer);
    mStandardOpts.pUseSoftwareRenderer->SetFlagDescription(
        "Use a software renderer instead of a hardware device, if available.");
    mStandardOpts.pUseSoftwareRenderer->SetValidator([gpuIndex{mStandardOpts.pGpuIndex->GetValue()}](bool useSoftwareRenderer) {
        // GPU index must be 0 if software renderer is used.
        return useSoftwareRenderer ? gpuIndex == 0 : true;
    });

#if defined(PPX_BUILD_XR)
    GetKnobManager().InitKnob(&mStandardOpts.pXrUiResolution, "xr-ui-resolution", mSettings.standardKnobsDefaultValue.xrUiResolution);
    mStandardOpts.pXrUiResolution->SetFlagDescription(
        "Specify the UI quad resolution in pixels. Width and Height must be two "
        "positive integers greater or equal to 1. If 0, window dimensions will be used.");
    mStandardOpts.pXrUiResolution->SetFlagParameters("<width>x<height>");
    mStandardOpts.pXrUiResolution->SetValidator([](std::pair<int, int> res) {
        return res.first >= 0 && res.second >= 0;
    });

    GetKnobManager().InitKnob(&mStandardOpts.pXrRequiredExtensions, "xr-required-extension", mSettings.standardKnobsDefaultValue.xrRequiredExtensions);
    mStandardOpts.pXrRequiredExtensions->SetFlagDescription(
        "Specify any additional OpenXR extensions that need to be loaded in addition "
        "to the base extensions. Any required extensions that are not supported by the "
        "target system will cause the application to immediately exit.");
    mStandardOpts.pXrRequiredExtensions->SetFlagParameters("<extension>");
#endif

    GetKnobManager().InitKnob(&mStandardOpts.pShadingRateMode, "shading-rate-mode", "");
    mStandardOpts.pShadingRateMode->SetFlagDescription(
        "Specify the shading rate mode to enable.");
    mStandardOpts.pShadingRateMode->SetFlagParameters("<none|fdm|vrs>");
    mStandardOpts.pShadingRateMode->SetValidator([](const std::string& res) {
        return res == "" || res == "none" || res == "fdm" || res == "vrs";
    });
}

void Application::TakeScreenshot()
{
    std::filesystem::path screenshotPath;
    screenshotPath = ppx::fs::GetFullPath(mStandardOpts.pScreenshotPath->GetValue(), ppx::fs::GetDefaultOutputDirectory(), "#", std::to_string(mFrameCount));

    auto swapchainImg = GetSwapchain()->GetColorImage(GetSwapchain()->GetCurrentImageIndex());
    auto queue        = mDevice->GetGraphicsQueue();

    const grfx::FormatDesc* formatDesc = grfx::GetFormatDescription(swapchainImg->GetFormat());
    const uint32_t          width      = swapchainImg->GetWidth();
    const uint32_t          height     = swapchainImg->GetHeight();

    // Create a buffer that will hold the swapchain image's texels.
    // Increase its size by a factor of 2 to ensure that a larger-than-needed
    // row pitch will not overflow the buffer.
    uint64_t bufferSize = 2ull * formatDesc->bytesPerTexel * width * height;

    grfx::BufferPtr        screenshotBuf = nullptr;
    grfx::BufferCreateInfo bufferCi      = {};
    bufferCi.size                        = bufferSize;
    bufferCi.initialState                = grfx::RESOURCE_STATE_COPY_DST;
    bufferCi.usageFlags.bits.transferDst = 1;
    bufferCi.memoryUsage                 = grfx::MEMORY_USAGE_GPU_TO_CPU;
    PPX_CHECKED_CALL(mDevice->CreateBuffer(&bufferCi, &screenshotBuf));

    // We wait for idle so that we can avoid tracking swapchain fences.
    // It's not ideal, but we won't be taking screenshots in
    // performance-critical scenarios.
    PPX_CHECKED_CALL(queue->WaitIdle());

    // Copy the swapchain image into the buffer.
    grfx::CommandBufferPtr cmdBuf;
    PPX_CHECKED_CALL(queue->CreateCommandBuffer(&cmdBuf, 0, 0));

    grfx::ImageToBufferOutputPitch outPitch;
    cmdBuf->Begin();
    {
        cmdBuf->TransitionImageLayout(swapchainImg, PPX_ALL_SUBRESOURCES, grfx::RESOURCE_STATE_PRESENT, grfx::RESOURCE_STATE_COPY_SRC);

        grfx::ImageToBufferCopyInfo bufCopyInfo = {};
        bufCopyInfo.extent                      = {width, height, 0};
        outPitch                                = cmdBuf->CopyImageToBuffer(&bufCopyInfo, swapchainImg, screenshotBuf);

        cmdBuf->TransitionImageLayout(swapchainImg, PPX_ALL_SUBRESOURCES, grfx::RESOURCE_STATE_COPY_SRC, grfx::RESOURCE_STATE_PRESENT);
    }
    cmdBuf->End();

    grfx::SubmitInfo submitInfo   = {};
    submitInfo.commandBufferCount = 1;
    submitInfo.ppCommandBuffers   = &cmdBuf;
    queue->Submit(&submitInfo);

    // Wait for the copy to be finished.
    queue->WaitIdle();

    // Export to PPM.
    unsigned char* texels = nullptr;
    screenshotBuf->MapMemory(0, (void**)&texels);

    PPX_CHECKED_CALL(ExportToPPM(screenshotPath.string(), swapchainImg->GetFormat(), texels, width, height, outPitch.rowPitch));
    PPX_LOG_INFO("Screenshot of frame " << mFrameCount << " saved to: " << screenshotPath.string());

    screenshotBuf->UnmapMemory();

    // Clean up temporary resources.
    mDevice->DestroyBuffer(screenshotBuf);
    queue->DestroyCommandBuffer(cmdBuf);
}

void Application::MoveCallback(int32_t x, int32_t y)
{
    Move(x, y);
}

void Application::ResizeCallback(uint32_t width, uint32_t height)
{
    mWindowSurfaceInvalid = ((width == 0) || (height == 0));
    // Vulkan will return an error if either dimension is 0
    if (mWindowSurfaceInvalid) {
        DispatchResize(width, height);
        return;
    }

    // D3D12 swapchain needs resizing
    if ((mDevice->GetApi() == grfx::API_DX_12_0) || (mDevice->GetApi() == grfx::API_DX_12_1)) {
        // Wait for device to idle
        mDevice->WaitIdle();

        PPX_ASSERT_MSG((mSwapchains.size() == 1), "Invalid number of swapchains for D3D12");

        auto ppxres = mSwapchains[0]->Resize(width, height);
        if (Failed(ppxres)) {
            PPX_ASSERT_MSG(false, "D3D12 swapchain resize failed");
            // Signal the app to quit if swapchain recreation fails
            mWindow->Quit();
        }

#if defined(PPX_MSW)
        mForceInvalidateClientArea = true;
#endif

        PPX_LOG_INFO("Resized application swapchain");
        PPX_LOG_INFO("   resolution  : " << width << "x" << height);
        PPX_LOG_INFO("   image count : " << mSettings.grfx.swapchain.imageCount);
    }
    // Vulkan swapchain needs recreation
    else {
        // This function will wait for device to idle
        DestroySwapchains();

        auto ppxres = CreateSwapchains();
        if (Failed(ppxres)) {
            PPX_ASSERT_MSG(false, "Vulkan swapchain recreate failed");
            // Signal the app to quit if swapchain recreation fails
            mWindow->Quit();
        }
    }

    // Dispatch resize event
    DispatchResize(width, height);
}

void Application::WindowIconifyCallback(bool iconified)
{
    mWindow->SetState(iconified ? WINDOW_STATE_ICONIFIED : WINDOW_STATE_RESTORED);
    DispatchWindowIconify(iconified);
}

void Application::WindowMaximizeCallback(bool maximized)
{
    mWindow->SetState(maximized ? WINDOW_STATE_MAXIMIZED : WINDOW_STATE_RESTORED);
    DispatchWindowMaximize(maximized);
}

void Application::KeyDownCallback(KeyCode key)
{
    if (mSettings.enableImGui && ImGui::GetIO().WantCaptureKeyboard) {
        return;
    }

    mKeyStates[key].down     = true;
    mKeyStates[key].timeDown = GetElapsedSeconds();
    DispatchKeyDown(key);
}

void Application::KeyUpCallback(KeyCode key)
{
    if (mSettings.enableImGui && ImGui::GetIO().WantCaptureKeyboard) {
        return;
    }

    mKeyStates[key].down     = false;
    mKeyStates[key].timeDown = FLT_MAX;
    DispatchKeyUp(key);
}

void Application::MouseMoveCallback(int32_t x, int32_t y, uint32_t buttons)
{
    if (mSettings.enableImGui && ImGui::GetIO().WantCaptureMouse) {
        return;
    }

    int32_t dx = (mPreviousMouseX != INT32_MAX) ? (x - mPreviousMouseX) : 0;
    int32_t dy = (mPreviousMouseY != INT32_MAX) ? (y - mPreviousMouseY) : 0;
    DispatchMouseMove(x, y, dx, dy, buttons);
    mPreviousMouseX = x;
    mPreviousMouseY = y;
}

void Application::MouseDownCallback(int32_t x, int32_t y, uint32_t buttons)
{
    if (mSettings.enableImGui && ImGui::GetIO().WantCaptureMouse) {
        return;
    }

    DispatchMouseDown(x, y, buttons);
}

void Application::MouseUpCallback(int32_t x, int32_t y, uint32_t buttons)
{
    if (mSettings.enableImGui && ImGui::GetIO().WantCaptureMouse) {
        return;
    }

    DispatchMouseUp(x, y, buttons);
}

void Application::ScrollCallback(float dx, float dy)
{
    if (mSettings.enableImGui && ImGui::GetIO().WantCaptureMouse) {
        return;
    }

    DispatchScroll(dx, dy);
}

void Application::DrawImGui(grfx::CommandBuffer* pCommandBuffer)
{
    if (!mImGui) {
        return;
    }

    mImGui->Render(pCommandBuffer);
}

bool Application::IsRunning() const
{
    return mWindow->IsRunning();
}

void Application::UpdateStandardSettings()
{
    mSettings.headless = mStandardOpts.pHeadless->GetValue();

#if defined(PPX_BUILD_XR)
    auto resolution = mStandardOpts.pResolution->GetValue();
    resolution      = mStandardOpts.pXrUiResolution->GetValue();
    if (resolution.first > 0 && resolution.second > 0) {
        mSettings.xr.uiWidth  = resolution.first;
        mSettings.xr.uiHeight = resolution.second;
    }
#endif

    // Disable ImGui in headless or deterministic mode.
    // ImGUI is not non-deterministic, but the visible informations (stats, timers) are.
    if ((mSettings.headless || mStandardOpts.pDeterministic->GetValue()) && mSettings.enableImGui) {
        mSettings.enableImGui = false;
        PPX_LOG_WARN("Headless or deterministic mode: disabling ImGui");
    }

    std::string shadingRateModeString = mStandardOpts.pShadingRateMode->GetValue();
    if (shadingRateModeString == "none") {
        mSettings.grfx.device.supportShadingRateMode = grfx::SHADING_RATE_NONE;
    }
    else if (shadingRateModeString == "fdm") {
        mSettings.grfx.device.supportShadingRateMode = grfx::SHADING_RATE_FDM;
    }
    else if (shadingRateModeString == "vrs") {
        mSettings.grfx.device.supportShadingRateMode = grfx::SHADING_RATE_VRS;
    }
}

#if defined(PPX_BUILD_XR)
void Application::InitializeXRComponentBeforeGrfxDeviceInit()
{
    if (mSettings.xr.enable) {
        XrComponentCreateInfo createInfo = {};
        createInfo.api                   = mSettings.grfx.api;
        createInfo.appName               = mSettings.appName;
#if defined(PPX_ANDROID)
        createInfo.androidContext = GetAndroidContext();
        createInfo.colorFormat    = grfx::FORMAT_R8G8B8A8_SRGB;
#else
        createInfo.colorFormat = grfx::FORMAT_B8G8R8A8_SRGB;
#endif
        createInfo.depthFormat          = grfx::FORMAT_D32_FLOAT;
        createInfo.refSpaceType         = XrRefSpace::XR_STAGE;
        createInfo.viewConfigType       = XR_VIEW_CONFIGURATION_TYPE_PRIMARY_STEREO;
        createInfo.enableDebug          = mSettings.grfx.enableDebug;
        createInfo.enableQuadLayer      = mSettings.enableImGui;
        createInfo.enableDepthSwapchain = mSettings.xr.enableDepthSwapchain;
        createInfo.resolution.width     = GetWindowWidth();
        createInfo.resolution.height    = GetWindowHeight();
        createInfo.uiResolution.width   = mSettings.xr.uiWidth;
        createInfo.uiResolution.height  = mSettings.xr.uiHeight;
        createInfo.requiredExtensions   = mStandardOpts.pXrRequiredExtensions->GetValue();

        mXrComponent.InitializeBeforeGrfxDeviceInit(createInfo);
    }
}

void Application::InitializeXRComponentAndUpdateSettingsAfterGrfxDeviceInit()
{
    if (mSettings.xr.enable) {
        mXrComponent.InitializeAfterGrfxDeviceInit(mInstance);
    }
}

void Application::DestroyXRComponent()
{
    if (mSettings.xr.enable) {
        mXrComponent.Destroy();
    }
}
#endif

void Application::ListGPUs() const
{
    uint32_t          count = GetInstance()->GetGpuCount();
    std::stringstream ss;
    for (uint32_t i = 0; i < count; ++i) {
        grfx::GpuPtr gpu;
        GetInstance()->GetGpu(i, &gpu);
        ss << i << " " << gpu->GetDeviceName() << std::endl;
    }
    PPX_LOG_INFO(ss.str());
}

Result Application::InitializeWindow()
{
    if (mWindow) {
        return SUCCESS;
    }

    if (mSettings.headless) {
        mWindow = Window::GetImplHeadless(this);
    }
    else {
        mWindow = Window::GetImplNative(this);
    }

    if (!mWindow) {
        PPX_ASSERT_MSG(false, "out of memory");
        return ERROR_OUT_OF_MEMORY;
    }

    return SUCCESS;
}

void Application::ProcessEvents()
{
#if defined(PPX_BUILD_XR)
    if (mSettings.xr.enable) {
        bool exitRenderLoop = false;
        mXrComponent.PollEvents(exitRenderLoop);
        if (exitRenderLoop) {
            Quit();
            return;
        }
    }
    else
#endif
    {
        mWindow->ProcessEvent();
        if (!IsRunning()) {
            return;
        }
    }
    if (mImGui) {
        mImGui->ProcessEvents();
    }
}

void Application::RenderFrame()
{
#if defined(PPX_BUILD_XR)
    if (mSettings.xr.enable) {
        if (mXrComponent.IsSessionRunning()) {
            mXrComponent.BeginFrame();
            if (mXrComponent.ShouldRender()) {
                XrSwapchainImageReleaseInfo releaseInfo = {XR_TYPE_SWAPCHAIN_IMAGE_RELEASE_INFO};
                uint32_t                    viewCount   = static_cast<uint32_t>(mXrComponent.GetViewCount());
                // Start new Imgui frame
                if (mImGui) {
                    mImGui->NewFrame();
                }
                for (uint32_t k = 0; k < viewCount; ++k) {
                    mSwapchainIndex = k;
                    mXrComponent.SetCurrentViewIndex(k);
                    DispatchRender();
                    grfx::SwapchainPtr swapchain = GetSwapchain(k + mStereoscopicSwapchainIndex);
                    CHECK_XR_CALL(xrReleaseSwapchainImage(swapchain->GetXrColorSwapchain(), &releaseInfo));
                    if (swapchain->GetXrDepthSwapchain() != XR_NULL_HANDLE) {
                        CHECK_XR_CALL(xrReleaseSwapchainImage(swapchain->GetXrDepthSwapchain(), &releaseInfo));
                    }
                }

                if (GetSettings()->enableImGui) {
                    grfx::SwapchainPtr swapchain = GetSwapchain(mUISwapchainIndex);
                    CHECK_XR_CALL(xrReleaseSwapchainImage(swapchain->GetXrColorSwapchain(), &releaseInfo));
                    if (swapchain->GetXrDepthSwapchain() != XR_NULL_HANDLE) {
                        CHECK_XR_CALL(xrReleaseSwapchainImage(swapchain->GetXrDepthSwapchain(), &releaseInfo));
                    }
                }
            }
            mXrComponent.EndFrame(mSwapchains, 0, mUISwapchainIndex);
        }
    }
    else
#endif
    {
        // Start new Imgui frame
        if (mImGui && !IsWindowIconified()) {
            mImGui->NewFrame();
        }

        // Call render
        DispatchRender();

#if defined(PPX_MSW)
        //
        // This is for D3D12 only.
        //
        // Why is it necessary to force invalidate the client area?
        //
        // Due to a bug (or feature) in the Windows event loop when interacting
        // with IDXGISwapchain::ResizeBuffers() - the client area is incorrectly
        // updated. The paint message seems to get missed or is using the an
        // outdated rect. The result is that the left and bottom of edges of the
        // window's client area is filled with garbage.
        //
        // Moving the window one pixel to the left and back again causes the client
        // area to be correctly invalidated.
        //
        // https://stackoverflow.com/questions/72956884/idxgiswapchain-resizebuffers-does-not-work-as-expected-with-wm-sizing
        //
        if (mForceInvalidateClientArea) {
            bool isD3D12 = (mDevice->GetApi() == grfx::API_DX_12_0) || (mDevice->GetApi() == grfx::API_DX_12_1);
            PPX_ASSERT_MSG(isD3D12, "Force invalidation of client area should only happen on D3D12");

            grfx::SurfaceCreateInfo ci = {};
            GetWindow()->FillSurfaceInfo(&ci);

            RECT wr = {};
            GetWindowRect(ci.hwnd, &wr);
            MoveWindow(ci.hwnd, wr.left + 1, wr.top, (wr.right - wr.left), (wr.bottom - wr.top), TRUE);
            MoveWindow(ci.hwnd, wr.left, wr.top, (wr.right - wr.left), (wr.bottom - wr.top), TRUE);

            mForceInvalidateClientArea = false;
        }
#endif
    }
}

void Application::MainLoop()
{
    while (IsRunning()) {
        // Frame start
        mFrameStartTime = static_cast<float>(mTimer.MillisSinceStart());

        ProcessEvents();
        if (!IsRunning()) {
            return;
        }
        RenderFrame();

        // Take screenshot if this is the requested frame.
        if (mFrameCount == static_cast<uint64_t>(mStandardOpts.pScreenshotFrameNumber->GetValue())) {
            TakeScreenshot();
        }

        // Frame end general metrics data, used for recorded metrics, display, screenshots, and pacing.
        double nowMs       = mTimer.MillisSinceStart();
        mFrameCount        = mFrameCount + 1;
        mPreviousFrameTime = static_cast<float>(nowMs) - mFrameStartTime;

        // Keep a rolling window of frame times to calculate stats, if requested.
        if (mStandardOpts.pStatsFrameWindow->GetValue() > 0) {
            mFrameTimesMs.push_back(mPreviousFrameTime);
            if (mFrameTimesMs.size() > mStandardOpts.pStatsFrameWindow->GetValue()) {
                mFrameTimesMs.pop_front();
            }
            float totalFrameTimeMs = std::accumulate(mFrameTimesMs.begin(), mFrameTimesMs.end(), 0.f);
            mAverageFPS            = mFrameTimesMs.size() / (totalFrameTimeMs / 1000.f);
            mAverageFrameTime      = totalFrameTimeMs / mFrameTimesMs.size();
        }
        else {
            mAverageFPS       = static_cast<float>(mFrameCount / (nowMs / 1000.f));
            mAverageFrameTime = static_cast<float>(nowMs / mFrameCount);
        }

        // Update the metrics. This can be used for both recorded AND displayed metrics,
        // and therefore should always be called.
        DispatchUpdateMetrics();

        // Pace frames - if needed
        if (mSettings.grfx.pacedFrameRate > 0) {
            if (mFrameCount > 0) {
                double currentTime  = nowMs / 1000.f;
                double pacedFPS     = 1.0 / static_cast<double>(mSettings.grfx.pacedFrameRate);
                double expectedTime = mFirstFrameTime + (mFrameCount * pacedFPS);
                double diff         = expectedTime - currentTime;
                if (diff > 0) {
                    Timer::SleepSeconds(diff);
                }
            }
            else {
                mFirstFrameTime = nowMs / 1000.f;
            }
        }
        // If we reach the maximum number of frames allowed
        if ((mStandardOpts.pFrameCount->GetValue() > 0 && mFrameCount >= mStandardOpts.pFrameCount->GetValue()) ||
            (nowMs / 1000.f) > mRunTimeSeconds) {
            Quit();
        }
    }
}

int Application::Run(int argc, char** argv)
{
    // Only allow one instance of Application. Since we can't stop
    // the app in the ctor - stop it here.
    //
    if (this != sApplicationInstance) {
        return false;
    }

    // Parse args.
    if (Failed(mCommandLineParser.Parse(argc, const_cast<const char**>(argv)))) {
        PPX_ASSERT_MSG(false, "Unable to parse command line arguments");
        return EXIT_FAILURE;
    }

    // Check deprecated flags
    PPX_ASSERT_MSG(!mCommandLineParser.GetOptions().HasExtraOption("assets-path"), "The --assets-path knob has been renamed to --extra-assets-path.");

    // Call config.
    // Put this early because it might disable the display.
    Config(mSettings);

    // Knobs need to be set up after commandline parsing.
    // This has dependency on Config() where standard knob default values are set
    DispatchInitKnobs();

    if (!mKnobManager.IsEmpty()) {
        mCommandLineParser.AppendUsageMsg(mKnobManager.GetUsageMsg());
    }

    if (mCommandLineParser.GetOptions().GetOptionValueOrDefault("help", false)) {
        PPX_LOG_INFO(mCommandLineParser.GetUsageMsg());
        return EXIT_SUCCESS;
    }

    // Command line will overwrite the knob settings from the application
    if (!mKnobManager.IsEmpty()) {
        auto options = mCommandLineParser.GetOptions();
        mKnobManager.UpdateFromFlags(options);
    }

    // Asset directories based on settings in mSettings, mCommandLineParser and mKnobManager
    // note that mKnobManager needs to be updated by options from mCommandLineParser before this call
    AddAssetDirs();

    UpdateStandardSettings();

    mDecoratedApiName = ToString(mSettings.grfx.api);

    // Initialize the window
    Result ppxres = InitializeWindow();
    if (Failed(ppxres)) {
        return EXIT_FAILURE;
    }

    mRunTimeSeconds = mStandardOpts.pRunTimeMs->GetValue() / 1000.f;
    if (mRunTimeSeconds == 0) {
        mRunTimeSeconds = std::numeric_limits<float>::max();
    }

    // Initialize the platform
    ppxres = InitializePlatform();
    if (Failed(ppxres)) {
        return EXIT_FAILURE;
    }

#if defined(PPX_BUILD_XR)
    InitializeXRComponentBeforeGrfxDeviceInit();
#endif

    // Create graphics instance
    ppxres = InitializeGrfxDevice();
    if (Failed(ppxres)) {
        return EXIT_FAILURE;
    }

#if defined(PPX_BUILD_XR)
    InitializeXRComponentAndUpdateSettingsAfterGrfxDeviceInit();
#endif

    // List gpus
    if (mStandardOpts.pListGpus->GetValue()) {
        ListGPUs();
        return EXIT_SUCCESS;
    }

    ppxres = CreatePlatformWindow();
    if (Failed(ppxres)) {
        return EXIT_FAILURE;
    }

    // Create surface
    ppxres = InitializeGrfxSurface();
    if (Failed(ppxres)) {
        return EXIT_FAILURE;
    }

    // Create swapchains
    //
    // This will be a "fake", manually-managed swapchain if headless mode is enabled.
    //
    ppxres = CreateSwapchains();
    if (Failed(ppxres)) {
        return EXIT_FAILURE;
    }

    // Setup ImGui
    if (mSettings.enableImGui) {
        ppxres = InitializeImGui();
        if (Failed(ppxres)) {
            return EXIT_FAILURE;
        }
    }

    // Call setup
    {
        ScopedTimer timer("Setup() finished");
        DispatchSetup();
    }

    // ---------------------------------------------------------------------------------------------
    // Main loop [BEGIN]
    // ---------------------------------------------------------------------------------------------

    // Initialize and start timer
    ppx::TimerResult tmres = mTimer.Start();
    if (tmres != ppx::TIMER_RESULT_SUCCESS) {
        PPX_ASSERT_MSG(false, "failed to start application timer");
        return EXIT_FAILURE;
    }

    MainLoop();

    // ---------------------------------------------------------------------------------------------
    // Main loop [END]
    // ---------------------------------------------------------------------------------------------

    // Stop graphics first before shutting down to make sure
    // that there aren't any command buffers in flight.
    //
    StopGrfx();

    // Call shutdown
    DispatchShutdown();

    // Shutdown Imgui
    ShutdownImGui();

    // Shutdown graphics
    ShutdownGrfx();

#if defined(PPX_BUILD_XR)
    // Destroy Xr
    DestroyXRComponent();
#endif

    // Destroy window
    DestroyPlatformWindow();

    // Success
    return EXIT_SUCCESS;
}

void Application::Quit()
{
    mWindow->Quit();
}

const CliOptions& Application::GetExtraOptions() const
{
    return mCommandLineParser.GetOptions();
}

bool Application::IsWindowIconified() const
{
    return GetWindow()->IsIconified();
}

bool Application::IsWindowMaximized() const
{
    return GetWindow()->IsMaximized();
}

uint32_t Application::GetUIWidth() const
{
#if defined(PPX_BUILD_XR)
    return (mSettings.xr.enable && mSettings.xr.uiWidth > 0) ? mSettings.xr.uiWidth : GetWindowWidth();
#else
    return GetWindowWidth();
#endif
}
uint32_t Application::GetUIHeight() const
{
#if defined(PPX_BUILD_XR)
    return (mSettings.xr.enable && mSettings.xr.uiHeight > 0) ? mSettings.xr.uiHeight : GetWindowHeight();
#else
    return GetWindowHeight();
#endif
}

grfx::Rect Application::GetScissor() const
{
    grfx::Rect rect = {};
    rect.x          = 0;
    rect.y          = 0;
    rect.width      = GetWindowWidth();
    rect.height     = GetWindowHeight();
    return rect;
}

grfx::Viewport Application::GetViewport(float minDepth, float maxDepth) const
{
    grfx::Viewport viewport = {};
    viewport.x              = 0.0f;
    viewport.y              = 0.0f;
    viewport.width          = static_cast<float>(GetWindowWidth());
    viewport.height         = static_cast<float>(GetWindowHeight());
    viewport.minDepth       = minDepth;
    viewport.maxDepth       = maxDepth;
    return viewport;
}

namespace {

std::optional<std::filesystem::path> GetShaderPathSuffix(const ppx::ApplicationSettings& settings, const std::filesystem::path& baseName)
{
    switch (settings.grfx.api) {
        case grfx::API_DX_12_0:
        case grfx::API_DX_12_1:
            return (std::filesystem::path("dxil") / baseName).concat(".dxil");
        case grfx::API_VK_1_1:
        case grfx::API_VK_1_2:
            return (std::filesystem::path("spv") / baseName).concat(".spv");
        default:
            return std::nullopt;
    }
};

} // namespace

std::vector<char> Application::LoadShader(const std::filesystem::path& baseDir, const std::filesystem::path& baseName) const
{
    PPX_ASSERT_MSG(baseDir.is_relative(), "baseDir must be relative. Do not call GetAssetPath() on the directory.");
    PPX_ASSERT_MSG(baseName.is_relative(), "baseName must be relative. Do not call GetAssetPath() on the directory.");
    auto suffix = GetShaderPathSuffix(mSettings, baseName);
    if (!suffix.has_value()) {
        PPX_ASSERT_MSG(false, "unsupported API");
        return {};
    }

    const auto filePath = GetAssetPath(baseDir / suffix.value());
    auto       bytecode = fs::load_file(filePath);
    if (!bytecode.has_value()) {
        PPX_ASSERT_MSG(false, "could not load file: " << filePath);
        return {};
    }

    PPX_LOG_INFO("Loaded shader from " << filePath);
    return bytecode.value();
}

Result Application::CreateShader(const std::filesystem::path& baseDir, const std::filesystem::path& baseName, grfx::ShaderModule** ppShaderModule) const
{
    std::vector<char> bytecode = LoadShader(baseDir, baseName);
    if (bytecode.empty()) {
        return ppx::ERROR_GRFX_INVALID_SHADER_BYTE_CODE;
    }

    grfx::ShaderModuleCreateInfo shaderCreateInfo = {static_cast<uint32_t>(bytecode.size()), bytecode.data()};
    Result                       ppxres           = GetDevice()->CreateShaderModule(&shaderCreateInfo, ppShaderModule);
    if (Failed(ppxres)) {
        return ppxres;
    }

    return ppx::SUCCESS;
}

grfx::SwapchainPtr Application::GetSwapchain(uint32_t index) const
{
    PPX_ASSERT_MSG(index < mSwapchains.size(), "Invalid Swapchain Index!");
    return mSwapchains[index];
}

Result Application::Present(
    const grfx::SwapchainPtr&     swapchain,
    uint32_t                      imageIndex,
    uint32_t                      waitSemaphoreCount,
    const grfx::Semaphore* const* ppWaitSemaphores)
{
    Result ppxres = swapchain->Present(imageIndex, waitSemaphoreCount, ppWaitSemaphores);
    if (!ppxres) {
        return ppxres;
    }

    return ppx::SUCCESS;
}

float Application::GetElapsedSeconds() const
{
    if (mStandardOpts.pDeterministic->GetValue()) {
        return static_cast<float>(mFrameCount * (1.f / 60.f));
    }
    return static_cast<float>(mTimer.SecondsSinceStart());
}

const KeyState& Application::GetKeyState(KeyCode code) const
{
    if ((code < KEY_RANGE_FIRST) || (code >= KEY_RANGE_LAST)) {
        return mKeyStates[0];
    }
    return mKeyStates[static_cast<uint32_t>(code)];
}

float2 Application::GetNormalizedDeviceCoordinates(int32_t x, int32_t y) const
{
    float  fx  = x / static_cast<float>(GetWindowWidth());
    float  fy  = y / static_cast<float>(GetWindowHeight());
    float2 ndc = float2(2.0f, -2.0f) * (float2(fx, fy) - float2(0.5f));
    return ndc;
}

void Application::StartMetricsRun(const std::string& name)
{
    // Callers should check mSettings.enableMetrics before making this call.
    PPX_ASSERT_MSG(mStandardOpts.pEnableMetrics->GetValue(), "Metrics must be enabled to use metrics capabilities");
    PPX_ASSERT_MSG(!mMetrics.manager.HasActiveRun(), "A run is already active; stop it before starting another one");
    mMetrics.manager.StartRun(name.c_str());

    // Add default metrics to every single run
    {
        metrics::MetricMetadata metadata = {};
        metadata.type                    = metrics::MetricType::GAUGE;
        metadata.name                    = "cpu_frame_time";
        metadata.unit                    = "ms";
        metadata.interpretation          = metrics::MetricInterpretation::LOWER_IS_BETTER;
        mMetrics.cpuFrameTimeId          = mMetrics.manager.AddMetric(metadata);
        PPX_ASSERT_MSG(mMetrics.cpuFrameTimeId != metrics::kInvalidMetricID, "Failed to create frame time metric");
    }
    {
        metrics::MetricMetadata metadata = {};
        metadata.type                    = metrics::MetricType::GAUGE;
        metadata.name                    = "framerate";
        metadata.unit                    = "";
        metadata.interpretation          = metrics::MetricInterpretation::HIGHER_IS_BETTER;
        mMetrics.framerateId             = mMetrics.manager.AddMetric(metadata);
        PPX_ASSERT_MSG(mMetrics.framerateId != metrics::kInvalidMetricID, "Failed to create framerate metric");
    }
    {
        metrics::MetricMetadata metadata = {};
        metadata.type                    = metrics::MetricType::COUNTER;
        metadata.name                    = "frame_count";
        metadata.unit                    = "";
        metadata.interpretation          = metrics::MetricInterpretation::NONE;
        mMetrics.frameCountId            = mMetrics.manager.AddMetric(metadata);
        PPX_ASSERT_MSG(mMetrics.frameCountId != metrics::kInvalidMetricID, "Failed to create frame count metric");
    }

    mMetrics.resetFramerateTracking = true;
}

void Application::StopMetricsRun()
{
    // Callers should check mSettings.enableMetrics before making this call.
    PPX_ASSERT_MSG(mStandardOpts.pEnableMetrics->GetValue(), "Metrics must be enabled to use metrics capabilities");

    if (!mMetrics.manager.HasActiveRun()) {
        // If no run is in progress, this is likely a bad code path. But this won't harm anything.
        PPX_LOG_WARN("Attempt to stop metrics without a run in progress!");
    }

    mMetrics.manager.EndRun();
    mMetrics.cpuFrameTimeId = metrics::kInvalidMetricID;
    mMetrics.framerateId    = metrics::kInvalidMetricID;
    mMetrics.frameCountId   = metrics::kInvalidMetricID;
}

bool Application::HasActiveMetricsRun() const
{
    return mStandardOpts.pEnableMetrics->GetValue() && mMetrics.manager.HasActiveRun();
}

metrics::MetricID Application::AddMetric(const metrics::MetricMetadata& metadata)
{
    if (!mStandardOpts.pEnableMetrics->GetValue()) {
        PPX_LOG_ERROR("Attempting to add a metric with metrics disabled; ignoring.");
        return metrics::kInvalidMetricID;
    }

    // This function already covers all other cases.
    return mMetrics.manager.AddMetric(metadata);
}

bool Application::RecordMetricData(metrics::MetricID id, const metrics::MetricData& data)
{
    if (!mStandardOpts.pEnableMetrics->GetValue()) {
        PPX_LOG_ERROR("Attempting to record metric dta with metrics disabled; ignoring.");
        return false;
    }

    // This function already covers all other cases.
    return mMetrics.manager.RecordMetricData(id, data);
}

void Application::AddAssetDirs()
{
    std::filesystem::path projectRootPath = GetApplicationPath().remove_filename() / RELATIVE_PATH_TO_PROJECT_ROOT;

    // On Android, the assets folder is independently specified
    // Assets are loaded relative to this folder
#if defined(PPX_ANDROID)
    AddAssetDir("");
#else
    AddAssetDir(projectRootPath / "assets");
#endif

    if (mSettings.allowThirdPartyAssets) {
        AddAssetDir(projectRootPath / "third_party/assets");
    }

    auto assetPaths = mStandardOpts.pAssetsPaths->GetValue();
    if (!assetPaths.empty()) {
        // Insert at front, in reverse order, so we respect the command line ordering for priority.
        for (auto it = assetPaths.rbegin(); it < assetPaths.rend(); it++) {
            AddAssetDir(*it, /* insert_at_front= */ true);
        }
    }
}

void Application::UpdateAppMetrics()
{
    // This data is the same for every call to increase the frame count.
    static const metrics::MetricData frameCountData = []() {
        metrics::MetricData data = {};
        data.type                = metrics::MetricType::COUNTER;
        data.counter.increment   = 1;
        return data;
    }();

    if (!HasActiveMetricsRun()) {
        return;
    }

    const double seconds = GetElapsedSeconds();

    // Record default metrics
    metrics::MetricData frameTimeData = {metrics::MetricType::GAUGE};
    frameTimeData.gauge.seconds       = seconds;
    frameTimeData.gauge.value         = mPreviousFrameTime;
    mMetrics.manager.RecordMetricData(mMetrics.cpuFrameTimeId, frameTimeData);
    mMetrics.manager.RecordMetricData(mMetrics.frameCountId, frameCountData);

    // Record the average framerate over a given period of time
    if (mMetrics.resetFramerateTracking) {
        // Start tracking time
        mMetrics.framerateRecordTimer   = seconds;
        mMetrics.framerateFrameCount    = 0;
        mMetrics.resetFramerateTracking = false;
    }
    else {
        // compute the framerate and record the result
        ++mMetrics.framerateFrameCount;
        const double     framerateSecondsDiff    = seconds - mMetrics.framerateRecordTimer;
        constexpr double FRAMERATE_RECORD_PERIOD = 1.0;
        if (framerateSecondsDiff >= FRAMERATE_RECORD_PERIOD) {
            const double        framerate     = mMetrics.framerateFrameCount / framerateSecondsDiff;
            metrics::MetricData framerateData = {metrics::MetricType::GAUGE};
            framerateData.gauge.seconds       = seconds;
            framerateData.gauge.value         = framerate;
            mMetrics.manager.RecordMetricData(mMetrics.framerateId, framerateData);
            mMetrics.framerateRecordTimer = seconds;
            mMetrics.framerateFrameCount  = 0;
        }
    }
}

void Application::DrawDebugInfo()
{
    if (!mImGui) {
        return;
    }
    uint32_t minWidth  = std::min(kImGuiMinWidth, GetUIWidth() / 2);
    uint32_t minHeight = std::min(kImGuiMinHeight, GetUIHeight() / 2);
#if defined(PPX_BUILD_XR)
    if (mSettings.xr.enable) {
        // For XR, force the diagnostic window to the center with automatic sizing for legibility and since control is limited.
        ImGui::SetNextWindowPos({(GetUIWidth() - lastImGuiWindowSize.x) / 2, (GetUIHeight() - lastImGuiWindowSize.y) / 2}, 0, {0.0f, 0.0f});
        ImGui::SetNextWindowSize({0, 0});
    }
#endif
    ImGui::PushStyleVar(ImGuiStyleVar_WindowMinSize, {static_cast<float>(minWidth), static_cast<float>(minHeight)});
    if (ImGui::Begin("Debug Info")) {
        ImGui::Columns(2);

        // Platform
        {
            ImGui::Text("Platform");
            ImGui::NextColumn();
            ImGui::Text("%s", Platform::GetPlatformString());
            ImGui::NextColumn();
        }

        ImGui::Separator();

        // Application PID
        {
            ImGui::Text("Application PID");
            ImGui::NextColumn();
            ImGui::Text("%d", GetProcessId());
            ImGui::NextColumn();
        }

        ImGui::Separator();

        // API
        {
            ImGui::Text("API");
            ImGui::NextColumn();
            ImGui::Text("%s", mDecoratedApiName.c_str());
            ImGui::NextColumn();
        }

        // GPU
        {
            ImGui::Text("GPU");
            ImGui::NextColumn();
            ImGui::Text("%s", GetDevice()->GetDeviceName());
            ImGui::NextColumn();
        }

        ImGui::Separator();

        // CPU
        {
            ImGui::Text("CPU");
            ImGui::NextColumn();
            ImGui::Text("%s", Platform::GetCpuInfo().GetBrandString());
            ImGui::NextColumn();

            ImGui::Text("Architecture");
            ImGui::NextColumn();
            ImGui::Text("%s", Platform::GetCpuInfo().GetMicroarchitectureString());
            ImGui::NextColumn();
        }

        ImGui::Separator();

        // Frame count
        {
            ImGui::Text("Frame Count");
            ImGui::NextColumn();
            ImGui::Text("%" PRIu64, mFrameCount);
            ImGui::NextColumn();
        }

        // Average FPS
        {
            ImGui::Text("Average FPS");
            ImGui::NextColumn();
            ImGui::Text("%f", mAverageFPS);
            ImGui::NextColumn();
        }

        // Previous frame time
        {
            ImGui::Text("Previous CPU Frame Time");
            ImGui::NextColumn();
            ImGui::Text("%f ms", mPreviousFrameTime);
            ImGui::NextColumn();
        }

        // Average frame time
        {
            ImGui::Text("Average Frame Time");
            ImGui::NextColumn();
            ImGui::Text("%f ms", mAverageFrameTime);
            ImGui::NextColumn();
        }

        ImGui::Separator();

        // Num Frame In Flight
        {
            ImGui::Text("Num Frames In Flight");
            ImGui::NextColumn();
            ImGui::Text("%d", mSettings.grfx.numFramesInFlight);
            ImGui::NextColumn();
        }

        ImGui::Separator();

        // Swapchain
        {
            ImGui::Text("Swapchain Resolution");
            ImGui::NextColumn();
            ImGui::Text("%dx%d", GetSwapchain()->GetWidth(), GetSwapchain()->GetHeight());
            ImGui::NextColumn();

            ImGui::Text("Swapchain Image Count");
            ImGui::NextColumn();
            ImGui::Text("%d", GetSwapchain()->GetImageCount());
            ImGui::NextColumn();
        }

        ImGui::Columns(1);

        // Draw additional elements
        DrawGui();
    }
#if defined(PPX_BUILD_XR)
    lastImGuiWindowSize = ImGui::GetWindowSize();
#endif
    ImGui::End();
    ImGui::PopStyleVar();
}

void Application::DrawProfilerGrfxApiFunctions()
{
    if (!mImGui) {
        return;
    }

    Profiler* pProfiler = Profiler::GetProfilerForThread();
    if (IsNull(pProfiler)) {
        return;
    }

    if (ImGui::Begin("Profiler: Graphics API Functions")) {
        static std::vector<bool> selected;

        if (ImGui::BeginTable("#profiler_grfx_api_functions", 6, ImGuiTableFlags_Resizable | ImGuiTableFlags_RowBg)) {
            ImGui::TableSetupColumn("Function");
            ImGui::TableSetupColumn("Call Count");
            ImGui::TableSetupColumn("Average");
            ImGui::TableSetupColumn("Min");
            ImGui::TableSetupColumn("Max");
            ImGui::TableSetupColumn("Total");
            ImGui::TableHeadersRow();

            const std::vector<ProfilerEvent>& events = pProfiler->GetEvents();
            if (selected.empty()) {
                selected.resize(events.size());
                std::fill(std::begin(selected), std::end(selected), false);
            }

            uint32_t i = 0;
            for (auto& event : events) {
                uint64_t count    = event.GetSampleCount();
                float    average  = 0;
                float    minValue = 0;
                float    maxValue = 0;
                float    total    = 0;

                if (count > 0) {
                    average  = static_cast<float>(Timer::TimestampToMillis(event.GetSampleTotal())) / static_cast<float>(count);
                    minValue = static_cast<float>(Timer::TimestampToMillis(event.GetSampleMin()));
                    maxValue = static_cast<float>(Timer::TimestampToMillis(event.GetSampleMax()));
                    total    = static_cast<float>(Timer::TimestampToMillis(event.GetSampleTotal()));
                }

                ImGui::TableNextRow();
                ImGui::TableNextColumn();
                ImGui::Text("%s", event.GetName().c_str());
                ImGui::TableNextColumn();
                ImGui::Text("%" PRIu64, count);
                ImGui::TableNextColumn();
                ImGui::Text("%f ms", average);
                ImGui::TableNextColumn();
                ImGui::Text("%f ms", minValue);
                ImGui::TableNextColumn();
                ImGui::Text("%f ms", maxValue);
                ImGui::TableNextColumn();
                ImGui::Text("%f ms", total);

                ++i;
            }

            ImGui::EndTable();
        }
    }
    ImGui::End();
}

} // namespace ppx
