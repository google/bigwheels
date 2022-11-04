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

#ifndef ppx_grfx_ray_tracing_h
#define ppx_grfx_ray_tracing_h

#if defined(PPX_ENABLE_RAYTRACING)

#include "ppx/grfx/grfx_config.h"
#include "ppx/grfx/grfx_pipeline.h"

namespace ppx {
namespace grfx {

struct BufferOrHostAddress
{
    grfx::Buffer* pBuffer      = nullptr;
    void*         pHostAddress = nullptr;
};

struct BufferOrHostAddressConst
{
    const grfx::Buffer* pBuffer      = nullptr;
    const void*         pHostAddress = nullptr;
};

struct AccelerationStructureGeometryTrianglesData
{
    grfx::Format                   vertexFormat  = grfx::FORMAT_UNDEFINED;
    grfx::BufferOrHostAddressConst vertexData    = {};
    uint32_t                       vertexStride  = 0;
    uint32_t                       maxVertex     = 0;
    grfx::IndexType                indexType     = grfx::INDEX_TYPE_UNDEFINED;
    grfx::BufferOrHostAddressConst indexData     = {};
    grfx::BufferOrHostAddressConst transformData = {};
};

struct AccelerationStructureGeometryAabbsData
{
    grfx::BufferOrHostAddressConst data   = {};
    uint32_t                       stride = 0;
};

struct AccelerationStructureGeometryInstancesData
{
    grfx::BufferOrHostAddressConst data = {};
};

struct AccelerationStructureGeometryData
{
    AccelerationStructureGeometryTrianglesData triangles = {};
    AccelerationStructureGeometryAabbsData     aabbs     = {};
    AccelerationStructureGeometryInstancesData instances = {};
};

struct AccelerationStructureGeometry
{
    grfx::RayTracingGeometryType            type  = grfx::RAY_TRACING_GEOMETRY_TYPE_TRIANGLES;
    grfx::RayTracingGeometryFlags           flags = 0;
    grfx::AccelerationStructureGeometryData data  = {};
};

struct AccelerationStructureBuildInfo
{
    grfx::BuildAccelerationStructureFlags       flags                     = 0;
    grfx::BuildAccelerationStructureMode        mode                      = grfx::BUILD_ACCELERATION_STRUCTURE_MODE_BUILD;
    grfx::AccelerationStructure*                pSrcAccelerationStructure = nullptr;
    grfx::AccelerationStructure*                pDstAccelerationStructure = nullptr;
    grfx::BufferOrHostAddress                   scratchData               = {};
    uint32_t                                    geometryCount             = 0;
    const AccelerationStructureGeometry*        pGeometries               = nullptr;
    const AccelerationStructureGeometry* const* ppGeometries              = nullptr;
};

// -------------------------------------------------------------------------------------------------

struct RayTracingShaderGroup
{
    RayTracingShaderGroupType type = grfx::RAY_TRACING_SHADER_GROUP_TYPE_UNDEFINED;
    std::string               generalName;
    std::string               anyHitName;
    std::string               closestHitname;
    std::string               intersectionName;
};

struct RayTracingPipelineCreateInfo
{
    uint32_t                       shaderStageCount                                = 0;
    grfx::ShaderStageInfo          shaderStages[PPX_MAX_RAY_TRACING_SHADER_STAGES] = {};
    uint32_t                       shaderGroupCount                                = 0;
    RayTracingShaderGroup          shaderGroups[PPX_MAX_RAY_TRACING_SHADER_GROUPS] = {};
    uint32_t                       maxRayPayloadSize                               = 0;
    uint32_t                       maxRayHitAttributeSize                          = 0;
    uint32_t                       maxTraceRecursionDepth                          = 1;
    const grfx::PipelineInterface* pGlobalPipelineInterface                        = nullptr;
};

class RayTracingPipeline
    : public grfx::DeviceObject<grfx::RayTracingPipelineCreateInfo>
{
public:
    RayTracingPipeline() {}
    virtual ~RayTracingPipeline() {}
};

} // namespace grfx
} // namespace ppx

#endif // defined(PPX_ENABLE_RAYTRACING)

#endif // ppx_grfx_ray_tracing_h
