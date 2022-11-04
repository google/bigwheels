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

#ifndef ppx_grfx_dx12_render_pass_h
#define ppx_grfx_dx12_render_pass_h

#include "ppx/grfx/dx12/dx12_config.h"
#include "ppx/grfx/grfx_render_pass.h"

namespace ppx {
namespace grfx {
namespace dx12 {

class RenderPass
    : public grfx::RenderPass
{
public:
    RenderPass() {}
    virtual ~RenderPass() {}

protected:
    virtual Result CreateApiObjects(const grfx::internal::RenderPassCreateInfo* pCreateInfo) override;
    virtual void   DestroyApiObjects() override;
};

} // namespace dx12
} // namespace grfx
} // namespace ppx

#endif // ppx_grfx_dx12_render_pass_h
