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

#ifndef BENCHMARKS_GRAPHICS_PIPELINE_SPHERE_MESH_H
#define BENCHMARKS_GRAPHICS_PIPELINE_SPHERE_MESH_H

#include "ppx/graphics_util.h"

using namespace ppx;

// =====================================================================
// OrderedGrid
// =====================================================================
//
// Evenly spaced 3D grid of points roughly in the shape of a cube.
// The points are ordered with a deterministic random ordering.
class OrderedGrid
{
public:
    // Construct a grid of `count` # of points and shuffle the order
    OrderedGrid(uint32_t count, uint32_t randomSeed);

    uint32_t GetCount() const { return static_cast<uint32_t>(mOrderedPointIndices.size()); }

    // Get model matrix that can be applied to move a model space object to this point
    float4x4 GetModelMatrix(uint32_t pointIndex, bool isXR) const;

private:
    uint32_t              mSizeX;
    uint32_t              mSizeY;
    uint32_t              mSizeZ;
    float                 mStep;
    std::vector<uint32_t> mOrderedPointIndices;
};

// =====================================================================
// SphereMesh
// =====================================================================
//
// Creates multi-sphere geometries consisting of spheres arranged in a grid.
// There are (PrecisionType * VertexLayoutType = 4) different variants of representations.
//
// Visualization of full buffers internal structure:
// i : sphere index
// j : vertex index within one sphere
// k : triangle index within one sphere
// v0, v1, v2: the three elements of triangle k
//
// Full VERTEX buffers contain a total of (mSphereCount * mSingleSphereVertexCount) vertices arranged like so:
//
// | j(0) | j(1) | ... | j(mSingleSphereVertexCount-1) | ... | j(0) | j(1) | ...  | j(mSingleSphereVertexCount-1) |
// |-----------------------i(0)------------------------| ... |-----------------i(mSphereCount-1)------------------|
//
// Full INDEX buffers contain a total of (mSphereCount * mSingleSphereTriCount * 3) indices arranged like so:
//
// | v0 | v1 | v2 |        ...        | v0 | v1 | v2 | ... | v0 | v1 | v2 |        ...        | v0 | v1 | v2 |
// |     k(0)     | ... | k(mSingleSphereTriCount-1) | ... |     k(0)     | ... | k(mSingleSphereTriCount-1) |
// |----------------------i(0)-----------------------| ... |----------------i(mSphereCount-1)----------------|
class SphereMesh
{
public:
    enum class PrecisionType
    {
        PRECISION_TYPE_LOW_PRECISION,
        PRECISION_TYPE_HIGH_PRECISION
    };

    enum class VertexLayoutType
    {
        VERTEX_LAYOUT_TYPE_INTERLEAVED,
        VERTEX_LAYOUT_TYPE_POSITION_PLANAR
    };

    // Creates a SphereMesh and populates info for one sphere
    SphereMesh(float radius, uint32_t longitudeSegments, uint32_t latitudeSegments, bool isXR)
    {
        mSingleSphereMesh        = TriMesh::CreateSphere(radius, longitudeSegments, latitudeSegments, TriMeshOptions().Indices().VertexColors().Normals().TexCoords().Tangents());
        mSingleSphereVertexCount = mSingleSphereMesh.GetCountPositions();
        mSingleSphereTriCount    = mSingleSphereMesh.GetCountTriangles();
        mIsXR                    = isXR;

        PPX_LOG_INFO("Creating SphereMesh:");
        PPX_LOG_INFO("  Sphere vertex count: " << mSingleSphereVertexCount << " | triangle count: " << mSingleSphereTriCount);
    };

    // Places copies of the spheres on the grid and creates all variants of geometry representations
    void ApplyGrid(const OrderedGrid& grid);

    // Fetch specific geometry variants
    const Geometry* GetLowPrecisionInterleaved() const { return &mLowInterleaved; }
    const Geometry* GetLowPrecisionPositionPlanar() const { return &mLowPlanar; }
    const Geometry* GetHighPrecisionInterleaved() const { return &mHighInterleaved; }
    const Geometry* GetHighPrecisionPositionPlanar() const { return &mHighPlanar; }

    bool IsXR() const { return mIsXR; }

private:
    // Create all single sphere and full geometries
    void CreateAllGeometries();

    // Create sphere geometry based on specified type
    void CreateSphereGeometry(PrecisionType precisionType, VertexLayoutType vertexLayoutType, Geometry* geometryPtr);

    // Populate vertex buffers for single sphere geometries
    void PopulateSingleSpheres();

    // Repeat necessary data from single sphere geometries to the full geometries
    void PrepareFullGeometries();

    // Resize dstGeom's vertex buffers and fill with srcGeom's vertex buffers repeated repeatCount times
    // If the position buffer is separate (position planar), do not touch it.
    void RepeatGeometryNonPositionVertexData(const Geometry& srcGeom, VertexLayoutType vertexLayoutType, uint32_t repeatCount, Geometry& dstGeom);

    // For a sphere in the grid, overwrite the position data for all its vertices within the full vertex buffers
    void WriteSpherePosition(const OrderedGrid& grid, uint32_t sphereIndex);

    // For a sphere, append all its triangles' three vertex indices to only the interleaved full index buffers
    void AppendSphereIndicesToInterleaved(uint32_t sphereIndex);

    // Compress all high precision data within vertexData to a size suitable for low precision
    TriMeshVertexDataCompressed CompressVertexData(const TriMeshVertexData& vertexData);

private:
    TriMesh  mSingleSphereMesh;
    uint32_t mSingleSphereVertexCount;
    uint32_t mSingleSphereTriCount;
    uint32_t mSphereCount;

    Geometry mLowInterleavedSingleSphere;
    Geometry mLowPlanarSingleSphere;
    Geometry mHighInterleavedSingleSphere;
    Geometry mHighPlanarSingleSphere;

    Geometry mLowInterleaved;
    Geometry mLowPlanar;
    Geometry mHighInterleaved;
    Geometry mHighPlanar;

    bool mIsXR;
};

// Overwrite the position data within a position buffer with vtx.position, at vertex elementIndex only
template <class T>
void OverwritePositionData(Geometry::Buffer* positionBufferPtr, const T& vtx, size_t elementIndex)
{
    size_t elementSize = positionBufferPtr->GetElementSize();
    size_t offset      = elementSize * elementIndex;

    const void* pSrc = &(vtx.position);
    void*       pDst = positionBufferPtr->GetData() + offset;
    memcpy(pDst, pSrc, sizeof(vtx.position));
}

// Maps a float between [-1, 1] to [-128, 127]
int8_t MapFloatToInt8(float x);

// Shuffles [`begin`, `end`) using function `f`.
template <class Iter, class F>
void Shuffle(Iter begin, Iter end, F&& f)
{
    size_t count = end - begin;
    for (size_t i = 0; i < count; i++) {
        std::swap(begin[i], begin[f() % (count - i) + i]);
    }
}

#endif // BENCHMARKS_GRAPHICS_PIPELINE_SPHERE_MESH_H