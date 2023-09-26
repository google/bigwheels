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

#include <algorithm>
#include <cmath>

namespace FluidSim {

// In a normal game, animations are linked to the frame delta-time to make then run
// as a fixed perceptible speed. For our use-case (benchmarking), determinism is important.
// Targeting 60 images per second.
constexpr float kFrameDeltaTime = 1.f / 60.f;

// Color formats used by textures.
const ppx::grfx::Format kR    = ppx::grfx::FORMAT_R16_FLOAT;
const ppx::grfx::Format kRG   = ppx::grfx::FORMAT_R16G16_FLOAT;
const ppx::grfx::Format kRGBA = ppx::grfx::FORMAT_R16G16B16A16_FLOAT;

ppx::Result FluidSimulation::Create(ppx::Application* app, ppx::grfx::DevicePtr device, ppx::uint2 resolution, const SimulationConfig& config, std::unique_ptr<FluidSimulation>* pSim)
{
    *pSim = std::make_unique<FluidSimulation>(app, device, resolution, config);
    return (*pSim)->Initialize();
}

ppx::Result FluidSimulation::Initialize()
{
    // Create descriptor pool shared by all pipelines.
    ppx::grfx::DescriptorPoolCreateInfo dpci = {};
    dpci.sampler                             = 1024;
    dpci.sampledImage                        = 1024;
    dpci.uniformBuffer                       = 1024;
    dpci.storageImage                        = 1024;
    PPX_CHECKED_CALL(GetDevice()->CreateDescriptorPool(&dpci, &mDescriptorPool));

    // Frame synchronization data.
    PerFrame                       frame = {};
    ppx::grfx::SemaphoreCreateInfo sci   = {};
    ppx::grfx::FenceCreateInfo     fci   = {};
    PPX_CHECKED_CALL(GetApp()->GetGraphicsQueue()->CreateCommandBuffer(&frame.cmd));
    PPX_CHECKED_CALL(GetDevice()->CreateSemaphore(&sci, &frame.imageAcquiredSemaphore));
    PPX_CHECKED_CALL(GetDevice()->CreateFence(&fci, &frame.imageAcquiredFence));
    PPX_CHECKED_CALL(GetDevice()->CreateSemaphore(&sci, &frame.renderCompleteSemaphore));
    fci = {true}; // Create signaled
    PPX_CHECKED_CALL(GetDevice()->CreateFence(&fci, &frame.renderCompleteFence));
    mPerFrame.push_back(frame);

    // Set up all the filters to use.
    InitComputeShaders();

    // Set up draw program to emit computed textures to the swapchain.
    InitGraphicsShaders();

    // Initialize textures
    InitTextures();
    return ppx::Result();
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
    lci.bindings.push_back(ppx::grfx::DescriptorBinding(12, ppx::grfx::DESCRIPTOR_TYPE_SAMPLER));
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
    PPX_CHECKED_CALL(GetApp()->GetDevice()->CreateSampler(&sci, &mCompute.mClampSampler));

    sci              = {};
    sci.magFilter    = ppx::grfx::FILTER_LINEAR;
    sci.minFilter    = ppx::grfx::FILTER_LINEAR;
    sci.mipmapMode   = ppx::grfx::SAMPLER_MIPMAP_MODE_NEAREST;
    sci.addressModeU = ppx::grfx::SAMPLER_ADDRESS_MODE_REPEAT;
    sci.addressModeV = ppx::grfx::SAMPLER_ADDRESS_MODE_REPEAT;
    sci.addressModeW = ppx::grfx::SAMPLER_ADDRESS_MODE_REPEAT;
    sci.minLod       = 0.0f;
    sci.maxLod       = FLT_MAX;
    PPX_CHECKED_CALL(GetApp()->GetDevice()->CreateSampler(&sci, &mCompute.mRepeatSampler));

    // Create compute shaders for filtering.
    mAdvection         = std::make_unique<AdvectionShader>(this);
    mBloomBlur         = std::make_unique<BloomBlurShader>(this);
    mBloomBlurAdditive = std::make_unique<BloomBlurAdditiveShader>(this);
    mBloomFinal        = std::make_unique<BloomFinalShader>(this);
    mBloomPrefilter    = std::make_unique<BloomPrefilterShader>(this);
    mBlur              = std::make_unique<BlurShader>(this);
    mCheckerboard      = std::make_unique<CheckerboardShader>(this);
    mClear             = std::make_unique<ClearShader>(this);
    mColor             = std::make_unique<ColorShader>(this);
    mCurl              = std::make_unique<CurlShader>(this);
    mDisplay           = std::make_unique<DisplayShader>(this);
    mDivergence        = std::make_unique<DivergenceShader>(this);
    mGradientSubtract  = std::make_unique<GradientSubtractShader>(this);
    mPressure          = std::make_unique<PressureShader>(this);
    mSplat             = std::make_unique<SplatShader>(this);
    mSunraysMask       = std::make_unique<SunraysMaskShader>(this);
    mSunrays           = std::make_unique<SunraysShader>(this);
    mVorticity         = std::make_unique<VorticityShader>(this);
}

void FluidSimulation::DispatchComputeShaders(const PerFrame& frame)
{
    for (auto& dr : mComputeDispatchQueue) {
        dr->mShader->Dispatch(frame, dr);
    }
}

void FluidSimulation::FreeComputeShaderResources()
{
    // Wait for any command buffers in-flight before freeing up resources.
    GetApp()->GetDevice()->WaitIdle();
    for (auto& shader : mComputeDispatchQueue) {
        shader->FreeResources();
    }
    mComputeDispatchQueue.clear();
}

void FluidSimulation::DispatchGraphicsShaders(const PerFrame& frame)
{
    for (auto& dr : mGraphicsDispatchQueue) {
        dr->mShader->Dispatch(frame, dr);
    }
}

void FluidSimulation::FreeGraphicsShaderResources()
{
    // Wait for any command buffers in-flight before freeing up resources.
    GetApp()->GetDevice()->WaitIdle();
    for (auto& shader : mGraphicsDispatchQueue) {
        shader->FreeResources();
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

    ppx::grfx::SamplerCreateInfo sci = {};
    sci.magFilter                    = ppx::grfx::FILTER_LINEAR;
    sci.minFilter                    = ppx::grfx::FILTER_LINEAR;
    PPX_CHECKED_CALL(GetApp()->GetDevice()->CreateSampler(&sci, &mGraphics.mSampler));

    ppx::grfx::PipelineInterfaceCreateInfo pici = {};
    pici.setCount                               = 1;
    pici.sets[0].set                            = 0;
    pici.sets[0].pLayout                        = mGraphics.mDescriptorSetLayout;
    PPX_CHECKED_CALL(GetApp()->GetDevice()->CreatePipelineInterface(&pici, &mGraphics.mPipelineInterface));

    mDraw = std::make_unique<GraphicsShader>(this);
}

ppx::uint2 FluidSimulation::GetResolution(uint32_t resolution)
{
    float aspectRatio = GetResolutionAspect();
    if (aspectRatio < 1.0f) {
        aspectRatio = 1.0f / aspectRatio;
    }

    uint32_t min = resolution;
    uint32_t max = static_cast<uint32_t>(std::round(static_cast<float>(resolution) * aspectRatio));

    return (mResolution.x > mResolution.y) ? ppx::uint2(max, min) : ppx::uint2(min, max);
}

void FluidSimulation::InitTextures()
{
    ppx::int2 simRes = GetResolution(GetConfig().pSimResolution->GetValue());
    ppx::int2 dyeRes = GetResolution(GetConfig().pDyeResolution->GetValue());

    // Generate all the textures.
    mCheckerboardTexture = std::make_unique<Texture>(this, "checkerboard", mResolution.x, mResolution.y, GetApp()->GetSwapchain()->GetColorFormat());
    mCurlTexture         = std::make_unique<Texture>(this, "curl", simRes.x, simRes.y, kR);
    mDivergenceTexture   = std::make_unique<Texture>(this, "divergence", simRes.x, simRes.y, kR);
    mDisplayTexture      = std::make_unique<Texture>(this, "display", mResolution.x, mResolution.y, kRGBA);
    mDitheringTexture    = std::make_unique<Texture>(this, "fluid_simulation/textures/LDR_LLL1_0.png");
    mDrawColorTexture    = std::make_unique<Texture>(this, "draw color", mResolution.x, mResolution.y, kRGBA);
    mDyeTexture[0]       = std::make_unique<Texture>(this, "dye[0]", dyeRes.x, dyeRes.y, kRGBA);
    mDyeTexture[1]       = std::make_unique<Texture>(this, "dye[1]", dyeRes.x, dyeRes.y, kRGBA);
    mPressureTexture[0]  = std::make_unique<Texture>(this, "pressure[0]", simRes.x, simRes.y, kR);
    mPressureTexture[1]  = std::make_unique<Texture>(this, "pressure[1]", simRes.x, simRes.y, kR);
    mVelocityTexture[0]  = std::make_unique<Texture>(this, "velocity[0]", simRes.x, simRes.y, kRG);
    mVelocityTexture[1]  = std::make_unique<Texture>(this, "velocity[1]", simRes.x, simRes.y, kRG);

    InitBloomTextures();
    InitSunraysTextures();
}

void FluidSimulation::InitBloomTextures()
{
    ppx::int2 res = GetResolution(GetConfig().pBloomResolution->GetValue());
    mBloomTexture = std::make_unique<Texture>(this, "bloom", res.x, res.y, kRGBA);
    PPX_ASSERT_MSG(mBloomTextures.empty(), "Bloom textures already initialized");
    for (int i = 0; i < GetConfig().pBloomIterations->GetValue(); i++) {
        uint32_t width  = res.x >> (i + 1);
        uint32_t height = res.y >> (i + 1);
        if (width < 2 || height < 2)
            break;
        mBloomTextures.emplace_back(std::make_unique<Texture>(this, std::string("bloom frame buffer[") + std::to_string(i) + "]", width, height, kRGBA));
    }
}

void FluidSimulation::InitSunraysTextures()
{
    ppx::int2 res       = GetResolution(GetConfig().pSunraysResolution->GetValue());
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
        ScheduleDR(mColor->GetDR(texture, ppx::float4(0.0f, 0.0f, 0.0f, 0.0f)));
    }
    mTexturesToInitialize.clear();

    MultipleSplats(GetConfig().pNumSplats->GetValue());
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
    ppx::float3 c = HSVtoRGB(ppx::float3(Random().Float(), 1, 1));
    c.r *= 0.15f;
    c.g *= 0.15f;
    c.b *= 0.15f;
    return c;
}

float FluidSimulation::CorrectRadius(float radius)
{
    float aspectRatio = GetResolutionAspect();
    return (aspectRatio > 1) ? radius * aspectRatio : radius;
}

void FluidSimulation::Splat(ppx::float2 point, ppx::float2 delta, ppx::float3 color)
{
    float       aspect     = GetResolutionAspect();
    float       radius     = CorrectRadius(GetConfig().pSplatRadius->GetValue() / 100.0f);
    ppx::float4 deltaColor = ppx::float4(delta.x, delta.y, 0.0f, 1.0f);
    ScheduleDR(mSplat->GetDR(mVelocityTexture[0].get(), mVelocityTexture[1].get(), point, aspect, radius, deltaColor));
    std::swap(mVelocityTexture[0], mVelocityTexture[1]);

    ScheduleDR(mSplat->GetDR(mDyeTexture[0].get(), mDyeTexture[1].get(), point, aspect, radius, ppx::float4(color, 1.0f)));
    std::swap(mDyeTexture[0], mDyeTexture[1]);
}

void FluidSimulation::MultipleSplats(uint32_t amount)
{
    // Emit a random number of splats if the stated amount is 0.
    if (amount == 0) {
        amount = Random().UInt32() % 20 + 5;
    }

    PPX_LOG_DEBUG("Emitting " << amount << " splashes of color\n");
    for (uint32_t i = 0; i < amount; i++) {
        ppx::float3 color = GenerateColor();
        color.r *= 10.0f;
        color.g *= 10.0f;
        color.b *= 10.0f;
        ppx::float2 point(Random().Float(), Random().Float());
        ppx::float2 delta(1000.0f * (Random().Float() - 0.5f), 1000.0f * (Random().Float() - 0.5f));
        PPX_LOG_DEBUG("Splash #" << i << " at " << point << " with color " << color << "\n");
        Splat(point, delta, color);
    }
}

void FluidSimulation::Render()
{
    if (GetConfig().pEnableBloom->GetValue()) {
        ApplyBloom(mDyeTexture[0].get(), mBloomTexture.get());
    }

    if (GetConfig().pEnableSunrays->GetValue()) {
        ApplySunrays(mDyeTexture[0].get(), mDyeTexture[1].get(), mSunraysTexture.get());
        Blur(mSunraysTexture.get(), mSunraysTempTexture.get(), 1);
    }

    DrawDisplay();

    if (GetApp()->GetSettings()->grfx.enableDebug) {
        DrawTextures();
    }
}

void FluidSimulation::ApplyBloom(Texture* source, Texture* destination)
{
    if (mBloomTextures.size() < 2)
        return;

    Texture* last = destination;

    float knee   = GetConfig().pBloomThreshold->GetValue() * GetConfig().pBloomSoftKnee->GetValue() + 0.0001f;
    float curve0 = GetConfig().pBloomThreshold->GetValue() - knee;
    float curve1 = knee * 2.0f;
    float curve2 = 0.25f / knee;
    ScheduleDR(mBloomPrefilter->GetDR(source, last, ppx::float3(curve0, curve1, curve2), GetConfig().pBloomThreshold->GetValue()));

    for (auto& dest : mBloomTextures) {
        ScheduleDR(mBloomBlur->GetDR(last, dest.get(), last->GetTexelSize()));
        last = dest.get();
    }

    for (int i = static_cast<int>(mBloomTextures.size() - 2); i >= 0; i--) {
        Texture* baseTex = mBloomTextures[i].get();
        ScheduleDR(mBloomBlurAdditive->GetDR(last, baseTex, last->GetTexelSize()));
        last = baseTex;
    }

    ScheduleDR(mBloomFinal->GetDR(last, destination, last->GetTexelSize(), GetConfig().pBloomIntensity->GetValue()));
}

void FluidSimulation::ApplySunrays(Texture* source, Texture* mask, Texture* destination)
{
    ScheduleDR(mSunraysMask->GetDR(source, mask));
    ScheduleDR(mSunrays->GetDR(mask, destination, GetConfig().pSunraysWeight->GetValue()));
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
    ScheduleDR(mDraw->GetDR(mDrawColorTexture.get(), ppx::float2(-1.0f, 1.0f)));
}

void FluidSimulation::DrawCheckerboard()
{
    ScheduleDR(mCheckerboard->GetDR(mCheckerboardTexture.get(), GetResolutionAspect()));
    ScheduleDR(mDraw->GetDR(mCheckerboardTexture.get(), ppx::float2(-1.0f, 1.0f)));
}

void FluidSimulation::DrawDisplay()
{
    ppx::float2 texelSize   = ppx::float2(1.0f / mResolution.x, 1.0f / mResolution.y);
    ppx::float2 ditherScale = mDitheringTexture->GetDitherScale(mResolution.x, mResolution.y);
    DrawColor(NormalizeColor(GetConfig().backColor));
    ScheduleDR(mDisplay->GetDR(mDyeTexture[0].get(), mBloomTexture.get(), mSunraysTexture.get(), mDitheringTexture.get(), mDisplayTexture.get(), texelSize, ditherScale));
    ScheduleDR(mDraw->GetDR(mDisplayTexture.get(), ppx::float2(-1.0f, 1.0f)));
}

void FluidSimulation::DrawTextures()
{
    std::vector<Texture*> v = {
        mBloomTexture.get(),
        mCurlTexture.get(),
        mDivergenceTexture.get(),
        mPressureTexture[0].get(),
        mPressureTexture[1].get(),
        mVelocityTexture[0].get(),
        mVelocityTexture[1].get(),
    };
    auto  coord   = ppx::float2(-1.0f, 1.0f);
    float maxDimY = 0.0f;
    for (const auto& t : v) {
        auto dim = t->GetNormalizedSize();
        if (coord.x + dim.x >= 1.0) {
            coord.x = -1.0;
            coord.y -= maxDimY;
            maxDimY = 0.0f;
        }
        PPX_LOG_DEBUG("Scheduling texture draw for " << t->GetName() << " with normalized dimensions " << t->GetNormalizedSize() << " at coordinate " << coord << "\n");
        ScheduleDR(mDraw->GetDR(t, coord));
        coord.x += dim.x + 0.005f;
        if (dim.y > maxDimY) {
            maxDimY = dim.y + 0.005f;
        }
    }
}

void FluidSimulation::Update()
{
    // If the marble has been selected, move it around and drop it at random.
    if (GetConfig().pEnableMarble->GetValue()) {
        MoveMarble();

        // Update the color of the marble.
        if (mRandom.Float() <= GetConfig().pColorUpdateFrequency->GetValue()) {
            mMarble.color = GenerateColor();
        }

        // Drop the marble at random.
        if (mRandom.Float() <= GetConfig().pMarbleDropFrequency->GetValue()) {
            ppx::float2 delta = mMarble.delta * GetConfig().pSplatForce->GetValue();
            Splat(mMarble.coord, delta, mMarble.color);
        }
    }

    // Queue up some splats at random. But limit the amount of outstanding splats so it doesn't get too busy.
    if (Random().Float() <= GetConfig().pSplatFrequency->GetValue()) {
        MultipleSplats(1);
    }

    Step(kFrameDeltaTime);
    Render();
}

void FluidSimulation::MoveMarble()
{
    // Move the marble so that it bounces off of the window borders.
    mMarble.coord += mMarble.delta;

    if (mMarble.coord.x < 0.0f) {
        mMarble.coord.x = 0.0f;
        mMarble.delta.x *= -1.0f;
    }
    if (mMarble.coord.x > 1.0f) {
        mMarble.coord.x = 1.0f;
        mMarble.delta.x *= -1.0f;
    }
    if (mMarble.coord.y < 0.0f) {
        mMarble.coord.y = 0.0f;
        mMarble.delta.y *= -1.0f;
    }
    if (mMarble.coord.y > 1.0f) {
        mMarble.coord.y = 1.0f;
        mMarble.delta.y *= -1.0f;
    }
}

void FluidSimulation::Step(float delta)
{
    ppx::float2 texelSize = mVelocityTexture[0]->GetTexelSize();

    ScheduleDR(mCurl->GetDR(mVelocityTexture[0].get(), mCurlTexture.get(), texelSize));

    ScheduleDR(mVorticity->GetDR(mVelocityTexture[0].get(), mCurlTexture.get(), mVelocityTexture[1].get(), texelSize, GetConfig().pCurl->GetValue(), delta));
    std::swap(mVelocityTexture[0], mVelocityTexture[1]);

    ScheduleDR(mDivergence->GetDR(mVelocityTexture[0].get(), mDivergenceTexture.get(), texelSize));

    ScheduleDR(mClear->GetDR(mPressureTexture[0].get(), mPressureTexture[1].get(), GetConfig().pPressure->GetValue()));
    std::swap(mPressureTexture[0], mPressureTexture[1]);

    for (int i = 0; i < GetConfig().pPressureIterations->GetValue(); ++i) {
        ScheduleDR(mPressure->GetDR(mPressureTexture[0].get(), mDivergenceTexture.get(), mPressureTexture[1].get(), texelSize));
        std::swap(mPressureTexture[0], mPressureTexture[1]);
    }

    ScheduleDR(mGradientSubtract->GetDR(mPressureTexture[0].get(), mVelocityTexture[0].get(), mVelocityTexture[1].get(), texelSize));
    std::swap(mVelocityTexture[0], mVelocityTexture[1]);

    ScheduleDR(mAdvection->GetDR(mVelocityTexture[0].get(), mVelocityTexture[0].get(), mVelocityTexture[1].get(), delta, GetConfig().pVelocityDissipation->GetValue(), texelSize, texelSize));
    std::swap(mVelocityTexture[0], mVelocityTexture[1]);

    ScheduleDR(mAdvection->GetDR(mVelocityTexture[0].get(), mDyeTexture[0].get(), mDyeTexture[1].get(), delta, GetConfig().pDensityDissipation->GetValue(), texelSize, mDyeTexture[0]->GetTexelSize()));
    std::swap(mDyeTexture[0], mDyeTexture[1]);
}

} // namespace FluidSim
