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

#ifndef ppx_scene_node_h
#define ppx_scene_node_h

#include "ppx/scene/scene_config.h"
#include "ppx/transform.h"
#include "ppx/camera.h"

namespace ppx {
namespace scene {

enum NodeType
{
    NODE_TYPE_TRANSFORM   = 0,
    NODE_TYPE_MESH        = 1,
    NODE_TYPE_CAMERA      = 2,
    NODE_TYPE_LIGHT       = 3,
    NODE_TYPE_UNSUPPORTED = 0x7FFFFFFF
};

// -------------------------------------------------------------------------------------------------

// Scene Graph Node
//
// This is the base class for scene graph nodes. It contains transform, parent, children,
// and visbility properties. scene::Node is instantiable and can be used as a locator/empty/group
// node that just contains children nodes.
//
// This node objects can also be used as standalone objects outside of a scene. Standalone
// nodes will have neither a parent or children. Loader implementations must not populate a
// standalone node's parent or children if loading a standalone node.
//
// When used as a standalone node, scene::Node stores only transform information.
//
class Node
    : public ppx::Transform,
      public grfx::NamedObjectTrait
{
public:
    Node(scene::Scene* pScene = nullptr);
    virtual ~Node();

    virtual scene::NodeType GetNodeType() const { return scene::NODE_TYPE_TRANSFORM; }

    bool IsVisible() const { return mVisible; }
    void SetVisible(bool visible);

    virtual void SetTranslation(const float3& translation) override;
    virtual void SetRotation(const float3& rotation) override;
    virtual void SetScale(const float3& scale) override;
    virtual void SetRotationOrder(Transform::RotationOrder rotationOrder) override;

    const float4x4& GetEvaluatedMatrix() const;

    scene::Node* GetParent() const { return mParent; }
    void         SetParent(scene::Node* pNewParent);

    uint32_t     GetChildCount() const { return CountU32(mChildren); }
    scene::Node* GetChild(uint32_t index) const;

    ppx::Result AddChild(scene::Node* pNew);
    void        RemoveChild(const scene::Node* pChild);

private:
    void SetEvaluatedDirty();

private:
    scene::Scene*             mScene           = nullptr;
    bool                      mVisible         = true;
    mutable ppx::Transform    mTransform       = {};
    mutable float4x4          mEvaluatedMatrix = float4x4(1);
    mutable bool              mEvaluatedDirty  = false;
    scene::Node*              mParent          = nullptr;
    std::vector<scene::Node*> mChildren        = {};
};

// -------------------------------------------------------------------------------------------------

// Scene Graph Mesh Node
//
// Scene graph nodes that contains reference to a scene::Mesh object.
//
// When used as a standalone node, scene::Mesh stores a mesh and its required objects
// as populated by a loader.
//
// See scene::Node class declaration for additional usages of scene::Node objects.
//
class MeshNode
    : public scene::Node
{
public:
    MeshNode(const scene::MeshRef& mesh, scene::Scene* pScene = nullptr);
    virtual ~MeshNode();

    virtual scene::NodeType GetNodeType() const override { return scene::NODE_TYPE_MESH; }

    const scene::Mesh* GetMesh() const { return mMesh.get(); }

    void SetMesh(const scene::MeshRef& mesh);

private:
    scene::MeshRef mMesh = nullptr;
};

// -------------------------------------------------------------------------------------------------

// Scene Graph Camera Node
//
// Scene graph nodes that contains reference contains an instance of
// a ppx::Camera object. Can be used for both perspective and orthographic
// cameras.
//
// When used as a standalone node, scene::Camera stores a camera object as populated
// by a loader.
//
// See scene::Node class declaration for additional usages of scene::Node objects.
//
class CameraNode
    : public scene::Node
{
public:
    CameraNode(std::unique_ptr<Camera>&& camera, scene::Scene* pScene = nullptr);
    virtual ~CameraNode();

    virtual scene::NodeType GetNodeType() const override { return scene::NODE_TYPE_CAMERA; }

    const ppx::Camera* GetCamera() const { return mCamera.get(); }

    virtual void SetTranslation(const float3& translation) override;
    virtual void SetRotation(const float3& rotation) override;

private:
    void UpdateCameraLookAt();

private:
    std::unique_ptr<ppx::Camera> mCamera;
};

// -------------------------------------------------------------------------------------------------

// Scene Graph Light Node
//
// Scene graph node that contains all properties to represent different light
// types.
//
// When used as a standalone node, scene::Camera stores light information as populated
// by a loader.
//
// See scene::Node class declaration for additional usages of scene::Node objects.
//
class LightNode
    : public scene::Node
{
public:
    LightNode(scene::Scene* pScene = nullptr);
    virtual ~LightNode();

    virtual scene::NodeType GetNodeType() const override { return scene::NODE_TYPE_LIGHT; }

    const float3& GetColor() const { return mColor; }
    float         GetIntensity() const { return mIntensity; }
    float         GetDistance() const { return mDistance; }
    const float3& GetDirection() const { return mDirection; }
    float         GetSpotInnerConeAngle() const { return mSpotInnerConeAngle; }
    float         GetSpotOuterConeAngle() const { return mSpotOuterConeAngle; }

    void SetType(const scene::LightType type) { mLightType = type; }
    void SetColor(const float3& color) { mColor = color; }
    void SetIntensity(float intensity) { mIntensity = intensity; }
    void SetDistance(float distance) { mDistance = distance; }
    void SetSpotInnerConeAngle(float angle) { mSpotInnerConeAngle = angle; }
    void SetSpotOuterConeAngle(float angle) { mSpotOuterConeAngle = angle; }

    virtual void SetRotation(const float3& rotation) override;

private:
    scene::LightType mLightType          = scene::LIGHT_TYPE_UNDEFINED;
    float3           mColor              = float3(1, 1, 1);
    float            mIntensity          = 1.0f;
    float            mDistance           = 100.0f;
    float3           mDirection          = float3(0, -1, 0);
    float            mSpotInnerConeAngle = glm::radians(45.0f);
    float            mSpotOuterConeAngle = glm::radians(50.0f);
};

} // namespace scene
} // namespace ppx

#endif // ppx_scene_node_h
