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

#include "ppx/grfx/dx/dx_util.h"

namespace ppx {
namespace grfx {
namespace dx {

DXGI_FORMAT ToDxgiFormat(grfx::Format value)
{
    //
    // NOTE: DXGI_FORMAT_UNKNOWN is returned for any format that is
    //       not supported by D3D12.
    //
    // NOTE: D3D12 support for 3 channel formats is limited.
    //
    // NOTE: D3D12 support for SRGB formats is limited.
    //

    // clang-format off
    switch (value) {
        default: break;

        // 8-bit signed normalized
        case grfx::FORMAT_R8_SNORM           : return DXGI_FORMAT_R8_SNORM; break;
        case grfx::FORMAT_R8G8_SNORM         : return DXGI_FORMAT_R8G8_SNORM; break;
        case grfx::FORMAT_R8G8B8_SNORM       : return DXGI_FORMAT_UNKNOWN; break;
        case grfx::FORMAT_R8G8B8A8_SNORM     : return DXGI_FORMAT_R8G8B8A8_SNORM; break;
        case grfx::FORMAT_B8G8R8_SNORM       : return DXGI_FORMAT_UNKNOWN; break;
        case grfx::FORMAT_B8G8R8A8_SNORM     : return DXGI_FORMAT_UNKNOWN; break;
                                             
        // 8-bit unsigned normalized         
        case grfx::FORMAT_R8_UNORM           : return DXGI_FORMAT_R8_UNORM; break;
        case grfx::FORMAT_R8G8_UNORM         : return DXGI_FORMAT_R8G8_UNORM; break;
        case grfx::FORMAT_R8G8B8_UNORM       : return DXGI_FORMAT_UNKNOWN; break;
        case grfx::FORMAT_R8G8B8A8_UNORM     : return DXGI_FORMAT_R8G8B8A8_UNORM; break;
        case grfx::FORMAT_B8G8R8_UNORM       : return DXGI_FORMAT_UNKNOWN; break;
        case grfx::FORMAT_B8G8R8A8_UNORM     : return DXGI_FORMAT_B8G8R8A8_UNORM; break;
                                             
        // 8-bit signed integer              
        case grfx::FORMAT_R8_SINT            : return DXGI_FORMAT_R8_SINT; break;
        case grfx::FORMAT_R8G8_SINT          : return DXGI_FORMAT_R8G8_SINT; break;
        case grfx::FORMAT_R8G8B8_SINT        : return DXGI_FORMAT_UNKNOWN; break;
        case grfx::FORMAT_R8G8B8A8_SINT      : return DXGI_FORMAT_R8G8B8A8_SINT; break;
        case grfx::FORMAT_B8G8R8_SINT        : return DXGI_FORMAT_UNKNOWN; break;
        case grfx::FORMAT_B8G8R8A8_SINT      : return DXGI_FORMAT_UNKNOWN; break;
                                             
        // 8-bit unsigned integer            
        case grfx::FORMAT_R8_UINT            : return DXGI_FORMAT_R8_UINT; break;
        case grfx::FORMAT_R8G8_UINT          : return DXGI_FORMAT_R8G8_UINT; break;
        case grfx::FORMAT_R8G8B8_UINT        : return DXGI_FORMAT_UNKNOWN; break;
        case grfx::FORMAT_R8G8B8A8_UINT      : return DXGI_FORMAT_R8G8B8A8_UINT; break;
        case grfx::FORMAT_B8G8R8_UINT        : return DXGI_FORMAT_UNKNOWN; break;
        case grfx::FORMAT_B8G8R8A8_UINT      : return DXGI_FORMAT_UNKNOWN; break;
                                             
        // 16-bit signed normalized          
        case grfx::FORMAT_R16_SNORM          : return DXGI_FORMAT_R16_SNORM; break;
        case grfx::FORMAT_R16G16_SNORM       : return DXGI_FORMAT_R16G16_SNORM; break;
        case grfx::FORMAT_R16G16B16_SNORM    : return DXGI_FORMAT_UNKNOWN; break;
        case grfx::FORMAT_R16G16B16A16_SNORM : return DXGI_FORMAT_R16G16B16A16_SNORM; break;
                                             
        // 16-bit unsigned normalized        
        case grfx::FORMAT_R16_UNORM          : return DXGI_FORMAT_R16_UNORM; break;
        case grfx::FORMAT_R16G16_UNORM       : return DXGI_FORMAT_R16G16_UNORM; break;
        case grfx::FORMAT_R16G16B16_UNORM    : return DXGI_FORMAT_UNKNOWN; break;
        case grfx::FORMAT_R16G16B16A16_UNORM : return DXGI_FORMAT_R16G16B16A16_UNORM; break;
                                             
        // 16-bit signed integer             
        case grfx::FORMAT_R16_SINT           : return DXGI_FORMAT_R16_SINT; break;
        case grfx::FORMAT_R16G16_SINT        : return DXGI_FORMAT_R16G16_SINT; break;
        case grfx::FORMAT_R16G16B16_SINT     : return DXGI_FORMAT_UNKNOWN; break;
        case grfx::FORMAT_R16G16B16A16_SINT  : return DXGI_FORMAT_R16G16B16A16_SINT; break;
                                             
        // 16-bit unsigned integer           
        case grfx::FORMAT_R16_UINT           : return DXGI_FORMAT_R16_UINT; break;
        case grfx::FORMAT_R16G16_UINT        : return DXGI_FORMAT_R16G16_UINT; break;
        case grfx::FORMAT_R16G16B16_UINT     : return DXGI_FORMAT_UNKNOWN; break;
        case grfx::FORMAT_R16G16B16A16_UINT  : return DXGI_FORMAT_R16G16B16A16_UINT; break;
                                             
        // 16-bit float                      
        case grfx::FORMAT_R16_FLOAT          : return DXGI_FORMAT_R16_FLOAT; break;
        case grfx::FORMAT_R16G16_FLOAT       : return DXGI_FORMAT_R16G16_FLOAT; break;
        case grfx::FORMAT_R16G16B16_FLOAT    : return DXGI_FORMAT_UNKNOWN; break;
        case grfx::FORMAT_R16G16B16A16_FLOAT : return DXGI_FORMAT_R16G16B16A16_FLOAT; break;
                                             
        // 32-bit signed integer             
        case grfx::FORMAT_R32_SINT           : return DXGI_FORMAT_R32_SINT; break;
        case grfx::FORMAT_R32G32_SINT        : return DXGI_FORMAT_R32G32_SINT; break;
        case grfx::FORMAT_R32G32B32_SINT     : return DXGI_FORMAT_R32G32B32_SINT; break;
        case grfx::FORMAT_R32G32B32A32_SINT  : return DXGI_FORMAT_R32G32B32A32_SINT; break;
                                             
        // 32-bit unsigned integer           
        case grfx::FORMAT_R32_UINT           : return DXGI_FORMAT_R32_UINT; break;
        case grfx::FORMAT_R32G32_UINT        : return DXGI_FORMAT_R32G32_UINT; break;
        case grfx::FORMAT_R32G32B32_UINT     : return DXGI_FORMAT_R32G32B32_UINT; break;
        case grfx::FORMAT_R32G32B32A32_UINT  : return DXGI_FORMAT_R32G32B32A32_UINT; break;
                                             
        // 32-bit float                      
        case grfx::FORMAT_R32_FLOAT          : return DXGI_FORMAT_R32_FLOAT; break;
        case grfx::FORMAT_R32G32_FLOAT       : return DXGI_FORMAT_R32G32_FLOAT; break;
        case grfx::FORMAT_R32G32B32_FLOAT    : return DXGI_FORMAT_R32G32B32_FLOAT; break;
        case grfx::FORMAT_R32G32B32A32_FLOAT : return DXGI_FORMAT_R32G32B32A32_FLOAT; break;
                                             
        // 8-bit unsigned integer stencil    
        case grfx::FORMAT_S8_UINT            : return DXGI_FORMAT_UNKNOWN; break;
                                             
        // 16-bit unsigned normalized depth  
        case grfx::FORMAT_D16_UNORM          : return DXGI_FORMAT_D16_UNORM; break;
                                             
        // 32-bit float depth                
        case grfx::FORMAT_D32_FLOAT          : return DXGI_FORMAT_D32_FLOAT; break;
                                             
        // Depth/stencil combinations        
        case grfx::FORMAT_D16_UNORM_S8_UINT  : return DXGI_FORMAT_UNKNOWN; break;
        case grfx::FORMAT_D24_UNORM_S8_UINT  : return DXGI_FORMAT_D24_UNORM_S8_UINT; break;
        case grfx::FORMAT_D32_FLOAT_S8_UINT  : return DXGI_FORMAT_D24_UNORM_S8_UINT; break;
                                             
        // SRGB                              
        case grfx::FORMAT_R8_SRGB            : return DXGI_FORMAT_UNKNOWN; break;
        case grfx::FORMAT_R8G8_SRGB          : return DXGI_FORMAT_UNKNOWN; break;
        case grfx::FORMAT_R8G8B8_SRGB        : return DXGI_FORMAT_UNKNOWN; break;
        case grfx::FORMAT_R8G8B8A8_SRGB      : return DXGI_FORMAT_R8G8B8A8_UNORM_SRGB; break;
        case grfx::FORMAT_B8G8R8_SRGB        : return DXGI_FORMAT_UNKNOWN; break;
        case grfx::FORMAT_B8G8R8A8_SRGB      : return DXGI_FORMAT_B8G8R8A8_UNORM_SRGB; break;

        // 10-bit
        case FORMAT_R10G10B10A2_UNORM        : return DXGI_FORMAT_R10G10B10A2_UNORM; break;

        // 11-bit R, 11-bit G, 10-bit B packed
        case FORMAT_R11G11B10_FLOAT          : return DXGI_FORMAT_R11G11B10_FLOAT; break;

        // Compressed images
        case FORMAT_BC1_RGBA_SRGB            : return DXGI_FORMAT_BC1_TYPELESS; break;
        case FORMAT_BC1_RGBA_UNORM           : return DXGI_FORMAT_BC1_UNORM; break;
        case FORMAT_BC1_RGB_SRGB             : return DXGI_FORMAT_BC1_UNORM_SRGB; break;
        case FORMAT_BC1_RGB_UNORM            : return DXGI_FORMAT_BC1_UNORM_SRGB; break; // FAILS with compressonator-generated DDS file
        case FORMAT_BC2_SRGB                 : return DXGI_FORMAT_BC2_UNORM_SRGB; break;
        case FORMAT_BC2_UNORM                : return DXGI_FORMAT_BC2_UNORM; break;
        case FORMAT_BC3_SRGB                 : return DXGI_FORMAT_BC3_UNORM_SRGB; break;
        case FORMAT_BC3_UNORM                : return DXGI_FORMAT_BC3_UNORM; break;
        case FORMAT_BC4_UNORM                : return DXGI_FORMAT_BC4_UNORM; break; // FAILS with compressonator-generated DDS file
        case FORMAT_BC4_SNORM                : return DXGI_FORMAT_BC4_SNORM; break;
        case FORMAT_BC5_UNORM                : return DXGI_FORMAT_BC5_UNORM; break;
        case FORMAT_BC5_SNORM                : return DXGI_FORMAT_BC5_SNORM; break;
        case FORMAT_BC6H_UFLOAT              : return DXGI_FORMAT_BC6H_UF16; break;
        case FORMAT_BC6H_SFLOAT              : return DXGI_FORMAT_BC6H_SF16; break;
        case FORMAT_BC7_UNORM                : return DXGI_FORMAT_BC7_UNORM; break;
        case FORMAT_BC7_SRGB                 : return DXGI_FORMAT_BC7_UNORM_SRGB; break;
    }
    // clang-format on

    return DXGI_FORMAT_UNKNOWN;
}

} // namespace dx
} // namespace grfx
} // namespace ppx
