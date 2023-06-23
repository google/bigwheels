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
// KnobCheckbox
// -------------------------------------------------------------------------------------------------

KnobCheckbox::KnobCheckbox(const std::string& flagName, bool defaultValue)
    : Knob(flagName, true), mDefaultValue(defaultValue), mValue(defaultValue)
{
    SetFlagParameters("<true|false>");
    RaiseUpdatedFlag();
}

void KnobCheckbox::Draw()
{
    if (!ImGui::Checkbox(mDisplayName.c_str(), &mValue)) {
        return;
    }
    RaiseUpdatedFlag();
}

void KnobCheckbox::UpdateFromFlags(const CliOptions& opts)
{
    SetDefaultAndValue(opts.GetExtraOptionValueOrDefault(mFlagName, mValue));
}

void KnobCheckbox::SetValue(bool newValue)
{
    if (newValue == mValue) {
        return;
    }
    mValue = newValue;
    RaiseUpdatedFlag();
}

void KnobCheckbox::SetDefaultAndValue(bool newValue)
{
    mDefaultValue = newValue;
    ResetToDefault();
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
        if (!knobPtr->mVisible) {
            continue;
        }
        for (size_t i = 0; i < knobPtr->mVisible; i++) {
            ImGui::Indent();
        }
        knobPtr->Draw();
        for (size_t i = 0; i < knobPtr->mIndent; i++) {
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
    std::string usageMsg = "\nApplication-Specific Flags:\n";
    // Tends to have longer params, do not attempt to align description
    // --flag_name <params> : description
    for (const auto& knobPtr : mKnobs) {
        usageMsg += "--" + knobPtr->mFlagName;
        if (knobPtr->mFlagParameters != "") {
            usageMsg += " " + knobPtr->mFlagParameters;
        }
        if (knobPtr->mFlagDescription != "") {
            usageMsg += " : " + knobPtr->mFlagDescription;
        }
        usageMsg += "\n";
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
}

} // namespace ppx
