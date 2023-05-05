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
    default:
        PPX_LOG_ERROR("invalid KnobType: " << static_cast<int>(kt));
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

void KnobBoolCheckbox::ResetToDefault()
{
    KnobBoolCheckbox::SetBoolValue(mDefaultValue);
}

bool KnobBoolCheckbox::GetBoolValue() const
{
    return mValue;
}

// Used for when knob value is altered outside of the GUI
void KnobBoolCheckbox::SetBoolValue(bool newVal)
{
    mValue = newVal;
    if (mCallback)
        mCallback(mValue);
}

// -------------------------------------------------------------------------------------------------
// KnobManager
// -------------------------------------------------------------------------------------------------

KnobManager::~KnobManager()
{
    for (auto pair : mKnobs) {
        delete pair.second;
    }
}

void KnobManager::ResetAllToDefault()
{
    for (auto pair : mKnobs) {
        pair.second->ResetToDefault();
    }
}

Knob* KnobManager::GetKnob(int id)
{
    if (mKnobs.count(id) == 0) {
        return nullptr;
    }
    return mKnobs.at(id);
}

Knob* KnobManager::GetKnob(const std::string& name)
{
    for (auto pair : mKnobs) {
        if (pair.second->GetFlagName() == name) {
            return pair.second;
        }
    }
    return nullptr;
}

void KnobManager::CreateBoolCheckbox(int i, BoolCheckboxConfig config)
{
    KnobBoolCheckbox* newKnob = new KnobBoolCheckbox(config);
    newKnob->SetBoolValue(config.defaultValue);
    KnobManager::InsertKnob(i, newKnob);
    KnobManager::ConfigureParent(newKnob, config.parentId);
}

bool KnobManager::GetKnobBoolValue(int id)
{
    auto knobPtr = GetKnob(id);
    return knobPtr ? knobPtr->GetBoolValue() : false;
}

void KnobManager::SetKnobBoolValue(int id, bool newVal)
{
    auto knobPtr = GetKnob(id);
    if (knobPtr)
        knobPtr->SetBoolValue(newVal);
}

void KnobManager::DrawAllKnobs(bool inExistingWindow)
{
    if (!inExistingWindow)
        ImGui::Begin("Knobs");
    DrawKnobs(mDrawOrder);
    if (!inExistingWindow)
        ImGui::End();
}

std::string KnobManager::GetUsageMsg()
{
    std::string usageMsg = "\nApplication-specific flags\n";
    for (auto pair : mKnobs) {
        usageMsg += "--" + pair.second->GetFlagName() + ": " + pair.second->GetFlagDesc() + "\n";
    }
    return usageMsg;
}

bool KnobManager::UpdateFromFlags(const CliOptions& cli)
{
    for (auto pair : mKnobs) {
        auto knobPtr = pair.second;
        if (knobPtr->GetType() == KnobType::Bool_Checkbox) {
            knobPtr->SetBoolValue(cli.GetExtraOptionValueOrDefault(knobPtr->GetFlagName(), knobPtr->GetBoolValue()));
        }
    }
    return true;
}

void KnobManager::InsertKnob(int id, Knob* knobPtr)
{
    mKnobs.insert(std::make_pair(id, knobPtr));
}

void KnobManager::ConfigureParent(Knob* knobPtr, int parentId)
{
    auto parentPtr = GetKnob(parentId);
    if (parentPtr) {
        parentPtr->AddChild(knobPtr);
    }
    else {
        mDrawOrder.push_back(knobPtr);
    }
}

void KnobManager::DrawKnobs(const std::vector<Knob*>& knobsToDraw)
{
    for (auto knobPtr : knobsToDraw) {
        knobPtr->Draw();
        if (!(knobPtr->GetChildren().empty())) {
            ImGui::Indent();
            KnobManager::DrawKnobs(knobPtr->GetChildren());
            ImGui::Unindent();
        }
    }
}

} // namespace ppx
