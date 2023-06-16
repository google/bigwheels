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
// Knob
// -------------------------------------------------------------------------------------------------

bool Knob::DigestUpdate()
{
    if (!mUpdatedFlag) {
        return false;
    }
    mUpdatedFlag = false;
    return true;
}

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
    if (!inExistingWindow) {
        ImGui::Begin("Knobs");
    }

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

    if (!inExistingWindow) {
        ImGui::End();
    }
}

std::string KnobManager::GetUsageMsg()
{
    std::string usageMsg = "\nGeneral Flags:\n";
    // Tends to have short params, will align description
    // --flag_name <params>     description
    size_t maxFlagLength = 0;
    for (const auto& knobPtr : mKnobConstants) {
        size_t flagLength = knobPtr->GetFlagName().size() + knobPtr->GetHelpParams().size() + 2;
        if (flagLength > maxFlagLength) {
            maxFlagLength = flagLength;
        }
    }
    for (const auto& knobPtr : mKnobConstants) {
        std::string flagText = "--" + knobPtr->GetFlagName() + knobPtr->GetHelpParams();
        size_t      spaces   = maxFlagLength - flagText.size() + 2;
        for (size_t i = 0; i < spaces; i++) {
            flagText += " ";
        }
        usageMsg += flagText + knobPtr->GetFlagHelp() + "\n";
    }

    usageMsg += "\nApplication-Specific Flags:\n";
    // Tends to have longer params, do not attempt to align description
    // --flag_name <params> : description
    for (const auto& knobPtr : mKnobs) {
        usageMsg += "--" + knobPtr->GetFlagName() + knobPtr->GetHelpParams();
        if (knobPtr->GetFlagHelp() != "") {
            usageMsg += " : " + knobPtr->GetFlagHelp();
        }
        usageMsg += "\n";
    }
    return usageMsg;
}

void KnobManager::UpdateFromFlags(const CliOptions& opts)
{
    for (auto& knobPtr : mKnobConstants) {
        knobPtr->UpdateFromFlags(opts);
    }
    for (auto& knobPtr : mKnobs) {
        knobPtr->UpdateFromFlags(opts);
    }
}

void KnobManager::RegisterKnob(const std::string& flagName, std::shared_ptr<Knob> newKnob, bool isConstant)
{
    mFlagNames.insert(flagName);
    if (isConstant) {
        mKnobConstants.emplace_back(std::move(newKnob));
    }
    else {
        mKnobs.emplace_back(std::move(newKnob));
    }
}

} // namespace ppx
