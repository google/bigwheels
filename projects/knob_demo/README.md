# Knob Demo

Using `sample_01_triangle` as base, this simple application demonstrates at least one of all available knobs display widgets. It also seeks to demonstrate when properties of the knobs are altered during runtime.

## GeneralKnob
* Display types:
    * `PLAIN`
    * `CHECKBOX` (bool)
* Unique functions:
    * `SetValidator()`

## RangeKnob
* Display types:
    * `PLAIN`
    * `FAST_SLIDER` (int, float) (N)
    * `SLOW_SLIDER` (int, float) (N)
* Unique functions:
    * `SetMax()`
    * `SetMin()`

## OptionKnob
* Display types:
    * `PLAIN`
    * `DROPDOWN`
* Unique functions:
    * `SetMask()`
