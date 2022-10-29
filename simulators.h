#pragma once
#include "wave.h"
#include <vector>
#include <list>

class Simulation;

constexpr SimT atmPressure = 101325.0;
constexpr SimT airDensity = 1.2;
constexpr SimT airAdiabaticFactor = 1.4;
constexpr SimT endCorrectionFactor = 0.6;

// creates the initial sound wave
class Cylinder
{
public:
    static constexpr SimT startFrequency = 500.0;
public:
    // wave that has been generated between oldSampleCount and newSampleCount
    Wave currentOutWave;

    Cylinder(Simulation& simulation);

    void start();
    void stop();
    void restart();
    void setFrequency(const SimT newFrequency) { this->frequency = newFrequency; }
    SimT getFrequency() const { return this->frequency; }
    void setAmplitude(const SimT newAmplitude);

    void progressSimulation(SimT oldSampleCount, SimT newSampleCount);
private:
    Simulation& simulation;
    SimT sampleCountStartPosition = 0.0, sampleCountStopPosition = 0.0;
    bool running = true, _restart = false;
    
    SimT frequency = startFrequency, counter = 0.0;
    SimT amplitude = 0.02;
};


/////////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////////


// closed-open pipe that reflects some of the wave;
// left side is closed
class Pipe
{
public:
    static constexpr size_t startEchoIterations = 10;
    static constexpr int startPipeLengthPhysicalCm = 20;
    static constexpr SimT startPipeRadiusCm = 1;
public:
    std::vector<Wave> radiatedWaves;
    std::list<Wave> pipeWaves; // list used for lax iterator invalidation rules

    Pipe(Simulation& simulation, Cylinder& cylinder);

    void setEchoIterationsAndReset(const size_t echoIterations);
    size_t getEchoIterations() const { return this->echoIterations; }
    void setPipePhysicalLengthAndReset(const SimT pipeLengthPhysical);
    SimT getPipePhysicalLength() const { return this->userPipeLengthPhysical; }
    void setPipeRadiusAndReset(const SimT pipeRadius);
    SimT getPipeRadius() const { return this->pipeRadius; }

    // sums radiated waves with atmospheric pressure
    Wave sumRadiatedWaves(const size_t sampleCount) const;
    void clearRadiatedWaves();

    void progressSimulation(
        const SimT oldSampleCount, const SimT newSampleCount, const SimT deltaSampleCount);
private:
    Simulation& simulation;
    Cylinder& cylinder;
    size_t radiatedSampleCount = 0;
    size_t echoIterations = startEchoIterations;

    SimT userPipeLengthPhysical;
    SimT pipeLengthPhysical;
    SimT pipeLength;
    SimT pipeRadius;
    SimT pipeCrossSectionalArea;

    void progressPipeWave(const std::list<Wave>::iterator waveIt);
    void prunePipeWaves();
    std::pair<Wave, Wave> splitToRadiatedAndReflectedWaves(const Wave& wave) const;
    // adds wave to slot indicated by pos
    void addRadiatedWave(Wave&& radiatedWave);
    void addPipeWave(Wave&& pipeWave, const std::list<Wave>::iterator pos);

    void reset();
};