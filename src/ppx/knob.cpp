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
// KnobBoolCheckbox
// -------------------------------------------------------------------------------------------------

void KnobBoolCheckbox::Draw()
{
    bool changed = ImGui::Checkbox(mDisplayName.c_str(), &mValue);
    if (changed && mCallback) {
        mCallback(mValue);
    }
}

std::string KnobBoolCheckbox::FlagText() const
{
    return "--" + mFlagName + " <true|false> : " + mFlagDesc + "\n";
}

Result KnobBoolCheckbox::UpdateFromFlag(const CliOptions& opts)
{
    SetValue(opts.GetExtraOptionValueOrDefault(mFlagName, mValue), true);
    return SUCCESS;
}

void KnobBoolCheckbox::SetValue(bool newVal, bool updateDefault)
{
    // update mDefaultValue
    if (updateDefault)
        mDefaultValue = newVal;

    // update mValue and trigger mCallback
    if (newVal != mValue) {
        mValue = newVal;
        if (mCallback)
            mCallback(mValue);
    }
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

std::string KnobIntSlider::FlagText() const
{
    return "--" + mFlagName + " <" + std::to_string(mMinValue) + "~" + std::to_string(mMaxValue) + "> : " + mFlagDesc + "\n";
}

Result KnobIntSlider::UpdateFromFlag(const CliOptions& opts)
{
    int    newVal = opts.GetExtraOptionValueOrDefault(mFlagName, mValue);
    Result res    = SetValue(newVal, true);
    if (res != SUCCESS) {
        PPX_LOG_ERROR(mFlagName << " invalid commandline flag value: " << newVal);
        return ERROR_OUT_OF_RANGE;
    }
    return SUCCESS;
}

Result KnobIntSlider::SetValue(int newVal, bool updateDefault)
{
    if (newVal < mMinValue || newVal > mMaxValue) {
        PPX_LOG_ERROR(mFlagName << " cannot be set to " << newVal << " because it's out of range " << mMinValue << "~" << mMaxValue);
        return ERROR_OUT_OF_RANGE;
    }

    // update mDefaultValue
    if (updateDefault)
        mDefaultValue = newVal;

    // update mValue and trigger mCallback
    if (newVal != mValue) {
        mValue = newVal;
        if (mCallback)
            mCallback(mValue);
    }

    return SUCCESS;
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

std::string KnobStrDropdown::FlagText() const
{
    std::string choiceStr = "";
    for (auto choice : mChoices) {
        choiceStr += '\"' + choice + '\"' + "|";
    }
    if (!choiceStr.empty())
        choiceStr.erase(choiceStr.size() - 1);
    return "--" + mFlagName + " <" + choiceStr + "> : " + mFlagDesc + "\n";
}

Result KnobStrDropdown::UpdateFromFlag(const CliOptions& opts)
{
    std::string newVal = opts.GetExtraOptionValueOrDefault(mFlagName, GetStr());
    Result      res    = SetIndex(newVal, true);
    if (res != SUCCESS) {
        PPX_LOG_ERROR(mFlagName << " invalid commandline flag value: " << newVal);
        return ERROR_OUT_OF_RANGE;
    }
    return SUCCESS;
}

Result KnobStrDropdown::SetIndex(int newI, bool updateDefault)
{
    if (newI < 0 || newI >= mChoices.size()) {
        PPX_LOG_ERROR(mFlagName << " does not have this index in allowed choices: " << newI);
        return ERROR_ELEMENT_NOT_FOUND;
    }

    // update mDefaultIndex
    if (updateDefault)
        mDefaultIndex = newI;

    // update mIndex and trigger mCallback
    if (newI != mIndex) {
        mIndex = newI;
        if (mCallback)
            mCallback(mIndex);
    }

    return SUCCESS;
}

Result KnobStrDropdown::SetIndex(std::string newVal, bool updateDefault)
{
    auto temp = std::find(mChoices.begin(), mChoices.end(), newVal);
    if (temp == mChoices.end()) {
        PPX_LOG_ERROR(mFlagName << " does not have this value in allowed range: " << newVal);
        return ERROR_ELEMENT_NOT_FOUND;
    }

    mIndex = static_cast<int>(temp - mChoices.begin());
    if (updateDefault)
        mDefaultIndex = mIndex;
    if (mCallback)
        mCallback(mIndex);
    return SUCCESS;
}

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
        usageMsg += knobPtr->FlagText();
    }
    return usageMsg;
}

Result KnobManager::UpdateFromFlags(const CliOptions& opts)
{
    for (auto knobPtr : mKnobs) {
        Result res = knobPtr->UpdateFromFlag(opts);
        if (res != SUCCESS)
            return res;
    }
    return SUCCESS;
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
