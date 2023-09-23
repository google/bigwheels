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

#ifndef ppx_grfx_scope_h
#define ppx_grfx_scope_h

#include "ppx/grfx/grfx_config.h"

namespace ppx {
namespace grfx {

class ScopeDestroyer
{
public:
    ScopeDestroyer(grfx::Device* pDevice);
    ~ScopeDestroyer();

    Result AddObject(grfx::Image* pObject);
    Result AddObject(grfx::Buffer* pObject);
    Result AddObject(grfx::Mesh* pObject);
    Result AddObject(grfx::Texture* pObject);
    Result AddObject(grfx::Sampler* pObject);
    Result AddObject(grfx::SampledImageView* pObject);
    Result AddObject(grfx::Queue* pParent, grfx::CommandBuffer* pObject);

    // Releases all objects without destroying them
    void ReleaseAll();

private:
    grfx::DevicePtr                                                mDevice;
    std::vector<grfx::ImagePtr>                                    mImages;
    std::vector<grfx::BufferPtr>                                   mBuffers;
    std::vector<grfx::MeshPtr>                                     mMeshes;
    std::vector<grfx::TexturePtr>                                  mTextures;
    std::vector<grfx::SamplerPtr>                                  mSamplers;
    std::vector<grfx::SampledImageViewPtr>                         mSampledImageViews;
    std::vector<std::pair<grfx::QueuePtr, grfx::CommandBufferPtr>> mTransientCommandBuffers;
};

} // namespace grfx
} // namespace ppx

#endif //  ppx_grfx_scope_h
