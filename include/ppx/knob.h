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
#include "ppx/config.h"
#include "ppx/log.h"

#include <functional>
#include <iostream>
#include <map>
#include <unordered_map>
#include <vector>

// ---------------------------------------------------------------------------------------------
// KnobType
// ---------------------------------------------------------------------------------------------

// KnobType indicates the data type and the display type of the knob
enum class KnobType
{
    Unknown,
    Bool_Checkbox,
    Int_Slider,
    Str_Dropdown
};

std::ostream& operator<<(std::ostream& strm, KnobType kt);

std::string KnobType2Str(KnobType kt);

namespace ppx {

// ---------------------------------------------------------------------------------------------
// Knob Classes
// ---------------------------------------------------------------------------------------------

// Knob is an abstract class which contains common features for all knobs and knob hierarchy
class Knob
{
public:
    Knob() {}
    Knob(KnobType type, const std::string& displayName, const std::string& flagName, const std::string& flagDesc)
        : mType(type), mDisplayName(displayName), mFlagName(flagName), mFlagDesc(flagDesc) {}
    virtual ~Knob() {}

    std::string        GetDisplayName() const { return mDisplayName; }
    std::string        GetFlagName() const { return mFlagName; }
    std::string        GetFlagDesc() const { return mFlagDesc; }
    KnobType           GetType() const { return mType; }
    
    // Child knobs are displayed in the UI below their parent and slightly indented
    void               AddChild(Knob* pChild) { mChildren.push_back(pChild); }
    std::vector<Knob*> GetChildren() const { return mChildren; }

    virtual void Draw() = 0;
    
    // Flag description in usage message
    virtual std::string FlagText() const = 0;

    // Default values are the values when the application starts. The knobs can be reset to default by a button in the UI
    virtual void ResetToDefault() = 0;

protected:
    std::string        mDisplayName;
    std::string        mFlagName;
    std::string        mFlagDesc;
    KnobType           mType;
    std::vector<Knob*> mChildren;
};

// KnobBoolCheckbox is a boolean knob that will be displayed as a checkbox in the UI
class KnobBoolCheckbox
    : public Knob
{
public:
    KnobBoolCheckbox() {}
    KnobBoolCheckbox(const std::string& displayName, const std::string& flagName, const std::string& flagDesc, bool defaultValue, std::function<void(bool)> callback = NULL)
        : Knob(KnobType::Bool_Checkbox, displayName, flagName, flagDesc)
    {
        mCallback     = callback;
        SetBoolValue(defaultValue, true);
    }
    virtual ~KnobBoolCheckbox() {}

    void Draw() override;
    void ResetToDefault() override { SetBoolValue(mDefaultValue); }

    // FlagText format:
    // --flag_name <true/false> : flag description
    std::string FlagText() const override;
    bool GetBoolValue() const { return mValue; }

    // If newVal is valid:
    // - If updateDefault, mDefaultValue is updated
    // - If newVal is different from mValue:
    //   - mValue is updated
    //   - mCallback is triggered
    void SetBoolValue(bool newVal, bool updateDefault = false);

private:
    bool                      mValue;
    bool                      mDefaultValue;

    // mCallback will be called on the new bool value every time it is toggled in the UI, or updated with SetBoolValue()
    std::function<void(bool)> mCallback;
};

// KnobIntSlider is an int knob that will be displayed as a slider in the UI
// ImGui sliders can also become input boxes with ctrl + right click
class KnobIntSlider
    : public Knob
{
public:
    KnobIntSlider() {}
    KnobIntSlider(const std::string& displayName, const std::string& flagName, const std::string& flagDesc, int defaultValue, int minValue, int maxValue, std::function<void(int)> callback = NULL) 
        : Knob(KnobType::Int_Slider, displayName, flagName, flagDesc) 
    {
        if (minValue >= maxValue) {
            PPX_LOG_FATAL(flagName << " KnobIntSlider: slider invalid range " << minValue << "~" << maxValue);
        }
        mMinValue = minValue;
        mMaxValue = maxValue;
        mCallback = callback;
        SetIntValue(defaultValue, true);
    }
    virtual ~KnobIntSlider() {}

    void Draw() override;
    void ResetToDefault() override { SetIntValue(mDefaultValue); }

    // FlagText format:
    // --flag_name <mMinValue~mMaxValue> : flag description
    std::string FlagText() const override;
    int GetIntValue() const { return mValue; }

    // Returns success if newVal is valid, fails otherwise
    // If newVal is valid:
    // - If updateDefault, mDefaultValue is updated
    // - If newVal is different from mValue:
    //   - mValue is updated
    //   - mCallback is triggered
    Result SetIntValue(int newVal, bool updateDefault = false);

private:
    int mValue;
    int mDefaultValue;

    // mValue will be clamped to the mMinValue to mMaxValue range, inclusive
    int mMinValue;
    int mMaxValue;

    // mCallback will be called on the new int value every time the slider is deselected in the UI, or updated with SetIntValue()
    std::function<void(int)> mCallback;
};

// KnobStrDropdown is an int knob that will be displayed as a dropdown in the UI
// The knob stores the index of a selected choice from a list of allowed strings
class KnobStrDropdown
    : public Knob
{
public:
    KnobStrDropdown() {}
    KnobStrDropdown(const std::string& displayName, const std::string& flagName, const std::string& flagDesc, int defaultIndex, const std::vector<std::string>& choices, std::function<void(int)> callback = NULL) 
        : Knob(KnobType::Str_Dropdown, displayName, flagName, flagDesc)
        {
        if (choices.size() < 1) {
            PPX_LOG_FATAL(flagName << " KnobStrDropdown: too few allowed choices");
        }
        mChoices = choices;
        mCallback = callback;
        auto res = SetIndex(defaultIndex, true);
        if (res != SUCCESS)
            PPX_LOG_FATAL(flagName << "cannot be initialized");
    }
    virtual ~KnobStrDropdown() {}

    void Draw() override;
    void ResetToDefault() override { SetIndex(mDefaultIndex); }

    // FlagText format:
    // --flag_name <"choice1"|"choice2"|...> : flag description
    std::string FlagText() const override;
    int GetIndex() const { return mIndex; }
    std::string GetStr() const { return mChoices[mIndex]; }

    // Returns success if newI is valid, fails otherwise
    // If newI is valid:
    // - If updateDefault, mDefaultIndex is updated
    // - If newI is different from mIndex:
    //   - mIndex is updated
    //   - mCallback is triggered
    Result SetIndex(int newI, bool updateDefault = false);

    // Used for setting from flags, will set mIndex to appropriate value if newVal matches a choice
    // Updates mDefaultIndex, mIndex, and calls mCallback with the same logic as above
    Result SetIndex(std::string newVal, bool updateDefault = false);

private:
    // mIndex indicates which of the mChoices is selected
    int mIndex;
    int mDefaultIndex;
    std::vector<std::string> mChoices;

    // mCallback will be called on the new index every time the dropdown is deselected in the UI, or updated with SetIndex()
    std::function<void(int)> mCallback;
};

// KnobManager owns the knobs in an application
class KnobManager
{
public:
    KnobManager() {}
    virtual ~KnobManager();
    KnobManager(const KnobManager&)            = delete;
    KnobManager& operator=(const KnobManager&) = delete;

    // Utilities
    bool  IsEmpty() { return mRoots.empty(); }
    void  ResetAllToDefault();

    // Create knobs
    template <typename T, typename... ArgsT>
    T* CreateKnob(Knob* parent, ArgsT... args) {
        T *knobPtr = new T(std::forward<ArgsT>(args)...);
        if (parent != nullptr) {
            parent->AddChild(knobPtr);
        } else {
            mRoots.push_back(knobPtr);
        }
        return knobPtr;
    }

    // ImGUI
    void DrawAllKnobs(bool inExistingWindow = false);

    // Command-line flags
    std::string GetUsageMsg();
    Result        UpdateFromFlags(const CliOptions& opts);

private:
    std::vector<Knob*>   mRoots;
};

std::vector<Knob*> FlattenDepthFirst(const std::vector<Knob*>& rootPtrs);
void DrawKnobs(const std::vector<Knob*>& knobPtrs);

} // namespace ppx

#endif // ppx_knob_h
