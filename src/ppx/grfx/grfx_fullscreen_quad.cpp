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

#include "ppx/grfx/grfx_fullscreen_quad.h"
#include "ppx/grfx/grfx_device.h"
#include "ppx/grfx/grfx_pipeline.h"

namespace ppx {
namespace grfx {

Result FullscreenQuad::CreateApiObjects(const grfx::FullscreenQuadCreateInfo* pCreateInfo)
{
    PPX_ASSERT_NULL_ARG(pCreateInfo);

    Result ppxres = ppx::ERROR_FAILED;

    // Pipeline interface
    {
        grfx::PipelineInterfaceCreateInfo createInfo = {};
        createInfo.setCount                          = pCreateInfo->setCount;
        for (uint32_t i = 0; i < createInfo.setCount; ++i) {
            createInfo.sets[i].set     = pCreateInfo->sets[i].set;
            createInfo.sets[i].pLayout = pCreateInfo->sets[i].pLayout;
        }

        ppxres = GetDevice()->CreatePipelineInterface(&createInfo, &mPipelineInterface);
        if (Failed(ppxres)) {
            PPX_ASSERT_MSG(false, "failed creating pipeline interface");
            return ppxres;
        }
    }

    // Pipeline
    {
        grfx::GraphicsPipelineCreateInfo2 createInfo = {};
        createInfo.VS                                = {pCreateInfo->VS, "vsmain"};
        createInfo.PS                                = {pCreateInfo->PS, "psmain"};
        createInfo.depthReadEnable                   = false;
        createInfo.depthWriteEnable                  = false;
        createInfo.pPipelineInterface                = mPipelineInterface;
        createInfo.outputState.depthStencilFormat    = pCreateInfo->depthStencilFormat;
        // Render target formats
        createInfo.outputState.renderTargetCount = pCreateInfo->renderTargetCount;
        for (uint32_t i = 0; i < createInfo.outputState.renderTargetCount; ++i) {
            createInfo.blendModes[i]                      = grfx::BLEND_MODE_NONE;
            createInfo.outputState.renderTargetFormats[i] = pCreateInfo->renderTargetFormats[i];
        }

        ppxres = GetDevice()->CreateGraphicsPipeline(&createInfo, &mPipeline);
        if (Failed(ppxres)) {
            PPX_ASSERT_MSG(false, "failed creating graphics pipeline");
            return ppxres;
        }
    }

    return ppx::SUCCESS;
}

void FullscreenQuad::DestroyApiObjects()
{
    if (mPipeline) {
        GetDevice()->DestroyGraphicsPipeline(mPipeline);
        mPipeline.Reset();
    }

    if (mPipelineInterface) {
        GetDevice()->DestroyPipelineInterface(mPipelineInterface);
        mPipelineInterface.Reset();
    }
}

} // namespace grfx
} // namespace ppx
