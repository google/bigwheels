// Copyright 2023 Google LLC
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

#if defined(PPX_ANDROID)

#include "ppx/application.h"
#include "ppx/window.h"
#include "ppx/grfx/grfx_swapchain.h"

#include <android_native_app_glue.h>

#include "backends/imgui_impl_android.h"

namespace ppx {

// -------------------------------------------------------------------------------------------------
// Android Window
// -------------------------------------------------------------------------------------------------

class WindowImplAndroid : public Window
{
public:
    WindowImplAndroid(Application*);

    Result Create(const char* title) final;
    bool   IsRunning() const final;
    void   ProcessEvent() final;

    WindowSize Size() const final;

    void FillSurfaceInfo(grfx::SurfaceCreateInfo*) const final;

    // Android callbacks.
    void    OnAppCmd(int32_t cmd);
    int32_t OnInputEvent(AInputEvent*);

private:
    android_app* mAndroidApp  = nullptr;
    bool         mWindowReady = false;
    WindowSize   mSize        = {};
};

std::unique_ptr<Window> Window::GetImplAndroid(Application* pApp)
{
    return std::unique_ptr<Window>(new WindowImplAndroid(pApp));
}

extern "C" void WindowImplAndroidOnAppCmd(android_app* pApp, int32_t cmd)
{
    static_cast<WindowImplAndroid*>(pApp->userData)->OnAppCmd(cmd);
}

extern "C" int32_t WindowImplAndroidOnInputEvent(android_app* pApp, AInputEvent* event)
{
    return static_cast<WindowImplAndroid*>(pApp->userData)->OnInputEvent(event);
}

WindowImplAndroid::WindowImplAndroid(Application* pApp)
    : Window(pApp), mAndroidApp(pApp->GetAndroidContext())
{
    mAndroidApp->onAppCmd     = WindowImplAndroidOnAppCmd;
    mAndroidApp->onInputEvent = WindowImplAndroidOnInputEvent;
    mAndroidApp->userData     = this;
}

Result WindowImplAndroid::Create(const char* title)
{
    // Wait until the android window is created.
    while (IsRunning() && !mWindowReady) {
        ProcessEvent();
    }
    return ppx::SUCCESS;
}

bool WindowImplAndroid::IsRunning() const
{
    return Window::IsRunning() && !mAndroidApp->destroyRequested;
}

void WindowImplAndroid::ProcessEvent()
{
    int                  events;
    android_poll_source* pSource;
    if (ALooper_pollAll(0, nullptr, &events, (void**)&pSource) >= 0) {
        if (pSource) {
            pSource->process(mAndroidApp, pSource);
        }
    }
}

WindowSize WindowImplAndroid::Size() const
{
    if (App()->IsXrEnabled() || mSize.width == 0 || mSize.height == 0) {
        // Return a default size if the window has not been initialized.
        return Window::Size();
    }
    return mSize;
}

void WindowImplAndroid::FillSurfaceInfo(grfx::SurfaceCreateInfo* pCreateInfo) const
{
    pCreateInfo->androidAppContext = App()->GetAndroidContext();
}

void WindowImplAndroid::OnAppCmd(int32_t cmd)
{
    switch (cmd) {
        case APP_CMD_INIT_WINDOW:
            mWindowReady = true;
            if (!App()->IsXrEnabled() && mAndroidApp->window != nullptr) {
                mSize.width  = static_cast<uint32_t>(ANativeWindow_getWidth(mAndroidApp->window));
                mSize.height = static_cast<uint32_t>(ANativeWindow_getHeight(mAndroidApp->window));
            }
            break;
        case APP_CMD_TERM_WINDOW:
            Quit();
            break;
        default:
            break;
    }
}

int32_t WindowImplAndroid::OnInputEvent(AInputEvent* pEvent)
{
    if (App()->GetSettings()->enableImGui) {
        return ImGui_ImplAndroid_HandleInputEvent(pEvent);
    }
    return 0;
}

} // namespace ppx

#endif
