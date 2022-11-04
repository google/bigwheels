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

#ifndef ppx_config_h
#define ppx_config_h

// clang-format off
#if defined(PPX_MSW)
#   if !defined(VC_EXTRALEAN)
#       define VC_EXTRALEAN
#   endif
#   if !defined(WIN32_LEAN_AND_MEAN)
#       define WIN32_LEAN_AND_MEAN 
#   endif
#   if !defined(NOMINMAX)
#       define NOMINMAX
#   endif
#endif
// clang-format on

#include <algorithm>
#include <array>
#include <functional>
#include <cassert>
#include <cstdlib>
#include <cstring>
#include <memory>
#include <string>
#include <vector>
#include <bitset>

#include "ppx/ccomptr.h"
#include "ppx/obj_ptr.h"
#include "ppx/log.h"
#include "ppx/util.h"

// clang-format off
#define PPX_STRINGIFY_(x)   #x
#define PPX_STRINGIFY(x)    PPX_STRINGIFY_(x)
#define PPX_LINE            PPX_STRINGIFY(__LINE__)
#define PPX_SOURCE_LOCATION __FUNCTION__ << " @ " __FILE__ ":" PPX_LINE
#define PPX_VAR_VALUE(var)  #var << ":" << var
#define PPX_ENDL            std::endl

#define PPX_ASSERT_MSG(COND, MSG)                                            \
    if ((COND) == false) {                                                   \
        PPX_LOG_RAW(PPX_ENDL                                                 \
            << "*** PPX ASSERT ***" << PPX_ENDL                              \
            << "Message   : " << MSG << " " << PPX_ENDL                      \
            << "Condition : " << #COND << " " << PPX_ENDL                    \
            << "Function  : " << __FUNCTION__ << PPX_ENDL                    \
            << "Location  : " << __FILE__ << " : " << PPX_LINE << PPX_ENDL); \
        assert(false);                                                       \
    }

#define PPX_ASSERT_NULL_ARG(ARG)                                             \
    if ((ARG) == nullptr) {                                                  \
        PPX_LOG_RAW(PPX_ENDL                                                 \
            << "*** PPX NULL ARGUMNET ***" << PPX_ENDL                       \
            << "Argument  : " << #ARG << " " << PPX_ENDL                     \
            << "Function  : " << __FUNCTION__ << PPX_ENDL                    \
            << "Location  : " << __FILE__ << " : " << PPX_LINE << PPX_ENDL); \
        assert(false);                                                       \
    }

#define PPX_CHECKED_CALL(EXPR)                                                         \
    {                                                                                  \
        ppx::Result ppx_checked_result_0xdeadbeef = EXPR;                              \
        if (ppx_checked_result_0xdeadbeef != ppx::SUCCESS) {                           \
            PPX_LOG_RAW(PPX_ENDL                                                       \
                << "*** PPX Call Failed ***" << PPX_ENDL                               \
                << "Return     : " << ppx_checked_result_0xdeadbeef << " " << PPX_ENDL \
                << "Expression : " << #EXPR << " " << PPX_ENDL                         \
                << "Function   : " << __FUNCTION__ << PPX_ENDL                         \
                << "Location   : " << __FILE__ << " : " << PPX_LINE << PPX_ENDL);      \
            assert(false);                                                             \
        }                                                                              \
    }
// clang-format on

namespace ppx {

enum Result
{
    SUCCESS                            = 0,
    ERROR_FAILED                       = -1,
    ERROR_ALLOCATION_FAILED            = -2,
    ERROR_OUT_OF_MEMORY                = -3,
    ERROR_ELEMENT_NOT_FOUND            = -4,
    ERROR_OUT_OF_RANGE                 = -5,
    ERROR_DUPLICATE_ELEMENT            = -6,
    ERROR_LIMIT_EXCEEDED               = -7,
    ERROR_PATH_DOES_NOT_EXIST          = -8,
    ERROR_SINGLE_INIT_ONLY             = -9,
    ERROR_UNEXPECTED_NULL_ARGUMENT     = -10,
    ERROR_UNEXPECTED_COUNT_VALUE       = -11,
    ERROR_UNSUPPORTED_API              = -12,
    ERROR_API_FAILURE                  = -13,
    ERROR_WAIT_FAILED                  = -14,
    ERROR_WAIT_TIMED_OUT               = -15,
    ERROR_NO_GPUS_FOUND                = -16,
    ERROR_REQUIRED_FEATURE_UNAVAILABLE = -17,
    ERROR_BAD_DATA_SOURCE              = -18,

    ERROR_GLFW_INIT_FAILED          = -200,
    ERROR_GLFW_CREATE_WINDOW_FAILED = -201,

    ERROR_INVALID_CREATE_ARGUMENT    = -300,
    ERROR_RANGE_ALIASING_NOT_ALLOWED = -301,

    ERROR_GRFX_INVALID_OWNERSHIP                    = -1000,
    ERROR_GRFX_OBJECT_OWNERSHIP_IS_RESTRICTED       = -1001,
    ERROR_GRFX_UNSUPPORTED_SWAPCHAIN_FORMAT         = -1002,
    ERROR_GRFX_UNSUPPORTED_PRESENT_MODE             = -1003,
    ERROR_GRFX_MAX_VERTEX_BINDING_EXCEEDED          = -1004,
    ERROR_GRFX_VERTEX_ATTRIBUTE_FORMAT_UNDEFINED    = -1005,
    ERROR_GRFX_VERTEX_ATTRIBUTE_OFFSET_OUT_OF_ORDER = -1006,
    ERROR_GRFX_CANNOT_MIX_VERTEX_INPUT_RATES        = -1007,
    ERROR_GRFX_UNKNOWN_DESCRIPTOR_TYPE              = -1008,
    ERROR_GRFX_INVALID_DESCRIPTOR_TYPE              = -1009,
    ERROR_GRFX_DESCRIPTOR_COUNT_EXCEEDED            = -1010,
    ERROR_GRFX_BINDING_NOT_IN_SET                   = -1011,
    ERROR_GRFX_NON_UNIQUE_SET                       = -1012,
    ERROR_GRFX_MINIMUM_BUFFER_SIZE_NOT_MET          = -1013,
    ERROR_GRFX_INVALID_SHADER_BYTE_CODE             = -1014,
    ERROR_INVALID_PIPELINE_INTERFACE                = -1015,
    ERROR_GRFX_INVALID_QUERY_TYPE                   = -1016,
    ERROR_GRFX_INVALID_QUERY_COUNT                  = -1017,
    ERROR_GRFX_NO_QUEUES_AVAILABLE                  = -1018,
    ERROR_GRFX_INVALID_INDEX_TYPE                   = -1019,
    ERROR_GRFX_INVALID_GEOMETRY_CONFIGURATION       = -1020,
    ERROR_GRFX_INVALID_VERTEX_ATTRIBUTE_COUNT       = -1021,
    ERROR_GRFX_INVALID_VERTEX_ATTRIBUTE_STRIDE      = -1022,

    ERROR_IMAGE_FILE_LOAD_FAILED               = -2000,
    ERROR_IMAGE_FILE_SAVE_FAILED               = -2001,
    ERROR_IMAGE_CANNOT_RESIZE_EXTERNAL_STORAGE = -2002,
    ERROR_IMAGE_INVALID_FORMAT                 = -2003,
    ERROR_IMAGE_RESIZE_FAILED                  = -2004,
    ERROR_BITMAP_CREATE_FAILED                 = -2005,
    ERROR_BITMAP_BAD_COPY_SOURCE               = -2006,
    ERROR_BITMAP_FOOTPRINT_MISMATCH            = -2007,

    ERROR_NO_INDEX_DATA                    = -2400,
    ERROR_GEOMETRY_FILE_LOAD_FAILED        = -2500,
    ERROR_GEOMETRY_FILE_NO_DATA            = -2501,
    ERROR_GEOMETRY_INVALID_VERTEX_SEMANTIC = -2502,

    ERROR_WINDOW_EVENTS_ALREADY_REGISTERED = -3000,
    ERROR_IMGUI_INITIALIZATION_FAILED      = -3001,

    ERROR_FONT_PARSE_FAILED   = -4000,
    ERROR_INVALID_UTF8_STRING = -4001,

    ERROR_PPM_EXPORT_FORMAT_NOT_SUPPORTED = -5000,
    ERROR_PPM_EXPORT_INVALID_SIZE         = -5001,
};

inline const char* ToString(ppx::Result value)
{
    // clang-format off
    switch (value) {
        default: break;
        case Result::SUCCESS                                          : return "SUCCESS";
        case Result::ERROR_FAILED                                     : return "ERROR_FAILED";
        case Result::ERROR_ALLOCATION_FAILED                          : return "ERROR_ALLOCATION_FAILED";
        case Result::ERROR_OUT_OF_MEMORY                              : return "ERROR_OUT_OF_MEMORY";
        case Result::ERROR_ELEMENT_NOT_FOUND                          : return "ERROR_ELEMENT_NOT_FOUND";
        case Result::ERROR_OUT_OF_RANGE                               : return "ERROR_OUT_OF_RANGE";
        case Result::ERROR_DUPLICATE_ELEMENT                          : return "ERROR_DUPLICATE_ELEMENT";
        case Result::ERROR_LIMIT_EXCEEDED                             : return "ERROR_LIMIT_EXCEEDED";
        case Result::ERROR_PATH_DOES_NOT_EXIST                        : return "ERROR_PATH_DOES_NOT_EXIST";
        case Result::ERROR_SINGLE_INIT_ONLY                           : return "ERROR_SINGLE_INIT_ONLY";
        case Result::ERROR_UNEXPECTED_NULL_ARGUMENT                   : return "ERROR_UNEXPECTED_NULL_ARGUMENT";
        case Result::ERROR_UNEXPECTED_COUNT_VALUE                     : return "ERROR_UNEXPECTED_COUNT_VALUE";
        case Result::ERROR_UNSUPPORTED_API                            : return "ERROR_UNSUPPORTED_API";
        case Result::ERROR_API_FAILURE                                : return "ERROR_API_FAILURE";
        case Result::ERROR_WAIT_FAILED                                : return "ERROR_WAIT_FAILED";
        case Result::ERROR_WAIT_TIMED_OUT                             : return "ERROR_WAIT_TIMED_OUT";
        case Result::ERROR_NO_GPUS_FOUND                              : return "ERROR_NO_GPUS_FOUND";
        case Result::ERROR_REQUIRED_FEATURE_UNAVAILABLE               : return "ERROR_REQUIRED_FEATURE_UNAVAILABLE";
        case Result::ERROR_BAD_DATA_SOURCE                            : return "ERROR_BAD_DATA_SOURCE";

        case Result::ERROR_GLFW_INIT_FAILED                           : return "ERROR_GLFW_INIT_FAILED";
        case Result::ERROR_GLFW_CREATE_WINDOW_FAILED                  : return "ERROR_GLFW_CREATE_WINDOW_FAILED";

        case Result::ERROR_INVALID_CREATE_ARGUMENT                    : return "ERROR_INVALID_CREATE_ARGUMENT";
        case Result::ERROR_RANGE_ALIASING_NOT_ALLOWED                 : return "ERROR_RANGE_ALIASING_NOT_ALLOWED";

        case Result::ERROR_GRFX_INVALID_OWNERSHIP                     : return "ERROR_GRFX_INVALID_OWNERSHIP";
        case Result::ERROR_GRFX_OBJECT_OWNERSHIP_IS_RESTRICTED        : return "ERROR_GRFX_OBJECT_OWNERSHIP_IS_RESTRICTED";
        case Result::ERROR_GRFX_UNSUPPORTED_SWAPCHAIN_FORMAT          : return "ERROR_GRFX_UNSUPPORTED_SWAPCHAIN_FORMAT";
        case Result::ERROR_GRFX_UNSUPPORTED_PRESENT_MODE              : return "ERROR_GRFX_UNSUPPORTED_PRESENT_MODE";
        case Result::ERROR_GRFX_MAX_VERTEX_BINDING_EXCEEDED           : return "ERROR_GRFX_MAX_VERTEX_BINDING_EXCEEDED";
        case Result::ERROR_GRFX_VERTEX_ATTRIBUTE_FORMAT_UNDEFINED     : return "ERROR_GRFX_VERTEX_ATTRIBUTE_FORMAT_UNDEFINED";
        case Result::ERROR_GRFX_VERTEX_ATTRIBUTE_OFFSET_OUT_OF_ORDER  : return "ERROR_GRFX_VERTEX_ATTRIBUTE_OFFSET_OUT_OF_ORDER";
        case Result::ERROR_GRFX_CANNOT_MIX_VERTEX_INPUT_RATES         : return "ERROR_GRFX_CANNOT_MIX_VERTEX_INPUT_RATES";
        case Result::ERROR_GRFX_UNKNOWN_DESCRIPTOR_TYPE               : return "ERROR_GRFX_UNKNOWN_DESCRIPTOR_TYPE";
        case Result::ERROR_GRFX_INVALID_DESCRIPTOR_TYPE               : return "ERROR_GRFX_INVALID_DESCRIPTOR_TYPE";
        case Result::ERROR_GRFX_DESCRIPTOR_COUNT_EXCEEDED             : return "ERROR_GRFX_DESCRIPTOR_COUNT_EXCEEDED";
        case Result::ERROR_GRFX_BINDING_NOT_IN_SET                    : return "ERROR_GRFX_BINDING_NOT_IN_SET";
        case Result::ERROR_GRFX_NON_UNIQUE_SET                        : return "ERROR_GRFX_NON_UNIQUE_SET";
        case Result::ERROR_GRFX_MINIMUM_BUFFER_SIZE_NOT_MET           : return "ERROR_GRFX_MINIMUM_BUFFER_SIZE_NOT_MET";
        case Result::ERROR_GRFX_INVALID_SHADER_BYTE_CODE              : return "ERROR_GRFX_INVALID_SHADER_BYTE_CODE";
        case Result::ERROR_INVALID_PIPELINE_INTERFACE                 : return "ERROR_INVALID_PIPELINE_INTERFACE";
        case Result::ERROR_GRFX_INVALID_QUERY_TYPE                    : return "ERROR_GRFX_INVALID_QUERY_TYPE";
        case Result::ERROR_GRFX_INVALID_QUERY_COUNT                   : return "ERROR_GRFX_INVALID_QUERY_COUNT";
        case Result::ERROR_GRFX_NO_QUEUES_AVAILABLE                   : return "ERROR_GRFX_NO_QUEUES_AVAILABLE";
        case Result::ERROR_GRFX_INVALID_INDEX_TYPE                    : return "ERROR_GRFX_INVALID_INDEX_TYPE";
        case Result::ERROR_GRFX_INVALID_GEOMETRY_CONFIGURATION        : return "ERROR_GRFX_INVALID_GEOMETRY_CONFIGURATION ";
        case Result::ERROR_GRFX_INVALID_VERTEX_ATTRIBUTE_COUNT        : return "ERROR_GRFX_INVALID_VERTEX_ATTRIBUTE_COUNT ";
        case Result::ERROR_GRFX_INVALID_VERTEX_ATTRIBUTE_STRIDE       : return "ERROR_GRFX_INVALID_VERTEX_ATTRIBUTE_STRIDE";

        case Result::ERROR_IMAGE_FILE_LOAD_FAILED                     : return "ERROR_IMAGE_FILE_LOAD_FAILED";
        case Result::ERROR_IMAGE_FILE_SAVE_FAILED                     : return "ERROR_IMAGE_FILE_SAVE_FAILED";
        case Result::ERROR_IMAGE_CANNOT_RESIZE_EXTERNAL_STORAGE       : return "ERROR_IMAGE_CANNOT_RESIZE_EXTERNAL_STORAGE";
        case Result::ERROR_IMAGE_INVALID_FORMAT                       : return "ERROR_IMAGE_INVALID_FORMAT";
        case Result::ERROR_IMAGE_RESIZE_FAILED                        : return "ERROR_IMAGE_RESIZE_FAILED";
        case Result::ERROR_BITMAP_CREATE_FAILED                       : return "ERROR_BITMAP_CREATE_FAILED";
        case Result::ERROR_BITMAP_BAD_COPY_SOURCE                     : return "ERROR_BITMAP_BAD_COPY_SOURCE";
        case Result::ERROR_BITMAP_FOOTPRINT_MISMATCH                  : return "ERROR_BITMAP_FOOTPRINT_MISMATCH";

        case Result::ERROR_NO_INDEX_DATA                              : return "ERROR_NO_INDEX_DATA";
        case Result::ERROR_GEOMETRY_FILE_LOAD_FAILED                  : return "ERROR_GEOMETRY_FILE_LOAD_FAILED";
        case Result::ERROR_GEOMETRY_FILE_NO_DATA                      : return "ERROR_GEOMETRY_FILE_NO_DATA";

        case Result::ERROR_WINDOW_EVENTS_ALREADY_REGISTERED           : return "ERROR_WINDOW_EVENTS_ALREADY_REGISTERED";
        case Result::ERROR_IMGUI_INITIALIZATION_FAILED                : return "ERROR_IMGUI_INITIALIZATION_FAILED";

        case Result::ERROR_FONT_PARSE_FAILED                          : return "ERROR_FONT_PARSE_FAILED";
        case Result::ERROR_INVALID_UTF8_STRING                        : return "ERROR_INVALID_UTF8_STRING";
    }
    // clang-format on
    return "<unknown ppx::Result value>";
}

inline bool Success(ppx::Result value)
{
    bool res = (value == ppx::SUCCESS);
    return res;
}

inline bool Failed(ppx::Result value)
{
    bool res = (value < ppx::SUCCESS);
    return res;
}

} // namespace ppx

#endif // ppx_config_h
