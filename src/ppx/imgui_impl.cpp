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

#include "ppx/imgui_impl.h"
#include "ppx/imgui/font_inconsolata.h"
#include "ppx/application.h"
#include "ppx/grfx/grfx_device.h"

#if !defined(PPX_ANDROID)
#include "backends/imgui_impl_glfw.h"
#else
#include "backends/imgui_impl_android.h"
#endif

#if defined(PPX_D3D12)
#include "backends/imgui_impl_dx12.h"

#include "ppx/grfx/dx12/dx12_command.h"
#include "ppx/grfx/dx12/dx12_device.h"
#endif // defined(PPX_D3D12)

#if defined(PPX_VULKAN)
#include "backends/imgui_impl_vulkan.h"

#include "ppx/grfx/vk/vk_command.h"
#include "ppx/grfx/vk/vk_descriptor.h"
#include "ppx/grfx/vk/vk_device.h"
#include "ppx/grfx/vk/vk_gpu.h"
#include "ppx/grfx/vk/vk_instance.h"
#include "ppx/grfx/vk/vk_queue.h"
#include "ppx/grfx/vk/vk_render_pass.h"
#endif // defined(PPX_VULKAN)

#if defined(PPX_MSW)
#include <ShellScalingApi.h>
#endif

namespace ppx {

// -------------------------------------------------------------------------------------------------
// ImGuiImpl
// -------------------------------------------------------------------------------------------------
Result ImGuiImpl::Init(ppx::Application* pApp)
{
    // Setup Dear ImGui binding
    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    ImGuiIO& io = ImGui::GetIO();

    float fontSize = 16.0f;
#if defined(PPX_MSW)
    HWND     activeWindow = GetActiveWindow();
    HMONITOR monitor      = MonitorFromWindow(activeWindow, MONITOR_DEFAULTTONEAREST);

    DEVICE_SCALE_FACTOR scale = SCALE_100_PERCENT;
    HRESULT             hr    = GetScaleFactorForMonitor(monitor, &scale);
    if (FAILED(hr)) {
        return ppx::ERROR_FAILED;
    }

    float fontScale = 1.0f;
    // clang-format off
    switch (scale) {
        default: break;
        case SCALE_120_PERCENT: fontScale = 1.20f; break;
        case SCALE_125_PERCENT: fontScale = 1.25f; break;
        case SCALE_140_PERCENT: fontScale = 1.40f; break;
        case SCALE_150_PERCENT: fontScale = 1.50f; break;
        case SCALE_160_PERCENT: fontScale = 1.60f; break;
        case SCALE_175_PERCENT: fontScale = 1.75f; break;
        case SCALE_180_PERCENT: fontScale = 1.80f; break;
        case SCALE_200_PERCENT: fontScale = 2.00f; break;
        case SCALE_225_PERCENT: fontScale = 2.25f; break;
        case SCALE_250_PERCENT: fontScale = 2.50f; break;
        case SCALE_300_PERCENT: fontScale = 3.00f; break;
        case SCALE_350_PERCENT: fontScale = 3.50f; break;
        case SCALE_400_PERCENT: fontScale = 4.00f; break;
        case SCALE_450_PERCENT: fontScale = 4.50f; break;
        case SCALE_500_PERCENT: fontScale = 5.00f; break;
    }
    // clang-format on

    //// Get the logical width and height of the monitor
    // MONITORINFOEX monitorInfoEx = {};
    // monitorInfoEx.cbSize        = sizeof(monitorInfoEx);
    // GetMonitorInfo(monitor, &monitorInfoEx);
    // auto cxLogical = monitorInfoEx.rcMonitor.right - monitorInfoEx.rcMonitor.left;
    // auto cyLogical = monitorInfoEx.rcMonitor.bottom - monitorInfoEx.rcMonitor.top;
    //
    //// Get the physical width and height of the monitor
    // DEVMODE devMode       = {};
    // devMode.dmSize        = sizeof(devMode);
    // devMode.dmDriverExtra = 0;
    // EnumDisplaySettings(monitorInfoEx.szDevice, ENUM_CURRENT_SETTINGS, &devMode);
    // auto cxPhysical = devMode.dmPelsWidth;
    // auto cyPhysical = devMode.dmPelsHeight;
    //
    //// Calculate the scaling factor
    // float horizontalScale = ((float)cxPhysical / (float)cxLogical);
    // float verticalScale   = ((float)cyPhysical / (float)cyLogical);

    // Scale fontSize based on scaling factor
    fontSize *= fontScale;
#endif

    ImFontConfig fontConfig         = {};
    fontConfig.FontDataOwnedByAtlas = false;

    ImFont* pFont = io.Fonts->AddFontFromMemoryTTF(
        const_cast<void*>(static_cast<const void*>(imgui::kFontInconsolata)),
        static_cast<int>(imgui::kFontInconsolataSize),
        fontSize,
        &fontConfig);

    PPX_ASSERT_MSG(!IsNull(pFont), "imgui add font failed");

    Result ppxres = InitApiObjects(pApp);
    if (Failed(ppxres)) {
        return ppxres;
    }

    return ppx::SUCCESS;
}

void ImGuiImpl::SetColorStyle()
{
    // ImGui::StyleColorsClassic();
    ImGui::StyleColorsDark();
    // ImGui::StyleColorsLight();
}

void ImGuiImpl::NewFrame()
{
    Application* pApp = Application::Get();

    ImGuiIO& io      = ImGui::GetIO();
    io.DisplaySize.x = static_cast<float>(pApp->GetUIWidth());
    io.DisplaySize.y = static_cast<float>(pApp->GetUIHeight());
    NewFrameApi();
}

// -------------------------------------------------------------------------------------------------
// ImGuiImplDx12
// -------------------------------------------------------------------------------------------------
#if defined(PPX_D3D12)

Result ImGuiImplDx12::InitApiObjects(ppx::Application* pApp)
{
    // Setup GLFW binding - yes...we're using the one for Vulkan :)
    GLFWwindow* pWindow = static_cast<GLFWwindow*>(pApp->GetWindow()->NativeHandle());
    ImGui_ImplGlfw_InitForVulkan(pWindow, false);

    // Setup style
    SetColorStyle();

    // Setup descriptor heap
    {
        D3D12_DESCRIPTOR_HEAP_DESC desc = {};
        desc.Type                       = D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV;

        desc.NumDescriptors = 1 + pApp->GetNumFramesInFlight(); // Texture + CBVs * #IFF

        desc.Flags    = D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE;
        desc.NodeMask = 0;

        grfx::dx12::D3D12DevicePtr device = grfx::dx12::ToApi(pApp->GetDevice())->GetDxDevice();
        HRESULT                    hr     = device->CreateDescriptorHeap(&desc, IID_PPV_ARGS(&mHeapCBVSRVUAV));
        if (FAILED(hr)) {
            PPX_ASSERT_MSG(false, "ID3D12Device::CreateDescriptorHeap(CBVSRVUAV) failed");
            return ppx::ERROR_API_FAILURE;
        }
        PPX_LOG_OBJECT_CREATION(D3D12DescriptorHeap(CBVSRVUAV), mHeapCBVSRVUAV.Get());
    }

    // Setup DX12 binding
    bool result = ImGui_ImplDX12_Init(
        grfx::dx12::ToApi(pApp->GetDevice())->GetDxDevice(),
        static_cast<int>(pApp->GetNumFramesInFlight()),
        grfx::dx::ToDxgiFormat(pApp->GetUISwapchain()->GetColorFormat()),
        mHeapCBVSRVUAV,
        mHeapCBVSRVUAV->GetCPUDescriptorHandleForHeapStart(),
        mHeapCBVSRVUAV->GetGPUDescriptorHandleForHeapStart());
    if (!result) {
        return ppx::ERROR_IMGUI_INITIALIZATION_FAILED;
    }

    return ppx::SUCCESS;
}

void ImGuiImplDx12::Shutdown(ppx::Application* pApp)
{
    ImGui_ImplDX12_Shutdown();
    ImGui_ImplGlfw_Shutdown();
    ImGui::DestroyContext();

    if (mHeapCBVSRVUAV != nullptr) {
        mHeapCBVSRVUAV->Release();
        mHeapCBVSRVUAV = nullptr;
    }
}

void ImGuiImplDx12::NewFrameApi()
{
    ImGui_ImplDX12_NewFrame();
    ImGui_ImplGlfw_NewFrame();
    ImGui::NewFrame();
}

void ImGuiImplDx12::Render(grfx::CommandBuffer* pCommandBuffer)
{
    grfx::dx12::D3D12GraphicsCommandListPtr commandList = grfx::dx12::ToApi(pCommandBuffer)->GetDxCommandList();
    commandList->SetDescriptorHeaps(1, &mHeapCBVSRVUAV);

    ImGui::Render();
    ImGui_ImplDX12_RenderDrawData(ImGui::GetDrawData(), grfx::dx12::ToApi(pCommandBuffer)->GetDxCommandList());
}

#endif // defined(PPX_D3D12)

// -------------------------------------------------------------------------------------------------
// ImGuiImplVk
// -------------------------------------------------------------------------------------------------
#if defined(PPX_VULKAN)

Result ImGuiImplVk::InitApiObjects(ppx::Application* pApp)
{
#if defined(PPX_ANDROID)
    ImGui_ImplAndroid_Init(pApp->GetAndroidContext()->window);
#else
    // Setup GLFW binding
    GLFWwindow* pWindow = static_cast<GLFWwindow*>(pApp->GetWindow()->NativeHandle());
    ImGui_ImplGlfw_InitForVulkan(pWindow, false);
#endif

    // Setup style
    SetColorStyle();

    // Create descriptor pool
    {
        grfx::DescriptorPoolCreateInfo ci = {};
        ci.combinedImageSampler           = 1;

        Result ppxres = pApp->GetDevice()->CreateDescriptorPool(&ci, &mPool);
        if (Failed(ppxres)) {
            return ppxres;
        }
    }

    // Setup Vulkan binding
    {
        ImGui_ImplVulkan_InitInfo init_info = {};
        init_info.Instance                  = grfx::vk::ToApi(pApp->GetInstance())->GetVkInstance();
        init_info.PhysicalDevice            = grfx::vk::ToApi(pApp->GetDevice()->GetGpu())->GetVkGpu();
        init_info.Device                    = grfx::vk::ToApi(pApp->GetDevice())->GetVkDevice();
        init_info.QueueFamily               = grfx::vk::ToApi(pApp->GetGraphicsQueue())->GetQueueFamilyIndex();
        init_info.Queue                     = grfx::vk::ToApi(pApp->GetGraphicsQueue())->GetVkQueue();
        init_info.PipelineCache             = VK_NULL_HANDLE;
        init_info.DescriptorPool            = grfx::vk::ToApi(mPool)->GetVkDescriptorPool();
        init_info.MinImageCount             = pApp->GetUISwapchain()->GetImageCount();
        init_info.ImageCount                = pApp->GetUISwapchain()->GetImageCount();
        init_info.Allocator                 = VK_NULL_HANDLE;
        init_info.CheckVkResultFn           = nullptr;
#if IMGUI_VERSION_NUM > 18970
        init_info.UseDynamicRendering                                 = pApp->GetSettings()->grfx.enableImGuiDynamicRendering;
        init_info.PipelineRenderingCreateInfo                         = {VK_STRUCTURE_TYPE_PIPELINE_RENDERING_CREATE_INFO};
        VkFormat colorFormat                                          = grfx::vk::ToVkFormat(pApp->GetUISwapchain()->GetColorFormat());
        init_info.PipelineRenderingCreateInfo.colorAttachmentCount    = 1;
        init_info.PipelineRenderingCreateInfo.pColorAttachmentFormats = &colorFormat;
#else
        PPX_ASSERT_MSG(!pApp->GetSettings()->grfx.enableImGuiDynamicRendering, "This version of ImGui does not have dynamic rendering support");
#endif

        grfx::RenderPassPtr renderPass = pApp->GetUISwapchain()->GetRenderPass(0, grfx::ATTACHMENT_LOAD_OP_LOAD);
        PPX_ASSERT_MSG(!renderPass.IsNull(), "[imgui:vk] failed to get swapchain renderpass");

        init_info.RenderPass = grfx::vk::ToApi(renderPass)->GetVkRenderPass();
        bool result          = ImGui_ImplVulkan_Init(&init_info);
        if (!result) {
            return ppx::ERROR_IMGUI_INITIALIZATION_FAILED;
        }
    }

    // Upload Fonts
    {
        // Create command buffer
        grfx::CommandBufferPtr commandBuffer;
        Result                 ppxres = pApp->GetGraphicsQueue()->CreateCommandBuffer(&commandBuffer, 0, 0);
        if (Failed(ppxres)) {
            PPX_ASSERT_MSG(false, "[imgui:vk] command buffer create failed");
            return ppxres;
        }

        // Begin command buffer
        ppxres = commandBuffer->Begin();
        if (Failed(ppxres)) {
            PPX_ASSERT_MSG(false, "[imgui:vk] command buffer begin failed");
            return ppxres;
        }

        // End command buffer
        ppxres = commandBuffer->End();
        if (Failed(ppxres)) {
            PPX_ASSERT_MSG(false, "[imgui:vk] command buffer end failed");
            return ppxres;
        }

        // Submit
        grfx::SubmitInfo submitInfo   = {};
        submitInfo.commandBufferCount = 1;
        submitInfo.ppCommandBuffers   = &commandBuffer;

        ppxres = pApp->GetGraphicsQueue()->Submit(&submitInfo);
        if (Failed(ppxres)) {
            PPX_ASSERT_MSG(false, "[imgui:vk] command buffer submit failed");
            return ppxres;
        }

        // Wait for idle
        ppxres = pApp->GetGraphicsQueue()->WaitIdle();
        if (Failed(ppxres)) {
            PPX_ASSERT_MSG(false, "[imgui:vk] queue wait idle failed");
            return ppxres;
        }

        // Destroy command buffer
        pApp->GetGraphicsQueue()->DestroyCommandBuffer(commandBuffer);
    }

    return ppx::SUCCESS;
}

void ImGuiImplVk::Shutdown(ppx::Application* pApp)
{
    ImGui_ImplVulkan_Shutdown();
#if defined(PPX_ANDROID)
    ImGui_ImplAndroid_Shutdown();
#else
    ImGui_ImplGlfw_Shutdown();
#endif
    ImGui::DestroyContext();

    if (mPool) {
        pApp->GetDevice()->DestroyDescriptorPool(mPool);
        mPool.Reset();
    }
}

void ImGuiImplVk::NewFrameApi()
{
    ImGui_ImplVulkan_NewFrame();
#if defined(PPX_ANDROID)
    ImGui_ImplAndroid_NewFrame();
#else
    ImGui_ImplGlfw_NewFrame();
#endif
    ImGui::NewFrame();
}

void ImGuiImplVk::Render(grfx::CommandBuffer* pCommandBuffer)
{
    ImGui::Render();
    ImGui_ImplVulkan_RenderDrawData(ImGui::GetDrawData(), grfx::vk::ToApi(pCommandBuffer)->GetVkCommandBuffer());
}

#if defined(PPX_BUILD_XR)
void ImGuiImplVk::ProcessXrInput()
{
    Application* pApp = Application::Get();
    if (!pApp->IsXrEnabled()) {
        return;
    }

    ImGuiIO&     io          = ImGui::GetIO();
    XrComponent& xrComponent = pApp->GetXrComponent();

    bool isMouseDown = xrComponent.GetUIClickState().value_or(false);
    if (isMouseDown != mSimulatedMouseDown) {
        mSimulatedMouseDown = isMouseDown;
        io.AddMouseButtonEvent(ImGuiMouseButton_Left, isMouseDown);
    }

    std::optional<XrVector2f> cursor = xrComponent.GetUICursor();
    if (cursor.has_value()) {
        // Mapping cursor location from meters to Imgui screen coordinate
        // x axis: [-0.5m, +0.5m] -> [0, 1] * swapchain.width
        // y axis: [-0.5m, +0.5m] -> [1, 0] * swapchain.height
        io.MousePos = ImVec2{
            (cursor->x + 0.5f) * pApp->GetUISwapchain()->GetWidth(),
            (-cursor->y + 0.5f) * pApp->GetUISwapchain()->GetHeight(),
        };
        io.MouseDrawCursor = true;
        ImGui::SetMouseCursor(ImGuiMouseCursor_Arrow);
    }
    else {
        io.MousePos        = ImVec2{-FLT_MAX, -FLT_MAX};
        io.MouseDrawCursor = false;
        ImGui::SetMouseCursor(ImGuiMouseCursor_None);
    }
}
#endif

void ImGuiImplVk::ProcessEvents()
{
#if defined(PPX_BUILD_XR)
    ProcessXrInput();
#endif
}

#endif // defined(PPX_VULKAN)

} // namespace ppx
