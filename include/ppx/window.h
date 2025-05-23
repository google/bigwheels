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

#ifndef ppx_window_h
#define ppx_window_h

#include "ppx/config.h"

#include <memory>

namespace ppx {

enum WindowState
{
    WINDOW_STATE_RESTORED  = 0,
    WINDOW_STATE_ICONIFIED = 1,
    WINDOW_STATE_MAXIMIZED = 2,
};

namespace grfx {
struct SurfaceCreateInfo;
} // namespace grfx

class Application;

struct WindowSize
{
    uint32_t width  = 0;
    uint32_t height = 0;

    WindowSize() = default;
    WindowSize(uint32_t width_, uint32_t height_)
        : width(width_), height(height_) {}

    bool operator==(const WindowSize& other) const { return width == other.width && height == other.height; };
};

class Window
{
public:
    static std::unique_ptr<Window> GetImplHeadless(Application*);
    static std::unique_ptr<Window> GetImplNative(Application* pApp)
    {
#if defined(PPX_ANDROID)
        return GetImplAndroid(pApp);
#else
        return GetImplGLFW(pApp);
#endif
    }

    virtual ~Window() = default;

    Window(const Window&) = delete;

    Window& operator=(const Window&) = delete;

    // Actually create a window.
    virtual Result Create(const char* title)
    {
        return ppx::SUCCESS;
    }

    // Signal an intent to quit.
    virtual void Quit()
    {
        mQuit = true;
    }

    // Destory the window.
    virtual Result Destroy()
    {
        return ppx::SUCCESS;
    }

    virtual bool IsRunning() const
    {
        return !mQuit;
    }
    virtual Result Resize(const WindowSize&)
    {
        return ppx::SUCCESS;
    }
    virtual void ProcessEvent()
    {
    }
    virtual void* NativeHandle()
    {
        return nullptr;
    }

    virtual WindowSize Size() const;

    WindowState GetState() const
    {
        return mState;
    }
    bool IsRestored() const
    {
        return (GetState() == WINDOW_STATE_RESTORED);
    }
    bool IsIconified() const
    {
        return (GetState() == WINDOW_STATE_ICONIFIED);
    }
    bool IsMaximized() const
    {
        return (GetState() == WINDOW_STATE_MAXIMIZED);
    }

    virtual void FillSurfaceInfo(grfx::SurfaceCreateInfo*) const
    {
    }

protected:
    Window(Application*);

    Application* App() const
    {
        return mApp;
    }

private:
    Application* mApp;

    bool        mQuit  = false;
    WindowState mState = WINDOW_STATE_RESTORED;

#if defined(PPX_ANDROID)
    static std::unique_ptr<Window> GetImplAndroid(Application*);
#else
    static std::unique_ptr<Window> GetImplGLFW(Application*);
#endif

    friend class Application;
    void SetState(WindowState state)
    {
        mState = state;
    }
};

} // namespace ppx

#endif // ppx_window_h
