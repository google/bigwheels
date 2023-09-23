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

#ifndef GLTF_LOAD_OBJECTS_H
#define GLTF_LOAD_OBJECTS_H

#include "ppx/ppx.h"
#include "ppx/scene/scene_material.h"
#include "ppx/scene/scene_mesh.h"
#include "ppx/scene/scene_pipeline_args.h"

class GltfLoadObjectsApp
    : public ppx::Application
{
public:
    virtual void Config(ppx::ApplicationSettings& settings) override;
    virtual void Setup() override;
    virtual void Shutdown() override;
    virtual void Render() override;

private:
    struct PerFrame
    {
        ppx::grfx::CommandBufferPtr cmd;
        ppx::grfx::SemaphorePtr     imageAcquiredSemaphore;
        ppx::grfx::FencePtr         imageAcquiredFence;
        ppx::grfx::SemaphorePtr     renderCompleteSemaphore;
        ppx::grfx::FencePtr         renderCompleteFence;
    };

    std::vector<PerFrame>           mPerFrame;
    ppx::grfx::ShaderModulePtr      mVS;
    ppx::grfx::ShaderModulePtr      mPS;
    ppx::grfx::PipelineInterfacePtr mPipelineInterface;

    ppx::scene::Node*                 mCameraNode            = nullptr;
    ppx::scene::Node*                 mNoMaterialTransform   = nullptr;
    ppx::scene::Mesh*                 mNoMaterialMesh        = nullptr;
    ppx::scene::Node*                 mBlueMaterialTransform = nullptr;
    ppx::scene::Mesh*                 mBlueMaterialMesh      = nullptr;
    ppx::scene::Node*                 mDrawImageTransform    = nullptr;
    ppx::scene::Mesh*                 mDrawImageMesh         = nullptr;
    ppx::scene::Node*                 mDrawTextureTransform  = nullptr;
    ppx::scene::Mesh*                 mDrawTextureMesh       = nullptr;
    ppx::scene::Node*                 mTextureTransform      = nullptr;
    ppx::scene::Mesh*                 mTextureMesh           = nullptr;
    ppx::scene::Node*                 mTextNode              = nullptr;
    ppx::scene::MaterialPipelineArgs* mPipelineArgs          = nullptr;

    std::vector<const ppx::scene::Material*>                                        mMaterials;
    std::unordered_map<const ppx::scene::Material*, uint32_t>                       mMaterialIndexMap;
    std::unordered_map<const ppx::scene::Material*, ppx::grfx::GraphicsPipelinePtr> mMaterialPipelineMap;

    ppx::grfx::DescriptorSetLayoutPtr mDescriptorSetLayout;
    ppx::grfx::SamplerPtr             mSampler;
};

#endif // GLTF_LOAD_OBJECTS_H
