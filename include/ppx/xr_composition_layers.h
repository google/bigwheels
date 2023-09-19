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
#include <optional>
#include <utility>
#include <vector>

namespace ppx {

// Wrapper interface around the OpenXr XrCompositionLayerBaseHeader struct.
// Also adds z-index based layering, so that frames can be composed and re-structured without having to rebuild the layers vector manually.
class XrLayerBase
{
public:
    virtual ~XrLayerBase() = default;

    // Return the OpenXR composition layer cast to a pointer of the base struct.
    virtual XrCompositionLayerBaseHeader* basePtr() const = 0;

    // Return the zIndex of this layer, higher zIndex values will be rendered in front of lower values.
    virtual uint32_t zIndex() const = 0;
};

// Base implementation of XrLayerBase for simple XrCompositionLayers.
// The underlying OpenXR composition layer struct is managed through this class.
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

    // Return a reference to the owned OpenXR layer struct.
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

// XrLayerBase implementation for XrCompositionLayerProjection layers.
// Projection layers contain references to projection views and depth info, both of which
// are managed within this class.
class XrProjectionLayer : public XrLayer<XrCompositionLayerProjection>
{
public:
    XrProjectionLayer();
    virtual ~XrProjectionLayer() override = default;

    // Add a new projection view to this layer that has no depth info.
    void AddView(XrCompositionLayerProjectionView view);
    // Add a new projection view to this layer that has associated depth info.
    void AddView(XrCompositionLayerProjectionView view, XrCompositionLayerDepthInfoKHR depthInfo);

private:
    std::vector<XrCompositionLayerProjectionView>                mViews;
    std::vector<std::unique_ptr<XrCompositionLayerDepthInfoKHR>> mDepthInfos;
};

// XrLayerBase implementation for XrCompositionLayerQuad layers.
using XrQuadLayer = XrLayer<XrCompositionLayerQuad>;

// XrLayerBase implementation for XrCompositionLayerPassthroughFB layers.
using XrPassthroughFbLayer = XrLayer<XrCompositionLayerPassthroughFB>;

} // namespace ppx

#endif // defined(PPX_BUILD_XR)
#endif // PPX_XR_COMPOSITION_LAYERS_H
