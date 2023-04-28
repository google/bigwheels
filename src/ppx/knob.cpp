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

namespace ppx {

// -------------------------------------------------------------------------------------------------
// KnobType
// -------------------------------------------------------------------------------------------------

std::ostream& operator<<(std::ostream& strm, KnobType kt)
{
    const std::string nameKnobType[] = {"Unknown", "Bool_Checkbox"};
    return strm << nameKnobType[static_cast<int>(kt)];
}

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
        free(pair.second);
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

Knob* KnobManager::GetKnob(std::string name, bool silentFail)
{
    for (auto pair : knobs) {
        if (pair.second->GetFlagName() == name) {
            return pair.second;
        }
    }

    if (!silentFail)
        PPX_LOG_ERROR("could not find knob with flag name: " << name);

    return nullptr;
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

void KnobManager::DrawAllKnobs(bool inExistingWindow)
{
    if (!inExistingWindow)
        ImGui::Begin("Knobs");
    DrawKnobs(drawOrder);
    if (!inExistingWindow)
        ImGui::End();
}

std::string KnobManager::GetUsageMsg()
{
    std::string usageMsg = "\nApplication-specific flags\n";
    for (auto pair : knobs) {
        usageMsg += "--" + pair.second->GetFlagName() + ": " + pair.second->GetFlagDesc() + "\n";
    }
    return usageMsg;
}

bool KnobManager::ParseOptions(std::unordered_map<std::string, CliOptions::Option>& optionsMap)
{
    for (auto pair : optionsMap) {
        auto  name    = pair.first;
        auto  opt     = pair.second;
        Knob* knobPtr = GetKnob(name, true);
        if (knobPtr) {
            switch (knobPtr->GetType()) {
                case KnobType::Bool_Checkbox:
                    knobPtr->SetBoolValue(opt.GetValueOrDefault(knobPtr->GetBoolValue()));
                    break;
            }
        }
    }
    return true;
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

void KnobManager::DrawKnobs(std::vector<Knob*> knobsToDraw)
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
