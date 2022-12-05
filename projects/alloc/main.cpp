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

#include "ppx/ppx.h"
using namespace ppx;

#if defined(USE_DX12)
const grfx::Api kApi = grfx::API_DX_12_0;
#elif defined(USE_VK)
const grfx::Api kApi = grfx::API_VK_1_1;
#endif

class ProjApp
    : public ppx::Application
{
public:
    virtual void Config(ppx::ApplicationSettings& settings) override;
    virtual void Setup() override;
    virtual void Render() override;

private:
    Result                          TryAllocateRange(uint32_t rangeStart, uint32_t rangeEnd, uint32_t usageFlags);
    ppx::grfx::PipelineInterfacePtr mPipelineInterface;
    ppx::grfx::GraphicsPipelinePtr  mPipeline;
    ppx::grfx::BufferPtr            mVertexBuffer;
};

void ProjApp::Config(ppx::ApplicationSettings& settings)
{
    settings.appName          = "alloc";
    settings.grfx.api         = kApi;
    settings.grfx.enableDebug = false;
#if defined(USE_DXIL)
    settings.grfx.enableDXIL = true;
#endif
}

static void YayOrNay(uint32_t first, uint32_t last, const char* status)
{
    if (first == last) {
        fprintf(stderr, "Size %d %s.\n", first, status);
    }
    else {
        fprintf(stderr, "Sizes %d through %d %s.\n", first, last, status);
    }
}

static void PrintRange(bool state, uint32_t first, uint32_t last)
{
    if (state) {
        YayOrNay(first, last, "succeeded");
    }
    else {
        YayOrNay(first, last, "failed");
    }
}

Result ProjApp::TryAllocateRange(uint32_t rangeStart, uint32_t rangeEnd, uint32_t usageFlags)
{
    bool     state;
    uint32_t first;
    uint32_t last;
    for (uint32_t i = rangeStart; i <= rangeEnd; i *= 2) {
        grfx::BufferCreateInfo bufferCreateInfo = {};
        bufferCreateInfo.size                   = i;
        bufferCreateInfo.usageFlags.flags       = usageFlags;
        bufferCreateInfo.memoryUsage            = grfx::MEMORY_USAGE_CPU_TO_GPU;

        // We allocate a buffer of size i and see if it succeeds. If there's
        // a change, we print the range of unchanging success/failure runs
        // that came before.
        ppx::grfx::BufferPtr buffer;
        Result               ppxres     = GetDevice()->CreateBuffer(&bufferCreateInfo, &buffer);
        bool                 didSucceed = (ppxres == ppx::SUCCESS) ? true : false;
        if (i == rangeStart) {
            state = didSucceed;
            first = i;
            continue;
        }
        if (state != didSucceed) {
            PrintRange(state, first, last);
            first = i;
        }
        state = didSucceed;
        last  = i;

        if (buffer) {
            GetDevice()->DestroyBuffer(buffer);
        }
    }
    PrintRange(state, first, last);
    return ppx::SUCCESS;
}

struct Range
{
    uint32_t start;
    uint32_t end;
};

void ProjApp::Setup()
{
    grfx::BufferUsageFlags usageFlags;

    Range range = {PPX_MINIMUM_UNIFORM_BUFFER_SIZE, 256 * 1024 * 1024};
    fprintf(stderr, "Trying uniform buffer allocations in [%d, %d] in powers of 2.\n", range.start, range.end);
    usageFlags.bits.uniformBuffer = 1;
    TryAllocateRange(range.start, range.end, usageFlags);

    range            = {4, 256};
    usageFlags.flags = 0;
    fprintf(stderr, "Trying storage texel buffer allocations in [%d, %d] in powers of 2.\n", range.start, range.end);
    usageFlags.bits.storageTexelBuffer = 1;
    TryAllocateRange(range.start, range.end, usageFlags);
    usageFlags.flags = 0;
    fprintf(stderr, "Trying storage buffer allocations in [%d, %d] in powers of 2.\n", range.start, range.end);
    usageFlags.bits.rawStorageBuffer = 1;
    TryAllocateRange(range.start, range.end, usageFlags);
    usageFlags.flags = 0;
    fprintf(stderr, "Trying uniform texel buffer allocations in [%d, %d] in powers of 2.\n", range.start, range.end);
    usageFlags.bits.uniformTexelBuffer = 1;
    TryAllocateRange(range.start, range.end, usageFlags);

    Quit();
}

void ProjApp::Render()
{
}

SETUP_APPLICATION(ProjApp)