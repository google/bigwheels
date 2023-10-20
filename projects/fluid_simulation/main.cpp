// Copyright 2017 Pavel Dobryakov
// Copyright 2022 Google LLC
//
// Use of this source code is governed by an MIT-style
// license that can be found in the LICENSE file or at
// https://opensource.org/licenses/MIT.

// Fluid simulation.
//
// This code has been translated and adapted from the original JavaScript WebGL implementation at
// https://github.com/PavelDoGreat/WebGL-Fluid-Simulation.
//
// The code is organized in 3 files:
//
// sim.cpp
//      Contains the main simulation logic. Everything is driven by `class FluidSimulationApp`. Simulation actions
//      queue compute shader dispatch commands in the frame's command buffer.  Most of this code resembles the original
//      JavaScript implementation (https://github.com/PavelDoGreat/WebGL-Fluid-Simulation/blob/master/script.js).
//
// shaders.cpp
//      Contains most the logic required to interact with the BigWheels API to setup and dispatch compute shaders.
//      The main method in those classes is `Dispatch()`, which generates/reuses a descriptor set for the dispatch and
//      queues it in the command buffer.

#include "sim.h"

SETUP_APPLICATION(FluidSim::FluidSimulationApp)