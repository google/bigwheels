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
#include "ppx/profiler.h"
#include "ppx/ppm_export.h"
#include "ppx/fs.h"
#include "backends/imgui_impl_glfw.h"

#include <map>
#include <numeric>
#include <unordered_map>
#include <optional>

#if defined(PPX_LINUX_XCB)
#include <X11/Xlib-xcb.h>
#elif defined(PPX_LINUX_XLIB)
#error "Xlib not implemented"
#elif defined(PPX_LINUX_WAYLAND)
#error "Wayland not implemented"
#endif

// clang-format off
#if defined(PPX_LINUX)
#   define GLFW_EXPOSE_NATIVE_X11 
#elif defined(PPX_MSW)
#   define GLFW_EXPOSE_NATIVE_WIN32
#endif
#include <GLFW/glfw3native.h>
// clang-format on

namespace ppx {

const char*    kDefaultAppName      = "PPX Application";
const uint32_t kDefaultWindowWidth  = 1280;
const uint32_t kDefaultWindowHeight = 720;

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
    std::filesystem::path path = GetApplicationPath();
    path.remove_filename();
    path /= RELATIVE_PATH_TO_PROJECT_ROOT;
    AddAssetDir(path / "assets");
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

    if (mSettings.enableDisplay) {
        // Initializ glfw
        int res = glfwInit();
        if (res != GLFW_TRUE) {
            PPX_ASSERT_MSG(false, "glfwInit failed");
            return ppx::ERROR_GLFW_INIT_FAILED;
        }
    }
    else {
        PPX_LOG_INFO("Display not enabled: skipping initialization of glfw");
    }

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
        ci.enableSwapchain          = mSettings.enableDisplay;
        ci.applicationName          = mSettings.appName;
        ci.engineName               = mSettings.appName;
        ci.useSoftwareRenderer      = mStandardOptions.use_software_renderer;
#if defined(PPX_BUILD_XR)
        ci.pXrComponent = mSettings.xr.enable ? &mXrComponent : nullptr;
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
        ci.enableDXIL             = mSettings.grfx.enableDXIL;
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
    if (!mSettings.enableDisplay) {
        PPX_LOG_INFO("Display not enabled: skipping creation of surface");
        PPX_LOG_INFO("Display not enabled: skipping creation of swapchain");
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
#if defined(PPX_LINUX_XCB)
        ci.connection = XGetXCBConnection(glfwGetX11Display());
        ci.window     = glfwGetX11Window(static_cast<GLFWwindow*>(mWindow));
#elif defined(PPX_LINUX_XLIB)
#error "Xlib not implemented"
#elif defined(PPX_LINUX_WAYLAND)
#error "Wayland not implemented"
#elif defined(PPX_MSW)
        ci.hinstance = ::GetModuleHandle(nullptr);
        ci.hwnd      = glfwGetWin32Window(static_cast<GLFWwindow*>(mWindow));
#endif

        Result ppxres = mInstance->CreateSurface(&ci, &mSurface);
        if (Failed(ppxres)) {
            PPX_ASSERT_MSG(false, "grfx::Instance::CreateSurface failed");
            return ppxres;
        }
    }

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
        const size_t swapchainCount = viewCount + 1;
        mStereoscopicSwapchainIndex = 0;
        mUISwapchainIndex           = static_cast<uint32_t>(viewCount);
        mSwapchain.resize(swapchainCount);
        for (size_t k = 0; k < swapchainCount; ++k) {
            Result ppxres = mDevice->CreateSwapchain(&ci, &mSwapchain[k]);
            if (Failed(ppxres)) {
                PPX_ASSERT_MSG(false, "grfx::Device::CreateSwapchain failed");
                return ppxres;
            }
        }

        // Image count is from xrEnumerateSwapchainImages
        mSettings.grfx.swapchain.imageCount = mSwapchain[0]->GetImageCount();
    }

    if (!mSettings.xr.enable
        // Extra swapchain for XR debug capture.
        || (mSettings.xr.enable && mSettings.xr.enableDebugCapture))
#endif
    // Swapchain
    {
        PPX_LOG_INFO("Creating application swapchain");
        PPX_LOG_INFO("   resolution  : " << mSettings.window.width << "x" << mSettings.window.height);
        PPX_LOG_INFO("   image count : " << mSettings.grfx.swapchain.imageCount);

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

#if defined(PPX_GGP)
        const uint32_t surfaceMinImageWidth  = mSurface->GetMinImageWidth();
        const uint32_t surfaceMinImageHeight = mSurface->GetMinImageHeight();
        if ((surfaceMinImageWidth > mSettings.window.width) || (surfaceMinImageHeight > mSettings.window.height)) {
            PPX_LOG_WARN("readjusting swapchain/window size from " << mSettings.window.width << "x" << mSettings.window.height << " to " << surfaceMinImageWidth << "x" << surfaceMinImageHeight << " to match surface requirements");
            mSettings.window.width  = surfaceMinImageWidth;
            mSettings.window.height = surfaceMinImageHeight;
        }
#endif

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
            mDebugCaptureSwapchainIndex = static_cast<uint32_t>(mSwapchain.size());
            // The window size could be smaller than the requested one in glfwCreateWindow
            // So the final swapchain size for window needs to be adjusted
            // In the case of debug capture, we don't care about the window size after creating the dummy window
            // restore width and heigh in the settings since they are used by some other systems in the renderer
            mSettings.window.width  = mXrComponent.GetWidth();
            mSettings.window.height = mXrComponent.GetHeight();
        }
#endif
        mSwapchain.push_back(swapchain);
    }

    return ppx::SUCCESS;
}

Result Application::InitializeImGui()
{
    switch (mSettings.grfx.api) {
        default: {
            PPX_ASSERT_MSG(false, "[imgui] unknown graphics API");
            return ppx::ERROR_UNSUPPORTED_API;
        } break;

#if defined(PPX_D3D11)
        case grfx::API_DX_11_0:
        case grfx::API_DX_11_1: {
            mImGui = std::unique_ptr<ImGuiImpl>(new ImGuiImplDx11());
        } break;
#endif // defined(PPX_D3D11)

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

    Result ppxres = mImGui->Init(this);
    if (Failed(ppxres)) {
        return ppxres;
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
        for (auto& sc : mSwapchain) {
            mDevice->DestroySwapchain(sc);
            sc.Reset();
        }
        mSwapchain.clear();

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
    if (!mSettings.enableDisplay) {
        PPX_LOG_INFO("Display not enabled: skipping creation of platform window");
        return ppx::SUCCESS;
    }

    glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
    glfwWindowHint(GLFW_RESIZABLE, mSettings.window.resizable ? GLFW_TRUE : GLFW_FALSE);

    // Decorated window title
    std::stringstream windowTitle;
    windowTitle << mSettings.window.title << " | " << ToString(mSettings.grfx.api) << " | " << mDevice->GetDeviceName() << " | " << Platform::GetPlatformString();

    GLFWwindow* pWindow = glfwCreateWindow(
        static_cast<int>(mSettings.window.width),
        static_cast<int>(mSettings.window.height),
        windowTitle.str().c_str(),
        nullptr,
        nullptr);
    if (IsNull(pWindow)) {
        PPX_ASSERT_MSG(false, "glfwCreateWindow failed");
        return ppx::ERROR_GLFW_CREATE_WINDOW_FAILED;
    }

    // Register window events
    Result ppxres = WindowEvents::RegisterWindowEvents(pWindow, this);
    if (Failed(ppxres)) {
        PPX_ASSERT_MSG(false, "RegisterWindowEvents failed");
        return ppxres;
    }

    mWindow = static_cast<void*>(pWindow);

    return ppx::SUCCESS;
}

void Application::DestroyPlatformWindow()
{
    if (!IsNull(mWindow)) {
        GLFWwindow* pWindow = static_cast<GLFWwindow*>(mWindow);
        glfwDestroyWindow(pWindow);
        mWindow = nullptr;
    }
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

    // Decorate DX's API name with shader bytecode mode
    std::stringstream ss;
    ss << ToString(mSettings.grfx.api);
    switch (mSettings.grfx.api) {
        default: break;
        case grfx::API_DX_12_0:
        case grfx::API_DX_12_1: {
            if (mSettings.grfx.enableDXIL) {
                ss << " (DXIL)";
            }
            else {
                ss << " (DXBC)";
            }
        } break;
    }

    mDecoratedApiName = ss.str();

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

void Application::DispatchShutdown()
{
    Shutdown();

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

void Application::DispatchMouseMove(int32_t x, int32_t y, int32_t dx12, int32_t dy, uint32_t buttons)
{
    MouseMove(x, y, dx12, dy, buttons);
}

void Application::DispatchMouseDown(int32_t x, int32_t y, uint32_t buttons)
{
    MouseDown(x, y, buttons);
}

void Application::DispatchMouseUp(int32_t x, int32_t y, uint32_t buttons)
{
    MouseUp(x, y, buttons);
}

void Application::DispatchScroll(float dx12, float dy)
{
    Scroll(dx12, dy);
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

    std::string filename = mStandardOptions.screenshot_out_dir + "screenshot_frame" + std::to_string(mFrameCount) + ".ppm";
    PPX_CHECKED_CALL(ExportToPPM(filename, swapchainImg->GetFormat(), texels, width, height, outPitch.rowPitch));

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

    int32_t dx12 = (mPreviousMouseX != INT32_MAX) ? (x - mPreviousMouseX) : 0;
    int32_t dy   = (mPreviousMouseY != INT32_MAX) ? (y - mPreviousMouseY) : 0;
    DispatchMouseMove(x, y, dx12, dy, buttons);
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

void Application::ScrollCallback(float dx12, float dy)
{
    if (mSettings.enableImGui && ImGui::GetIO().WantCaptureMouse) {
        return;
    }

    DispatchScroll(dx12, dy);
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
    if (mSettings.enableDisplay) {
        int  value     = glfwWindowShouldClose(static_cast<GLFWwindow*>(mWindow));
        bool isRunning = (value == 0);
        return isRunning;
    }
    else {
        return mRunningWithoutDisplay;
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
    if (auto error = mCommandLineParser.Parse(argc, const_cast<const char**>(argv))) {
        PPX_LOG_ERROR(error->errorMsg);
        PPX_ASSERT_MSG(false, "Unable to parse command line arguments");
        return EXIT_FAILURE;
    }
    mStandardOptions = mCommandLineParser.GetOptions().GetStandardOptions();

    if (mStandardOptions.help) {
        PPX_LOG_INFO(mCommandLineParser.GetUsageMsg());
        return EXIT_SUCCESS;
    }

    // Call config.
    // Put this early because it might disable the display.
    DispatchConfig();

    // Initialize the platform
    Result ppxres = InitializePlatform();
    if (Failed(ppxres)) {
        return EXIT_FAILURE;
    }

    // If command line argument provided width and height
    if ((mStandardOptions.resolution.first != -1) && (mStandardOptions.resolution.second != -1)) {
        mSettings.window.width  = mStandardOptions.resolution.first;
        mSettings.window.height = mStandardOptions.resolution.second;
    }

    mMaxFrame = UINT64_MAX;
    // If command line provided a maximum number of frames to draw
    if (mStandardOptions.frame_count != -1) {
        mMaxFrame = mStandardOptions.frame_count;
    }
    else if (!mSettings.enableDisplay) {
        mMaxFrame = 1;
        PPX_LOG_INFO("Display not enabled: frame count is " + std::to_string(mMaxFrame));
    }

#if defined(PPX_BUILD_XR)
    if (mSettings.xr.enable) {
        XrComponentCreateInfo createInfo = {};
        createInfo.api                   = mSettings.grfx.api;
        createInfo.appName               = mSettings.appName;
        createInfo.colorFormat           = grfx::FORMAT_B8G8R8A8_SRGB;
        createInfo.depthFormat           = grfx::FORMAT_D32_FLOAT;
        createInfo.depthNearPlane        = mSettings.xr.depthNearPlane;
        createInfo.depthFarPlane         = mSettings.xr.depthFarPlane;
        createInfo.refSpaceType          = XrRefSpace::XR_STAGE;
        createInfo.viewConfigType        = XR_VIEW_CONFIGURATION_TYPE_PRIMARY_STEREO;
        createInfo.enableDebug           = mSettings.grfx.enableDebug;
        createInfo.enableQuadLayer       = mSettings.enableImGui;
        createInfo.quadLayerPos          = XrVector3f{mSettings.xr.ui.pos.x, mSettings.xr.ui.pos.y, mSettings.xr.ui.pos.z};
        createInfo.quadLayerSize         = XrExtent2Df{mSettings.xr.ui.size.x, mSettings.xr.ui.size.y};

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

    if (mSettings.enableDisplay) {
        // Create window
        ppxres = CreatePlatformWindow();
        if (Failed(ppxres)) {
            return EXIT_FAILURE;
        }

        // Create surface
        ppxres = InitializeGrfxSurface();
        if (Failed(ppxres)) {
            return EXIT_FAILURE;
        }

        if (!mSettings.xr.enable) {
            // Update the window size if the settings got changed due to surface requirements
            {
                int windowWidth  = 0;
                int windowHeight = 0;
                glfwGetWindowSize(static_cast<GLFWwindow*>(mWindow), &windowWidth, &windowHeight);
                if ((static_cast<uint32_t>(windowWidth) != mSettings.window.width) || (static_cast<uint32_t>(windowHeight) != mSettings.window.width)) {
                    glfwSetWindowSize(
                        static_cast<GLFWwindow*>(mWindow),
                        static_cast<int>(mSettings.window.width),
                        static_cast<int>(mSettings.window.height));
                }
            }
        }
    }
    else {
        PPX_LOG_INFO("Display not enabled: skipping creation of platform window");
        PPX_LOG_INFO("Display not enabled: skipping creation of surface");
    }

    // Setup ImGui
    if (mSettings.enableImGui) {
        ppxres = InitializeImGui();
        if (Failed(ppxres)) {
            return EXIT_FAILURE;
        }
    }

    // Call setup
    DispatchSetup();

    // ---------------------------------------------------------------------------------------------
    // Main loop [BEGIN]
    // ---------------------------------------------------------------------------------------------

    // Initialize and start timer
    ppx::TimerResult tmres = mTimer.Start();
    if (tmres != ppx::TIMER_RESULT_SUCCESS) {
        PPX_ASSERT_MSG(false, "failed to start application timer");
        return EXIT_FAILURE;
    }

    mRunningWithoutDisplay = !mSettings.enableDisplay;
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
                mXrComponent.BeginFrame(mSwapchain, 0, mUISwapchainIndex);
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
                mXrComponent.EndFrame();
            }
        }
        else
#endif
        {
            if (mSettings.enableDisplay) {
                // Poll events
                glfwPollEvents();
            }

            // Start new Imgui frame
            if (mImGui) {
                mImGui->NewFrame();
            }

            // Call render
            DispatchRender();
        }

        // Take screenshot if this is the requested frame.
        if (mFrameCount == mStandardOptions.screenshot_frame_number) {
            TakeScreenshot();
        }

        // Frame end
        mFrameCount        = mFrameCount + 1;
        mFrameEndTime      = static_cast<float>(mTimer.MillisSinceStart());
        mPreviousFrameTime = mFrameEndTime - mFrameStartTime;

        // Keep a rolling window of frame times to calculate stats,
        // if requested.
        if (mStandardOptions.stats_frame_window > 0) {
            mFrameTimesMs.push_back(mPreviousFrameTime);
            if (mFrameTimesMs.size() > mStandardOptions.stats_frame_window) {
                mFrameTimesMs.pop_front();
            }
            float totalFrameTimeMs = std::accumulate(mFrameTimesMs.begin(), mFrameTimesMs.end(), 0.f);
            mAverageFPS            = mFrameTimesMs.size() / (totalFrameTimeMs / 1000.0f);
            mAverageFrameTime      = totalFrameTimeMs / mFrameTimesMs.size();
        }
        else {
            mAverageFPS       = static_cast<float>(mFrameCount / mTimer.SecondsSinceStart());
            mAverageFrameTime = static_cast<float>(mTimer.MillisSinceStart() / mFrameCount);
        }

        // Pace frames - if needed
        if (mSettings.grfx.pacedFrameRate > 0) {
            if (mFrameCount > 0) {
                double currentTime  = mTimer.SecondsSinceStart();
                double pacedFPS     = 1.0 / static_cast<double>(mSettings.grfx.pacedFrameRate);
                double expectedTime = mFirstFrameTime + (mFrameCount * pacedFPS);
                double diff         = expectedTime - currentTime;
                if (diff > 0) {
                    Timer::SleepSeconds(diff);
                }
            }
            else {
                mFirstFrameTime = mTimer.SecondsSinceStart();
            }
        }
        // If we reach the maximum number of frames allowed
        if (mFrameCount >= mMaxFrame) {
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
    if (mSettings.enableDisplay) {
        glfwSetWindowShouldClose(static_cast<GLFWwindow*>(mWindow), 1);
    }
    else {
        mRunningWithoutDisplay = false;
    }
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
        case grfx::API_DX_11_0:
        case grfx::API_DX_11_1:
            return std::filesystem::path("dxbc50") / (baseName + ".dxbc50");
        case grfx::API_DX_12_0:
        case grfx::API_DX_12_1: {
            if (settings.grfx.enableDXIL) {
                return std::filesystem::path("dxil") / (baseName + ".dxil");
            }
            return std::filesystem::path("dxbc51") / (baseName + ".dxbc51");
        }
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
    if (!std::filesystem::exists(filePath)) {
        PPX_ASSERT_MSG(false, "shader file not found: " << filePath);
        return {};
    }

    auto bytecode = fs::load_file(filePath);
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

float Application::GetElapsedSeconds() const
{
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

void Application::DrawDebugInfo(std::function<void(void)> drawAdditionalFn)
{
    if (!mImGui) {
        return;
    }
    ImGui::PushStyleVar(ImGuiStyleVar_WindowMinSize, {static_cast<float>(GetWindowWidth() / 2), static_cast<float>(GetWindowHeight() / 2)});
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

        // Average FPS
        {
            ImGui::Text("Frame Count");
            ImGui::NextColumn();
            ImGui::Text("%lu", mFrameCount);
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
        if (drawAdditionalFn) {
            drawAdditionalFn();
        }
    }
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
                ImGui::Text("%lu", count);
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
