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
    const std::string nameKnobType[] = {"Unknown", "Bool_Checkbox"};
    return os << nameKnobType[static_cast<int>(kt)];
}

namespace ppx {

// -------------------------------------------------------------------------------------------------
// KnobBoolCheckbox
// -------------------------------------------------------------------------------------------------

void KnobBoolCheckbox::Draw()
{
    bool changed = ImGui::Checkbox(displayName.c_str(), &value);
    if (changed && callback) {
        callback(value);
    }
}

void KnobBoolCheckbox::Reset()
{
    KnobBoolCheckbox::SetBoolValue(defaultValue, false);
}

bool KnobBoolCheckbox::GetBoolValue()
{
    return value;
}

// Used for when knob value is altered outside of the GUI
void KnobBoolCheckbox::SetBoolValue(bool newVal, bool updateDefault)
{
    value = newVal;
    if (updateDefault)
        defaultValue = newVal;
    if (callback)
        callback(value);
}

// -------------------------------------------------------------------------------------------------
// KnobManager
// -------------------------------------------------------------------------------------------------

KnobManager::~KnobManager()
{
    for (auto pair : knobs) {
        delete pair.second;
    }
}

void KnobManager::Reset()
{
    for (auto pair : knobs) {
        pair.second->Reset();
    }
}

Knob* KnobManager::GetKnob(int id, bool silentFail)
{
    if (knobs.count(id) == 0) {
        if (!silentFail) {
            PPX_LOG_ERROR("could not find knob with id: " << id);
        }
        return nullptr;
    }
    return knobs.at(id);
}

void KnobManager::CreateBoolCheckbox(int i, BoolCheckboxConfig config)
{
    KnobBoolCheckbox* newKnob = new KnobBoolCheckbox(config);
    newKnob->SetBoolValue(config.defaultValue, true);
    KnobManager::InsertKnob(i, newKnob);
    KnobManager::ConfigureParent(newKnob, config.parentId);
}

bool KnobManager::GetKnobBoolValue(int id)
{
    auto knobPtr = GetKnob(id);
    return knobPtr ? knobPtr->GetBoolValue() : false;
}

void KnobManager::SetKnobBoolValue(int id, bool newVal, bool updateDefault)
{
    auto knobPtr = GetKnob(id);
    if (knobPtr)
        knobPtr->SetBoolValue(newVal, updateDefault);
}

void KnobManager::InsertKnob(int id, Knob* knobPtr)
{
    knobs.insert(std::make_pair(id, knobPtr));
}

void KnobManager::ConfigureParent(Knob* knobPtr, int parentId)
{
    auto parentPtr = GetKnob(parentId, true);
    if (parentPtr) {
        parentPtr->AddChild(knobPtr);
    }
    else {
        drawOrder.push_back(knobPtr);
    }
}

} // namespace ppx
