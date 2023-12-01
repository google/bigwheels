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

#include "ppx/ppm_export.h"

#include <filesystem>

namespace ppx {

unsigned char ConvertToUint(const char* value, grfx::FormatDataType dataType)
{
    switch (dataType) {
        case grfx::FORMAT_DATA_TYPE_SINT:
        case grfx::FORMAT_DATA_TYPE_SNORM: {
            return *value + 128;
        }
        case grfx::FORMAT_DATA_TYPE_UINT:
        case grfx::FORMAT_DATA_TYPE_UNORM:
        case grfx::FORMAT_DATA_TYPE_SRGB:
        default: {
            return *(unsigned char*)value;
        }
    }
}

bool IsOptimalFormat(const grfx::FormatDesc* desc, uint32_t width, uint32_t rowStride)
{
    // The optimal format has RGB components in the right order and no alpha,
    // no row padding, and no format conversion necessary.
    return desc->componentBits == grfx::FORMAT_COMPONENT_RED_GREEN_BLUE &&
           desc->componentOffset.red == 0 && desc->componentOffset.green == 1 && desc->componentOffset.blue == 2 &&
           rowStride == desc->bytesPerTexel * width &&
           (desc->dataType == grfx::FORMAT_DATA_TYPE_UINT || desc->dataType == grfx::FORMAT_DATA_TYPE_UNORM || desc->dataType == grfx::FORMAT_DATA_TYPE_SRGB);
}

Result ExportToPPM(const std::string& outputFilename, grfx::Format inputFormat, const void* texels, uint32_t width, uint32_t height, uint32_t rowStride)
{
    std::filesystem::create_directories(std::filesystem::path(outputFilename).parent_path());
    std::ofstream file(outputFilename, std::ios::out | std::ios::binary | std::ios::trunc);
    ppx::Result   result = ExportToPPM(file, inputFormat, texels, width, height, rowStride);
    file.close();
    return result;
}

Result ExportToPPM(std::ostream& outputStream, grfx::Format inputFormat, const void* texels, uint32_t width, uint32_t height, uint32_t rowStride)
{
    if (width == 0 || height == 0) {
        return ERROR_PPM_EXPORT_INVALID_SIZE;
    }

    const grfx::FormatDesc* desc = grfx::GetFormatDescription(inputFormat);

    // We don't support compressed or packed formats.
    if (desc->layout != grfx::FORMAT_LAYOUT_LINEAR) {
        return ERROR_PPM_EXPORT_FORMAT_NOT_SUPPORTED;
    }
    // We don't support FLOAT formats.
    if (desc->dataType == grfx::FORMAT_DATA_TYPE_FLOAT) {
        return ERROR_PPM_EXPORT_FORMAT_NOT_SUPPORTED;
    }

    // We only support color formats for now.
    // The alpha channel, if present, is ignored, as the PPM file format
    // does not support transparency.
    auto rgbMask = grfx::FORMAT_COMPONENT_RED_GREEN_BLUE;
    if ((desc->componentBits & rgbMask) == 0) {
        return ERROR_PPM_EXPORT_FORMAT_NOT_SUPPORTED;
    }
    // We only support 8-bit formats.
    if (desc->bytesPerComponent != 1) {
        return ERROR_PPM_EXPORT_FORMAT_NOT_SUPPORTED;
    }

    PPX_ASSERT_MSG(rowStride >= desc->bytesPerTexel * width, "row stride must be at least equal to texel size * width");

    // PPM format specification: http://netpbm.sourceforge.net/doc/ppm.html.
    outputStream << "P6\n"
                 << width << "\n"
                 << height << "\n"
                 << 255 << "\n";

    // This is a naive implementation, and favors flexibility over performance
    // with the aim to support as many format variations as possible.
    // We only optimize for the best possible scenario, and fall back to the
    // generic implemention in other cases.
    if (IsOptimalFormat(desc, width, rowStride)) {
        outputStream.write((const char*)texels, static_cast<std::streamsize>(rowStride) * height);
        return SUCCESS;
    }

    char        zero = 0;
    const char* data = (const char*)texels;
    for (uint32_t y = 0; y < height; y++) {
        const char* row = data;
        for (uint32_t x = 0; x < width; x++) {
            if (desc->componentBits & grfx::FORMAT_COMPONENT_RED) {
                unsigned char red = ConvertToUint(row + desc->componentOffset.red, desc->dataType);
                outputStream.write((const char*)&red, 1);
            }
            else {
                outputStream.write(&zero, 1);
            }

            if (desc->componentBits & grfx::FORMAT_COMPONENT_GREEN) {
                unsigned char green = ConvertToUint(row + desc->componentOffset.green, desc->dataType);
                outputStream.write((const char*)&green, 1);
            }
            else {
                outputStream.write(&zero, 1);
            }

            if (desc->componentBits & grfx::FORMAT_COMPONENT_BLUE) {
                unsigned char blue = ConvertToUint(row + desc->componentOffset.blue, desc->dataType);
                outputStream.write((const char*)&blue, 1);
            }
            else {
                outputStream.write(&zero, 1);
            }

            row += desc->bytesPerTexel;
        }
        data += rowStride;
    }

    return SUCCESS;
}

} // namespace ppx
