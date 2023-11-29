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
#include "ppx/string_util.h"

#include <cstring>

namespace ppx {

// Spacing:
//
// --flag_name <params>    description...
//                         continued description...
// |kUsageMsgIndentWidth--|
// |kUsageMsgTotalWidth-----------------------------|
constexpr size_t kUsageMsgIndentWidth = 20;
constexpr size_t kUsageMsgTotalWidth  = 80;

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
    : Knob(flagName, true), mValue(defaultValue), mDefaultValue(defaultValue)
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

std::string KnobCheckbox::ValueString()
{
    return ppx::string_util::ToString(mValue);
}

void KnobCheckbox::UpdateFromFlags(const CliOptions& opts)
{
    SetDefaultAndValue(opts.GetOptionValueOrDefault(mFlagName, mValue));
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
        for (size_t i = 0; i < knobPtr->mIndent; i++) {
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
    std::string usageMsg = "\nFlags:\n";
    for (const auto& knobPtr : mKnobs) {
        std::string knobMsg = "--" + knobPtr->mFlagName;
        if (knobPtr->mFlagParameters != "") {
            knobMsg += " " + knobPtr->mFlagParameters;
        }
        knobMsg += "\n";
        std::string knobDefault = "(Default: " + knobPtr->ValueString() + ")";
        knobMsg += ppx::string_util::WrapText(knobDefault, kUsageMsgTotalWidth, kUsageMsgIndentWidth);
        if (knobPtr->mFlagDescription != "") {
            knobMsg += ppx::string_util::WrapText(knobPtr->mFlagDescription, kUsageMsgTotalWidth, kUsageMsgIndentWidth);
        }
        usageMsg += knobMsg + "\n";
    }
    return usageMsg;
}

void KnobManager::UpdateFromFlags(const CliOptions& opts)
{
    for (auto& knobPtr : mKnobs) {
        knobPtr->UpdateFromFlags(opts);
        PPX_LOG_INFO("KNOB: " << knobPtr->mFlagName << " : (" << knobPtr->ValueString() << ")");
    }
}

void KnobManager::RegisterKnob(const std::string& flagName, std::shared_ptr<Knob> newKnob)
{
    mFlagNames.insert(flagName);
    mKnobs.emplace_back(std::move(newKnob));
}

} // namespace ppx
