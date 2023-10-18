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

#ifndef BENCHMARKS_GRAPHICS_PIPELINE_HELPER_H
#define BENCHMARKS_GRAPHICS_PIPELINE_HELPER_H

#include "ppx/camera.h"
#include "ppx/ppx.h"

using namespace ppx;

// ----------------------------------------------------------------------------
// Structs
// ----------------------------------------------------------------------------

struct SkyBoxData
{
    float4x4 MVP;
};

struct SphereData
{
    float4x4 modelMatrix;                // Transforms object space to world space.
    float4x4 ITModelMatrix;              // Inverse transpose of the ModelMatrix.
    float4   ambient;                    // Object's ambient intensity.
    float4x4 cameraViewProjectionMatrix; // Camera's view projection matrix.
    float4   lightPosition;              // Light's position.
    float4   eyePosition;                // Eye (camera) position.
};

struct PerFrame
{
    grfx::CommandBufferPtr cmd;
    grfx::SemaphorePtr     imageAcquiredSemaphore;
    grfx::FencePtr         imageAcquiredFence;
    grfx::SemaphorePtr     renderCompleteSemaphore;
    grfx::FencePtr         renderCompleteFence;
    grfx::QueryPtr         timestampQuery;
};

struct Texture
{
    grfx::ImagePtr            image;
    grfx::SampledImageViewPtr sampledImageView;
    grfx::SamplerPtr          sampler;
};

struct Entity
{
    grfx::MeshPtr                mesh;
    grfx::BufferPtr              uniformBuffer;
    grfx::DescriptorSetLayoutPtr descriptorSetLayout;
    grfx::PipelineInterfacePtr   pipelineInterface;
    grfx::GraphicsPipelinePtr    pipeline;
};

struct Entity2D
{
    grfx::BufferPtr            vertexBuffer;
    grfx::VertexBinding        vertexBinding;
    grfx::PipelineInterfacePtr pipelineInterface;
    grfx::GraphicsPipelinePtr  pipeline;
};

struct Grid
{
    uint32_t xSize;
    uint32_t ySize;
    uint32_t zSize;
    float    step;
};

struct LOD
{
    uint32_t    longitudeSegments;
    uint32_t    latitudeSegments;
    std::string name;
};

// ----------------------------------------------------------------------------
// Camera
// ----------------------------------------------------------------------------
static constexpr float kCameraSpeed = 0.2f;

class FreeCamera
    : public ppx::PerspCamera
{
public:
    enum class MovementDirection
    {
        FORWARD,
        LEFT,
        RIGHT,
        BACKWARD
    };

    // Initializes a FreeCamera located at `eyePosition` and looking at the
    // spherical coordinates in world space defined by `theta` and `phi`.
    // `mTheta` (longitude) is an angle in the range [0, 2pi].
    // `mPhi` (latitude) is an angle in the range [0, pi].
    FreeCamera(float3 eyePosition, float theta, float phi);

    // Moves the location of the camera in dir direction for distance units.
    void Move(MovementDirection dir, float distance);

    // Changes the location where the camera is looking at by turning `deltaTheta`
    // (longitude) radians and looking up `deltaPhi` (latitude) radians.
    void Turn(float deltaTheta, float deltaPhi);

private:
    // Spherical coordinates in world space where the camera is looking at.
    // `mTheta` (longitude) is an angle in the range [0, 2pi].
    // `mPhi` (latitude) is an angle in the range [0, pi].
    float mTheta;
    float mPhi;
};

// ----------------------------------------------------------------------------
// MultiDimensional Indexer
// ----------------------------------------------------------------------------

class MultiDimensionalIndexer
{
public:
    // Adds a new dimension with the given `size`.
    void AddDimension(size_t size);

    // Gets the index for the given dimension `indices`.
    size_t GetIndex(const std::vector<size_t>& indices);

private:
    // `mSizes` are the sizes for each dimension.
    std::vector<size_t> mSizes;
    // `mMultipliers` are the multipliers for each dimension to get the index.
    std::vector<size_t> mMultipliers;
};

// ----------------------------------------------------------------------------
// Geometry vertex buffer helper functions
// ----------------------------------------------------------------------------

// Populate dstGeom's vertex buffers with repeatCount number of srcGeom's vertex buffers
// For position planar, modify the other vertex buffers but do not touch the position buffer.
void RepeatGeometryNonPositionVertexData(const Geometry& srcGeom, size_t repeatCount, Geometry& dstGeom);

// Overwrite the position data of the vertex corresponding to elementIndex with the position data from vtx
template <class T>
void OverwritePositionData(Geometry::Buffer* positionBufferPtr, const T& vtx, size_t elementIndex)
{
    size_t elementSize = positionBufferPtr->GetElementSize();
    size_t offset      = elementSize * elementIndex;

    const void* pSrc = &(vtx.position);
    void*       pDst = positionBufferPtr->GetData() + offset;
    memcpy(pDst, pSrc, sizeof(vtx.position));
}

// ----------------------------------------------------------------------------
// General helper functions
// ----------------------------------------------------------------------------

// Shuffles [`begin`, `end`) using function `f`.
template <class Iter, class F>
void Shuffle(Iter begin, Iter end, F&& f)
{
    size_t count = end - begin;
    for (size_t i = 0; i < count; i++) {
        std::swap(begin[i], begin[f() % (count - i) + i]);
    }
}

// Maps a float between [-1, 1] to [-128, 127]
int8_t MapFloatToInt8(float x);

#endif // BENCHMARKS_GRAPHICS_PIPELINE_HELPER_H
