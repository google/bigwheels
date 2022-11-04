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

#ifndef ppx_wire_mesh_h
#define ppx_wire_mesh_h

#include "ppx/config.h"
#include "ppx/math_config.h"
#include "ppx/grfx/grfx_config.h"

namespace ppx {

//! @enum WireMeshPlane
//!
//!
enum WireMeshPlane
{
    WIRE_MESH_PLANE_POSITIVE_X = 0,
    WIRE_MESH_PLANE_NEGATIVE_X = 1,
    WIRE_MESH_PLANE_POSITIVE_Y = 2,
    WIRE_MESH_PLANE_NEGATIVE_Y = 3,
    WIRE_MESH_PLANE_POSITIVE_Z = 4,
    WIRE_MESH_PLANE_NEGATIVE_Z = 5,
};

//! @struct WireMeshVertexData
//!
//!
struct WireMeshVertexData
{
    float3 position;
    float3 color;
};

//! @class WireMeshOptions
//!
//!
class WireMeshOptions
{
public:
    WireMeshOptions() {}
    ~WireMeshOptions() {}
    // clang-format off
    //! Enable/disable indices
    WireMeshOptions& Indices(bool value = true) { mEnableIndices = value; return *this; }
    //! Enable/disable vertex colors
    WireMeshOptions& VertexColors(bool value = true) { mEnableVertexColors = value; return *this; }
    //! Set and/or enable/disable object color, object color will override vertex colors
    WireMeshOptions& ObjectColor(const float3& color, bool enable = true) { mObjectColor = color; mEnableObjectColor = enable; return *this;}
    //! Set the scale of geometry position, default is (1, 1, 1)
    WireMeshOptions& Scale(const float3& scale) { mScale = scale; return *this; }
    // clang-format on
private:
    bool   mEnableIndices      = false;
    bool   mEnableVertexColors = false;
    bool   mEnableObjectColor  = false;
    float3 mObjectColor        = float3(0.7f);
    float3 mScale              = float3(1, 1, 1);
    friend class WireMesh;
};

//! @class WireMesh
//!
//!
class WireMesh
{
public:
    WireMesh();
    WireMesh(grfx::IndexType indexType);
    ~WireMesh();

    grfx::IndexType GetIndexType() const { return mIndexType; }

    bool HasColors() const { return GetCountColors() > 0; }

    uint32_t GetCountEdges() const;
    uint32_t GetCountIndices() const;
    uint32_t GetCountPositions() const;
    uint32_t GetCountColors() const;

    uint64_t GetDataSizeIndices() const;
    uint64_t GetDataSizePositions() const;
    uint64_t GetDataSizeColors() const;

    const uint16_t* GetDataIndicesU16(uint32_t index = 0) const;
    const uint32_t* GetDataIndicesU32(uint32_t index = 0) const;
    const float3*   GetDataPositions(uint32_t index = 0) const;
    const float3*   GetDataColors(uint32_t index = 0) const;

    const float3& GetBoundingBoxMin() const { return mBoundingBoxMin; }
    const float3& GetBoundingBoxMax() const { return mBoundingBoxMax; }

    uint32_t AppendEdge(uint32_t v0, uint32_t v1);
    uint32_t AppendPosition(const float3& value);
    uint32_t AppendColor(const float3& value);

    Result GetEdge(uint32_t lineIndex, uint32_t& v0, uint32_t& v1) const;
    Result GetVertexData(uint32_t vtxIndex, WireMeshVertexData* pVertexData) const;

    static WireMesh CreatePlane(WireMeshPlane plane, const float2& size, uint32_t usegs, uint32_t vsegs, const WireMeshOptions& options = WireMeshOptions());
    static WireMesh CreateCube(const float3& size, const WireMeshOptions& options = WireMeshOptions());
    static WireMesh CreateSphere(float radius, uint32_t usegs, uint32_t vsegs, const WireMeshOptions& options = WireMeshOptions());

private:
    void AppendIndexU16(uint16_t value);
    void AppendIndexU32(uint32_t value);

    static void AppendIndexAndVertexData(
        std::vector<uint32_t>&    indexData,
        const std::vector<float>& vertexData,
        const uint32_t            expectedVertexCount,
        const WireMeshOptions&    options,
        WireMesh&                 mesh);

private:
    grfx::IndexType      mIndexType = grfx::INDEX_TYPE_UNDEFINED;
    std::vector<uint8_t> mIndices;        // Stores both 16 and 32 bit indices
    std::vector<float3>  mPositions;      // Vertex positions
    std::vector<float3>  mColors;         // Vertex colors
    float3               mBoundingBoxMin; // Bounding box min
    float3               mBoundingBoxMax; // Bounding box max
};

} // namespace ppx

#endif // ppx_wire_mesh_h
