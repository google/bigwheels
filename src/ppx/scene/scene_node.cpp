#include "ppx/scene/scene_node.h"

namespace ppx {
namespace scene {

// -------------------------------------------------------------------------------------------------
// Node
// -------------------------------------------------------------------------------------------------
Node::Node(scene::Scene* pScene)
    : mScene(pScene)
{
}

Node::~Node()
{
}

void Node::SetVisible(bool visible)
{
    mVisible = visible;
}

const float4x4& Node::GetEvaluatedMatrix() const
{
    if (mEvaluatedDirty) {
        float4x4 parentEvaluatedMatrix = float4x4(1);
        if (!IsNull(mParent)) {
            parentEvaluatedMatrix = mParent->GetEvaluatedMatrix();
        }
        auto& concatenatedMatrix = GetConcatenatedMatrix();
        mEvaluatedMatrix         = parentEvaluatedMatrix * concatenatedMatrix;
        mEvaluatedDirty          = false;
    }
    return mEvaluatedMatrix;
}

void Node::SetEvaluatedDirty()
{
    mEvaluatedDirty = true;
    for (auto& pChild : mChildren) {
        pChild->SetEvaluatedDirty();
    }
}

void Node::SetTranslation(const float3& translation)
{
    Transform::SetTranslation(translation);
    SetEvaluatedDirty();
}

void Node::SetRotation(const float3& rotation)
{
    Transform::SetRotation(rotation);
    SetEvaluatedDirty();
}

void Node::SetScale(const float3& scale)
{
    Transform::SetScale(scale);
    SetEvaluatedDirty();
}

void Node::SetRotationOrder(Transform::RotationOrder value)
{
    Transform::SetRotationOrder(value);
    SetEvaluatedDirty();
}

void Node::SetParent(scene::Node* pNewParent)
{
    auto pCurrentParent = GetParent();
    if (!IsNull(pCurrentParent)) {
        pCurrentParent->RemoveChild(this);
    }

    mParent = pNewParent;
}

scene::Node* Node::GetChild(uint32_t index) const
{
    scene::Node* pChild = nullptr;
    if (ppx::GetElement(index, mChildren, &pChild)) {
        return nullptr;
    }
    return pChild;
}

ppx::Result Node::AddChild(scene::Node* pNode)
{
    if (IsNull(mScene)) {
        return ppx::ERROR_SCENE_INVALID_STANDALONE_OPERATION;
    }

    if (IsNull(pNode)) {
        return ppx::ERROR_UNEXPECTED_NULL_ARGUMENT;
    }

    // Make sure node isn't the parent or in the hierarchy.
    scene::Node* pParent = this;
    while (!IsNull(pParent)) {
        if (pParent == pNode) {
            return ppx::ERROR_SCENE_INVALID_NODE_HIERARCHY;
        }
        pParent = pParent->GetParent();
    }

    // Make sure child isn't already in mChildren
    if (ElementExists(pNode, mChildren)) {
        return ppx::ERROR_DUPLICATE_ELEMENT;
    }

    // Set node's parent, this will remove it from the current parent
    pNode->SetParent(this);

    // Add child to current node
    mChildren.push_back(pNode);

    return ppx::SUCCESS;
}

void Node::RemoveChild(const scene::Node* pChild)
{
    if (IsNull(pChild)) {
        return;
    }

    mChildren.erase(
        std::remove(mChildren.begin(), mChildren.end(), pChild),
        mChildren.end());
}

// -------------------------------------------------------------------------------------------------
// MeshNode
// -------------------------------------------------------------------------------------------------
MeshNode::MeshNode(const scene::MeshRef& mesh, scene::Scene* pScene)
    : scene::Node(pScene),
      mMesh(mesh)
{
}

MeshNode::~MeshNode()
{
}

void MeshNode::SetMesh(const scene::MeshRef& mesh)
{
    mMesh = mesh;
}

// -------------------------------------------------------------------------------------------------
// CameraNode
// -------------------------------------------------------------------------------------------------
CameraNode::CameraNode(std::unique_ptr<Camera>&& camera, scene::Scene* pScene)
    : scene::Node(pScene),
      mCamera(std::move(camera))
{
}

CameraNode::~CameraNode()
{
}

void CameraNode::UpdateCameraLookAt()
{
    const float4x4& rotationMatrix = this->GetRotationMatrix();

    // Rotate the view direction
    float3 viewDir = rotationMatrix * float4(0, 0, -1, 0);

    const float3& eyePos = this->GetTranslation();
    float3        target = eyePos + viewDir;

    mCamera->LookAt(eyePos, target, mCamera->GetWorldUp());
}

void CameraNode::SetTranslation(const float3& translation)
{
    scene::Node::SetTranslation(translation);
    UpdateCameraLookAt();
}

void CameraNode::SetRotation(const float3& rotation)
{
    scene::Node::SetRotation(rotation);
    UpdateCameraLookAt();
}

// -------------------------------------------------------------------------------------------------
// LightNode
// -------------------------------------------------------------------------------------------------
LightNode::LightNode(scene::Scene* pScene)
    : scene::Node(pScene)
{
}

LightNode::~LightNode()
{
}

void LightNode::SetRotation(const float3& rotation)
{
    scene::Node::SetRotation(rotation);
}

} // namespace scene
} // namespace ppx
