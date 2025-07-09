// Copyright 2024 Google LLC
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

#ifndef FOVEATIONBENCHMARKAPP_H
#define FOVEATIONBENCHMARKAPP_H

#include "ppx/grfx/grfx_config.h"
#include "ppx/grfx/grfx_shading_rate_util.h"
#include "ppx/knob.h"
#include "ppx/math_config.h"
#include "ppx/ppx.h"
#include "ppx/ppm_export.h"
#include "ppx/util.h"

using namespace ppx;

const grfx::Api kApi = grfx::API_VK_1_1;

// Patterns that can be rendered.
enum class RenderPattern
{
    // Clear the render target, but don't do any draws.
    CLEAR_ONLY,

    // Constant color over the entire screen.
    //
    // Still runs the shader to compute a noise value, but the value gets
    // scaled down so it rounds to 0.
    //
    // If showFragmentSize is enabled, only the R channel will be constant, and
    // the G and B channel will show the fragment size.
    CONSTANT,

    // Pseudo-random noise that stays the same every frame.
    //
    // If showFragmentSize is enabled, only the R channel will be noise, and
    // the G and B channel will show the fragment size.
    STATIC_NOISE,

    // Pseudo-random noise that changes every frame.
    //
    // If showFragmentSize is enabled, only the R channel will be noise, and
    // the G and B channel will show the fragment size.
    DYNAMIC_NOISE,
};

enum class FoveationPattern
{
    // Render the entire image at full fragment density.
    UNIFORM_1X1,

    // Render the entire image at with fragments of 2x2 pixels.
    UNIFORM_2X2,

    // Render the entire image at with fragments of 4x4 pixels.
    UNIFORM_4X4,

    // Render the center of the screen at full density, with lower density
    // toward the edges. All requested fragment sizes will be square (but the
    // driver may change this).
    RADIAL,

    // Render the the center of the screen at full density, with the fragments
    // becoming wider further from the center in the X direction, and the
    // fragments becoming taller further from the center in the Y direction.
    ANISOTROPIC,
};

constexpr grfx::ShadingRateMode kDefaultShadingRateMode       = grfx::SHADING_RATE_NONE;
constexpr RenderPattern         kDefaultRenderPattern         = RenderPattern::DYNAMIC_NOISE;
constexpr FoveationPattern      kDefaultFoveationPattern      = FoveationPattern::RADIAL;
constexpr bool                  kDefaultEnableSubsampledImage = false;
constexpr bool                  kDefaultShowFragmentSize      = false;
constexpr uint32_t              kDefaultWidth                 = 1920;
constexpr uint32_t              kDefaultHeight                = 1080;

// Struct corresponding to the Params struct in the FoveationBenchmark shaders.
struct ShaderParams
{
    union
    {
        struct
        {
            // Seed for pseudo-random noise generator.
            // This is combined with the fragment location to generate the color.
            uint32_t seed;

            // Number of additional hash computations to perform for each fragment.
            uint32_t extraHashRounds;
        };
        // _vec0 ensures noiseWeights does not cross a 16-byte boundary, to
        // match how the shader struct is aligned.
        uint32_t _vec0[4];
    };

    union
    {
        // Per-channel weights of the noise in the output color.
        float3 noiseWeights;

        // _vec1 ensures color does not cross a 16-byte boundary, to
        // match how the shader struct is aligned.
        uint32_t _vec1[4];
    };

    // Color to mix with the noise.
    float3 color;
};

static_assert(sizeof(ShaderParams) == 11 * sizeof(uint32_t));
static_assert(offsetof(ShaderParams, seed) == 0 * sizeof(uint32_t));
static_assert(offsetof(ShaderParams, extraHashRounds) == 1 * sizeof(uint32_t));
static_assert(offsetof(ShaderParams, noiseWeights) == 4 * sizeof(uint32_t));
static_assert(offsetof(ShaderParams, color) == 8 * sizeof(uint32_t));

class FoveationBenchmarkApp
    : public ppx::Application
{
public:
    FoveationBenchmarkApp() {}
    void InitKnobs() override;
    void Config(ppx::ApplicationSettings& settings) override;
    void Setup() override;
    void Render() override;

private:
    void SetupSync();
    void SetupRender();
    void SetupPost();
    void RecordRenderCommands();
    void UpdateRenderShaderParams();
    void RecordPostCommands(uint32_t imageIndex);

    struct
    {
        std::shared_ptr<KnobFlag<std::string>>         pRenderPattern;
        std::shared_ptr<KnobFlag<std::string>>         pFoveationPattern;
        std::shared_ptr<KnobFlag<bool>>                pSubsampledImage;
        std::shared_ptr<KnobFlag<std::pair<int, int>>> pRenderResolution;
        std::shared_ptr<KnobFlag<std::pair<int, int>>> pPostResolution;
        std::shared_ptr<KnobFlag<bool>>                pShowFragmentSize;
        std::shared_ptr<KnobFlag<std::string>>         pRenderScreenshotPath;
        std::shared_ptr<KnobFlag<std::string>>         pPostScreenshotPath;
        std::shared_ptr<KnobFlag<int>>                 pExtraHashRounds;
        std::shared_ptr<KnobFlag<int>>                 pSampleCount;
    } mKnobs;

    struct
    {
        grfx::FencePtr     imageAcquiredFence;
        grfx::SemaphorePtr imageAcquiredSemaphore;
        grfx::SemaphorePtr renderCompleteSemaphore;
        grfx::SemaphorePtr postCompleteSemaphore;
        grfx::FencePtr     postCompleteFence;
    } mSync;

    struct
    {
        uint32_t              width;
        uint32_t              height;
        RenderPattern         renderPattern;
        grfx::ShadingRateMode shadingRateMode;
        FoveationPattern      foveationPattern;
        bool                  subsampledImage;
        bool                  showFragmentSize;
        uint32_t              extraHashRounds;
        grfx::SampleCount     sampleCount;

        grfx::DescriptorPoolPtr      descriptorPool;
        grfx::DescriptorSetLayoutPtr descriptorSetLayout;
        grfx::DescriptorSetPtr       descriptorSet;
        grfx::BufferPtr              uniformBuffer;
        grfx::CommandBufferPtr       cmd;
        grfx::PipelineInterfacePtr   pipelineInterface;
        grfx::GraphicsPipelinePtr    pipeline;
        grfx::DrawPassPtr            drawPass;
        grfx::ShadingRatePatternPtr  shadingRatePattern;
    } mRender;

    struct
    {
        uint32_t width;
        uint32_t height;

        grfx::CommandBufferPtr       cmd;
        grfx::FullscreenQuadPtr      fullscreenQuad;
        grfx::DescriptorPoolPtr      descriptorPool;
        grfx::DescriptorSetLayoutPtr descriptorSetLayout;
        grfx::DescriptorSetPtr       descriptorSet;
        grfx::SamplerPtr             sampler;
        grfx::DrawPassPtr            drawPass;
    } mPost;
};

#endif // FOVEATIONBENCHMARKAPP_H
