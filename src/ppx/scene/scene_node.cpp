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

void Node::SetVisible(bool visible, bool recursive)
{
    mVisible = visible;
    if (recursive) {
        for (auto pChild : mChildren) {
            pChild->SetVisible(visible, recursive);
        }
    }
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

void Node::SetParent(scene::Node* pNewParent)
{
    mParent = pNewParent;
    SetEvaluatedDirty();
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

scene::Node* Node::GetChild(uint32_t index) const
{
    scene::Node* pChild = nullptr;
    ppx::GetElement(index, mChildren, &pChild);
    return pChild;
}

bool Node::IsInSubTree(const scene::Node* pNode)
{
    bool inSubTree = (pNode == this);
    if (!inSubTree) {
        for (auto pChild : mChildren) {
            inSubTree = pChild->IsInSubTree(pNode);
            if (inSubTree) {
                break;
            }
        }
    }
    return inSubTree;
}

ppx::Result Node::AddChild(scene::Node* pNewChild)
{
    // Cannot add child if current node is standalone
    if (IsNull(mScene)) {
        return ppx::ERROR_SCENE_INVALID_STANDALONE_OPERATION;
    }

    // Cannot add NULL child
    if (IsNull(pNewChild)) {
        return ppx::ERROR_UNEXPECTED_NULL_ARGUMENT;
    }

    // Cannot add self as a child
    if (pNewChild == this) {
        return ppx::ERROR_SCENE_INVALID_NODE_HIERARCHY;
    }

    // Cannot add child if current node is in child's subtree
    if (pNewChild->IsInSubTree(this)) {
        return ppx::ERROR_SCENE_INVALID_NODE_HIERARCHY;
    }

    // Don't add new child if it already exists
    auto it = std::find(mChildren.begin(), mChildren.end(), pNewChild);
    if (it != mChildren.end()) {
        return ppx::ERROR_DUPLICATE_ELEMENT;
    }

    // Don't add new child if it currently has a parent
    const auto pCurrentParent = pNewChild->GetParent();
    if (!IsNull(pCurrentParent)) {
        return ppx::ERROR_SCENE_NODE_ALREADY_HAS_PARENT;
    }

    pNewChild->SetParent(this);

    mChildren.push_back(pNewChild);

    return ppx::SUCCESS;
}

scene::Node* Node::RemoveChild(const scene::Node* pChild)
{
    // Return null if pChild is null or if pChild is self
    if (IsNull(pChild) || (pChild == this)) {
        return nullptr;
    }

    // Return NULL if pChild isn't in mChildren
    auto it = std::find(mChildren.begin(), mChildren.end(), pChild);
    if (it == mChildren.end()) {
        return nullptr;
    }

    // Remove pChild from mChildren
    mChildren.erase(
        std::remove(mChildren.begin(), mChildren.end(), pChild),
        mChildren.end());

    scene::Node* pParentlessChild = const_cast<scene::Node*>(pChild);
    pParentlessChild->SetParent(nullptr);

    return pParentlessChild;
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
