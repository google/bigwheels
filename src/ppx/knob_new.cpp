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

#include "ppx/knob_new.h"
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
// KnobNew
// -------------------------------------------------------------------------------------------------

void KnobNew::SetStartupOnly()
{
    PPX_ASSERT_MSG(!mFinalized, "knob " + mFlagName + " cannot be made startup only, has not been finalized yet");

    mStartupOnly    = true;
    mDisplayVisible = false;
}

bool KnobNew::DigestUpdate()
{
    PPX_ASSERT_MSG(mFinalized, "knob " + mFlagName + " cannot check if updated, has not been finalized yet");

    if (!mUpdatedFlag) {
        return false;
    }
    mUpdatedFlag = false;
    return true;
}

void KnobNew::DrawPlain()
{
    std::string flagText = mDisplayName + ": " + ValueString();
    ImGui::Text("%s", flagText.c_str());
    DrawToolTip();
    return;
}

void KnobNew::DrawToolTip()
{
    if (mFlagDescription.length() == 0) {
        return;
    }

    ImGui::SameLine();
    ImGui::TextDisabled("(?)");
    if (ImGui::BeginItemTooltip()) {
        ImGui::TextUnformatted(mFlagDescription.c_str());
        ImGui::EndTooltip();
    }
}

void KnobNew::Finalize()
{
    PPX_ASSERT_MSG(!mFinalized, "knob " + mFlagName + " has been finalized already");

    mStartupDisplayVisible = mDisplayVisible;
    mStartupDisplayType    = mDisplayType;
    FinalizeValues();
    mFinalized = true;
}

void KnobNew::ResetToStartup()
{
    PPX_ASSERT_MSG(mFinalized, "knob " + mFlagName + " cannot be reset, has not been finalized yet");

    if (mFinalized && mStartupOnly) {
        return;
    }

    mDisplayVisible = mStartupDisplayVisible;
    mDisplayType    = mStartupDisplayType;
    ResetValuesToStartup();
}

std::string KnobNew::GetFlagParameters()
{
    if (mFlagParameters != "") {
        return mFlagParameters;
    }
    return GetDefaultFlagParameters();
}

// -------------------------------------------------------------------------------------------------
// KnobManagerNew
// -------------------------------------------------------------------------------------------------

void KnobManagerNew::SetAllStartupOnly()
{
    for (auto& knobPtr : mKnobs) {
        knobPtr->SetStartupOnly();
    }
}

void KnobManagerNew::FinalizeAll()
{
    for (auto& knobPtr : mKnobs) {
        knobPtr->Finalize();
    }

    mFinalized = true;
}

void KnobManagerNew::ResetAllToStartup()
{
    PPX_ASSERT_MSG(mFinalized, "cannot reset to startup before finalization");
    for (auto& knobPtr : mKnobs) {
        knobPtr->ResetToStartup();
    }
}

void KnobManagerNew::DrawAllKnobs(bool inExistingWindow)
{
    if (!inExistingWindow) {
        ImGui::Begin("Knobs");
    }

    for (const auto& knobPtr : mKnobs) {
        if (!(knobPtr->mDisplayVisible)) {
            continue;
        }
        for (size_t i = 0; i < knobPtr->mDisplayIndent; i++) {
            ImGui::Indent();
        }
        knobPtr->Draw();
        for (size_t i = 0; i < knobPtr->mDisplayIndent; i++) {
            ImGui::Unindent();
        }
    }

    ImGui::Separator();

    if (ImGui::Button("Reset to Startup Values")) {
        ResetAllToStartup();
    }

    ImGui::InputText("Config File Path", mConfigFilePath, IM_ARRAYSIZE(mConfigFilePath));

    bool clickedLoad = ImGui::Button("Load");
    ImGui::SameLine();
    bool clickedSave = ImGui::Button("Save");
    ImGui::SameLine();
    bool clickedSaveAll = ImGui::Button("Save All");

    if (clickedSave || clickedSaveAll || clickedLoad) {
        OptionsNew       options;
        JsonConverterNew jsonConverter;

        if (clickedSave) {
            PPX_LOG_INFO("Saving partial config: " << mConfigFilePath);
            Save(options, true);
            jsonConverter.ExportOptionsToFile(options, mConfigFilePath);
        }
        else if (clickedSaveAll) {
            PPX_LOG_INFO("Saving full config: " << mConfigFilePath);
            Save(options, false);
            jsonConverter.ExportOptionsToFile(options, mConfigFilePath);
        }
        else if (clickedLoad) {
            PPX_LOG_INFO("Loading config: " << mConfigFilePath);
            jsonConverter.ParseOptionsFromFile(mConfigFilePath, options);
            Load(options);
        }
    }

    if (!inExistingWindow) {
        ImGui::End();
    }
}

std::string KnobManagerNew::GetUsageMsg()
{
    std::string usageMsg = "\nFlags:\n";
    for (const auto& knobPtr : mKnobs) {
        std::string knobMsg        = "--" + knobPtr->mFlagName;
        auto        flagParameters = knobPtr->GetFlagParameters();
        if (flagParameters != "") {
            knobMsg += " " + flagParameters;
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

void KnobManagerNew::Load(const OptionsNew& opts)
{
    std::unordered_map<std::string, std::vector<std::string>> optsMap;
    optsMap = opts.GetMap();

    // Validate that all keys correspond to existing knobs
    for (auto& it : optsMap) {
        PPX_ASSERT_MSG(mFlagNames.count(it.first), "option does not exist as knob: " + it.first);
    }

    // Load knob values from opts
    for (auto& knobPtr : mKnobs) {
        if (!optsMap.count(knobPtr->mFlagName)) {
            continue;
        }
        knobPtr->Load(optsMap.at(knobPtr->mFlagName));
        PPX_LOG_INFO("KNOB: " << knobPtr->mFlagName << " : (" << knobPtr->ValueString() << ")");
    }
}

void KnobManagerNew::Save(OptionsNew& opts, bool excludeStartupOnly)
{
    // Save knob values to opts
    for (auto& knobPtr : mKnobs) {
        if (excludeStartupOnly && knobPtr->mStartupOnly) {
            continue;
        }
        std::vector<std::string> valueStrings = knobPtr->Save();
        opts.AddOption(knobPtr->mFlagName, valueStrings);
    }
}

void KnobManagerNew::RegisterKnob(const std::string& flagName, std::shared_ptr<KnobNew> newKnob)
{
    mFlagNames.insert(flagName);
    mKnobs.emplace_back(std::move(newKnob));
}

} // namespace ppx
