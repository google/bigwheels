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

#include "ppx/grfx/grfx_scope.h"
#include "ppx/grfx/grfx_device.h"
#include "ppx/grfx/grfx_queue.h"

#define NULL_ARGUMENT_MSG   "unexpected null argument"
#define WRONG_OWNERSHIP_MSG "object has invalid ownership value"

namespace ppx {
namespace grfx {

ScopeDestroyer::ScopeDestroyer(grfx::Device* pDevice)
    : mDevice(pDevice)
{
    PPX_ASSERT_MSG(!IsNull(pDevice), NULL_ARGUMENT_MSG);
}

ScopeDestroyer::~ScopeDestroyer()
{
    for (auto& object : mImages) {
        if (object->GetOwnership() == grfx::OWNERSHIP_EXCLUSIVE) {
            mDevice->DestroyImage(object);
        }
    }
    mImages.clear();

    for (auto& object : mBuffers) {
        if (object->GetOwnership() == grfx::OWNERSHIP_EXCLUSIVE) {
            mDevice->DestroyBuffer(object);
        }
    }
    mBuffers.clear();

    for (auto& object : mMeshes) {
        if (object->GetOwnership() == grfx::OWNERSHIP_EXCLUSIVE) {
            mDevice->DestroyMesh(object);
        }
    }
    mMeshes.clear();

    for (auto& object : mTextures) {
        if (object->GetOwnership() == grfx::OWNERSHIP_EXCLUSIVE) {
            mDevice->DestroyTexture(object);
        }
    }
    mTextures.clear();

    for (auto& object : mTransientCommandBuffers) {
        if (object.second->GetOwnership() == grfx::OWNERSHIP_EXCLUSIVE) {
            object.first->DestroyCommandBuffer(object.second);
        }
    }
}

Result ScopeDestroyer::AddObject(grfx::Image* pObject)
{
    if (IsNull(pObject)) {
        PPX_ASSERT_MSG(false, NULL_ARGUMENT_MSG);
        return ppx::ERROR_UNEXPECTED_NULL_ARGUMENT;
    }
    if (pObject->GetOwnership() != grfx::OWNERSHIP_REFERENCE) {
        PPX_ASSERT_MSG(false, WRONG_OWNERSHIP_MSG);
        return ppx::ERROR_GRFX_INVALID_OWNERSHIP;
    }
    pObject->SetOwnership(grfx::OWNERSHIP_EXCLUSIVE);
    mImages.push_back(pObject);
    return ppx::SUCCESS;
}

Result ScopeDestroyer::AddObject(grfx::Buffer* pObject)
{
    if (IsNull(pObject)) {
        PPX_ASSERT_MSG(false, NULL_ARGUMENT_MSG);
        return ppx::ERROR_UNEXPECTED_NULL_ARGUMENT;
    }
    if (pObject->GetOwnership() != grfx::OWNERSHIP_REFERENCE) {
        PPX_ASSERT_MSG(false, WRONG_OWNERSHIP_MSG);
        return ppx::ERROR_GRFX_INVALID_OWNERSHIP;
    }
    pObject->SetOwnership(grfx::OWNERSHIP_EXCLUSIVE);
    mBuffers.push_back(pObject);
    return ppx::SUCCESS;
}

Result ScopeDestroyer::AddObject(grfx::Mesh* pObject)
{
    if (IsNull(pObject)) {
        PPX_ASSERT_MSG(false, NULL_ARGUMENT_MSG);
        return ppx::ERROR_UNEXPECTED_NULL_ARGUMENT;
    }
    if (pObject->GetOwnership() != grfx::OWNERSHIP_REFERENCE) {
        PPX_ASSERT_MSG(false, WRONG_OWNERSHIP_MSG);
        return ppx::ERROR_GRFX_INVALID_OWNERSHIP;
    }
    pObject->SetOwnership(grfx::OWNERSHIP_EXCLUSIVE);
    mMeshes.push_back(pObject);
    return ppx::SUCCESS;
}

Result ScopeDestroyer::AddObject(grfx::Texture* pObject)
{
    if (IsNull(pObject)) {
        PPX_ASSERT_MSG(false, NULL_ARGUMENT_MSG);
        return ppx::ERROR_UNEXPECTED_NULL_ARGUMENT;
    }
    if (pObject->GetOwnership() != grfx::OWNERSHIP_REFERENCE) {
        PPX_ASSERT_MSG(false, WRONG_OWNERSHIP_MSG);
        return ppx::ERROR_GRFX_INVALID_OWNERSHIP;
    }
    pObject->SetOwnership(grfx::OWNERSHIP_EXCLUSIVE);
    mTextures.push_back(pObject);
    return ppx::SUCCESS;
}

Result ScopeDestroyer::AddObject(grfx::Queue* pParent, grfx::CommandBuffer* pObject)
{
    if (IsNull(pParent) || IsNull(pObject)) {
        PPX_ASSERT_MSG(false, NULL_ARGUMENT_MSG);
        return ppx::ERROR_UNEXPECTED_NULL_ARGUMENT;
    }
    if (pObject->GetOwnership() != grfx::OWNERSHIP_REFERENCE) {
        PPX_ASSERT_MSG(false, WRONG_OWNERSHIP_MSG);
        return ppx::ERROR_GRFX_INVALID_OWNERSHIP;
    }
    pObject->SetOwnership(grfx::OWNERSHIP_EXCLUSIVE);
    mTransientCommandBuffers.push_back(std::make_pair(pParent, pObject));
    return ppx::SUCCESS;
}

} // namespace grfx
} // namespace ppx
