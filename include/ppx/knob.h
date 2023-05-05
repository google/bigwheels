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

#include "ppx/command_line_parser.h"
#include "ppx/log.h"

#include <functional>
#include <iostream>
#include <map>
#include <unordered_map>
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

std::string KnobType2Str(KnobType kt);

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
        : mDisplayName(config.displayName), mFlagName(config.flagName), mFlagDesc(config.flagDesc), mType(type) {}
    virtual ~Knob() {}

    std::string        GetDisplayName() const { return mDisplayName; }
    std::string        GetFlagName() const { return mFlagName; }
    std::string        GetFlagDesc() const { return mFlagDesc; }
    KnobType           GetType() const { return mType; }
    void               AddChild(Knob* pChild) { mChildren.push_back(pChild); }
    std::vector<Knob*> GetChildren() const { return mChildren; }

    virtual void Draw() = 0;
    virtual void ResetToDefault() = 0;

    // Used for Bool_Checkbox
    virtual bool GetBoolValue() const
    {
        PPX_LOG_ERROR("Flag of type " << mType << " does not support GetBoolValue()");
        return false;
    };
    virtual void SetBoolValue(bool newVal)
    {
        PPX_LOG_ERROR("Flag of type " << mType << " does not support SetBoolValue()");
    };

protected:
    std::string        mDisplayName;
    std::string        mFlagName;
    std::string        mFlagDesc;
    KnobType           mType;
    std::vector<Knob*> mChildren;
};

class KnobBoolCheckbox
    : public Knob
{
public:
    KnobBoolCheckbox() {}
    KnobBoolCheckbox(const BoolCheckboxConfig& config)
        : Knob(config, KnobType::Bool_Checkbox)
    {
        mDefaultValue = config.defaultValue;
        mCallback     = config.callback;
    }
    virtual ~KnobBoolCheckbox() {}

    void Draw() override;
    void ResetToDefault() override;

    bool GetBoolValue() const override;
    void SetBoolValue(bool newVal) override;

private:
    bool                      mValue;
    bool                      mDefaultValue;
    std::function<void(bool)> mCallback;
};

class KnobManager
{
public:
    KnobManager() {}
    virtual ~KnobManager();
    KnobManager(const KnobManager&)            = delete;
    KnobManager& operator=(const KnobManager&) = delete;

    // Utilities
    bool  IsEmpty() { return mKnobs.empty(); }
    void  ResetAllToDefault();
    Knob* GetKnob(int id);
    Knob* GetKnob(const std::string& name);

    // Create knobs
    void CreateBoolCheckbox(int i, BoolCheckboxConfig config);

    // Read/Write knobs
    bool GetKnobBoolValue(int id);
    void SetKnobBoolValue(int id, bool newVal);

    // ImGUI
    void DrawAllKnobs(bool inExistingWindow = false);

    // Command-line flags
    std::string GetUsageMsg();
    bool        UpdateFromFlags(const CliOptions& opts);

private:
    void InsertKnob(int id, Knob* knobPtr);
    void ConfigureParent(Knob* knobPtr, int parentId);
    void DrawKnobs(const std::vector<Knob*>& knobsToDraw);

private:
    std::map<int, Knob*> mKnobs;
    std::vector<Knob*>   mDrawOrder;
};

} // namespace ppx

#endif // ppx_knob_h
