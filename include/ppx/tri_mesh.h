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

#ifndef ppx_tri_mesh_h
#define ppx_tri_mesh_h

#include "ppx/config.h"
#include "ppx/math_config.h"
#include "ppx/grfx/grfx_config.h"

#include <filesystem>

namespace ppx {

//! @enum TriMeshAttributeDim
//!
//!
enum TriMeshAttributeDim
{
    TRI_MESH_ATTRIBUTE_DIM_UNDEFINED = 0,
    TRI_MESH_ATTRIBUTE_DIM_2         = 2,
    TRI_MESH_ATTRIBUTE_DIM_3         = 3,
    TRI_MESH_ATTRIBUTE_DIM_4         = 4,
};

//! @enum TriMeshPlane
//!
//!
enum TriMeshPlane
{
    TRI_MESH_PLANE_POSITIVE_X = 0,
    TRI_MESH_PLANE_NEGATIVE_X = 1,
    TRI_MESH_PLANE_POSITIVE_Y = 2,
    TRI_MESH_PLANE_NEGATIVE_Y = 3,
    TRI_MESH_PLANE_POSITIVE_Z = 4,
    TRI_MESH_PLANE_NEGATIVE_Z = 5,
};

//! @struct TriMeshVertexData
//!
//!
struct TriMeshVertexData
{
    float3 position;
    float3 color;
    float3 normal;
    float2 texCoord;
    float4 tangent;
    float3 bitangent;
};

//! @class TriMeshOptions
//!
//!
class TriMeshOptions
{
public:
    TriMeshOptions() {}
    ~TriMeshOptions() {}
    // clang-format off
    //! Enable/disable indices
    TriMeshOptions& Indices(bool value = true) { mEnableIndices = value; return *this; }
    //! Enable/disable vertex colors
    TriMeshOptions& VertexColors(bool value = true) { mEnableVertexColors = value; return *this; }
    //! Enable/disable normals
    TriMeshOptions& Normals(bool value = true) { mEnableNormals = value; return *this; }
    //! Enable/disable texture coordinates, most geometry will have 2-dimensional texture coordinates
    TriMeshOptions& TexCoords(bool value = true) { mEnableTexCoords = value; return *this; }
    //! Enable/disable tangent and bitangent creation flag
    TriMeshOptions& Tangents(bool value = true) { mEnableTangents = value; return *this; }
    //! Set and/or enable/disable object color, object color will override vertex colors
    TriMeshOptions& ObjectColor(const float3& color, bool enable = true) { mObjectColor = color; mEnableObjectColor = enable; return *this;}
    //! Set the translate of geometry position, default is (0, 0, 0)
    TriMeshOptions& Translate(const float3& translate) { mTranslate = translate; return *this; }
    //! Set the scale of geometry position, default is (1, 1, 1)
    TriMeshOptions& Scale(const float3& scale) { mScale = scale; return *this; }
    //! Sets the UV texture coordinate scale, default is (1, 1)
    TriMeshOptions& TexCoordScale(const float2& scale) { mTexCoordScale = scale; return *this; }
    //! Enable all attributes
    TriMeshOptions& AllAttributes() { mEnableVertexColors = true; mEnableNormals = true; mEnableTexCoords = true; mEnableTangents = true; return *this; }
    //! Inverts tex coords vertically
    TriMeshOptions& InvertTexCoordsV() { mInvertTexCoordsV = true; return *this; }
    //! Inverts winding order of ONLY indices
    TriMeshOptions& InvertWinding() { mInvertWinding = true; return *this; }
    // clang-format on
private:
    bool   mEnableIndices      = false;
    bool   mEnableVertexColors = false;
    bool   mEnableNormals      = false;
    bool   mEnableTexCoords    = false;
    bool   mEnableTangents     = false;
    bool   mEnableObjectColor  = false;
    bool   mInvertTexCoordsV   = false;
    bool   mInvertWinding      = false;
    float3 mObjectColor        = float3(0.7f);
    float3 mTranslate          = float3(0, 0, 0);
    float3 mScale              = float3(1, 1, 1);
    float2 mTexCoordScale      = float2(1, 1);
    friend class TriMesh;
};

//! @class TriMesh
//!
//!
class TriMesh
{
public:
    TriMesh();
    TriMesh(grfx::IndexType indexType);
    TriMesh(TriMeshAttributeDim texCoordDim);
    TriMesh(grfx::IndexType indexType, TriMeshAttributeDim texCoordDim);
    ~TriMesh();

    grfx::IndexType     GetIndexType() const { return mIndexType; }
    TriMeshAttributeDim GetTexCoordDim() const { return mTexCoordDim; }

    bool HasColors() const { return GetCountColors() > 0; }
    bool HasNormals() const { return GetCountNormals() > 0; }
    bool HasTexCoords() const { return GetCountTexCoords() > 0; }
    bool HasTangents() const { return GetCountTangents() > 0; }
    bool HasBitangents() const { return GetCountBitangents() > 0; }

    uint32_t GetCountTriangles() const;
    uint32_t GetCountIndices() const;
    uint32_t GetCountPositions() const;
    uint32_t GetCountColors() const;
    uint32_t GetCountNormals() const;
    uint32_t GetCountTexCoords() const;
    uint32_t GetCountTangents() const;
    uint32_t GetCountBitangents() const;

    uint64_t GetDataSizeIndices() const;
    uint64_t GetDataSizePositions() const;
    uint64_t GetDataSizeColors() const;
    uint64_t GetDataSizeNormalls() const;
    uint64_t GetDataSizeTexCoords() const;
    uint64_t GetDataSizeTangents() const;
    uint64_t GetDataSizeBitangents() const;

    const uint16_t* GetDataIndicesU16(uint32_t index = 0) const;
    const uint32_t* GetDataIndicesU32(uint32_t index = 0) const;
    const float3*   GetDataPositions(uint32_t index = 0) const;
    const float3*   GetDataColors(uint32_t index = 0) const;
    const float3*   GetDataNormalls(uint32_t index = 0) const;
    const float2*   GetDataTexCoords2(uint32_t index = 0) const;
    const float3*   GetDataTexCoords3(uint32_t index = 0) const;
    const float4*   GetDataTexCoords4(uint32_t index = 0) const;
    const float4*   GetDataTangents(uint32_t index = 0) const;
    const float3*   GetDataBitangents(uint32_t index = 0) const;

    const float3& GetBoundingBoxMin() const { return mBoundingBoxMin; }
    const float3& GetBoundingBoxMax() const { return mBoundingBoxMax; }

    uint32_t AppendTriangle(uint32_t v0, uint32_t v1, uint32_t v2);
    uint32_t AppendPosition(const float3& value);
    uint32_t AppendColor(const float3& value);
    uint32_t AppendTexCoord(const float2& value);
    uint32_t AppendTexCoord(const float3& value);
    uint32_t AppendTexCoord(const float4& value);
    uint32_t AppendNormal(const float3& value);
    uint32_t AppendTangent(const float4& value);
    uint32_t AppendBitangent(const float3& value);

    Result GetTriangle(uint32_t triIndex, uint32_t& v0, uint32_t& v1, uint32_t& v2) const;
    Result GetVertexData(uint32_t vtxIndex, TriMeshVertexData* pVertexData) const;

    static TriMesh CreatePlane(TriMeshPlane plane, const float2& size, uint32_t usegs, uint32_t vsegs, const TriMeshOptions& options = TriMeshOptions());
    static TriMesh CreateCube(const float3& size, const TriMeshOptions& options = TriMeshOptions());
    static TriMesh CreateSphere(float radius, uint32_t usegs, uint32_t vsegs, const TriMeshOptions& options = TriMeshOptions());

    static Result  CreateFromOBJ(const std::filesystem::path& path, const TriMeshOptions& options, TriMesh* pTriMesh);
    static TriMesh CreateFromOBJ(const std::filesystem::path& path, const TriMeshOptions& options = TriMeshOptions());

private:
    void AppendIndexU16(uint16_t value);
    void AppendIndexU32(uint32_t value);

    static void AppendIndexAndVertexData(
        std::vector<uint32_t>&    indexData,
        const std::vector<float>& vertexData,
        const uint32_t            expectedVertexCount,
        const TriMeshOptions&     options,
        TriMesh&                  mesh);

private:
    grfx::IndexType      mIndexType   = grfx::INDEX_TYPE_UNDEFINED;
    TriMeshAttributeDim  mTexCoordDim = TRI_MESH_ATTRIBUTE_DIM_UNDEFINED;
    std::vector<uint8_t> mIndices;        // Stores both 16 and 32 bit indices
    std::vector<float3>  mPositions;      // Vertex positions
    std::vector<float3>  mColors;         // Vertex colors
    std::vector<float3>  mNormals;        // Vertex normals
    std::vector<float>   mTexCoords;      // Vertex texcoords, dimension can be 2, 3, or 4
    std::vector<float4>  mTangents;       // Vertex tangents
    std::vector<float3>  mBitangents;     // Vertex bitangents
    float3               mBoundingBoxMin; // Bounding box min
    float3               mBoundingBoxMax; // Bounding box max
};

} // namespace ppx

#endif // ppx_tri_mesh_h
