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

#ifndef ppx_grfx_shading_rate_util_h
#define ppx_grfx_shading_rate_util_h

#include "ppx/grfx/grfx_shading_rate.h"

namespace ppx {
namespace grfx {
// These set of functions create a bitmap.The bitmap can be used to produce
// a ShadingRatePattern that in tun can be used in a render pass creation to
// specify a non - uniform shading pattern.

// A uniform bitmap that has cells of fragmentWidth and fragementHeight size
void FillShadingRateUniformFragmentSize(ShadingRatePatternPtr pattern, uint32_t fragmentWidth, uint32_t fragmentHeight, Bitmap* bitmap);
// A uniform bit map that has cells of xDensity and yDensity size
void FillShadingRateUniformFragmentDensity(ShadingRatePatternPtr pattern, uint32_t xDensity, uint32_t yDensity, Bitmap* bitmap);
// A map with cells of radial scale size
void FillShadingRateRadial(ShadingRatePatternPtr pattern, float scale, Bitmap* bitmap);
// A map with cells produced by anisotropic filter of scale size
void FillShadingRateAnisotropic(ShadingRatePatternPtr pattern, float scale, Bitmap* bitmap);

} // namespace grfx
} // namespace ppx
#endif // ppx_grfx_shading_rate_util_h
