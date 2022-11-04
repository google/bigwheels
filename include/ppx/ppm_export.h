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

#ifndef ppm_export_h
#define ppm_export_h

#include "ppx/config.h"
#include "ppx/grfx/grfx_format.h"

#include <ostream>

namespace ppx {

//! @brief Exports a 1D or 2D image to a PPM file.
//! @param outputFilename The name of the PPM file to be written.
//! @param inputFormat The input image format.
//! @param texels An array of texels of size \b rowStride * \b height.
//! @param width The width of the input image.
//! @param height The height of the input image.
//! @param rowStride The row stride, in bytes.
Result ExportToPPM(const std::string& outputFilename, grfx::Format inputFormat, const void* texels, uint32_t width, uint32_t height, uint32_t rowStride);

//! @brief Exports a 1D or 2D image as a PPM stream.
//! @param outputStream The output stream where the PPM data will be written.
//! @param inputFormat The input image format.
//! @param texels An array of texels of size \b rowStride * \b height.
//! @param width The width of the input image.
//! @param height The height of the input image.
//! @param rowStride The row stride, in bytes.
Result ExportToPPM(std::ostream& outputStream, grfx::Format inputFormat, const void* texels, uint32_t width, uint32_t height, uint32_t rowStride);

} // namespace ppx

#endif // ppm_export_h
