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

#ifndef KNOB_DEMO_H
#define KNOB_DEMO_H

#include "ppx/ppx.h"
#include "ppx/knob_new.h"
using namespace ppx;

static constexpr std::array<OptionKnobEntry<int>, 3> kOptionBoolA = {{
    {"red", true},
    {"orange", true},
    {"banana", false},
}};

static constexpr std::array<OptionKnobEntry<int>, 5> kOptionIntA = {{
    {"Ten", 10},
    {"Fifteen", 15},
    {"Twenty", 20},
    {"Twenty-five", 25},
    {"Thirty", 30},
}};

class KnobDemoApp
    : public Application
{
public:
    virtual void InitKnobs() override;
    virtual void Config(ApplicationSettings& settings) override;
    virtual void Setup() override;
    virtual void Render() override;
    void         ProcessKnobs();

private:
    struct PerFrame
    {
        grfx::CommandBufferPtr cmd;
        grfx::SemaphorePtr     imageAcquiredSemaphore;
        grfx::FencePtr         imageAcquiredFence;
        grfx::SemaphorePtr     renderCompleteSemaphore;
        grfx::FencePtr         renderCompleteFence;
    };

    std::vector<PerFrame>      mPerFrame;
    grfx::ShaderModulePtr      mVS;
    grfx::ShaderModulePtr      mPS;
    grfx::PipelineInterfacePtr mPipelineInterface;
    grfx::GraphicsPipelinePtr  mPipeline;
    grfx::BufferPtr            mVertexBuffer;
    grfx::VertexBinding        mVertexBinding;

    struct Knobs
    {
        std::shared_ptr<GeneralKnob<std::string>> pGeneralStringA; // PLAIN
        std::shared_ptr<GeneralKnob<bool>>        pGeneralBoolA;   // CHECKBOX
        std::shared_ptr<GeneralKnob<bool>>        pGeneralBoolB;   // CHECKBOX
        std::shared_ptr<RangeKnob<int>>           pRangeIntA;      // PLAIN
        std::shared_ptr<RangeKnob<int>>           pRangeIntB;      // SLOW_SLIDER
        std::shared_ptr<RangeKnob<int>>           pRangeIntC;      // FAST_SLIDER
        std::shared_ptr<RangeKnob<int>>           pRangeInt3A;     // 3x FAST_SLIDER
        std::shared_ptr<RangeKnob<float>>         pRangeFloatA;    // SLOW_SLIDER
        std::shared_ptr<RangeKnob<float>>         pRangeFloatB;    // FAST_SLIDER
        std::shared_ptr<RangeKnob<float>>         pRangeFloat3A;   // 3x FAST_SLIDER
        std::shared_ptr<OptionKnob<bool>>         pOptionBoolA;    // PLAIN
        std::shared_ptr<OptionKnob<int>>          pOptionIntA;     // DROPDOWN
    };

    Knobs mKnobs;
};

#endif // KNOB_DEMO_H
