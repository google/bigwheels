// Copyright 2017 Pavel Dobryakov
// Copyright 2022 Google LLC
//
// Use of this source code is governed by an MIT-style
// license that can be found in the LICENSE file or at
// https://opensource.org/licenses/MIT.

#include "sim.h"
#include "shaders.h"

#include "ppx/math_config.h"
#include "ppx/application.h"
#include "ppx/config.h"
#include "ppx/grfx/grfx_format.h"
#include "ppx/timer.h"

#include <algorithm>
#include <cmath>

namespace FluidSim {

// Color formats used by textures.
const ppx::grfx::Format kR    = ppx::grfx::FORMAT_R16_FLOAT;
const ppx::grfx::Format kRG   = ppx::grfx::FORMAT_R16G16_FLOAT;
const ppx::grfx::Format kRGBA = ppx::grfx::FORMAT_R16G16B16A16_FLOAT;

FluidSimulation::FluidSimulation(ProjApp* app)
    : mApp(app)
{
    // Create descriptor pool shared by all pipelines.
    ppx::grfx::DescriptorPoolCreateInfo dpci = {};
    dpci.sampler                             = 1024;
    dpci.sampledImage                        = 1024;
    dpci.uniformBuffer                       = 1024;
    dpci.storageImage                        = 1024;
    PPX_CHECKED_CALL(GetApp()->GetDevice()->CreateDescriptorPool(&dpci, &mDescriptorPool));

    // Frame synchronization data.
    PerFrame                       frame = {};
    ppx::grfx::SemaphoreCreateInfo sci   = {};
    ppx::grfx::FenceCreateInfo     fci   = {};
    PPX_CHECKED_CALL(GetApp()->GetGraphicsQueue()->CreateCommandBuffer(&frame.cmd));
    PPX_CHECKED_CALL(GetApp()->GetDevice()->CreateSemaphore(&sci, &frame.imageAcquiredSemaphore));
    PPX_CHECKED_CALL(GetApp()->GetDevice()->CreateFence(&fci, &frame.imageAcquiredFence));
    PPX_CHECKED_CALL(GetApp()->GetDevice()->CreateSemaphore(&sci, &frame.renderCompleteSemaphore));
    fci = {true}; // Create signaled
    PPX_CHECKED_CALL(GetApp()->GetDevice()->CreateFence(&fci, &frame.renderCompleteFence));
    mPerFrame.push_back(frame);

    // Set up all the filters to use.
    InitComputeShaders();

    // Set up draw program to emit computed textures to the swapchain.
    InitGraphicsShaders();

    // Initialize textures
    InitTextures();
}

void FluidSimulation::InitComputeShaders()
{
    // Descriptor set layout.  This must match assets/fluid_simulation/shaders/config.hlsli and it
    // is shared across all ComputeShader instances.
    ppx::grfx::DescriptorSetLayoutCreateInfo lci = {};
    lci.bindings.push_back(ppx::grfx::DescriptorBinding(0, ppx::grfx::DESCRIPTOR_TYPE_UNIFORM_BUFFER));
    lci.bindings.push_back(ppx::grfx::DescriptorBinding(1, ppx::grfx::DESCRIPTOR_TYPE_SAMPLER));
    lci.bindings.push_back(ppx::grfx::DescriptorBinding(2, ppx::grfx::DESCRIPTOR_TYPE_SAMPLED_IMAGE));
    lci.bindings.push_back(ppx::grfx::DescriptorBinding(3, ppx::grfx::DESCRIPTOR_TYPE_SAMPLED_IMAGE));
    lci.bindings.push_back(ppx::grfx::DescriptorBinding(4, ppx::grfx::DESCRIPTOR_TYPE_SAMPLED_IMAGE));
    lci.bindings.push_back(ppx::grfx::DescriptorBinding(5, ppx::grfx::DESCRIPTOR_TYPE_SAMPLED_IMAGE));
    lci.bindings.push_back(ppx::grfx::DescriptorBinding(6, ppx::grfx::DESCRIPTOR_TYPE_SAMPLED_IMAGE));
    lci.bindings.push_back(ppx::grfx::DescriptorBinding(7, ppx::grfx::DESCRIPTOR_TYPE_SAMPLED_IMAGE));
    lci.bindings.push_back(ppx::grfx::DescriptorBinding(8, ppx::grfx::DESCRIPTOR_TYPE_SAMPLED_IMAGE));
    lci.bindings.push_back(ppx::grfx::DescriptorBinding(9, ppx::grfx::DESCRIPTOR_TYPE_SAMPLED_IMAGE));
    lci.bindings.push_back(ppx::grfx::DescriptorBinding(10, ppx::grfx::DESCRIPTOR_TYPE_SAMPLED_IMAGE));
    lci.bindings.push_back(ppx::grfx::DescriptorBinding(11, ppx::grfx::DESCRIPTOR_TYPE_STORAGE_IMAGE));
    PPX_CHECKED_CALL(GetApp()->GetDevice()->CreateDescriptorSetLayout(&lci, &mCompute.mDescriptorSetLayout));

    // Compute pipeline interface.
    ppx::grfx::PipelineInterfaceCreateInfo pici = {};
    pici.setCount                               = 1;
    pici.sets[0].set                            = 0;
    pici.sets[0].pLayout                        = mCompute.mDescriptorSetLayout;
    PPX_CHECKED_CALL(GetApp()->GetDevice()->CreatePipelineInterface(&pici, &mCompute.mPipelineInterface));

    // Compute sampler.
    ppx::grfx::SamplerCreateInfo sci = {};
    sci.magFilter                    = ppx::grfx::FILTER_LINEAR;
    sci.minFilter                    = ppx::grfx::FILTER_LINEAR;
    sci.mipmapMode                   = ppx::grfx::SAMPLER_MIPMAP_MODE_NEAREST;
    sci.addressModeU                 = ppx::grfx::SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
    sci.addressModeV                 = ppx::grfx::SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
    sci.addressModeW                 = ppx::grfx::SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
    sci.minLod                       = 0.0f;
    sci.maxLod                       = FLT_MAX;
    PPX_CHECKED_CALL(GetApp()->GetDevice()->CreateSampler(&sci, &mCompute.mSampler));

    // Create compute shaders for filtering.
    mBloomBlur         = std::make_unique<BloomBlurShader>(this);
    mBloomBlurAdditive = std::make_unique<BloomBlurAdditiveShader>(this);
    mBloomFinal        = std::make_unique<BloomFinalShader>(this);
    mBloomPrefilter    = std::make_unique<BloomPrefilterShader>(this);
    mBlur              = std::make_unique<BlurShader>(this);
    mCheckerboard      = std::make_unique<CheckerboardShader>(this);
    mColor             = std::make_unique<ColorShader>(this);
    mDisplayMaterial   = std::make_unique<DisplayShader>(this);
    mSplat             = std::make_unique<SplatShader>(this);
    mSunraysMask       = std::make_unique<SunraysMaskShader>(this);
    mSunrays           = std::make_unique<SunraysShader>(this);
}

void FluidSimulation::DispatchComputeShaders(const PerFrame& frame)
{
    for (auto& dr : mComputeDispatchQueue) {
        dr.mShader->Dispatch(frame, dr);
    }
}

void FluidSimulation::FreeComputeShaderResources()
{
    // Wait for any command buffers in-flight before freeing up resources.
    GetApp()->GetDevice()->WaitIdle();

    for (auto& dr : mComputeDispatchQueue) {
        dr.FreeResources();
    }
    mComputeDispatchQueue.clear();
}

void FluidSimulation::Render(const PerFrame& frame)
{
    for (auto& dr : mGraphicsDispatchQueue) {
        dr.mShader->Dispatch(frame, dr);
    }
}

void FluidSimulation::FreeGraphicsShaders()
{
    // Wait for any command buffers in-flight before freeing up resources.
    GetApp()->GetDevice()->WaitIdle();

    for (auto& dr : mGraphicsDispatchQueue) {
        dr.FreeResources();
    }
    mGraphicsDispatchQueue.clear();
}

void FluidSimulation::InitGraphicsShaders()
{
    // Descriptor set layout.  This is shared across all GraphicsShader instances.
    ppx::grfx::DescriptorSetLayoutCreateInfo lci = {};
    lci.bindings.push_back(ppx::grfx::DescriptorBinding(0, ppx::grfx::DESCRIPTOR_TYPE_SAMPLED_IMAGE));
    lci.bindings.push_back(ppx::grfx::DescriptorBinding(1, ppx::grfx::DESCRIPTOR_TYPE_SAMPLER));
    PPX_CHECKED_CALL(GetApp()->GetDevice()->CreateDescriptorSetLayout(&lci, &mGraphics.mDescriptorSetLayout));

    mGraphics.mVertexBinding.AppendAttribute({"POSITION", 0, ppx::grfx::FORMAT_R32G32B32_FLOAT, 0, PPX_APPEND_OFFSET_ALIGNED, ppx::grfx::VERTEX_INPUT_RATE_VERTEX});
    mGraphics.mVertexBinding.AppendAttribute({"TEXCOORD", 1, ppx::grfx::FORMAT_R32G32_FLOAT, 0, PPX_APPEND_OFFSET_ALIGNED, ppx::grfx::VERTEX_INPUT_RATE_VERTEX});

    // Initialize vertex and geometry data.
    // clang-format off
    std::vector<float> vertexData = {
        // Texture position   // Texture coordinates
        -1.0f,  1.0f, 0.0f,   0.0f, 0.0f,
        -1.0f, -1.0f, 0.0f,   0.0f, 1.0f,
         1.0f, -1.0f, 0.0f,   1.0f, 1.0f,

        -1.0f,  1.0f, 0.0f,   0.0f, 0.0f,
         1.0f, -1.0f, 0.0f,   1.0f, 1.0f,
         1.0f,  1.0f, 0.0f,   1.0f, 0.0f,
    };
    // clang-format on

    ppx::grfx::SamplerCreateInfo sci = {};
    PPX_CHECKED_CALL(GetApp()->GetDevice()->CreateSampler(&sci, &mGraphics.mSampler));

    ppx::grfx::BufferCreateInfo bci  = {};
    bci.size                         = ppx::SizeInBytesU32(vertexData);
    bci.usageFlags.bits.vertexBuffer = true;
    bci.memoryUsage                  = ppx::grfx::MEMORY_USAGE_CPU_TO_GPU;
    PPX_CHECKED_CALL(GetApp()->GetDevice()->CreateBuffer(&bci, &mGraphics.mVertexBuffer));

    void* pAddr = nullptr;
    PPX_CHECKED_CALL(mGraphics.mVertexBuffer->MapMemory(0, &pAddr));
    memcpy(pAddr, vertexData.data(), ppx::SizeInBytesU32(vertexData));
    mGraphics.mVertexBuffer->UnmapMemory();

    ppx::grfx::PipelineInterfaceCreateInfo pici = {};
    pici.setCount                               = 1;
    pici.sets[0].set                            = 0;
    pici.sets[0].pLayout                        = mGraphics.mDescriptorSetLayout;
    PPX_CHECKED_CALL(GetApp()->GetDevice()->CreatePipelineInterface(&pici, &mGraphics.mPipelineInterface));

    mDraw = std::make_unique<GraphicsShader>(this);
}

ppx::int2 FluidSimulation::GetResolution(float resolution)
{
    float aspectRatio = static_cast<float>(GetApp()->GetWindowWidth()) / static_cast<float>(GetApp()->GetWindowHeight());
    if (aspectRatio < 1.0f) {
        aspectRatio = 1.0f / aspectRatio;
    }

    int min = round(resolution);
    int max = round(resolution * aspectRatio);

    return (GetApp()->GetWindowWidth() > GetApp()->GetWindowHeight()) ? ppx::int2(max, min) : ppx::int2(min, max);
}

void FluidSimulation::InitTextures()
{
    ppx::int2 simRes = GetResolution(mConfig.simResolution);
    ppx::int2 dyeRes = GetResolution(mConfig.dyeResolution);

    // Generate all the textures.
    mCheckerboardTexture = std::make_unique<Texture>(this, "checkerboard", GetApp()->GetWindowWidth(), GetApp()->GetWindowHeight(), GetApp()->GetSwapchain()->GetColorFormat());
    mDisplayTexture      = std::make_unique<Texture>(this, "display", GetApp()->GetWindowWidth(), GetApp()->GetWindowHeight(), GetApp()->GetSwapchain()->GetColorFormat());
    mDitheringTexture    = std::make_unique<Texture>(this, "fluid_simulation/textures/LDR_LLL1_0.png");
    mDrawColorTexture    = std::make_unique<Texture>(this, "draw color", GetApp()->GetWindowWidth(), GetApp()->GetWindowHeight(), GetApp()->GetSwapchain()->GetColorFormat());
    mDyeTexture[0]       = std::make_unique<Texture>(this, "dye[0]", dyeRes.x, dyeRes.y, kRGBA);
    mDyeTexture[1]       = std::make_unique<Texture>(this, "dye[1]", dyeRes.x, dyeRes.y, kRGBA);
    mVelocityTexture[0]  = std::make_unique<Texture>(this, "velocity[0]", simRes.x, simRes.y, kRG);
    mVelocityTexture[1]  = std::make_unique<Texture>(this, "velocity[1]", simRes.x, simRes.y, kRG);

    InitBloomTextures();
    InitSunraysTextures();
}

void FluidSimulation::InitBloomTextures()
{
    ppx::int2 res = GetResolution(mConfig.bloomResolution);
    mBloomTexture = std::make_unique<Texture>(this, "bloom", res.x, res.y, kRGBA);
    PPX_ASSERT_MSG(mBloomTextures.empty(), "Bloom textures already initialized");
    for (int i = 0; i < mConfig.bloomIterations; i++) {
        uint32_t width  = res.x >> (i + 1);
        uint32_t height = res.y >> (i + 1);
        if (width < 2 || height < 2)
            break;
        mBloomTextures.emplace_back(std::make_unique<Texture>(this, std::string("bloom frame buffer[") + std::to_string(i) + "]", width, height, kRGBA));
    }
}

void FluidSimulation::InitSunraysTextures()
{
    ppx::int2 res       = GetResolution(mConfig.sunraysResolution);
    mSunraysTexture     = std::make_unique<Texture>(this, "sunrays", res.x, res.y, kR);
    mSunraysTempTexture = std::make_unique<Texture>(this, "sunrays temp", res.x, res.y, kR);
}

void FluidSimulation::AddTextureToInitialize(Texture* texture)
{
    mTexturesToInitialize.push_back(texture);
}

void FluidSimulation::GenerateInitialSplat()
{
    for (const auto& texture : mTexturesToInitialize) {
        ScheduleDR(mColor->GetDR(texture, ppx::float4(0.0f, 0.0f, 0.0f, 1.0f)));
    }
    mTexturesToInitialize.clear();

    MultipleSplats(Random().UInt32() % 20 + 5);
    Render();
}

ppx::float3 FluidSimulation::HSVtoRGB(ppx::float3 hsv)
{
    float       h = hsv[0];
    float       s = hsv[1];
    float       v = hsv[2];
    float       i = floorf(h * 6);
    float       f = h * 6 - i;
    float       p = v * (1 - s);
    float       q = v * (1 - f * s);
    float       t = v * (1 - (1 - f) * s);
    ppx::float3 rgb(0, 0, 0);

    switch (static_cast<int>(i) % 6) {
        case 0: rgb = ppx::float3(v, t, p); break;
        case 1: rgb = ppx::float3(q, v, p); break;
        case 2: rgb = ppx::float3(p, v, t); break;
        case 3: rgb = ppx::float3(p, q, v); break;
        case 4: rgb = ppx::float3(t, p, v); break;
        case 5: rgb = ppx::float3(v, p, q); break;
        default: PPX_ASSERT_MSG(false, "Invalid HSV to RGB conversion");
    }

    return rgb;
}

ppx::float3 FluidSimulation::GenerateColor()
{
    ppx::float3 c = HSVtoRGB(ppx::float3(Random().Float(0, 1), 1, 1));
    c.r *= 0.15f;
    c.g *= 0.15f;
    c.b *= 0.15f;
    return c;
}

float FluidSimulation::CorrectRadius(float radius)
{
    float aspectRatio = GetApp()->GetWindowAspect();
    return (aspectRatio > 1) ? radius * aspectRatio : radius;
}

void FluidSimulation::Splat(ppx::float2 point, ppx::float2 delta, ppx::float3 color)
{
    float       aspect     = GetApp()->GetWindowAspect();
    float       radius     = CorrectRadius(mConfig.splatRadius / 100.0f);
    ppx::float4 deltaColor = ppx::float4(delta.x, delta.y, 0.0f, 1.0f);
    ScheduleDR(mSplat->GetDR(mVelocityTexture[0].get(), mVelocityTexture[1].get(), point, aspect, radius, deltaColor));
    std::swap(mVelocityTexture[0], mVelocityTexture[1]);

    ScheduleDR(mSplat->GetDR(mDyeTexture[0].get(), mDyeTexture[1].get(), point, aspect, radius, ppx::float4(color, 1.0f)));
    std::swap(mDyeTexture[0], mDyeTexture[1]);
}

void FluidSimulation::MultipleSplats(uint32_t amount)
{
    PPX_LOG_DEBUG("Emitting " << amount << " splashes of color\n");
    for (uint32_t i = 0; i < amount; i++) {
        ppx::float3 color = GenerateColor();
        color.r *= 10.0f;
        color.g *= 10.0f;
        color.b *= 10.0f;
        ppx::float2       point(Random().Float(0, 1), Random().Float(0, 1));
        ppx::float2       delta(1000.0f * (Random().Float(0, 1) - 0.5f), 1000.0f * (Random().Float(0, 1) - 0.5f));
        std::stringstream s;
        s << point << " with color " << color << "\n";
        PPX_LOG_DEBUG("Splash #" << i << " at " << s.str());
        Splat(point, delta, color);
    }
}

void FluidSimulation::Render()
{
    if (mConfig.bloom) {
        ApplyBloom(mDyeTexture[0].get(), mBloomTexture.get());
    }

    if (mConfig.sunrays) {
        ApplySunrays(mDyeTexture[0].get(), mDyeTexture[1].get(), mSunraysTexture.get());
        Blur(mSunraysTexture.get(), mSunraysTempTexture.get(), 1);
    }

    if (!mConfig.transparent) {
        DrawColor(NormalizeColor(mConfig.backColor));
    }
    else {
        DrawCheckerboard();
    }
    DrawDisplay();
}

void FluidSimulation::ApplyBloom(Texture* source, Texture* destination)
{
    if (mBloomTextures.size() < 2)
        return;

    Texture* last = destination;

    float knee   = mConfig.bloomThreshold * mConfig.bloomSoftKnee + 0.0001f;
    float curve0 = mConfig.bloomThreshold - knee;
    float curve1 = knee * 2.0f;
    float curve2 = 0.25f / knee;
    ScheduleDR(mBloomPrefilter->GetDR(source, last, ppx::float3(curve0, curve1, curve2), mConfig.bloomThreshold));

    for (auto& dest : mBloomTextures) {
        ScheduleDR(mBloomBlur->GetDR(last, dest.get(), last->GetTexelSize()));
        last = dest.get();
    }

    for (int i = mBloomTextures.size() - 2; i >= 0; i--) {
        Texture* baseTex = mBloomTextures[i].get();
        ScheduleDR(mBloomBlurAdditive->GetDR(last, baseTex, last->GetTexelSize()));
        last = baseTex;
    }

    ScheduleDR(mBloomFinal->GetDR(last, destination, last->GetTexelSize(), mConfig.bloomIntensity));
}

void FluidSimulation::ApplySunrays(Texture* source, Texture* mask, Texture* destination)
{
    ScheduleDR(mSunraysMask->GetDR(source, mask));
    ScheduleDR(mSunrays->GetDR(mask, destination, mConfig.sunraysWeight));
}

void FluidSimulation::Blur(Texture* target, Texture* temp, uint32_t iterations)
{
    for (uint32_t i = 0; i < iterations; i++) {
        ScheduleDR(mBlur->GetDR(target, temp, ppx::float2(target->GetTexelSize().x, 0.0f)));
        ScheduleDR(mBlur->GetDR(temp, target, ppx::float2(0.0f, target->GetTexelSize().y)));
    }
}

ppx::float4 FluidSimulation::NormalizeColor(ppx::float4 input)
{
    return ppx::float4(input.r / 255, input.g / 255, input.b / 255, input.a);
}

void FluidSimulation::DrawColor(ppx::float4 color)
{
    ScheduleDR(mColor->GetDR(mDrawColorTexture.get(), color));
    ScheduleDR(mDraw->GetDR(mDrawColorTexture.get()));
}

void FluidSimulation::DrawCheckerboard()
{
    ScheduleDR(mCheckerboard->GetDR(mCheckerboardTexture.get(), GetApp()->GetWindowAspect()));
    ScheduleDR(mDraw->GetDR(mCheckerboardTexture.get()));
}

void FluidSimulation::DrawDisplay()
{
    uint32_t    width       = GetApp()->GetWindowWidth();
    uint32_t    height      = GetApp()->GetWindowHeight();
    ppx::float2 texelSize   = ppx::float2(1.0f / width, 1.0f / height);
    ppx::float2 ditherScale = mDitheringTexture->GetDitherScale(width, height);
    ScheduleDR(mDisplayMaterial->GetDR(mDyeTexture[0].get(), mBloomTexture.get(), mSunraysTexture.get(), mDitheringTexture.get(), mDisplayTexture.get(), texelSize, ditherScale));
    ScheduleDR(mDraw->GetDR(mDisplayTexture.get()));
}

} // namespace FluidSim
