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

#include "helper.h"

#include "ppx/math_util.h"

using namespace ppx;

// ----------------------------------------------------------------------------
// Camera
// ----------------------------------------------------------------------------
FreeCamera::FreeCamera(float3 eyePosition, float theta, float phi)
{
    mEyePosition = eyePosition;
    mTheta       = theta;
    mPhi         = phi;
    mTarget      = eyePosition + SphericalToCartesian(theta, phi);
}

void FreeCamera::Move(MovementDirection dir, float distance)
{
    // Given that v = (1, mTheta, mPhi) is where the camera is looking at in the Spherical
    // coordinates and moving forward goes in this direction, we have to update the
    // camera location for each movement as follows:
    //      FORWARD:     distance * unitVectorOf(v)
    //      BACKWARD:    -distance * unitVectorOf(v)
    //      RIGHT:       distance * unitVectorOf(1, mTheta + pi/2, pi/2)
    //      LEFT:        -distance * unitVectorOf(1, mTheta + pi/2, pi/2)
    switch (dir) {
        case MovementDirection::FORWARD: {
            float3 unitVector = glm::normalize(SphericalToCartesian(mTheta, mPhi));
            mEyePosition += distance * unitVector;
            break;
        }
        case MovementDirection::LEFT: {
            float3 perpendicularUnitVector = glm::normalize(SphericalToCartesian(mTheta + pi<float>() / 2.0f, pi<float>() / 2.0f));
            mEyePosition -= distance * perpendicularUnitVector;
            break;
        }
        case MovementDirection::RIGHT: {
            float3 perpendicularUnitVector = glm::normalize(SphericalToCartesian(mTheta + pi<float>() / 2.0f, pi<float>() / 2.0f));
            mEyePosition += distance * perpendicularUnitVector;
            break;
        }
        case MovementDirection::BACKWARD: {
            float3 unitVector = glm::normalize(SphericalToCartesian(mTheta, mPhi));
            mEyePosition -= distance * unitVector;
            break;
        }
    }
    mTarget = mEyePosition + SphericalToCartesian(mTheta, mPhi);
    LookAt(mEyePosition, mTarget);
}

void FreeCamera::Turn(float deltaTheta, float deltaPhi)
{
    mTheta += deltaTheta;
    mPhi += deltaPhi;

    // Saturate mTheta values by making wrap around.
    if (mTheta < 0) {
        mTheta = 2 * pi<float>();
    }
    else if (mTheta > 2 * pi<float>()) {
        mTheta = 0;
    }

    // mPhi is saturated by making it stop, so the world doesn't turn upside down.
    mPhi = std::clamp(mPhi, 0.1f, pi<float>() - 0.1f);

    mTarget = mEyePosition + SphericalToCartesian(mTheta, mPhi);
    LookAt(mEyePosition, mTarget);
}

// ----------------------------------------------------------------------------
// MultiDimensional Indexer
// ----------------------------------------------------------------------------

void MultiDimensionalIndexer::AddDimension(size_t size)
{
    for (size_t i = 0; i < mMultipliers.size(); i++) {
        mMultipliers[i] *= size;
    }
    mSizes.push_back(size);
    mMultipliers.push_back(1);
}

size_t MultiDimensionalIndexer::GetIndex(const std::vector<size_t>& indices)
{
    PPX_ASSERT_MSG(indices.size() == mSizes.size(), "The number of indices must be the same as the number of dimensions");
    size_t index = 0;
    for (size_t i = 0; i < indices.size(); i++) {
        PPX_ASSERT_MSG(indices[i] < mSizes[i], "Index out of range");
        index += indices[i] * mMultipliers[i];
    }
    return index;
}

// ----------------------------------------------------------------------------
// Geometry vertex buffer helper functions
// ----------------------------------------------------------------------------

void RepeatGeometryNonPositionVertexData(const Geometry& srcGeom, size_t repeatCount, Geometry& dstGeom)
{
    size_t nVertexBufferCount = srcGeom.GetVertexBufferCount();
    PPX_ASSERT_MSG(nVertexBufferCount == dstGeom.GetVertexBufferCount(), "Mismatched source and destination vertex data format");
    PPX_ASSERT_MSG(nVertexBufferCount > 0, "Geometry cannot have 0 vertex buffers");

    // If there is one interleaved (1 vb), repeat position data as well
    // For position planar (2 vb), repeat only non-position vertex data, starting from buffer 1
    size_t firstBufferToCopy = (nVertexBufferCount == 1) ? 0 : 1;

    for (size_t vertexBufferIndex = firstBufferToCopy; vertexBufferIndex < nVertexBufferCount; vertexBufferIndex++) {
        const Geometry::Buffer* srcBufferPtr  = srcGeom.GetVertexBuffer(vertexBufferIndex);
        Geometry::Buffer*       dstBufferPtr  = dstGeom.GetVertexBuffer(vertexBufferIndex);
        size_t                  srcBufferSize = srcBufferPtr->GetSize();
        size_t                  dstBufferSize = srcBufferSize * repeatCount;

        dstBufferPtr->SetSize(dstBufferSize);

        size_t offset = 0;
        for (size_t j = 0; j < repeatCount; j++) {
            const void* pSrc = srcBufferPtr->GetData();
            void*       pDst = dstBufferPtr->GetData() + offset;
            memcpy(pDst, pSrc, srcBufferSize);
            offset += srcBufferSize;
        }
    }
}

// ----------------------------------------------------------------------------
// General helper functions
// ----------------------------------------------------------------------------

// Maps a float between [-1, 1] to [-128, 127]
int8_t MapFloatToInt8(float x)
{
    PPX_ASSERT_MSG(-1.0f <= x && x <= 1.0f, "The value must be between -1.0 and 1.0");
    return static_cast<int8_t>((x + 1.0f) * 127.5f - 128.0f);
}
