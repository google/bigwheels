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

#include "ppx/scene/scene_scene.h"

#include <set>

namespace ppx {
namespace scene {

Scene::Scene(std::unique_ptr<scene::ResourceManager>&& resourceManager)
    : mResourceManager(std::move(resourceManager))
{
}

scene::Node* Scene::GetNode(uint32_t index) const
{
    if (index >= CountU32(mNodes)) {
        return nullptr;
    }

    return mNodes[index].get();
}

scene::MeshNode* Scene::GetMeshNode(uint32_t index) const
{
    scene::MeshNode* pNode = nullptr;
    if (!ppx::GetElement(index, mMeshNodes, &pNode)) {
        return nullptr;
    }
    return pNode;
}

scene::CameraNode* Scene::GetCameraNode(uint32_t index) const
{
    scene::CameraNode* pNode = nullptr;
    if (!ppx::GetElement(index, mCameraNodes, &pNode)) {
        return nullptr;
    }
    return pNode;
}

scene::LightNode* Scene::GetLightNode(uint32_t index) const
{
    scene::LightNode* pNode = nullptr;
    if (!ppx::GetElement(index, mLightNodes, &pNode)) {
        return nullptr;
    }
    return pNode;
}

scene::Node* Scene::FindNode(const std::string& name) const
{
    auto it = ppx::FindIf(
        mNodes,
        [name](const scene::NodeRef& elem) {
                bool match = (elem->GetName() == name);
                return match; });
    if (it == mNodes.end()) {
        return nullptr;
    }

    scene::Node* pNode = (*it).get();
    return pNode;
}

scene::MeshNode* Scene::FindMeshNode(const std::string& name) const
{
    return FindNodeByName(name, mMeshNodes);
}

scene::CameraNode* Scene::FindCameraNode(const std::string& name) const
{
    return FindNodeByName(name, mCameraNodes);
}

scene::LightNode* Scene::FindLightNode(const std::string& name) const
{
    return FindNodeByName(name, mLightNodes);
}

ppx::Result Scene::AddNode(scene::NodeRef&& node)
{
    if (!node) {
        return ppx::ERROR_UNEXPECTED_NULL_ARGUMENT;
    }

    scene::MeshNode*   pMeshNode   = nullptr;
    scene::CameraNode* pCameraNode = nullptr;
    scene::LightNode*  pLightNode  = nullptr;
    //
    switch (node->GetNodeType()) {
        default: return ppx::ERROR_SCENE_UNSUPPORTED_NODE_TYPE;
        case scene::NODE_TYPE_TRANSFORM: break;
        case scene::NODE_TYPE_MESH: pMeshNode = static_cast<scene::MeshNode*>(node.get()); break;
        case scene::NODE_TYPE_CAMERA: pCameraNode = static_cast<scene::CameraNode*>(node.get()); break;
        case scene::NODE_TYPE_LIGHT: pLightNode = static_cast<scene::LightNode*>(node.get()); break;
    }

    auto it = std::find_if(
        mNodes.begin(),
        mNodes.end(),
        [&node](const scene::NodeRef& elem) -> bool {
			bool match = (elem.get() == node.get());
			return match; });
    if (it != mNodes.end()) {
        return ppx::ERROR_DUPLICATE_ELEMENT;
    }

    mNodes.push_back(std::move(node));

    if (!IsNull(pMeshNode)) {
        mMeshNodes.push_back(pMeshNode);
    }
    else if (!IsNull(pCameraNode)) {
        mCameraNodes.push_back(pCameraNode);
    }
    else if (!IsNull(pLightNode)) {
        mLightNodes.push_back(pLightNode);
    }

    return ppx::SUCCESS;
}

scene::ResourceArrayIndexMap<scene::Sampler> Scene::GetSamplersArrayIndexMap() const
{
    auto array = mResourceManager->GetSamplers();

    std::unordered_map<const scene::Sampler*, uint32_t> indexMap;
    //
    const uint32_t objectCount = CountU32(array);
    for (uint32_t idx = 0; idx < objectCount; ++idx) {
        auto pObject      = array[idx];
        indexMap[pObject] = idx;
    }

    return std::make_pair(array, indexMap);
}

scene::ResourceArrayIndexMap<scene::Image> Scene::GetImagesArrayIndexMap() const
{
    auto array = mResourceManager->GetImages();

    std::unordered_map<const scene::Image*, uint32_t> indexMap;
    //
    const uint32_t objectCount = CountU32(array);
    for (uint32_t idx = 0; idx < objectCount; ++idx) {
        auto pObject      = array[idx];
        indexMap[pObject] = idx;
    }

    return std::make_pair(array, indexMap);
}

scene::ResourceArrayIndexMap<scene::Material> Scene::GetMaterialsArrayIndexMap() const
{
    auto array = mResourceManager->GetMaterials();

    std::unordered_map<const scene::Material*, uint32_t> indexMap;
    //
    const uint32_t objectCount = CountU32(array);
    for (uint32_t idx = 0; idx < objectCount; ++idx) {
        auto pObject      = array[idx];
        indexMap[pObject] = idx;
    }

    return std::make_pair(array, indexMap);
}

} // namespace scene
} // namespace ppx
