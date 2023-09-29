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

#ifndef ppx_scene_graph_h
#define ppx_scene_graph_h

#include "ppx/scene/scene_config.h"
#include "ppx/scene/scene_material.h"
#include "ppx/scene/scene_mesh.h"
#include "ppx/scene/scene_node.h"
#include "ppx/scene/scene_resource_manager.h"

namespace ppx {
namespace scene {

// Scene Graph
//
// Basic scene graph designed to feed into a renderer. Manages nodes and all
// their required objects: meshes, mesh geometry data, materials, textures,
// images, and samplers.
//
// See scene::Resource manager for details on object sharing among the various
// elements of the scene.
//
class Scene
    : public grfx::NamedObjectTrait
{
public:
    Scene();
    virtual ~Scene();

    scene::ResourceManager*       GetResourceManager() { return &mResourceManager; }
    const scene::ResourceManager* GetResourceManager() const { return &mResourceManager; }

    // Returns the number of all the nodes in the scene
    uint32_t GetNodeCount() const { return CountU32(mNodes); }
    // Returns the number of mesh nodes in the scene
    uint32_t GetMeshNodeCount() const { return CountU32(mMeshNodes); }
    // Returns the number of camera nodes in the scene
    uint32_t GetCameraNodeCount() const { return CountU32(mCameraNodes); }
    // Returns the number of light nodes in the scene
    uint32_t GetLightNodeCount() const { return CountU32(mLightNodes); }

    // Returns the node at index or NULL if idx is out of range
    scene::Node* GetNode(uint32_t index) const;
    // Returns the mesh node at index or NULL if idx is out of range
    scene::MeshNode* GetMeshNode(uint32_t index) const;
    // Returns the camera node at index or NULL if idx is out of range
    scene::CameraNode* GetCameraNode(uint32_t index) const;
    // Returns the light node at index or NULL if idx is out of range
    scene::LightNode* GetLightNode(uint32_t index) const;

    // ---------------------------------------------------------------------------------------------
    // Find*Node functions return the first node that matches the name argument.
    // Since it's possible for source data to have multiple nodes of the same
    // name, there can't be any type of ordering or search consistency guarantees.
    //
    // Best to avoid using the same name for multiple nodes in source data.
    //
    // ---------------------------------------------------------------------------------------------
    // Returns a node that matches name
    scene::Node* FindNode(const std::string& name) const;
    // Returns a mesh node that matches name
    scene::MeshNode* FindMeshNode(const std::string& name) const;
    // Returns a camera node that matches name
    scene::CameraNode* FindCameraNode(const std::string& name) const;
    // Returns a light node that matches name
    scene::LightNode* FindLightNode(const std::string& name) const;

    ppx::Result AddNode(const scene::NodeRef& node);

private:
    template <typename NodeT>
    NodeT* FindNodeByName(const std::string& name, const std::vector<NodeT*>& container) const
    {
        auto it = ppx::FindIf(
            container,
            [name](const NodeT* pElem) {
                bool match = (pElem->GetName() == name);
                return match; });
        if (it == container.end()) {
            return nullptr;
        }

        NodeT* pNode = (*it);
        return pNode;
    }

private:
    scene::ResourceManager          mResourceManager = {};
    std::vector<scene::NodeRef>     mNodes           = {};
    std::vector<scene::MeshNode*>   mMeshNodes       = {};
    std::vector<scene::CameraNode*> mCameraNodes     = {};
    std::vector<scene::LightNode*>  mLightNodes      = {};
};

} // namespace scene
} // namespace ppx

#endif // ppx_scene_graph_h
