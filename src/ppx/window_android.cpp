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
// WindowEvents
// -------------------------------------------------------------------------------------------------

struct WindowEvents
{
    void ProcessInputEvent(Application* pApp, AInputEvent* pEvent);

    int32_t InputCallback(Application* pApp, AInputEvent* pEvent);

    void ResizeCallback(Application* pApp, int width, int height);

private:
    // Pointer ID of the first touch event when touch events began,
    // it is reset to -1 when that first touch event has ended, even
    // if other touch events are still active.
    int32_t mFirstTouchId = -1;
};

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

    void OnResizeEvent();

private:
    // Processes an input event to emulate a mouse.
    void ProcessInputEvent(AInputEvent* pEvent);

    android_app* mAndroidApp  = nullptr;
    bool         mWindowReady = false;
    WindowSize   mSize        = {};
    WindowEvents mEvents      = {};
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
    if (App()->IsXrEnabled()) {
        // xrComponent is taking care of window size on XR builds.
        return Window::Size();
    }
    if (mSize.width == 0 || mSize.height == 0) {
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
            if (mAndroidApp->window != nullptr) {
                mSize.width  = static_cast<uint32_t>(ANativeWindow_getWidth(mAndroidApp->window));
                mSize.height = static_cast<uint32_t>(ANativeWindow_getHeight(mAndroidApp->window));
            }
            break;
        case APP_CMD_TERM_WINDOW:
            Quit();
            break;
        case APP_CMD_WINDOW_RESIZED:
            mSize.width  = static_cast<uint32_t>(ANativeWindow_getWidth(mAndroidApp->window));
            mSize.height = static_cast<uint32_t>(ANativeWindow_getHeight(mAndroidApp->window));
            OnResizeEvent();
            break;
        default:
            break;
    }
}

int32_t WindowImplAndroid::OnInputEvent(AInputEvent* pEvent)
{
    return mEvents.InputCallback(App(), pEvent);
}

void WindowImplAndroid::OnResizeEvent()
{
    if (App()->IsXrEnabled()) {
        // Do not send resize event on XR.
        return;
    }
    WindowSize size = Size();
    mEvents.ResizeCallback(App(), size.width, size.height);
}

void WindowEvents::ProcessInputEvent(Application* pApp, AInputEvent* pEvent)
{
    int32_t eventType = AInputEvent_getType(pEvent);
    if (eventType != AINPUT_EVENT_TYPE_MOTION) {
        return;
    }

    int32_t  eventAction = AMotionEvent_getAction(pEvent);
    uint32_t flags       = eventAction & AMOTION_EVENT_ACTION_MASK;

    switch (flags) {
        default:
            break;
        case AMOTION_EVENT_ACTION_DOWN: {
            // First touch event becomes the primary touch we track.
            mFirstTouchId = AMotionEvent_getPointerId(pEvent, 0);
            float x       = AMotionEvent_getX(pEvent, 0);
            float y       = AMotionEvent_getY(pEvent, 0);
            // Call MouseMoveCallback first without any buttons "pressed",
            // to update the mouse location, since it is not tracked otherwise.
            pApp->MouseMoveCallback(x, y, 0);
            pApp->MouseDownCallback(x, y, MOUSE_BUTTON_LEFT);
            break;
        }
        case AMOTION_EVENT_ACTION_UP: {
            // Last touch event is up, if this was the first touch
            // that went up, end mouse down event.
            if (mFirstTouchId > 0) {
                mFirstTouchId = -1;
                float x       = AMotionEvent_getX(pEvent, 0);
                float y       = AMotionEvent_getY(pEvent, 0);
                pApp->MouseUpCallback(x, y, MOUSE_BUTTON_LEFT);
            }
            break;
        }
        case AMOTION_EVENT_ACTION_POINTER_UP: {
            int32_t pointerIndex = (eventAction & AMOTION_EVENT_ACTION_POINTER_INDEX_MASK) >> AMOTION_EVENT_ACTION_POINTER_INDEX_SHIFT;
            int32_t pointerId    = AMotionEvent_getPointerId(pEvent, pointerIndex);
            // Only issue mouse up event, if the first registered touch has ended.
            if (pointerId == mFirstTouchId) {
                mFirstTouchId = -1;
                float x       = AMotionEvent_getX(pEvent, pointerIndex);
                float y       = AMotionEvent_getY(pEvent, pointerIndex);
                pApp->MouseUpCallback(x, y, MOUSE_BUTTON_LEFT);
            }
            break;
        }
        case AMOTION_EVENT_ACTION_MOVE: {
            if (mFirstTouchId != -1) {
                // Only track the first registered touch event.
                float x = AMotionEvent_getX(pEvent, 0);
                float y = AMotionEvent_getY(pEvent, 0);
                pApp->MouseMoveCallback(x, y, MOUSE_BUTTON_LEFT);
            }
            break;
        }
    }
}

int32_t WindowEvents::InputCallback(Application* pApp, AInputEvent* pEvent)
{
    if (pApp->GetSettings()->emulateMouseAndroid) {
        ProcessInputEvent(pApp, pEvent);
    }

    if (pApp->GetSettings()->enableImGui) {
        return ImGui_ImplAndroid_HandleInputEvent(pEvent);
    }
    return 0;
}

void WindowEvents::ResizeCallback(Application* pApp, int width, int height)
{
    pApp->ResizeCallback(width, height);
}
} // namespace ppx

#endif
