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

#include "ppx/grfx/grfx_text_draw.h"
#include "ppx/grfx/grfx_device.h"
#include "ppx/bitmap.h"
#include "ppx/graphics_util.h"

#define STB_TRUETYPE_IMPLEMENTATION
#include "stb_truetype.h"

#include "utf8.h"

namespace ppx {
namespace grfx {

// -------------------------------------------------------------------------------------------------
// TextureFont
// -------------------------------------------------------------------------------------------------
std::string TextureFont::GetDefaultCharacters()
{
    std::string characters;
    for (char c = 32; c < 127; ++c) {
        characters.push_back(c);
    }
    return characters;
}

Result TextureFont::CreateApiObjects(const grfx::TextureFontCreateInfo* pCreateInfo)
{
    std::string characters = pCreateInfo->characters;
    if (characters.empty()) {
        characters             = GetDefaultCharacters();
        mCreateInfo.characters = characters;
    }

    if (!utf8::is_valid(characters.cbegin(), characters.cend())) {
        return ppx::ERROR_INVALID_UTF8_STRING;
    }

    // Font metrics
    pCreateInfo->font.GetFontMetrics(pCreateInfo->size, &mFontMetrics);

    // Subpixel shift
    float subpixelShiftX = 0.5f;
    float subpixelShiftY = 0.5f;

    // Get glyph metrics and max bounds
    utf8::iterator<std::string::iterator> it(characters.begin(), characters.begin(), characters.end());
    utf8::iterator<std::string::iterator> it_end(characters.end(), characters.begin(), characters.end());
    bool                                  hasSpace = false;
    while (it != it_end) {
        const uint32_t codepoint = utf8::next(it, it_end);
        GlyphMetrics   metrics   = {};
        pCreateInfo->font.GetGlyphMetrics(pCreateInfo->size, codepoint, subpixelShiftX, subpixelShiftY, &metrics);
        mGlyphMetrics.emplace_back(grfx::TextureFontGlyphMetrics{codepoint, metrics});

        if (!hasSpace) {
            hasSpace = (codepoint == 32);
        }
    }
    if (!hasSpace) {
        const uint32_t codepoint = 32;
        GlyphMetrics   metrics   = {};
        pCreateInfo->font.GetGlyphMetrics(pCreateInfo->size, codepoint, subpixelShiftX, subpixelShiftY, &metrics);
        mGlyphMetrics.emplace_back(grfx::TextureFontGlyphMetrics{codepoint, metrics});
    }

    // Figure out a squarish somewhat texture size
    const size_t  nc           = characters.size();
    const int32_t sqrtnc       = static_cast<int32_t>(sqrtf(static_cast<float>(nc)) + 0.5f) + 1;
    int32_t       bitmapWidth  = 0;
    int32_t       bitmapHeight = 0;
    size_t        glyphIndex   = 0;
    for (int32_t i = 0; (i < sqrtnc) && (glyphIndex < nc); ++i) {
        int32_t height = 0;
        int32_t width  = 0;
        for (int32_t j = 0; (j < sqrtnc) && (glyphIndex < nc); ++j, ++glyphIndex) {
            const GlyphMetrics& metrics = mGlyphMetrics[glyphIndex].glyphMetrics;
            int32_t             w       = (metrics.box.x1 - metrics.box.x0) + 1;
            int32_t             h       = (metrics.box.y1 - metrics.box.y0) + 1;
            width                       = width + w;
            height                      = std::max<int32_t>(height, h);
        }
        bitmapWidth  = std::max<int32_t>(bitmapWidth, width);
        bitmapHeight = bitmapHeight + height;
    }

    // Storage bitmap
    Bitmap bitmap = Bitmap::Create(bitmapWidth, bitmapHeight, Bitmap::Format::FORMAT_R_UINT8);

    // Render glyph bitmaps
    const float    invBitmapWidth  = 1.0f / static_cast<float>(bitmapWidth);
    const float    invBitmapHeight = 1.0f / static_cast<float>(bitmapHeight);
    const uint32_t rowStride       = bitmap.GetRowStride();
    const uint32_t pixelStride     = bitmap.GetPixelStride();
    uint32_t       y               = 0;
    glyphIndex                     = 0;
    for (int32_t i = 0; (i < sqrtnc) && (glyphIndex < nc); ++i) {
        uint32_t x      = 0;
        uint32_t height = 0;
        for (int32_t j = 0; (j < sqrtnc) && (glyphIndex < nc); ++j, ++glyphIndex) {
            uint32_t            codepoint = mGlyphMetrics[glyphIndex].codepoint;
            const GlyphMetrics& metrics   = mGlyphMetrics[glyphIndex].glyphMetrics;
            uint32_t            w         = static_cast<uint32_t>(metrics.box.x1 - metrics.box.x0) + 1;
            uint32_t            h         = static_cast<uint32_t>(metrics.box.y1 - metrics.box.y0) + 1;

            uint32_t offset  = (y * rowStride) + (x * pixelStride);
            char*    pOutput = bitmap.GetData() + offset;
            pCreateInfo->font.RenderGlyphBitmap(pCreateInfo->size, codepoint, subpixelShiftX, subpixelShiftY, w, h, rowStride, reinterpret_cast<unsigned char*>(pOutput));

            mGlyphMetrics[glyphIndex].size.x = static_cast<float>(w);
            mGlyphMetrics[glyphIndex].size.y = static_cast<float>(h);

            mGlyphMetrics[glyphIndex].uvRect.u0 = x * invBitmapWidth;
            mGlyphMetrics[glyphIndex].uvRect.v0 = y * invBitmapHeight;
            mGlyphMetrics[glyphIndex].uvRect.u1 = (x + w - 1) * invBitmapWidth;
            mGlyphMetrics[glyphIndex].uvRect.v1 = (y + h - 1) * invBitmapHeight;

            x      = x + w;
            height = std::max<int32_t>(height, h);
        }
        y += height;
    }

    ppx::Result ppxres = grfx_util::CreateTextureFromBitmap(GetDevice()->GetGraphicsQueue(), &bitmap, &mTexture);
    if (Failed(ppxres)) {
        return ppxres;
    }

    // Release the font since we don't need it anymore
    mCreateInfo.font = Font();

    return ppx::SUCCESS;
}

void TextureFont::DestroyApiObjects()
{
    if (mTexture) {
        GetDevice()->DestroyTexture(mTexture);
        mTexture.Reset();
    }
}

const grfx::TextureFontGlyphMetrics* TextureFont::GetGlyphMetrics(uint32_t codepoint) const
{
    const grfx::TextureFontGlyphMetrics* ptr = nullptr;
    auto                                 it  = FindIf(
        mGlyphMetrics,
        [codepoint](const grfx::TextureFontGlyphMetrics& elem) -> bool {
            bool match = (elem.codepoint == codepoint);
            return match; });
    if (it != std::end(mGlyphMetrics)) {
        ptr = &(*it);
    }
    return ptr;
}

// -------------------------------------------------------------------------------------------------
// TextDraw
// -------------------------------------------------------------------------------------------------
struct Vertex
{
    float2   position;
    float2   uv;
    uint32_t rgba;
};

constexpr size_t kGlyphIndicesSize  = 6 * sizeof(uint32_t);
constexpr size_t kGlyphVerticesSize = 4 * sizeof(Vertex);

static grfx::SamplerPtr sSampler;

Result TextDraw::CreateApiObjects(const grfx::TextDrawCreateInfo* pCreateInfo)
{
    if (IsNull(pCreateInfo->pFont)) {
        PPX_ASSERT_MSG(false, "Pointer to texture font object is null");
        return ppx::ERROR_UNEXPECTED_NULL_ARGUMENT;
    }

    // Index buffer
    {
        uint64_t size = pCreateInfo->maxTextLength * kGlyphIndicesSize;

        grfx::BufferCreateInfo createInfo      = {};
        createInfo.size                        = size;
        createInfo.usageFlags.bits.transferSrc = true;
        createInfo.memoryUsage                 = grfx::MEMORY_USAGE_CPU_TO_GPU;
        createInfo.initialState                = grfx::RESOURCE_STATE_COPY_SRC;

        ppx::Result ppxres = GetDevice()->CreateBuffer(&createInfo, &mCpuIndexBuffer);
        if (Failed(ppxres)) {
            PPX_ASSERT_MSG(false, "failed creating CPU index buffer");
            return ppxres;
        }

        createInfo.usageFlags.bits.transferSrc = false;
        createInfo.usageFlags.bits.transferDst = true;
        createInfo.usageFlags.bits.indexBuffer = true;
        createInfo.memoryUsage                 = grfx::MEMORY_USAGE_GPU_ONLY;
        createInfo.initialState                = grfx::RESOURCE_STATE_INDEX_BUFFER;

        ppxres = GetDevice()->CreateBuffer(&createInfo, &mGpuIndexBuffer);
        if (Failed(ppxres)) {
            PPX_ASSERT_MSG(false, "failed creating GPU index buffer");
            return ppxres;
        }

        mIndexBufferView.pBuffer   = mGpuIndexBuffer;
        mIndexBufferView.indexType = grfx::INDEX_TYPE_UINT32;
        mIndexBufferView.offset    = 0;
    }

    // Vertex buffer
    {
        uint64_t size = pCreateInfo->maxTextLength * kGlyphVerticesSize;

        grfx::BufferCreateInfo createInfo      = {};
        createInfo.size                        = size;
        createInfo.usageFlags.bits.transferSrc = true;
        createInfo.memoryUsage                 = grfx::MEMORY_USAGE_CPU_TO_GPU;
        createInfo.initialState                = grfx::RESOURCE_STATE_COPY_SRC;

        ppx::Result ppxres = GetDevice()->CreateBuffer(&createInfo, &mCpuVertexBuffer);
        if (Failed(ppxres)) {
            PPX_ASSERT_MSG(false, "failed creating CPU vertex buffer");
            return ppxres;
        }

        createInfo.usageFlags.bits.transferSrc  = false;
        createInfo.usageFlags.bits.transferDst  = true;
        createInfo.usageFlags.bits.vertexBuffer = true;
        createInfo.memoryUsage                  = grfx::MEMORY_USAGE_GPU_ONLY;
        createInfo.initialState                 = grfx::RESOURCE_STATE_VERTEX_BUFFER;

        ppxres = GetDevice()->CreateBuffer(&createInfo, &mGpuVertexBuffer);
        if (Failed(ppxres)) {
            PPX_ASSERT_MSG(false, "failed creating GPU vertex buffer");
            return ppxres;
        }

        mVertexBufferView.pBuffer = mGpuVertexBuffer;
        mVertexBufferView.stride  = sizeof(Vertex);
        mVertexBufferView.offset  = 0;
    }

    if (!sSampler) {
        grfx::SamplerCreateInfo createInfo = {};
        createInfo.magFilter               = grfx::FILTER_LINEAR;
        createInfo.minFilter               = grfx::FILTER_LINEAR;
        createInfo.mipmapMode              = grfx::SAMPLER_MIPMAP_MODE_LINEAR;
        createInfo.addressModeU            = grfx::SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
        createInfo.addressModeV            = grfx::SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
        createInfo.addressModeW            = grfx::SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
        createInfo.mipLodBias              = 0.0f;
        createInfo.anisotropyEnable        = false;
        createInfo.maxAnisotropy           = 0.0f;
        createInfo.compareEnable           = false;
        createInfo.compareOp               = grfx::COMPARE_OP_NEVER;
        createInfo.minLod                  = 0.0f;
        createInfo.maxLod                  = 1.0f;
        createInfo.borderColor             = grfx::BORDER_COLOR_FLOAT_TRANSPARENT_BLACK;

        ppx::Result ppxres = GetDevice()->CreateSampler(&createInfo, &sSampler);
        if (Failed(ppxres)) {
            PPX_ASSERT_MSG(false, "failed creating sampler");
            return ppxres;
        }
    }

    // Constant buffer
    {
        uint64_t size = PPX_MINIMUM_CONSTANT_BUFFER_SIZE;

        grfx::BufferCreateInfo createInfo      = {};
        createInfo.size                        = size;
        createInfo.usageFlags.bits.transferSrc = true;
        createInfo.memoryUsage                 = grfx::MEMORY_USAGE_CPU_TO_GPU;
        createInfo.initialState                = grfx::RESOURCE_STATE_COPY_SRC;

        ppx::Result ppxres = GetDevice()->CreateBuffer(&createInfo, &mCpuConstantBuffer);
        if (Failed(ppxres)) {
            PPX_ASSERT_MSG(false, "failed creating CPU constant buffer");
            return ppxres;
        }

        createInfo.usageFlags.bits.transferSrc   = false;
        createInfo.usageFlags.bits.transferDst   = true;
        createInfo.usageFlags.bits.uniformBuffer = true;
        createInfo.memoryUsage                   = grfx::MEMORY_USAGE_GPU_ONLY;
        createInfo.initialState                  = grfx::RESOURCE_STATE_CONSTANT_BUFFER;

        ppxres = GetDevice()->CreateBuffer(&createInfo, &mGpuConstantBuffer);
        if (Failed(ppxres)) {
            PPX_ASSERT_MSG(false, "failed creating GPU constant buffer");
            return ppxres;
        }
    }

    // Descriptor pool
    {
        grfx::DescriptorPoolCreateInfo createInfo = {};
        createInfo.sampler                        = 1;
        createInfo.sampledImage                   = 1;
        createInfo.uniformBuffer                  = 1;

        ppx::Result ppxres = GetDevice()->CreateDescriptorPool(&createInfo, &mDescriptorPool);
        if (Failed(ppxres)) {
            PPX_ASSERT_MSG(false, "failed creating descriptor pool");
            return ppxres;
        }
    }

    // Descriptor set layout
    {
        std::vector<grfx::DescriptorBinding> bindings = {
            {0, grfx::DESCRIPTOR_TYPE_UNIFORM_BUFFER, 1, grfx::SHADER_STAGE_ALL_GRAPHICS},
            {1, grfx::DESCRIPTOR_TYPE_SAMPLED_IMAGE, 1, grfx::SHADER_STAGE_ALL_GRAPHICS},
            {2, grfx::DESCRIPTOR_TYPE_SAMPLER, 1, grfx::SHADER_STAGE_ALL_GRAPHICS},
        };

        grfx::DescriptorSetLayoutCreateInfo createInfo = {};
        createInfo.bindings                            = bindings;

        ppx::Result ppxres = GetDevice()->CreateDescriptorSetLayout(&createInfo, &mDescriptorSetLayout);
        if (Failed(ppxres)) {
            PPX_ASSERT_MSG(false, "failed creating descriptor set layout");
            return ppxres;
        }
    }

    // Descriptor Set
    {
        ppx::Result ppxres = GetDevice()->AllocateDescriptorSet(mDescriptorPool, mDescriptorSetLayout, &mDescriptorSet);
        if (Failed(ppxres)) {
            PPX_ASSERT_MSG(false, "failed allocating descriptor set");
            return ppxres;
        }

        mDescriptorSet->UpdateUniformBuffer(0, 0, mGpuConstantBuffer);
        mDescriptorSet->UpdateSampledImage(1, 0, pCreateInfo->pFont->GetTexture());
        mDescriptorSet->UpdateSampler(2, 0, sSampler);
    }

    // Pipeline interface
    {
        grfx::PipelineInterfaceCreateInfo createInfo = {};
        createInfo.setCount                          = 1;
        createInfo.sets[0].set                       = 0;
        createInfo.sets[0].pLayout                   = mDescriptorSetLayout;

        ppx::Result ppxres = GetDevice()->CreatePipelineInterface(&createInfo, &mPipelineInterface);
        if (Failed(ppxres)) {
            PPX_ASSERT_MSG(false, "failed creating pipeline interface");
            return ppxres;
        }
    }

    // Pipeline
    {
        grfx::VertexBinding vertexBinding;
        vertexBinding.AppendAttribute({"POSITION", 0, grfx::FORMAT_R32G32_FLOAT, 0, PPX_APPEND_OFFSET_ALIGNED, grfx::VERTEX_INPUT_RATE_VERTEX});
        vertexBinding.AppendAttribute({"TEXCOORD", 1, grfx::FORMAT_R32G32_FLOAT, 0, PPX_APPEND_OFFSET_ALIGNED, grfx::VERTEX_INPUT_RATE_VERTEX});
        vertexBinding.AppendAttribute({"COLOR", 2, grfx::FORMAT_R8G8B8A8_UNORM, 0, PPX_APPEND_OFFSET_ALIGNED, grfx::VERTEX_INPUT_RATE_VERTEX});

        grfx::GraphicsPipelineCreateInfo2 createInfo  = {};
        createInfo.VS                                 = {pCreateInfo->VS.pModule, pCreateInfo->VS.entryPoint};
        createInfo.PS                                 = {pCreateInfo->PS.pModule, pCreateInfo->PS.entryPoint};
        createInfo.vertexInputState.bindingCount      = 1;
        createInfo.vertexInputState.bindings[0]       = vertexBinding;
        createInfo.topology                           = grfx::PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;
        createInfo.polygonMode                        = grfx::POLYGON_MODE_FILL;
        createInfo.cullMode                           = grfx::CULL_MODE_BACK;
        createInfo.frontFace                          = grfx::FRONT_FACE_CCW;
        createInfo.depthReadEnable                    = false;
        createInfo.depthWriteEnable                   = false;
        createInfo.blendModes[0]                      = pCreateInfo->blendMode;
        createInfo.outputState.renderTargetCount      = 1;
        createInfo.outputState.renderTargetFormats[0] = pCreateInfo->renderTargetFormat;
        createInfo.outputState.depthStencilFormat     = pCreateInfo->depthStencilFormat;
        createInfo.pPipelineInterface                 = mPipelineInterface;

        ppx::Result ppxres = GetDevice()->CreateGraphicsPipeline(&createInfo, &mPipeline);
        if (Failed(ppxres)) {
            PPX_ASSERT_MSG(false, "failed creating pipeline");
            return ppxres;
        }
    }

    return ppx::SUCCESS;
}

void TextDraw::DestroyApiObjects()
{
    if (mCpuIndexBuffer) {
        GetDevice()->DestroyBuffer(mCpuIndexBuffer);
        mCpuIndexBuffer.Reset();
    }

    if (mGpuIndexBuffer) {
        GetDevice()->DestroyBuffer(mGpuIndexBuffer);
        mGpuIndexBuffer.Reset();
    }

    if (mCpuVertexBuffer) {
        GetDevice()->DestroyBuffer(mCpuVertexBuffer);
        mCpuVertexBuffer.Reset();
    }

    if (mGpuVertexBuffer) {
        GetDevice()->DestroyBuffer(mGpuVertexBuffer);
        mGpuVertexBuffer.Reset();
    }

    if (mCpuConstantBuffer) {
        GetDevice()->DestroyBuffer(mCpuConstantBuffer);
        mCpuConstantBuffer.Reset();
    }

    if (mGpuConstantBuffer) {
        GetDevice()->DestroyBuffer(mGpuConstantBuffer);
        mGpuConstantBuffer.Reset();
    }

    if (mPipeline) {
        GetDevice()->DestroyGraphicsPipeline(mPipeline);
        mPipeline.Reset();
    }

    if (mPipelineInterface) {
        GetDevice()->DestroyPipelineInterface(mPipelineInterface);
        mPipelineInterface.Reset();
    }

    if (mDescriptorSet) {
        GetDevice()->FreeDescriptorSet(mDescriptorSet);
        mDescriptorSet.Reset();
    }

    if (mDescriptorSetLayout) {
        GetDevice()->DestroyDescriptorSetLayout(mDescriptorSetLayout);
        mDescriptorSetLayout.Reset();
    }

    if (mDescriptorPool) {
        GetDevice()->DestroyDescriptorPool(mDescriptorPool);
        mDescriptorPool.Reset();
    }
}

void TextDraw::Clear()
{
    mTextLength = 0;
}

void TextDraw::AddString(
    const float2&      position,
    const std::string& string,
    float              tabSpacing,
    float              lineSpacing,
    const float3&      color,
    float              opacity)
{
    if (mTextLength >= mCreateInfo.maxTextLength) {
        return;
    }

    // Map index buffer
    void*       mappedAddress = nullptr;
    ppx::Result ppxres        = mCpuIndexBuffer->MapMemory(0, &mappedAddress);
    if (Failed(ppxres)) {
        return;
    }
    uint8_t* pIndicesBaseAddr = static_cast<uint8_t*>(mappedAddress);

    // Map vertex buffer
    ppxres = mCpuVertexBuffer->MapMemory(0, &mappedAddress);
    if (Failed(ppxres)) {
        return;
    }
    uint8_t* pVerticesBaseAddr = static_cast<uint8_t*>(mappedAddress);

    // Convert to 8 bit color
    uint32_t r    = std::min<uint32_t>(static_cast<uint32_t>(color.r * 255.0f), 255);
    uint32_t g    = std::min<uint32_t>(static_cast<uint32_t>(color.g * 255.0f), 255);
    uint32_t b    = std::min<uint32_t>(static_cast<uint32_t>(color.b * 255.0f), 255);
    uint32_t a    = std::min<uint32_t>(static_cast<uint32_t>(opacity * 255.0f), 255);
    uint32_t rgba = (a << 24) | (b << 16) | (g << 8) | (r << 0);

    utf8::iterator<std::string::const_iterator> it(string.begin(), string.begin(), string.end());
    utf8::iterator<std::string::const_iterator> it_end(string.end(), string.begin(), string.end());
    float2                                      baseline = position;
    float                                       ascent   = mCreateInfo.pFont->GetAscent();
    float                                       descent  = mCreateInfo.pFont->GetDescent();
    float                                       lineGap  = mCreateInfo.pFont->GetLineGap();
    lineSpacing                                          = lineSpacing * (ascent - descent + lineGap);

    while (it != it_end) {
        uint32_t codepoint = utf8::next(it, it_end);
        if (codepoint == '\n') {
            baseline.x = position.x;
            baseline.y += lineSpacing;
            continue;
        }
        else if (codepoint == '\t') {
            const grfx::TextureFontGlyphMetrics* pMetrics = mCreateInfo.pFont->GetGlyphMetrics(32);
            baseline.x += tabSpacing * pMetrics->glyphMetrics.advance;
            continue;
        }

        const grfx::TextureFontGlyphMetrics* pMetrics = mCreateInfo.pFont->GetGlyphMetrics(codepoint);
        if (IsNull(pMetrics)) {
            pMetrics = mCreateInfo.pFont->GetGlyphMetrics(32);
        }

        size_t indexBufferOffset    = mTextLength * kGlyphIndicesSize;
        size_t vertexBufferOffset   = mTextLength * kGlyphVerticesSize;
        bool   exceededIndexBuffer  = (indexBufferOffset >= mCpuIndexBuffer->GetSize());
        bool   exceededVertexBuffer = (vertexBufferOffset >= mCpuVertexBuffer->GetSize());
        if (exceededIndexBuffer || exceededVertexBuffer) {
            break;
        }

        uint32_t* pIndices  = reinterpret_cast<uint32_t*>(pIndicesBaseAddr + indexBufferOffset);
        Vertex*   pVertices = reinterpret_cast<Vertex*>(pVerticesBaseAddr + vertexBufferOffset);

        float2 P   = baseline + float2(pMetrics->glyphMetrics.box.x0, pMetrics->glyphMetrics.box.y0);
        float2 P0  = P;
        float2 P1  = P + float2(0, pMetrics->size.y);
        float2 P2  = P + pMetrics->size;
        float2 P3  = P + float2(pMetrics->size.x, 0);
        float2 uv0 = float2(pMetrics->uvRect.u0, pMetrics->uvRect.v0);
        float2 uv1 = float2(pMetrics->uvRect.u0, pMetrics->uvRect.v1);
        float2 uv2 = float2(pMetrics->uvRect.u1, pMetrics->uvRect.v1);
        float2 uv3 = float2(pMetrics->uvRect.u1, pMetrics->uvRect.v0);

        pVertices[0] = Vertex{P0, uv0, rgba};
        pVertices[1] = Vertex{P1, uv1, rgba};
        pVertices[2] = Vertex{P2, uv2, rgba};
        pVertices[3] = Vertex{P3, uv3, rgba};

        uint32_t vertexCount = mTextLength * 4;
        pIndices[0]          = vertexCount + 0;
        pIndices[1]          = vertexCount + 1;
        pIndices[2]          = vertexCount + 2;
        pIndices[3]          = vertexCount + 0;
        pIndices[4]          = vertexCount + 2;
        pIndices[5]          = vertexCount + 3;

        mTextLength += 1;
        baseline.x += pMetrics->glyphMetrics.advance;
    }

    mCpuIndexBuffer->UnmapMemory();
    mCpuVertexBuffer->UnmapMemory();
}

void TextDraw::AddString(
    const float2&      position,
    const std::string& string,
    const float3&      color,
    float              opacity)
{
    AddString(position, string, 3.0f, 1.0f, color, opacity);
}

ppx::Result TextDraw::UploadToGpu(grfx::Queue* pQueue)
{
    grfx::BufferToBufferCopyInfo copyInfo = {};
    copyInfo.size                         = mCpuIndexBuffer->GetSize();
    copyInfo.srcBuffer.offset             = 0;
    copyInfo.dstBuffer.offset             = 0;

    ppx::Result ppxres = pQueue->CopyBufferToBuffer(&copyInfo, mCpuIndexBuffer, mGpuIndexBuffer, grfx::RESOURCE_STATE_INDEX_BUFFER, grfx::RESOURCE_STATE_INDEX_BUFFER);
    if (Failed(ppxres)) {
        return ppxres;
    }

    copyInfo.size = mCpuVertexBuffer->GetSize();
    ppxres        = pQueue->CopyBufferToBuffer(&copyInfo, mCpuVertexBuffer, mGpuVertexBuffer, grfx::RESOURCE_STATE_VERTEX_BUFFER, grfx::RESOURCE_STATE_VERTEX_BUFFER);
    if (Failed(ppxres)) {
        return ppxres;
    }

    return ppx::SUCCESS;
}

void TextDraw::UploadToGpu(grfx::CommandBuffer* pCommandBuffer)
{
    grfx::BufferToBufferCopyInfo copyInfo = {};
    copyInfo.size                         = mTextLength * kGlyphIndicesSize;
    copyInfo.srcBuffer.offset             = 0;
    copyInfo.dstBuffer.offset             = 0;

    pCommandBuffer->BufferResourceBarrier(mGpuIndexBuffer, grfx::RESOURCE_STATE_INDEX_BUFFER, grfx::RESOURCE_STATE_COPY_DST);
    pCommandBuffer->CopyBufferToBuffer(&copyInfo, mCpuIndexBuffer, mGpuIndexBuffer);
    pCommandBuffer->BufferResourceBarrier(mGpuIndexBuffer, grfx::RESOURCE_STATE_COPY_DST, grfx::RESOURCE_STATE_INDEX_BUFFER);

    copyInfo.size = mTextLength * kGlyphVerticesSize;
    pCommandBuffer->BufferResourceBarrier(mGpuVertexBuffer, grfx::RESOURCE_STATE_VERTEX_BUFFER, grfx::RESOURCE_STATE_COPY_DST);
    pCommandBuffer->CopyBufferToBuffer(&copyInfo, mCpuVertexBuffer, mGpuVertexBuffer);
    pCommandBuffer->BufferResourceBarrier(mGpuVertexBuffer, grfx::RESOURCE_STATE_COPY_DST, grfx::RESOURCE_STATE_VERTEX_BUFFER);
}

void TextDraw::PrepareDraw(const float4x4& MVP, grfx::CommandBuffer* pCommandBuffer)
{
    void*       mappedAddress = nullptr;
    ppx::Result ppxres        = mCpuConstantBuffer->MapMemory(0, &mappedAddress);
    if (Failed(ppxres)) {
        return;
    }

    std::memcpy(mappedAddress, &MVP, sizeof(float4x4));

    mCpuConstantBuffer->UnmapMemory();

    grfx::BufferToBufferCopyInfo copyInfo = {};
    copyInfo.size                         = mCpuConstantBuffer->GetSize();
    copyInfo.srcBuffer.offset             = 0;
    copyInfo.dstBuffer.offset             = 0;

    pCommandBuffer->BufferResourceBarrier(mGpuConstantBuffer, grfx::RESOURCE_STATE_CONSTANT_BUFFER, grfx::RESOURCE_STATE_COPY_DST);
    pCommandBuffer->CopyBufferToBuffer(&copyInfo, mCpuConstantBuffer, mGpuConstantBuffer);
    pCommandBuffer->BufferResourceBarrier(mGpuConstantBuffer, grfx::RESOURCE_STATE_COPY_DST, grfx::RESOURCE_STATE_CONSTANT_BUFFER);
}

void TextDraw::Draw(grfx::CommandBuffer* pCommandBuffer)
{
    pCommandBuffer->BindIndexBuffer(&mIndexBufferView);
    pCommandBuffer->BindVertexBuffers(1, &mVertexBufferView);
    pCommandBuffer->BindGraphicsDescriptorSets(mPipelineInterface, 1, &mDescriptorSet);
    pCommandBuffer->BindGraphicsPipeline(mPipeline);
    pCommandBuffer->DrawIndexed(mTextLength * 6);
}

} // namespace grfx
} // namespace ppx
