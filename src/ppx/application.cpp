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
// Key code character map
// -------------------------------------------------------------------------------------------------
// clang-format off
static std::map<int32_t, char> sCharMap = {
    {KEY_SPACE,         ' '  },
    {KEY_APOSTROPHE,    '\'' },
    {KEY_COMMA,         ','  },
    {KEY_MINUS,         '-'  },
    {KEY_PERIOD,         '.' },
    {KEY_SLASH,         '\\' },
    {KEY_0,             '0'  },
    {KEY_1,             '1'  },
    {KEY_2,             '2'  },
    {KEY_3,             '3'  },
    {KEY_4,             '4'  },
    {KEY_5,             '5'  },
    {KEY_6,             '6'  },
    {KEY_7,             '7'  },
    {KEY_8,             '8'  },
    {KEY_9,             '9'  },
    {KEY_SEMICOLON,     ';'  },
    {KEY_EQUAL,         '='  },
    {KEY_A,             'A'  },
    {KEY_B,             'B'  },
    {KEY_C,             'C'  },
    {KEY_D,             'D'  },
    {KEY_E,             'E'  },
    {KEY_F,             'F'  },
    {KEY_G,             'G'  },
    {KEY_H,             'H'  },
    {KEY_I,             'I'  },
    {KEY_J,             'J'  },
    {KEY_K,             'K'  },
    {KEY_L,             'L'  },
    {KEY_M,             'M'  },
    {KEY_N,             'N'  },
    {KEY_O,             'O'  },
    {KEY_P,             'P'  },
    {KEY_Q,             'Q'  },
    {KEY_R,             'R'  },
    {KEY_S,             'S'  },
    {KEY_T,             'T'  },
    {KEY_U,             'U'  },
    {KEY_V,             'V'  },
    {KEY_W,             'W'  },
    {KEY_X,             'X'  },
    {KEY_Y,             'Y'  },
    {KEY_Z,             'Z'  },
    {KEY_LEFT_BRACKET,  '['  },
    {KEY_BACKSLASH,     '/'  },
    {KEY_RIGHT_BRACKET, ']'  },
    {KEY_GRAVE_ACCENT,  '`'  },
};
// clang-format on

// -------------------------------------------------------------------------------------------------
// Key string
// -------------------------------------------------------------------------------------------------
// clang-format off
static std::map<uint32_t, const char*> sKeyCodeString = {
    {KEY_UNDEFINED,        "KEY_UNDEFINED"         },
    {KEY_SPACE,            "KEY_SPACE"             },
    {KEY_APOSTROPHE,       "KEY_APOSTROPHE"        },
    {KEY_COMMA,            "KEY_COMMA"             },
    {KEY_MINUS,            "KEY_MINUS"             },
    {KEY_PERIOD,           "KEY_PERIOD"            },
    {KEY_SLASH,            "KEY_SLASH"             },
    {KEY_0,                "KEY_0"                 },
    {KEY_1,                "KEY_1"                 },
    {KEY_2,                "KEY_2"                 },
    {KEY_3,                "KEY_3"                 },
    {KEY_4,                "KEY_4"                 },
    {KEY_5,                "KEY_5"                 },
    {KEY_6,                "KEY_6"                 },
    {KEY_7,                "KEY_7"                 },
    {KEY_8,                "KEY_8"                 },
    {KEY_9,                "KEY_9"                 },
    {KEY_SEMICOLON,        "KEY_SEMICOLON"         },
    {KEY_EQUAL,            "KEY_EQUAL"             },
    {KEY_A,                "KEY_A"                 },
    {KEY_B,                "KEY_B"                 },
    {KEY_C,                "KEY_C"                 },
    {KEY_D,                "KEY_D"                 },
    {KEY_E,                "KEY_E"                 },
    {KEY_F,                "KEY_F"                 },
    {KEY_G,                "KEY_G"                 },
    {KEY_H,                "KEY_H"                 },
    {KEY_I,                "KEY_I"                 },
    {KEY_J,                "KEY_J"                 },
    {KEY_K,                "KEY_K"                 },
    {KEY_L,                "KEY_L"                 },
    {KEY_M,                "KEY_M"                 },
    {KEY_N,                "KEY_N"                 },
    {KEY_O,                "KEY_O"                 },
    {KEY_P,                "KEY_P"                 },
    {KEY_Q,                "KEY_Q"                 },
    {KEY_R,                "KEY_R"                 },
    {KEY_S,                "KEY_S"                 },
    {KEY_T,                "KEY_T"                 },
    {KEY_U,                "KEY_U"                 },
    {KEY_V,                "KEY_V"                 },
    {KEY_W,                "KEY_W"                 },
    {KEY_X,                "KEY_X"                 },
    {KEY_Y,                "KEY_Y"                 },
    {KEY_Z,                "KEY_Z"                 },
    {KEY_LEFT_BRACKET,     "KEY_LEFT_BRACKET"      },
    {KEY_BACKSLASH,        "KEY_BACKSLASH"         },
    {KEY_RIGHT_BRACKET,    "KEY_RIGHT_BRACKET"     },
    {KEY_GRAVE_ACCENT,     "KEY_GRAVE_ACCENT"      },
    {KEY_WORLD_1,          "KEY_WORLD_1"           },
    {KEY_WORLD_2,          "KEY_WORLD_2"           },
    {KEY_ESCAPE,           "KEY_ESCAPE"            },
    {KEY_ENTER,            "KEY_ENTER"             },
    {KEY_TAB,              "KEY_TAB"               },
    {KEY_BACKSPACE,        "KEY_BACKSPACE"         },
    {KEY_INSERT,           "KEY_INSERT"            },
    {KEY_DELETE,           "KEY_DELETE"            },
    {KEY_RIGHT,            "KEY_RIGHT"             },
    {KEY_LEFT,             "KEY_LEFT"              },
    {KEY_DOWN,             "KEY_DOWN"              },
    {KEY_UP,               "KEY_UP"                },
    {KEY_PAGE_UP,          "KEY_PAGE_UP"           },
    {KEY_PAGE_DOWN,        "KEY_PAGE_DOWN"         },
    {KEY_HOME,             "KEY_HOME"              },
    {KEY_END,              "KEY_END"               },
    {KEY_CAPS_LOCK,        "KEY_CAPS_LOCK"         },
    {KEY_SCROLL_LOCK,      "KEY_SCROLL_LOCK"       },
    {KEY_NUM_LOCK,         "KEY_NUM_LOCK"          },
    {KEY_PRINT_SCREEN,     "KEY_PRINT_SCREEN"      },
    {KEY_PAUSE,            "KEY_PAUSE"             },
    {KEY_F1,               "KEY_F1"                },
    {KEY_F2,               "KEY_F2"                },
    {KEY_F3,               "KEY_F3"                },
    {KEY_F4,               "KEY_F4"                },
    {KEY_F5,               "KEY_F5"                },
    {KEY_F6,               "KEY_F6"                },
    {KEY_F7,               "KEY_F7"                },
    {KEY_F8,               "KEY_F8"                },
    {KEY_F9,               "KEY_F9"                },
    {KEY_F10,              "KEY_F10"               },
    {KEY_F11,              "KEY_F11"               },
    {KEY_F12,              "KEY_F12"               },
    {KEY_F13,              "KEY_F13"               },
    {KEY_F14,              "KEY_F14"               },
    {KEY_F15,              "KEY_F15"               },
    {KEY_F16,              "KEY_F16"               },
    {KEY_F17,              "KEY_F17"               },
    {KEY_F18,              "KEY_F18"               },
    {KEY_F19,              "KEY_F19"               },
    {KEY_F20,              "KEY_F20"               },
    {KEY_F21,              "KEY_F21"               },
    {KEY_F22,              "KEY_F22"               },
    {KEY_F23,              "KEY_F23"               },
    {KEY_F24,              "KEY_F24"               },
    {KEY_F25,              "KEY_F25"               },
    {KEY_KEY_PAD_0,        "KEY_KEY_PAD_0"         },
    {KEY_KEY_PAD_1,        "KEY_KEY_PAD_1"         },
    {KEY_KEY_PAD_2,        "KEY_KEY_PAD_2"         },
    {KEY_KEY_PAD_3,        "KEY_KEY_PAD_3"         },
    {KEY_KEY_PAD_4,        "KEY_KEY_PAD_4"         },
    {KEY_KEY_PAD_5,        "KEY_KEY_PAD_5"         },
    {KEY_KEY_PAD_6,        "KEY_KEY_PAD_6"         },
    {KEY_KEY_PAD_7,        "KEY_KEY_PAD_7"         },
    {KEY_KEY_PAD_8,        "KEY_KEY_PAD_8"         },
    {KEY_KEY_PAD_9,        "KEY_KEY_PAD_9"         },
    {KEY_KEY_PAD_DECIMAL,  "KEY_KEY_PAD_DECIMAL"   },
    {KEY_KEY_PAD_DIVIDE,   "KEY_KEY_PAD_DIVIDE"    },
    {KEY_KEY_PAD_MULTIPLY, "KEY_KEY_PAD_MULTIPLY"  },
    {KEY_KEY_PAD_SUBTRACT, "KEY_KEY_PAD_SUBTRACT"  },
    {KEY_KEY_PAD_ADD,      "KEY_KEY_PAD_ADD"       },
    {KEY_KEY_PAD_ENTER,    "KEY_KEY_PAD_ENTER"     },
    {KEY_KEY_PAD_EQUAL,    "KEY_KEY_PAD_EQUAL"     },
    {KEY_LEFT_SHIFT,       "KEY_LEFT_SHIFT"        },
    {KEY_LEFT_CONTROL,     "KEY_LEFT_CONTROL"      },
    {KEY_LEFT_ALT,         "KEY_LEFT_ALT"          },
    {KEY_LEFT_SUPER,       "KEY_LEFT_SUPER"        },
    {KEY_RIGHT_SHIFT,      "KEY_RIGHT_SHIFT"       },
    {KEY_RIGHT_CONTROL,    "KEY_RIGHT_CONTROL"     },
    {KEY_RIGHT_ALT,        "KEY_RIGHT_ALT"         },
    {KEY_RIGHT_SUPER,      "KEY_RIGHT_SUPER"       },
    {KEY_MENU,             "KEY_MENU"              },
};
// clang-format on

// -------------------------------------------------------------------------------------------------
// Application
// -------------------------------------------------------------------------------------------------
Application::Application()
{
    InternalCtor();

    mSettings.appName       = kDefaultAppName;
    mSettings.window.width  = kDefaultWindowWidth;
    mSettings.window.height = kDefaultWindowHeight;
}

Application::Application(uint32_t windowWidth, uint32_t windowHeight, const char* windowTitle)
{
    InternalCtor();

    mSettings.appName       = windowTitle;
    mSettings.window.width  = windowWidth;
    mSettings.window.height = windowHeight;
    mSettings.window.title  = windowTitle;
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

    InitializeAssetDirs();
}

Application* Application::Get()
{
    return sApplicationInstance;
}

void Application::InitializeAssetDirs()
{
#if defined(PPX_ANDROID)
    // On Android, the assets folder is independently specified
    // Assets are loaded relative to this folder
    AddAssetDir("");
#else
    std::filesystem::path path = GetApplicationPath();
    path.remove_filename();
    path /= RELATIVE_PATH_TO_PROJECT_ROOT;
    AddAssetDir(path / "assets");
#endif
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
        ci.useSoftwareRenderer      = mStandardOptions.use_software_renderer;
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

        uint32_t gpuIndex = 0;
        if (!mStandardOptions.use_software_renderer && mStandardOptions.gpu_index != -1) {
            gpuIndex = mStandardOptions.gpu_index;
        }

        grfx::GpuPtr gpu;
        Result       ppxres = mInstance->GetGpu(gpuIndex, &gpu);
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
        ci.width                     = mSettings.window.width;
        ci.height                    = mSettings.window.height;
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
        PPX_LOG_INFO("Creating application swapchain");
        PPX_LOG_INFO("   resolution  : " << mSettings.window.width << "x" << mSettings.window.height);
        PPX_LOG_INFO("   image count : " << mSettings.grfx.swapchain.imageCount);

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
            if ((mSettings.window.width > surfaceMaxImageWidth) || (mSettings.window.height > surfaceMaxImageHeight)) {
                PPX_LOG_WARN("readjusting swapchain/window size from " << mSettings.window.width << "x" << mSettings.window.height << " to " << surfaceMaxImageWidth << "x" << surfaceMaxImageHeight << " to match surface requirements");
                mSettings.window.width  = std::min(mSettings.window.width, surfaceMaxImageWidth);
                mSettings.window.height = std::min(mSettings.window.height, surfaceMaxImageHeight);
            }

            const uint32_t surfaceCurrentImageWidth  = mSurface->GetCurrentImageWidth();
            const uint32_t surfaceCurrentImageHeight = mSurface->GetCurrentImageHeight();
            if ((surfaceCurrentImageWidth != grfx::Surface::kInvalidExtent) &&
                (surfaceCurrentImageHeight != grfx::Surface::kInvalidExtent)) {
                if ((mSettings.window.width != surfaceCurrentImageWidth) ||
                    (mSettings.window.height != surfaceCurrentImageHeight)) {
                    PPX_LOG_WARN("window size " << mSettings.window.width << "x" << mSettings.window.height << " does not match current surface extent " << surfaceCurrentImageWidth << "x" << surfaceCurrentImageHeight);
                }
                PPX_LOG_WARN("surface current extent " << surfaceCurrentImageWidth << "x" << surfaceCurrentImageHeight);
            }
        }

        grfx::SwapchainCreateInfo ci = {};
        ci.pQueue                    = mDevice->GetGraphicsQueue();
        ci.pSurface                  = mSurface;
        ci.width                     = mSettings.window.width;
        ci.height                    = mSettings.window.height;
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
            // The window size could be smaller than the requested one in glfwCreateWindow
            // So the final swapchain size for window needs to be adjusted
            // In the case of debug capture, we don't care about the window size after creating the dummy window
            // restore width and heigh in the settings since they are used by some other systems in the renderer
            mSettings.window.width  = mXrComponent.GetWidth();
            mSettings.window.height = mXrComponent.GetHeight();
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
    windowTitle << mSettings.window.title << " | " << ToString(mSettings.grfx.api) << " | " << mDevice->GetDeviceName() << " | " << Platform::GetPlatformString();

    {
        Result ppxres = mWindow->Create(windowTitle.str().c_str());
        if (ppxres != ppx::SUCCESS) {
            return ppxres;
        }
    }

    // Update window size to the actual size.
    if (!IsXrEnabled()) {
        auto windowSize         = mWindow->Size();
        mSettings.window.width  = windowSize.width;
        mSettings.window.height = windowSize.height;
    }
    return ppx::SUCCESS;
}

void Application::DestroyPlatformWindow()
{
    mWindow->Destroy();
}

void Application::DispatchInitKnobs()
{
    InitKnobs();
}

void Application::DispatchConfig()
{
    Config(mSettings);

    if (mSettings.appName.empty()) {
        mSettings.appName = "PPX Application";
    }

    if (mSettings.window.title.empty()) {
        mSettings.window.title = mSettings.appName;
    }

    mDecoratedApiName = ToString(mSettings.grfx.api);

    if (mSettings.allowThirdPartyAssets) {
        std::filesystem::path path = GetApplicationPath();
        path.remove_filename();
        path /= RELATIVE_PATH_TO_PROJECT_ROOT;
        AddAssetDir(path / "third_party/assets");
    }
}

void Application::DispatchSetup()
{
    Setup();
}

void Application::SaveMetricsReportToDisk()
{
    if (!mSettings.enableMetrics) {
        return;
    }

    // Export the report from the metrics manager to the disk.
    mMetrics.manager.ExportToDisk("report");
}

void Application::DispatchShutdown()
{
    Shutdown();

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

void Application::TakeScreenshot()
{
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

    std::string filepath = mStandardOptions.screenshot_path.empty()
                               ? "screenshot_frame" + std::to_string(mFrameCount) + ".ppm"
                               : mStandardOptions.screenshot_path;
    PPX_CHECKED_CALL(ExportToPPM(filepath, swapchainImg->GetFormat(), texels, width, height, outPitch.rowPitch));

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
    bool widthChanged  = (width != mSettings.window.width);
    bool heightChanged = (height != mSettings.window.height);
    if (widthChanged || heightChanged) {
        // Update the configuration's width and height
        mSettings.window.width  = width;
        mSettings.window.height = height;
        mWindowSurfaceInvalid   = ((width == 0) || (height == 0));

        // Vulkan will return an error if either dimension is 0
        if (!mWindowSurfaceInvalid) {
            // D3D12 swapchain needs resizing
            if ((mDevice->GetApi() == grfx::API_DX_12_0) || (mDevice->GetApi() == grfx::API_DX_12_1)) {
                // Wait for device to idle
                mDevice->WaitIdle();

                PPX_ASSERT_MSG((mSwapchains.size() == 1), "Invalid number of swapchains for D3D12");

                auto ppxres = mSwapchains[0]->Resize(mSettings.window.width, mSettings.window.height);
                if (Failed(ppxres)) {
                    PPX_ASSERT_MSG(false, "D3D12 swapchain resize failed");
                    // Signal the app to quit if swapchain recreation fails
                    mWindow->Quit();
                }

#if defined(PPX_MSW)
                mForceInvalidateClientArea = true;
#endif

                PPX_LOG_INFO("Resized application swapchain");
                PPX_LOG_INFO("   resolution  : " << mSettings.window.width << "x" << mSettings.window.height);
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
        }

        // Dispatch resize event
        DispatchResize(mSettings.window.width, mSettings.window.height);
    }
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

int Application::Run(int argc, char** argv)
{
    // Only allow one instance of Application. Since we can't stop
    // the app in the ctor - stop it here.
    //
    if (this != sApplicationInstance) {
        return false;
    }

    // Parse args.
    if (auto error = mCommandLineParser.Parse(argc, const_cast<const char**>(argv))) {
        PPX_LOG_ERROR(error->errorMsg);
        PPX_ASSERT_MSG(false, "Unable to parse command line arguments");
        return EXIT_FAILURE;
    }
    mStandardOptions = mCommandLineParser.GetOptions().GetStandardOptions();

    // Knobs need to be set up before commandline parsing.
    DispatchInitKnobs();

    if (!mKnobManager.IsEmpty()) {
        mCommandLineParser.AppendUsageMsg(mKnobManager.GetUsageMsg());
    }

    if (mStandardOptions.help) {
        PPX_LOG_INFO(mCommandLineParser.GetUsageMsg());
        return EXIT_SUCCESS;
    }

    if (!mKnobManager.IsEmpty()) {
        auto options = mCommandLineParser.GetOptions();
        mKnobManager.UpdateFromFlags(options);
    }

    // Call config.
    // Put this early because it might disable the display.
    DispatchConfig();

    // If command line argument specified headless.
    if (mStandardOptions.headless) {
        mSettings.headless = true;
    }

#if defined(PPX_LINUX_HEADLESS)
    // Force headless if BigWheels was built without surface support.
    mSettings.headless = true;
#endif

    if (!mWindow) {
        if (mSettings.headless) {
            mWindow = Window::GetImplHeadless(this);
        }
        else {
            mWindow = Window::GetImplNative(this);
        }
        if (!mWindow) {
            PPX_ASSERT_MSG(false, "out of memory");
            return EXIT_FAILURE;
        }
    }

    // If command line argument provided width and height
    bool hasResolutionFlag = (mStandardOptions.resolution.first > 0 && mStandardOptions.resolution.second > 0);
    if (hasResolutionFlag) {
        mSettings.window.width  = mStandardOptions.resolution.first;
        mSettings.window.height = mStandardOptions.resolution.second;
    }

#if defined(PPX_BUILD_XR)
    if (mStandardOptions.xrUIResolution.first > 0 && mStandardOptions.xrUIResolution.second > 0) {
        mSettings.xr.uiWidth  = mStandardOptions.xrUIResolution.first;
        mSettings.xr.uiHeight = mStandardOptions.xrUIResolution.second;
    }
#endif

    mMaxFrames = UINT64_MAX;
    // If command line provided a maximum number of frames to draw.
    if (mStandardOptions.frame_count > 0) {
        mMaxFrames = mStandardOptions.frame_count;
    }

    mRunTimeSeconds = std::numeric_limits<float>::max();
    // If command line provides a maximum number of milliseconds to run.
    if (mStandardOptions.run_time_ms > 0) {
        mRunTimeSeconds = mStandardOptions.run_time_ms / 1000.f;
    }

    // Disable ImGui in headless or deterministic mode.
    // ImGUI is not non-deterministic, but the visible informations (stats, timers) are.
    if ((mSettings.headless || mStandardOptions.deterministic) && mSettings.enableImGui) {
        mSettings.enableImGui = false;
        PPX_LOG_WARN("Headless mode: disabling ImGui");
    }

    // Initialize the platform
    Result ppxres = InitializePlatform();
    if (Failed(ppxres)) {
        return EXIT_FAILURE;
    }

#if defined(PPX_BUILD_XR)
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
        if (hasResolutionFlag) {
            createInfo.resolution.width  = mSettings.window.width;
            createInfo.resolution.height = mSettings.window.height;
        }
        createInfo.uiResolution.width  = mSettings.xr.uiWidth;
        createInfo.uiResolution.height = mSettings.xr.uiHeight;

        mXrComponent.InitializeBeforeGrfxDeviceInit(createInfo);
    }
#endif

    // Create graphics instance
    ppxres = InitializeGrfxDevice();
    if (Failed(ppxres)) {
        return EXIT_FAILURE;
    }

#if defined(PPX_BUILD_XR)
    if (mSettings.xr.enable) {
        mXrComponent.InitializeAfterGrfxDeviceInit(mInstance);
        mSettings.window.width  = mXrComponent.GetWidth();
        mSettings.window.height = mXrComponent.GetHeight();
    }
#endif

    // List gpus
    if (mStandardOptions.list_gpus) {
        uint32_t          count = GetInstance()->GetGpuCount();
        std::stringstream ss;
        for (uint32_t i = 0; i < count; ++i) {
            grfx::GpuPtr gpu;
            GetInstance()->GetGpu(i, &gpu);
            ss << i << " " << gpu->GetDeviceName() << std::endl;
        }
        PPX_LOG_INFO(ss.str());
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

    if (!mSettings.xr.enable) {
        mWindow->Resize({mSettings.window.width, mSettings.window.height});
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
        ScopedTimer("Setup() finished");
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

    while (IsRunning()) {
        // Frame start
        mFrameStartTime = static_cast<float>(mTimer.MillisSinceStart());

#if defined(PPX_BUILD_XR)
        if (mSettings.xr.enable) {
            bool exitRenderLoop = false;
            mXrComponent.PollEvents(exitRenderLoop);
            if (exitRenderLoop) {
                break;
            }

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
            mWindow->ProcessEvent();

            // Start new Imgui frame
            if (mImGui) {
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

        // Take screenshot if this is the requested frame.
        if (mFrameCount == static_cast<uint64_t>(mStandardOptions.screenshot_frame_number)) {
            TakeScreenshot();
        }

        // Frame end
        double nowMs       = mTimer.MillisSinceStart();
        mFrameCount        = mFrameCount + 1;
        mPreviousFrameTime = static_cast<float>(nowMs) - mFrameStartTime;

        // Keep a rolling window of frame times to calculate stats,
        // if requested.
        if (mStandardOptions.stats_frame_window > 0) {
            mFrameTimesMs.push_back(mPreviousFrameTime);
            if (mFrameTimesMs.size() > mStandardOptions.stats_frame_window) {
                mFrameTimesMs.pop_front();
            }
            float totalFrameTimeMs = std::accumulate(mFrameTimesMs.begin(), mFrameTimesMs.end(), 0.f);
            mAverageFPS            = mFrameTimesMs.size() / (totalFrameTimeMs / 1000.f);
            mAverageFrameTime      = totalFrameTimeMs / mFrameTimesMs.size();
        }
        else {
            mAverageFPS       = static_cast<float>(mFrameCount / nowMs / 1000.f);
            mAverageFrameTime = static_cast<float>(nowMs / mFrameCount);
        }

        // Update the metrics.
        UpdateMetrics();

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
        if (mFrameCount >= mMaxFrames || (nowMs / 1000.f) > mRunTimeSeconds) {
            Quit();
        }
    }
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
    if (mSettings.xr.enable) {
        mXrComponent.Destroy();
    }
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

const StandardOptions Application::GetStandardOptions() const
{
    return mStandardOptions;
}

const CliOptions& Application::GetExtraOptions() const
{
    return mCommandLineParser.GetOptions();
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

std::optional<std::filesystem::path> GetShaderPathSuffix(const ppx::ApplicationSettings& settings, const std::string& baseName)
{
    switch (settings.grfx.api) {
        case grfx::API_DX_12_0:
        case grfx::API_DX_12_1:
            return std::filesystem::path("dxil") / (baseName + ".dxil");
        case grfx::API_VK_1_1:
        case grfx::API_VK_1_2:
            return std::filesystem::path("spv") / (baseName + ".spv");
        default:
            return std::nullopt;
    }
};

} // namespace

std::vector<char> Application::LoadShader(const std::filesystem::path& baseDir, const std::string& baseName) const
{
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

Result Application::CreateShader(const std::filesystem::path& baseDir, const std::string& baseName, grfx::ShaderModule** ppShaderModule) const
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

float Application::GetElapsedSeconds() const
{
    if (mStandardOptions.deterministic) {
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

metrics::Run* Application::StartMetricsRun(const std::string& name)
{
    PPX_ASSERT_MSG(mSettings.enableMetrics, "Application::Settings::enableMetrics must be set to true before using the metrics capabilities");
    if (!mSettings.enableMetrics) {
        return nullptr;
    }

    PPX_ASSERT_MSG(mMetrics.pCurrentRun == nullptr, "a run is already active; stop it before starting another one");
    mMetrics.pCurrentRun = mMetrics.manager.AddRun(name.c_str());

    // Add default metrics to every single run
    {
        metrics::MetricMetadata metadata = {};
        metadata.name                    = "cpu_frame_time";
        metadata.unit                    = "ms";
        metadata.interpretation          = metrics::MetricInterpretation::LOWER_IS_BETTER;
        mMetrics.pCpuFrameTime           = mMetrics.pCurrentRun->AddMetric<metrics::MetricGauge>(metadata);
    }
    {
        metrics::MetricMetadata metadata = {};
        metadata.name                    = "framerate";
        metadata.unit                    = "";
        metadata.interpretation          = metrics::MetricInterpretation::HIGHER_IS_BETTER;
        mMetrics.pFramerate              = mMetrics.pCurrentRun->AddMetric<metrics::MetricGauge>(metadata);
    }
    {
        metrics::MetricMetadata metadata = {};
        metadata.name                    = "frame_count";
        metadata.unit                    = "";
        metadata.interpretation          = metrics::MetricInterpretation::NONE;
        mMetrics.pFrameCount             = mMetrics.pCurrentRun->AddMetric<metrics::MetricCounter>(metadata);
    }

    mMetrics.resetFramerateTracking = true;
    return mMetrics.pCurrentRun;
}

void Application::StopMetricsRun()
{
    PPX_ASSERT_MSG(mSettings.enableMetrics, "Application::Settings::enableMetrics must be set to true before using the metrics capabilities");
    if (!mSettings.enableMetrics) {
        return;
    }

    PPX_ASSERT_MSG(mMetrics.pCurrentRun != nullptr, "there are no active runs");
    mMetrics.pCurrentRun   = nullptr;
    mMetrics.pCpuFrameTime = nullptr;
    mMetrics.pFramerate    = nullptr;
    mMetrics.pFrameCount   = nullptr;
}

bool Application::HasActiveMetricsRun() const
{
    return mSettings.enableMetrics && mMetrics.pCurrentRun != nullptr;
}

void Application::UpdateMetrics()
{
    if (!HasActiveMetricsRun()) {
        return;
    }

    PPX_ASSERT_NULL_ARG(mMetrics.pCpuFrameTime);
    PPX_ASSERT_NULL_ARG(mMetrics.pFramerate);
    PPX_ASSERT_NULL_ARG(mMetrics.pFrameCount);

    const double seconds = GetElapsedSeconds();

    // Record default metrics
    mMetrics.pCpuFrameTime->RecordEntry(seconds, mPreviousFrameTime);
    mMetrics.pFrameCount->Increment(1);

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
            const double framerate = mMetrics.framerateFrameCount / framerateSecondsDiff;
            mMetrics.pFramerate->RecordEntry(seconds, framerate);
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

const char* GetKeyCodeString(KeyCode code)
{
    if ((code < KEY_RANGE_FIRST) || (code >= KEY_RANGE_LAST)) {
        return sKeyCodeString[0];
    }
    return sKeyCodeString[static_cast<uint32_t>(code)];
}

} // namespace ppx
