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

#include "ppx/imgui_impl.h"
#include "ppx/knob.h"

#include <cstring>

namespace ppx {

// -------------------------------------------------------------------------------------------------
// KnobManager
// -------------------------------------------------------------------------------------------------

KnobManager::~KnobManager()
{
    for (auto knobPtr : mKnobs) {
        delete knobPtr;
    }
}

void KnobManager::ResetAllToDefault()
{
    for (auto knobPtr : mKnobs) {
        knobPtr->ResetToDefault();
    }
}

void KnobManager::DrawAllKnobs(bool inExistingWindow)
{
    if (!inExistingWindow)
        ImGui::Begin("Knobs");

    DrawKnobs(mRoots);

    if (ImGui::Button("Reset to Default Values")) {
        ResetAllToDefault();
    }

    if (!inExistingWindow)
        ImGui::End();
}

std::string KnobManager::GetUsageMsg()
{
    std::string usageMsg = "\nApplication-specific flags\n";
    for (auto knobPtr : mKnobs) {
        usageMsg += knobPtr->GetFlagHelpText();
    }
    return usageMsg;
}

void KnobManager::UpdateFromFlags(const CliOptions& opts)
{
    for (auto knobPtr : mKnobs) {
        knobPtr->UpdateFromFlag(opts);
    }
}

void KnobManager::RegisterKnob(std::string flagName, Knob* newKnob, Knob* parent)
{
    // Store knob in members
    mFlagNames.insert(flagName);
    mKnobs.push_back(newKnob);
    if (parent != nullptr) {
        parent->AddChild(newKnob);
    }
    else {
        mRoots.push_back(newKnob);
    }

    PPX_LOG_INFO("Created knob " << flagName);
}

// -------------------------------------------------------------------------------------------------
// KnobBoolCheckbox
// -------------------------------------------------------------------------------------------------

void KnobBoolCheckbox::Draw()
{
    bool changed = ImGui::Checkbox(mDisplayName.c_str(), &mValue);
    if (changed && mCallback) {
        mCallback(mValue);
    }
}

std::string KnobBoolCheckbox::GetFlagHelpText() const
{
    std::string flagHelpText = "--" + mFlagName + " <true|false>";
    if (!mFlagDesc.empty()) {
        flagHelpText += " : " + mFlagDesc;
    }
    return flagHelpText + "\n";
}

void KnobBoolCheckbox::UpdateFromFlag(const CliOptions& opts)
{
    SetDefault(opts.GetExtraOptionValueOrDefault(mFlagName, mValue));
}

void KnobBoolCheckbox::SetValue(bool newVal)
{
    if (newVal != mValue) {
        mValue = newVal;
        if (mCallback)
            mCallback(mValue);
    }
}

void KnobBoolCheckbox::SetDefault(bool newVal)
{
    mDefaultValue = newVal;
    ResetToDefault();
}

// -------------------------------------------------------------------------------------------------
// KnobIntSlider
// -------------------------------------------------------------------------------------------------

void KnobIntSlider::Draw()
{
    ImGui::SliderInt(mDisplayName.c_str(), &mValue, mMinValue, mMaxValue, NULL, ImGuiSliderFlags_AlwaysClamp);

    if (ImGui::IsItemDeactivatedAfterEdit() && mCallback) {
        mCallback(mValue);
    }
}

std::string KnobIntSlider::GetFlagHelpText() const
{
    std::string flagHelpText = "--" + mFlagName + " <" + std::to_string(mMinValue) + "~" + std::to_string(mMaxValue) + ">";
    if (!mFlagDesc.empty()) {
        flagHelpText += " : " + mFlagDesc;
    }
    return flagHelpText + "\n";
}

void KnobIntSlider::UpdateFromFlag(const CliOptions& opts)
{
    SetDefault(opts.GetExtraOptionValueOrDefault(mFlagName, mValue));
}

void KnobIntSlider::SetValue(int newVal)
{
    if (!IsValidValue(newVal)) {
        PPX_LOG_ERROR(mFlagName << " cannot be set to " << newVal << " because it's out of range " << mMinValue << "~" << mMaxValue);
        return;
    }

    // update mValue and trigger mCallback
    if (newVal != mValue) {
        mValue = newVal;
        if (mCallback)
            mCallback(mValue);
    }
}

bool KnobIntSlider::IsValidValue(int val)
{
    if (val < mMinValue || val > mMaxValue)
        return false;
    return true;
}

void KnobIntSlider::SetDefault(int newVal)
{
    PPX_ASSERT_MSG(IsValidValue(newVal), "invalid default value");
    mDefaultValue = newVal;
    ResetToDefault();
}

// -------------------------------------------------------------------------------------------------
// KnobStrDropdown
// -------------------------------------------------------------------------------------------------

void KnobStrDropdown::Draw()
{
    if (ImGui::BeginCombo(mDisplayName.c_str(), mChoices.at(mIndex).c_str())) {
        for (int i = 0; i < mChoices.size(); ++i) {
            bool isSelected = (i == mIndex);
            if (ImGui::Selectable(mChoices.at(i).c_str(), isSelected)) {
                if (i != mIndex) { // A new choice is selected
                    mIndex = i;
                    if (mCallback)
                        mCallback(mIndex);
                }
            }
            if (isSelected) {
                ImGui::SetItemDefaultFocus();
            }
        }
        ImGui::EndCombo();
    }
}

std::string KnobStrDropdown::GetFlagHelpText() const
{
    std::string choiceStr = "";
    for (auto choice : mChoices) {
        choiceStr += '\"' + choice + '\"' + "|";
    }
    if (!choiceStr.empty())
        choiceStr.erase(choiceStr.size() - 1);

    std::string flagHelpText = "--" + mFlagName + " <" + choiceStr + ">";
    if (!mFlagDesc.empty()) {
        flagHelpText += " : " + mFlagDesc;
    }
    return flagHelpText + "\n";
}

void KnobStrDropdown::UpdateFromFlag(const CliOptions& opts)
{
    std::string newVal = opts.GetExtraOptionValueOrDefault(mFlagName, GetStr());
    SetDefault(newVal);
}

void KnobStrDropdown::SetIndex(size_t newI)
{
    if (!IsValidIndex(newI)) {
        PPX_LOG_ERROR(mFlagName << " does not have this index in allowed choices: " << newI);
        return;
    }

    // update mIndex and trigger mCallback
    if (newI != mIndex) {
        mIndex = newI;
        if (mCallback)
            mCallback(mIndex);
    }
}

void KnobStrDropdown::SetIndex(std::string newVal)
{
    auto temp = std::find(mChoices.begin(), mChoices.end(), newVal);
    if (temp == mChoices.end()) {
        PPX_LOG_ERROR(mFlagName << " does not have this value in allowed range: " << newVal);
        return;
    }

    // update mIndex and trigger mCallback
    size_t newI = static_cast<size_t>(temp - mChoices.begin());
    if (newI != mIndex) {
        mIndex = newI;
        if (mCallback)
            mCallback(mIndex);
    }
}

bool KnobStrDropdown::IsValidIndex(size_t i)
{
    if (static_cast<int>(i) < 0 || static_cast<int>(i) >= mChoices.size())
        return false;
    return true;
}

void KnobStrDropdown::SetDefault(size_t newI)
{
    PPX_ASSERT_MSG(IsValidIndex(newI), "invalid default index");
    mDefaultIndex = newI;
    ResetToDefault();
}

void KnobStrDropdown::SetDefault(std::string newVal)
{
    auto temp = std::find(mChoices.begin(), mChoices.end(), newVal);
    PPX_ASSERT_MSG(temp != mChoices.end(), "invalid default value");

    mDefaultIndex = static_cast<size_t>(temp - mChoices.begin());
    ResetToDefault();
}

// -------------------------------------------------------------------------------------------------
// Helper functions
// -------------------------------------------------------------------------------------------------

void DrawKnobs(const std::vector<Knob*>& knobPtrs)
{
    for (auto knobPtr : knobPtrs) {
        knobPtr->Draw();
        if (!(knobPtr->GetChildren().empty())) {
            ImGui::Indent();
            DrawKnobs(knobPtr->GetChildren());
            ImGui::Unindent();
        }
    }
}

} // namespace ppx
