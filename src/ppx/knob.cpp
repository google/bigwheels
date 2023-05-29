// Copyright 2023 Google LLC
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

#include "ppx/knob.h"

#include <cstring>

namespace ppx {

// -------------------------------------------------------------------------------------------------
// KnobManager
// -------------------------------------------------------------------------------------------------

void KnobManager::ResetAllToDefault()
{
    for (auto& knobPtr : mKnobs) {
        knobPtr->ResetToDefault();
    }
}

void KnobManager::DrawAllKnobs(bool inExistingWindow)
{
    if (!inExistingWindow)
        ImGui::Begin("Knobs");

    for (const auto& knobPtr : mKnobs) {
        for (size_t i = 0; i < knobPtr->GetIndent(); i++) {
            ImGui::Indent();
        }
        knobPtr->Draw();
        for (size_t i = 0; i < knobPtr->GetIndent(); i++) {
            ImGui::Unindent();
        }
    }

    if (ImGui::Button("Reset to Default Values")) {
        ResetAllToDefault();
    }

    if (!inExistingWindow)
        ImGui::End();
}

std::string KnobManager::GetUsageMsg()
{
    std::string usageMsg = "\nApplication-specific flags\n";
    for (const auto& knobPtr : mKnobs) {
        usageMsg += knobPtr->GetFlagHelpText();
    }
    return usageMsg;
}

void KnobManager::UpdateFromFlags(const CliOptions& opts)
{
    for (auto& knobPtr : mKnobs) {
        knobPtr->UpdateFromFlags(opts);
    }
}

void KnobManager::RegisterKnob(const std::string& flagName, std::shared_ptr<Knob> newKnob)
{
    mFlagNames.insert(flagName);
    mKnobs.emplace_back(std::move(newKnob));
    PPX_LOG_INFO("Created knob " << flagName);
}

} // namespace ppx
