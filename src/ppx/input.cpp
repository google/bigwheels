// Copyright 2024 Google LLC
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

#include "ppx/input.h"

namespace ppx {

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

const char* GetKeyCodeString(KeyCode code)
{
    if ((code < KEY_RANGE_FIRST) || (code >= KEY_RANGE_LAST)) {
        return sKeyCodeString[0];
    }
    return sKeyCodeString[static_cast<uint32_t>(code)];
}

} // namespace ppx
