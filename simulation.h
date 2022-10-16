#pragma once

#include "wave.h"
#include "simulators.h"

constexpr SimT pipeLength = 10.0f;
constexpr SimT pipeRadius = 0.025f;

// contains the simulators of different parts of the engine simulation;
// runs the simulators in correct order to preserve causality of different parts of the simulation;
// SI units are used
class Simulation
{
public:
    const SimT samplingRate;
    Wave outWave;

    Simulation(const SimT samplingRate);

    // amount of samples to be processed;
    // returns the generated wave of sample count
    const Wave& progressSimulation(const SimT sampleCountProgress);

private:
    SimT oldSampleCount = 0;
    Cylinder cylinder;
    Pipe pipe;
};