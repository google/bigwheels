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

#if defined(PPX_BUILD_XR)
#include "ppx/xr_composition_layers.h"

#include <memory>
#include <optional>
#include <utility>
#include <vector>

namespace ppx {

XrProjectionLayer::XrProjectionLayer()
{
    layer().views = mViews.data();
}

void XrProjectionLayer::AddView(XrCompositionLayerProjectionView view)
{
    mViews.emplace_back(view);

    layer().views     = mViews.data();
    layer().viewCount = static_cast<uint32_t>(mViews.size());
}

void XrProjectionLayer::AddView(XrCompositionLayerProjectionView view, XrCompositionLayerDepthInfoKHR depthInfo)
{
    mDepthInfos.emplace_back(std::make_unique<XrCompositionLayerDepthInfoKHR>(depthInfo));
    mViews.emplace_back(view);

    mViews.back().next = mDepthInfos.back().get();
    layer().views      = mViews.data();
    layer().viewCount  = static_cast<uint32_t>(mViews.size());
}

} // namespace ppx

#endif // defined(PPX_BUILD_XR)
