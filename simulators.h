#pragma once
#include "wave.h"
#include <vector>
#include <list>

class Simulation;

constexpr SimT atmosphericPressure = 101325.0f;
constexpr SimT airDensity = 1.2f;
constexpr SimT airAdiabaticFactor = 1.4f;
constexpr SimT endCorrectionFactor = 0.6f;

// creates the initial sound wave
class Cylinder
{
public:
    // wave that has been generated between oldSampleCount and newSampleCount
    Wave currentOutWave;

    Cylinder(Simulation& simulation);

    void progressSimulation(const SimT oldSampleCount, const SimT newSampleCount);
private:
    Simulation& simulation;
};


/////////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////////


// closed-open pipe that reflects some of the wave;
// left side is closed
class Pipe
{
public:
public:
    // note that pipeLength includes the end correction length
    const SimT pipeLength;
    const SimT pipeLengthPhysical;
    const SimT pipeRadius;
    const SimT pipeCrossSectionalArea;

    std::vector<Wave> radiatedWaves;
    std::list<Wave> pipeWaves; // list used for lax iterator invalidation rules

    Pipe(Simulation& simulation, Cylinder& cylinder, 
        const SimT pipeRadius, const SimT pipeLengthPhysical);

    // sums radiated waves with atmospheric pressure
    Wave sumRadiatedWaves(const size_t sampleCount) const;
    void clearRadiatedWaves();

    void progressSimulation(
        const SimT oldSampleCount, const SimT newSampleCount, const SimT deltaSampleCount);
private:
    Simulation& simulation;
    Cylinder& cylinder;
    size_t radiatedSampleCount = 0;

    void progressPipeWave(const std::list<Wave>::iterator waveIt);
    void prunePipeWaves();
    std::pair<Wave, Wave> splitToRadiatedAndReflectedWaves(const Wave& wave) const;
    // adds wave to slot indicated by pos
    void addRadiatedWave(Wave&& radiatedWave);
    void addPipeWave(Wave&& pipeWave, const std::list<Wave>::iterator pos);
};