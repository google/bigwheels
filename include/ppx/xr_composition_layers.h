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

#ifndef PPX_XR_COMPOSITION_LAYERS_H
#define PPX_XR_COMPOSITION_LAYERS_H

#if defined(PPX_BUILD_XR)

#include <openxr/openxr.h>

#include <memory>
#include <utility>
#include <vector>

namespace ppx {

//! @class XrLayerBase
class XrLayerBase
{
public:
    virtual ~XrLayerBase()                                = default;
    virtual XrCompositionLayerBaseHeader* basePtr() const = 0;
    virtual uint32_t                      zIndex() const  = 0;
};

//! @class XrLayer
template <typename T>
class XrLayer : public XrLayerBase
{
public:
    XrLayer()
        : mLayer(std::make_unique<T>()) {}
    virtual ~XrLayer() override = default;

    XrCompositionLayerBaseHeader* basePtr() const override
    {
        return reinterpret_cast<XrCompositionLayerBaseHeader*>(mLayer.get());
    }

    T& layer()
    {
        return *mLayer;
    }

    uint32_t zIndex() const override
    {
        return mZIndex;
    }
    void SetZIndex(uint32_t zIndex)
    {
        mZIndex = zIndex;
    }

private:
    uint32_t           mZIndex = 0;
    std::unique_ptr<T> mLayer;
};

//! @class XrProjectionLayer
class XrProjectionLayer : public XrLayer<XrCompositionLayerProjection>
{
public:
    XrProjectionLayer();
    void AddView(XrCompositionLayerProjectionView view);
    void AddView(XrCompositionLayerProjectionView view, XrCompositionLayerDepthInfoKHR depthInfo);

private:
    std::vector<XrCompositionLayerProjectionView> mViews;
    std::vector<XrCompositionLayerDepthInfoKHR>   mDepthInfos;
};

typedef XrLayer<XrCompositionLayerQuad>          XrQuadLayer;
typedef XrLayer<XrCompositionLayerPassthroughFB> XrPassthroughFbLayer;

} // namespace ppx

#endif // defined(PPX_BUILD_XR)
#endif // PPX_XR_COMPOSITION_LAYERS_H
