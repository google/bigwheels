// Copyright 2024 Google LLC
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

#ifndef ppx_scene_loader_h
#define ppx_scene_loader_h

#include "ppx/scene/scene_config.h"
#include "ppx/scene/scene_material.h"
#include "ppx/scene/scene_mesh.h"
#include "ppx/scene/scene_node.h"
#include "ppx/scene/scene_resource_manager.h"
#include "ppx/scene/scene_scene.h"

namespace ppx {
namespace scene {

// Load Options
//
// Stores optional paramters that are passed to scene loader implementations.
//
class LoadOptions
{
public:
    LoadOptions() {}
    ~LoadOptions() {}

    // Returns current material factory or NULL if one has not been set.
    scene::MaterialFactory* GetMaterialFactory() const { return mMaterialFactory; }

    // Sets material factory used to create materials during loading.
    LoadOptions& SetMaterialFactory(scene::MaterialFactory* pMaterialFactory)
    {
        mMaterialFactory = pMaterialFactory;
        return *this;
    }

private:
    // Pointer to custom material factory for loader to use.
    scene::MaterialFactory* mMaterialFactory = nullptr;
};

} // namespace scene
} // namespace ppx

#endif // ppx_scene_loader_h
