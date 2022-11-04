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

#include <iostream>
#include <string>

#include "ppx/grfx/dx/d3dcompile_util.h"
#include "ppx/fs.h"

namespace ppx {
namespace grfx {
namespace dx {

namespace {
static std::string LoadHlslFile(const std::filesystem::path& baseDir, const std::string& baseName)
{
    std::filesystem::path filePath = baseDir / (baseName + ".hlsl");
    auto                  result   = fs::load_file(filePath);
    if (!result.has_value()) {
        PPX_ASSERT_MSG(false, "HLSL file not found: " << filePath);
    }
    std::vector<char> hlslCode = result.value();
    return std::string(hlslCode.data(), hlslCode.size());
}

static const char* EntryPoint(const char* shaderModel)
{
    switch (shaderModel[0]) {
        case 'v':
            return "vsmain";
        case 'p':
            return "psmain";
        case 'c':
            return "csmain";
    }
    return nullptr;
}

} // namespace

std::vector<char> CompileShader(const std::filesystem::path& baseDir, const std::string& baseName, const char* shaderModel, ShaderIncludeHandler* includeHandler)
{
    D3D_SHADER_MACRO defines[2] = {
        {"PPX_D3D11", "1"},
        {nullptr, nullptr}};
    ID3DBlob*         spirv        = nullptr;
    ID3DBlob*         errorMessage = nullptr;
    std::vector<char> spirvCode;

    auto hlslCode = LoadHlslFile(baseDir, baseName);

    HRESULT hr = D3DCompile(reinterpret_cast<LPCVOID>(hlslCode.data()), hlslCode.size(), baseName.data(), defines, includeHandler, EntryPoint(shaderModel), shaderModel, D3DCOMPILE_OPTIMIZATION_LEVEL3, 0, &spirv, &errorMessage);
    if (errorMessage) {
        std::cerr << static_cast<char*>(errorMessage->GetBufferPointer())
                  << std::endl;
        errorMessage->Release();
        if (spirv)
            spirv->Release();
        PPX_ASSERT_MSG(false, "D3DCompile failed");
        return spirvCode;
    }

    PPX_ASSERT_MSG(hr == S_OK, "D3DCompile failed");

    spirvCode.resize(spirv->GetBufferSize());
    memcpy(spirvCode.data(), spirv->GetBufferPointer(), spirvCode.size());
    spirv->Release();
    if (errorMessage)
        errorMessage->Release();
    return spirvCode;
}

} // namespace dx
} // namespace grfx
} // namespace ppx
