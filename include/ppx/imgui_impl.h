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

#ifndef ppx_imgui_impl_h
#define ppx_imgui_impl_h

#include "imgui.h"

#include "grfx/grfx_config.h"

#if defined(PPX_D3D12)
struct ID3D12DescriptorHeap;
#endif // defined(PPX_D3D12)

namespace ppx {

class Application;

class ImGuiImpl
{
public:
    ImGuiImpl() {}
    virtual ~ImGuiImpl() {}

    virtual Result Init(ppx::Application* pApp);
    virtual void   Shutdown(ppx::Application* pApp) = 0;
    virtual void   NewFrame();
    virtual void   Render(grfx::CommandBuffer* pCommandBuffer) = 0;
    virtual void   ProcessEvent() {}

protected:
    virtual Result InitApiObjects(ppx::Application* pApp) = 0;
    void           SetColorStyle();
    virtual void   NewFrameApi() = 0;
};

class ImGuiImplVk
    : public ImGuiImpl
{
public:
    ImGuiImplVk() {}
    virtual ~ImGuiImplVk() {}

    virtual void Shutdown(ppx::Application* pApp) override;
    virtual void Render(grfx::CommandBuffer* pCommandBuffer) override;
    virtual void ProcessEvent() override;
#if defined(PPX_BUILD_XR)
    void ProcessXrInput();
#endif

protected:
    virtual Result InitApiObjects(ppx::Application* pApp) override;
    virtual void   NewFrameApi() override;

private:
    grfx::DescriptorPoolPtr mPool;
#if defined(PPX_BUILD_XR)
    bool mSimulatedMouseDown = false;
#endif
};

#if defined(PPX_D3D12)
class ImGuiImplDx12
    : public ImGuiImpl
{
public:
    ImGuiImplDx12() {}
    virtual ~ImGuiImplDx12() {}

    virtual void Shutdown(ppx::Application* pApp) override;
    virtual void Render(grfx::CommandBuffer* pCommandBuffer) override;

protected:
    virtual Result InitApiObjects(ppx::Application* pApp) override;
    virtual void   NewFrameApi() override;

private:
    ID3D12DescriptorHeap* mHeapCBVSRVUAV = nullptr;
};
#endif // defined(PPX_D3D12)

} // namespace ppx

#endif // ppx_imgui_impl_h
