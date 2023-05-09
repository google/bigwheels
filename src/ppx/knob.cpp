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

#include "backends/imgui_impl_glfw.h"
#include "ppx/knob.h"

#include <cstring>

// -------------------------------------------------------------------------------------------------
// KnobType
// -------------------------------------------------------------------------------------------------

std::ostream& operator<<(std::ostream& os, KnobType kt)
{
    return os << KnobType2Str(kt);
}

std::string KnobType2Str(KnobType kt)
{
    switch(kt) {
    case KnobType::Unknown:
        return "Unknown";
    case KnobType::Bool_Checkbox:
        return "Bool_Checkbox";
    case KnobType::Int_Slider:
        return "Int_Slider";
    case KnobType::Str_Dropdown:
        return "Str_Dropdown";
    default:
        PPX_LOG_FATAL("invalid KnobType: " << static_cast<int>(kt));
        return "";
    }
}

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
    return "--" + mFlagName + " <true/false> : " + mFlagDesc + "\n";
}

// Used for when knob value is altered outside of the GUI
void KnobBoolCheckbox::SetBoolValue(bool newVal, bool updateDefault)
{
    // update mDefaultValue
    if (updateDefault) mDefaultValue = newVal;

    // update mValue and trigger mCallback
    if (newVal != mValue) {
        mValue = newVal;
        if (mCallback) mCallback(mValue);
    }
}

// -------------------------------------------------------------------------------------------------
// KnobIntSlider
// -------------------------------------------------------------------------------------------------

void KnobIntSlider::Draw()
{
    int oldValue = mValue;
    ImGui::SliderInt(mDisplayName.c_str(), &mValue, mMinValue, mMaxValue, NULL, ImGuiSliderFlags_AlwaysClamp);

    if (ImGui::IsItemDeactivatedAfterEdit() && mCallback && oldValue != mValue) {
        mCallback(mValue);
    }
}

std::string KnobIntSlider::FlagText() const
{
    return "--" + mFlagName + " <" + std::to_string(mMinValue) + "~" + std::to_string(mMaxValue) + "> : " + mFlagDesc + "\n";
}

Result KnobIntSlider::SetIntValue(int newVal, bool updateDefault)
{
    if (newVal < mMinValue || newVal > mMaxValue) {
        PPX_LOG_ERROR(mFlagName << " cannot be set to " << newVal << " because it's out of range " << mMinValue << "~" << mMaxValue);
        return ERROR_OUT_OF_RANGE;
    }

    // update mDefaultValue
    if (updateDefault) mDefaultValue = newVal;

    // update mValue and trigger mCallback
    if (newVal != mValue) {
        mValue = newVal;
        if (mCallback) mCallback(mValue);
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
    if (!choiceStr.empty()) choiceStr.erase(choiceStr.size() - 1);
    return "--" + mFlagName + " <" + choiceStr + "> : " + mFlagDesc + "\n";
}

Result KnobStrDropdown::SetIndex(int newI, bool updateDefault)
{
    if (newI < 0 || newI >= mChoices.size()) {
        PPX_LOG_ERROR(mFlagName << " does not have this index in allowed choices: " << newI);
        return ERROR_ELEMENT_NOT_FOUND;
    }

    // update mDefaultIndex
    if (updateDefault) mDefaultIndex = newI;

    // update mIndex and trigger mCallback
    if (newI != mIndex) {
        mIndex = newI;
        if (mCallback) mCallback(mIndex);
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
    auto knobPtrs = FlattenDepthFirst(mRoots);
    for (auto knobPtr : knobPtrs) {
        delete knobPtr;
    }
}

void KnobManager::ResetAllToDefault()
{
    auto knobPtrs = FlattenDepthFirst(mRoots);
    for (auto knobPtr : knobPtrs) {
        knobPtr->ResetToDefault();
    }
}

void KnobManager::DrawAllKnobs(bool inExistingWindow)
{
    if (!inExistingWindow)
        ImGui::Begin("Knobs");
    
    DrawKnobs(mRoots);

    if (ImGui::Button("Reset to Default Values")){
        ResetAllToDefault();
    }

    if (!inExistingWindow)
        ImGui::End();
}

std::string KnobManager::GetUsageMsg()
{
    std::string usageMsg = "\nApplication-specific flags\n";
    auto knobPtrs = FlattenDepthFirst(mRoots);
    for (auto knobPtr : knobPtrs) {
        usageMsg += knobPtr->FlagText();
    }
    return usageMsg;
}

bool KnobManager::UpdateFromFlags(const CliOptions& opts)
{
    auto knobPtrs = FlattenDepthFirst(mRoots);
    for (auto knobPtr : knobPtrs) {
        switch (knobPtr->GetType()) {
        case (KnobType::Bool_Checkbox):
        {
            KnobBoolCheckbox *boolPtr = dynamic_cast<KnobBoolCheckbox*>(knobPtr);
            if (boolPtr) {
                boolPtr->SetBoolValue(opts.GetExtraOptionValueOrDefault(boolPtr->GetFlagName(), boolPtr->GetBoolValue()), true);
            } else {
                PPX_LOG_ERROR("could not cast as Bool_Checkbox: " << knobPtr->GetFlagName());
                return false;
            }
            break;
        }
        case (KnobType::Int_Slider):
        {
            KnobIntSlider *intPtr = dynamic_cast<KnobIntSlider*>(knobPtr);
            if (intPtr) {
                int  newVal = opts.GetExtraOptionValueOrDefault(intPtr->GetFlagName(), intPtr->GetIntValue());
                bool wasValid = intPtr->SetIntValue(newVal, true);
                if (!wasValid) {
                    PPX_LOG_ERROR(intPtr->GetFlagName() << " invalid value: " << newVal);
                    return false;
                }
            } else {
                PPX_LOG_ERROR("could not cast as Int_Slider: " << knobPtr->GetFlagName());
                return false;
            }
            break;
        }
        case (KnobType::Str_Dropdown):
        {
            KnobStrDropdown *strPtr = dynamic_cast<KnobStrDropdown*>(knobPtr);
            if (strPtr) {
                std::string newVal = opts.GetExtraOptionValueOrDefault(strPtr->GetFlagName(), strPtr->GetStr());
                bool wasValid = strPtr->SetIndex(newVal, true);
                if (!wasValid) {
                    PPX_LOG_ERROR(strPtr->GetFlagName() << " invalid value: " << newVal);
                    return false;
                }
            } else {
                PPX_LOG_ERROR("could not cast as Str_Dropdown: " << knobPtr->GetFlagName());
                return false;
            }
            break;
        }
        default:
            PPX_LOG_ERROR("invalid knob: " << knobPtr->GetFlagName() << ", type: " << knobPtr->GetType());
            return false;
        }
    }
    return true;
}

// -------------------------------------------------------------------------------------------------
// Helper functions
// -------------------------------------------------------------------------------------------------

std::vector<Knob*> FlattenDepthFirst(const std::vector<Knob*>& rootPtrs)
{
    if (rootPtrs.empty())
        return {};

    std::vector<Knob*> knobPtrs;
    for (auto knobPtr : rootPtrs) {
        knobPtrs.push_back(knobPtr);
        auto flatChildren = FlattenDepthFirst(knobPtr->GetChildren());
        if (!flatChildren.empty()) knobPtrs.insert(knobPtrs.end(), flatChildren.begin(), flatChildren.end());
    }
    return knobPtrs;
}

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
