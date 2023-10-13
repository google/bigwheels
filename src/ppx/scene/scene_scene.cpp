#include "ppx/scene/scene_scene.h"

namespace ppx {
namespace scene {

Scene::Scene(std::unique_ptr<scene::ResourceManager>&& resourceManager)
    : mResourceManager(std::move(resourceManager))
{
}

Scene::~Scene()
{
    mResourceManager->DestroyAll();
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

} // namespace scene
} // namespace ppx
