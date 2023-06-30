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

#if !defined(PPX_ANDROID)

#include "ppx/application.h"
#include "ppx/window.h"
#include "ppx/grfx/grfx_swapchain.h"

#include "backends/imgui_impl_glfw.h"

#include <map>

#if defined(PPX_LINUX_XCB)
#include <X11/Xlib-xcb.h>
#elif defined(PPX_LINUX_XLIB)
#error "Xlib not implemented"
#elif defined(PPX_LINUX_WAYLAND)
#error "Wayland not implemented"
#endif

#include <GLFW/glfw3.h>
// clang-format off
#if defined(PPX_LINUX)
#   define GLFW_EXPOSE_NATIVE_X11
#elif defined(PPX_MSW)
#   define GLFW_EXPOSE_NATIVE_WIN32
#endif
#include <GLFW/glfw3native.h>
// clang-format on

namespace ppx {

// -------------------------------------------------------------------------------------------------
// WindowEvents
// -------------------------------------------------------------------------------------------------
// clang-format off
static std::map<int32_t, int32_t> sKeyCodeMap = {
    { GLFW_KEY_SPACE,           KEY_SPACE            },
    { GLFW_KEY_APOSTROPHE,      KEY_APOSTROPHE       },
    { GLFW_KEY_COMMA,           KEY_COMMA            },
    { GLFW_KEY_MINUS,           KEY_MINUS            },
    { GLFW_KEY_PERIOD,          KEY_PERIOD           },
    { GLFW_KEY_SLASH,           KEY_SLASH            },
    { GLFW_KEY_0,               KEY_0                },
    { GLFW_KEY_1,               KEY_1                },
    { GLFW_KEY_2,               KEY_2                },
    { GLFW_KEY_3,               KEY_3                },
    { GLFW_KEY_4,               KEY_4                },
    { GLFW_KEY_5,               KEY_5                },
    { GLFW_KEY_6,               KEY_6                },
    { GLFW_KEY_7,               KEY_7                },
    { GLFW_KEY_8,               KEY_8                },
    { GLFW_KEY_9,               KEY_9                },
    { GLFW_KEY_SEMICOLON,       KEY_SEMICOLON        },
    { GLFW_KEY_EQUAL,           KEY_EQUAL            },
    { GLFW_KEY_A,               KEY_A                },
    { GLFW_KEY_B,               KEY_B                },
    { GLFW_KEY_C,               KEY_C                },
    { GLFW_KEY_D,               KEY_D                },
    { GLFW_KEY_E,               KEY_E                },
    { GLFW_KEY_F,               KEY_F                },
    { GLFW_KEY_G,               KEY_G                },
    { GLFW_KEY_H,               KEY_H                },
    { GLFW_KEY_I,               KEY_I                },
    { GLFW_KEY_J,               KEY_J                },
    { GLFW_KEY_K,               KEY_K                },
    { GLFW_KEY_L,               KEY_L                },
    { GLFW_KEY_M,               KEY_M                },
    { GLFW_KEY_N,               KEY_N                },
    { GLFW_KEY_O,               KEY_O                },
    { GLFW_KEY_P,               KEY_P                },
    { GLFW_KEY_Q,               KEY_Q                },
    { GLFW_KEY_R,               KEY_R                },
    { GLFW_KEY_S,               KEY_S                },
    { GLFW_KEY_T,               KEY_T                },
    { GLFW_KEY_U,               KEY_U                },
    { GLFW_KEY_V,               KEY_V                },
    { GLFW_KEY_W,               KEY_W                },
    { GLFW_KEY_X,               KEY_X                },
    { GLFW_KEY_Y,               KEY_Y                },
    { GLFW_KEY_Z,               KEY_Z                },
    { GLFW_KEY_LEFT_BRACKET,    KEY_LEFT_BRACKET     },
    { GLFW_KEY_BACKSLASH,       KEY_BACKSLASH        },
    { GLFW_KEY_RIGHT_BRACKET,   KEY_RIGHT_BRACKET    },
    { GLFW_KEY_GRAVE_ACCENT,    KEY_GRAVE_ACCENT     },
    { GLFW_KEY_WORLD_1,         KEY_WORLD_1          },
    { GLFW_KEY_WORLD_2,         KEY_WORLD_2          },
    { GLFW_KEY_ESCAPE,          KEY_ESCAPE           },
    { GLFW_KEY_ENTER,           KEY_ENTER            },
    { GLFW_KEY_TAB,             KEY_TAB              },
    { GLFW_KEY_BACKSPACE,       KEY_BACKSPACE        },
    { GLFW_KEY_INSERT,          KEY_INSERT           },
    { GLFW_KEY_DELETE,          KEY_DELETE           },
    { GLFW_KEY_RIGHT,           KEY_RIGHT            },
    { GLFW_KEY_LEFT,            KEY_LEFT             },
    { GLFW_KEY_DOWN,            KEY_DOWN             },
    { GLFW_KEY_UP,              KEY_UP               },
    { GLFW_KEY_PAGE_UP,         KEY_PAGE_UP          },
    { GLFW_KEY_PAGE_DOWN,       KEY_PAGE_DOWN        },
    { GLFW_KEY_HOME,            KEY_HOME             },
    { GLFW_KEY_END,             KEY_END              },
    { GLFW_KEY_CAPS_LOCK,       KEY_CAPS_LOCK        },
    { GLFW_KEY_SCROLL_LOCK,     KEY_SCROLL_LOCK      },
    { GLFW_KEY_NUM_LOCK,        KEY_NUM_LOCK         },
    { GLFW_KEY_PRINT_SCREEN,    KEY_PRINT_SCREEN     },
    { GLFW_KEY_PAUSE,           KEY_PAUSE            },
    { GLFW_KEY_F1,              KEY_F1               },
    { GLFW_KEY_F2,              KEY_F2               },
    { GLFW_KEY_F3,              KEY_F3               },
    { GLFW_KEY_F4,              KEY_F4               },
    { GLFW_KEY_F5,              KEY_F5               },
    { GLFW_KEY_F6,              KEY_F6               },
    { GLFW_KEY_F7,              KEY_F7               },
    { GLFW_KEY_F8,              KEY_F8               },
    { GLFW_KEY_F9,              KEY_F9               },
    { GLFW_KEY_F10,             KEY_F10              },
    { GLFW_KEY_F11,             KEY_F11              },
    { GLFW_KEY_F12,             KEY_F12              },
    { GLFW_KEY_F13,             KEY_F13              },
    { GLFW_KEY_F14,             KEY_F14              },
    { GLFW_KEY_F15,             KEY_F15              },
    { GLFW_KEY_F16,             KEY_F16              },
    { GLFW_KEY_F17,             KEY_F17              },
    { GLFW_KEY_F18,             KEY_F18              },
    { GLFW_KEY_F19,             KEY_F19              },
    { GLFW_KEY_F20,             KEY_F20              },
    { GLFW_KEY_F21,             KEY_F21              },
    { GLFW_KEY_F22,             KEY_F22              },
    { GLFW_KEY_F23,             KEY_F23              },
    { GLFW_KEY_F24,             KEY_F24              },
    { GLFW_KEY_F25,             KEY_F25              },
    { GLFW_KEY_KP_0,            KEY_KEY_PAD_0        },
    { GLFW_KEY_KP_1,            KEY_KEY_PAD_1        },
    { GLFW_KEY_KP_2,            KEY_KEY_PAD_2        },
    { GLFW_KEY_KP_3,            KEY_KEY_PAD_3        },
    { GLFW_KEY_KP_4,            KEY_KEY_PAD_4        },
    { GLFW_KEY_KP_5,            KEY_KEY_PAD_5        },
    { GLFW_KEY_KP_6,            KEY_KEY_PAD_6        },
    { GLFW_KEY_KP_7,            KEY_KEY_PAD_7        },
    { GLFW_KEY_KP_8,            KEY_KEY_PAD_8        },
    { GLFW_KEY_KP_9,            KEY_KEY_PAD_9        },
    { GLFW_KEY_KP_DECIMAL,      KEY_KEY_PAD_DECIMAL  },
    { GLFW_KEY_KP_DIVIDE,       KEY_KEY_PAD_DIVIDE   },
    { GLFW_KEY_KP_MULTIPLY,     KEY_KEY_PAD_MULTIPLY },
    { GLFW_KEY_KP_SUBTRACT,     KEY_KEY_PAD_SUBTRACT },
    { GLFW_KEY_KP_ADD,          KEY_KEY_PAD_ADD      },
    { GLFW_KEY_KP_ENTER,        KEY_KEY_PAD_ENTER    },
    { GLFW_KEY_KP_EQUAL,        KEY_KEY_PAD_EQUAL    },
    { GLFW_KEY_LEFT_SHIFT,      KEY_LEFT_SHIFT       },
    { GLFW_KEY_LEFT_CONTROL,    KEY_LEFT_CONTROL     },
    { GLFW_KEY_LEFT_ALT,        KEY_LEFT_ALT         },
    { GLFW_KEY_LEFT_SUPER,      KEY_LEFT_SUPER       },
    { GLFW_KEY_RIGHT_SHIFT,     KEY_RIGHT_SHIFT      },
    { GLFW_KEY_RIGHT_CONTROL,   KEY_RIGHT_CONTROL    },
    { GLFW_KEY_RIGHT_ALT,       KEY_RIGHT_ALT        },
    { GLFW_KEY_RIGHT_SUPER,     KEY_RIGHT_SUPER      },
    { GLFW_KEY_MENU,            KEY_MENU             },
};
// clang-format on

// -------------------------------------------------------------------------------------------------
// WindowEvents
// -------------------------------------------------------------------------------------------------
struct WindowEvents
{
    static std::unordered_map<GLFWwindow*, Application*> sWindows;

    static void MoveCallback(GLFWwindow* window, int event_x, int event_y)
    {
        auto it = sWindows.find(window);
        if (it == sWindows.end()) {
            return;
        }
        Application* p_application = it->second;

        p_application->MoveCallback(
            static_cast<int32_t>(event_x),
            static_cast<int32_t>(event_y));
    }

    static void ResizeCallback(GLFWwindow* window, int event_width, int event_height)
    {
        auto it = sWindows.find(window);
        if (it == sWindows.end()) {
            return;
        }
        Application* p_application = it->second;

        p_application->ResizeCallback(
            static_cast<uint32_t>(event_width),
            static_cast<uint32_t>(event_height));
    }

    static void IconifyCallback(GLFWwindow* window, int iconified)
    {
        auto it = sWindows.find(window);
        if (it == sWindows.end()) {
            return;
        }
        Application* p_application = it->second;

        p_application->WindowIconifyCallback((iconified == GLFW_TRUE) ? true : false);
    }

    static void MaximizeCallback(GLFWwindow* window, int maximized)
    {
        auto it = sWindows.find(window);
        if (it == sWindows.end()) {
            return;
        }
        Application* p_application = it->second;

        p_application->WindowMaximizeCallback((maximized == GLFW_TRUE) ? true : false);
    }

    static void MouseButtonCallback(GLFWwindow* window, int event_button, int event_action, int event_mods)
    {
        auto it = sWindows.find(window);
        if (it == sWindows.end()) {
            return;
        }
        Application* p_application = it->second;

        uint32_t buttons = 0;
        if (event_button == GLFW_MOUSE_BUTTON_LEFT) {
            buttons |= MOUSE_BUTTON_LEFT;
        }
        if (event_button == GLFW_MOUSE_BUTTON_RIGHT) {
            buttons |= MOUSE_BUTTON_RIGHT;
        }
        if (event_button == GLFW_MOUSE_BUTTON_MIDDLE) {
            buttons |= MOUSE_BUTTON_MIDDLE;
        }

        double event_x;
        double event_y;
        glfwGetCursorPos(window, &event_x, &event_y);

        if (event_action == GLFW_PRESS) {
            p_application->MouseDownCallback(
                static_cast<int32_t>(event_x),
                static_cast<int32_t>(event_y),
                buttons);
        }
        else if (event_action == GLFW_RELEASE) {
            p_application->MouseUpCallback(
                static_cast<int32_t>(event_x),
                static_cast<int32_t>(event_y),
                buttons);
        }

        if (p_application->GetSettings()->enableImGui) {
            ImGui_ImplGlfw_MouseButtonCallback(window, event_button, event_action, event_mods);
        }
    }

    static void MouseMoveCallback(GLFWwindow* window, double event_x, double event_y)
    {
        auto it = sWindows.find(window);
        if (it == sWindows.end()) {
            return;
        }
        Application* p_application = it->second;

        uint32_t buttons = 0;
        if (glfwGetMouseButton(window, GLFW_MOUSE_BUTTON_LEFT) == GLFW_PRESS) {
            buttons |= MOUSE_BUTTON_LEFT;
        }
        if (glfwGetMouseButton(window, GLFW_MOUSE_BUTTON_RIGHT) == GLFW_PRESS) {
            buttons |= MOUSE_BUTTON_RIGHT;
        }
        if (glfwGetMouseButton(window, GLFW_MOUSE_BUTTON_MIDDLE) == GLFW_PRESS) {
            buttons |= MOUSE_BUTTON_MIDDLE;
        }
        p_application->MouseMoveCallback(
            static_cast<int32_t>(event_x),
            static_cast<int32_t>(event_y),
            buttons);
    }

    static void ScrollCallback(GLFWwindow* window, double xoffset, double yoffset)
    {
        auto it = sWindows.find(window);
        if (it == sWindows.end()) {
            return;
        }
        Application* p_application = it->second;

        p_application->ScrollCallback(
            static_cast<float>(xoffset),
            static_cast<float>(yoffset));

        if (p_application->GetSettings()->enableImGui) {
            ImGui_ImplGlfw_ScrollCallback(window, xoffset, yoffset);
        }
    }

    static void KeyCallback(GLFWwindow* window, int key, int scancode, int action, int mods)
    {
        auto it = sWindows.find(window);
        if (it == sWindows.end()) {
            return;
        }
        Application* p_application = it->second;

        bool hasKey = (sKeyCodeMap.find(key) != sKeyCodeMap.end());
        if (!hasKey) {
            PPX_LOG_WARN("GLFW key not supported, key=" << key);
        }

        if (hasKey) {
            KeyCode appKey = static_cast<KeyCode>(sKeyCodeMap[key]);

            if (action == GLFW_PRESS) {
                p_application->KeyDownCallback(appKey);
            }
            else if (action == GLFW_RELEASE) {
                p_application->KeyUpCallback(appKey);
            }
        }

        if (p_application->GetSettings()->enableImGui) {
            ImGui_ImplGlfw_KeyCallback(window, key, scancode, action, mods);
        }
    }

    static void CharCallback(GLFWwindow* window, unsigned int c)
    {
        auto it = sWindows.find(window);
        if (it == sWindows.end()) {
            return;
        }
        Application* p_application = it->second;

        if (p_application->GetSettings()->enableImGui) {
            ImGui_ImplGlfw_CharCallback(window, c);
        }
    }

    static Result RegisterWindowEvents(GLFWwindow* window, Application* application)
    {
        auto it = sWindows.find(window);
        if (it != sWindows.end()) {
            return ppx::ERROR_WINDOW_EVENTS_ALREADY_REGISTERED;
        }

        glfwSetWindowPosCallback(window, WindowEvents::MoveCallback);
        glfwSetWindowSizeCallback(window, WindowEvents::ResizeCallback);
        glfwSetWindowIconifyCallback(window, WindowEvents::IconifyCallback);
        glfwSetWindowMaximizeCallback(window, WindowEvents::MaximizeCallback);
        glfwSetMouseButtonCallback(window, WindowEvents::MouseButtonCallback);
        glfwSetCursorPosCallback(window, WindowEvents::MouseMoveCallback);
        glfwSetScrollCallback(window, WindowEvents::ScrollCallback);
        glfwSetKeyCallback(window, WindowEvents::KeyCallback);
        glfwSetCharCallback(window, WindowEvents::CharCallback);

        sWindows[window] = application;
        return ppx::SUCCESS;
    }
};

std::unordered_map<GLFWwindow*, Application*> WindowEvents::sWindows;

// -------------------------------------------------------------------------------------------------
// WindowImplGLFW
// -------------------------------------------------------------------------------------------------
class WindowImplGLFW : public Window
{
public:
    using Window::Window;

    Result     Create(const char* title) final;
    void       Quit() final;
    Result     Destroy() final;
    bool       IsRunning() const final;
    WindowSize Size() const final;
    Result     Resize(const WindowSize&) final;
    void       FillSurfaceInfo(grfx::SurfaceCreateInfo*) const final;
    void       ProcessEvent() final;
    void*      NativeHandle() final;

private:
    GLFWwindow* mNative = nullptr;
};

std::unique_ptr<Window> Window::GetImplGLFW(Application* pApp)
{
    return std::unique_ptr<Window>(new WindowImplGLFW(pApp));
}

Result WindowImplGLFW::Create(const char* title)
{
    // Initializ glfw
    int res = glfwInit();
    if (res != GLFW_TRUE) {
        PPX_ASSERT_MSG(false, "glfwInit failed");
        return ppx::ERROR_GLFW_INIT_FAILED;
    }

    const ApplicationSettings* settings = App()->GetSettings();
    glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
    glfwWindowHint(GLFW_RESIZABLE, settings->window.resizable ? GLFW_TRUE : GLFW_FALSE);

    GLFWwindow* pWindow = glfwCreateWindow(
        static_cast<int>(settings->window.width),
        static_cast<int>(settings->window.height),
        title,
        nullptr,
        nullptr);
    if (IsNull(pWindow)) {
        PPX_ASSERT_MSG(false, "glfwCreateWindow failed");
        return ppx::ERROR_GLFW_CREATE_WINDOW_FAILED;
    }

    // Register window events
    Result ppxres = WindowEvents::RegisterWindowEvents(pWindow, App());
    if (Failed(ppxres)) {
        PPX_ASSERT_MSG(false, "RegisterWindowEvents failed");
        return ppxres;
    }
    mNative = pWindow;
    return ppx::SUCCESS;
}

void WindowImplGLFW::Quit()
{
    glfwSetWindowShouldClose(mNative, 1);
}

Result WindowImplGLFW::Destroy()
{
    if (!IsNull(mNative)) {
        glfwDestroyWindow(mNative);
        mNative = nullptr;
    }
    return ppx::SUCCESS;
}

bool WindowImplGLFW::IsRunning() const
{
    int  value     = glfwWindowShouldClose(mNative);
    bool isRunning = (value == 0);
    return isRunning;
}

Result WindowImplGLFW::Resize(const WindowSize& size)
{
    if (Size() == size) {
        return ppx::SUCCESS;
    }

    glfwSetWindowSize(
        mNative,
        static_cast<int>(size.width),
        static_cast<int>(size.height));
    return ppx::SUCCESS;
}

WindowSize WindowImplGLFW::Size() const
{
    int width  = 0;
    int height = 0;
    glfwGetWindowSize(mNative, &width, &height);
    return {static_cast<uint32_t>(width), static_cast<uint32_t>(height)};
}

void WindowImplGLFW::FillSurfaceInfo(grfx::SurfaceCreateInfo* pCreateInfo) const
{
#if defined(PPX_LINUX_XCB)
    pCreateInfo->connection = XGetXCBConnection(glfwGetX11Display());
    pCreateInfo->window     = glfwGetX11Window(mNative);
#elif defined(PPX_LINUX_XLIB)
#error "Xlib not implemented"
#elif defined(PPX_LINUX_WAYLAND)
#error "Wayland not implemented"
#elif defined(PPX_MSW)
    pCreateInfo->hinstance = ::GetModuleHandle(nullptr);
    pCreateInfo->hwnd      = glfwGetWin32Window(mNative);
#endif
}

void WindowImplGLFW::ProcessEvent()
{
    glfwPollEvents();
}

void* WindowImplGLFW::NativeHandle()
{
    return mNative;
}

} // namespace ppx

#endif
