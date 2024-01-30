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

#include "ppx/grfx/grfx_shading_rate.h"
#include "ppx/grfx/grfx_device.h"
#include "ppx/graphics_util.h"

namespace ppx {
namespace grfx {

std::unique_ptr<Bitmap> ShadingRatePattern::CreateBitmap() const
{
    auto bitmap = std::make_unique<Bitmap>();
    PPX_CHECKED_CALL(Bitmap::Create(
        GetAttachmentWidth(), GetAttachmentHeight(), GetBitmapFormat(), bitmap.get()));
    return bitmap;
}

Result ShadingRatePattern::LoadFromBitmap(Bitmap* bitmap)
{
    return grfx_util::CopyBitmapToImage(
        GetDevice()->GetGraphicsQueue(), bitmap, mAttachmentImage, 0, 0, mAttachmentImage->GetInitialState(), mAttachmentImage->GetInitialState());
}

} // namespace grfx
} // namespace ppx
