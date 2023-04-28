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

#ifndef ppx_knob_h
#define ppx_knob_h

#include "ppx/log.h"

#include <functional>
#include <iostream>
#include <map>
#include <vector>

// ---------------------------------------------------------------------------------------------
// KnobType
// ---------------------------------------------------------------------------------------------

enum class KnobType
{
    Unknown,
    Bool_Checkbox
};

std::ostream& operator<<(std::ostream& strm, KnobType kt);

namespace ppx {

// ---------------------------------------------------------------------------------------------
// KnobConfigs
// ---------------------------------------------------------------------------------------------

struct KnobConfig
{
    std::string flagName;
    std::string flagDesc;
    std::string displayName;
    int         parentId;
};

// Available knob types for the developer to specify
struct BoolCheckboxConfig : KnobConfig
{
    bool                      defaultValue;
    std::function<void(bool)> callback;
};

// ---------------------------------------------------------------------------------------------
// Knob Classes
// ---------------------------------------------------------------------------------------------

class Knob
{
public:
    Knob() {}
    Knob(const KnobConfig& config, KnobType type)
        : displayName(config.displayName), flagName(config.flagName), flagDesc(config.flagDesc), type(type) {}
    virtual ~Knob() {}

    std::string        GetDisplayName() { return displayName; }
    std::string        GetFlagName() { return flagName; }
    std::string        GetFlagDesc() { return flagDesc; }
    KnobType           GetType() { return type; }
    void               AddChild(Knob* child) { children.push_back(child); }
    std::vector<Knob*> GetChildren() { return children; }

    virtual void Draw()  = 0;
    virtual void Reset() = 0;

    // Used for Bool_Checkbox
    virtual bool GetBoolValue()
    {
        PPX_LOG_ERROR("Flag of type " << type << " does not support GetBoolValue()");
        return false;
    };
    virtual void SetBoolValue(bool newVal, bool isDefault)
    {
        PPX_LOG_ERROR("Flag of type " << type << " does not support SetBoolValue()");
    };

protected:
    std::string        displayName;
    std::string        flagName;
    std::string        flagDesc;
    KnobType           type;
    std::vector<Knob*> children;
};

class KnobBoolCheckbox
    : public Knob
{
public:
    KnobBoolCheckbox() {}
    KnobBoolCheckbox(const BoolCheckboxConfig& config)
        : Knob(config, KnobType::Bool_Checkbox)
    {
        defaultValue = config.defaultValue;
        callback     = config.callback;
    }
    virtual ~KnobBoolCheckbox() {}

    void Draw() override;
    void Reset() override;

    bool GetBoolValue() override;
    void SetBoolValue(bool newVal, bool updateDefault) override;

private:
    bool                      value;
    bool                      defaultValue;
    std::function<void(bool)> callback;
};

class KnobManager
{
public:
    KnobManager() {}
    virtual ~KnobManager();
    KnobManager(const KnobManager&)            = delete;
    KnobManager& operator=(const KnobManager&) = delete;

    // Utilities
    bool  IsEmpty() { return knobs.empty(); }
    void  Reset();
    Knob* GetKnob(int id, bool silentFail = false);

    // Create knobs
    void CreateBoolCheckbox(int i, BoolCheckboxConfig config);

    // Read/Write knobs
    bool GetKnobBoolValue(int id);
    void SetKnobBoolValue(int id, bool newVal, bool updateDefault = false);

    // ImGUI
    void DrawAllKnobs(bool inExistingWindow = false);

private:
    void InsertKnob(int id, Knob* knobPtr);
    void ConfigureParent(Knob* knobPtr, int parentId);
    void DrawKnobs(const std::vector<Knob*>& knobsToDraw);

private:
    std::map<int, Knob*> knobs;
    std::vector<Knob*>   drawOrder;
};

} // namespace ppx

#endif // ppx_knob_h
