#include "simulators.h"
#include "simulation.h"
#include <cmath>
#include <algorithm>
#include <numbers>
#include <cassert>

#include <iostream>
#include <iomanip>

Cylinder::Cylinder(Simulation& simulation) : 
    currentOutWave(simulation),
    simulation(simulation)
{}

void Cylinder::progressSimulation(const SimT oldSampleCount, const SimT newSampleCount)
{
    // half a second of linear smoothing at the beginning so that audio artifacts don't occur
    const SimT smoothingDuration = 0.5f / this->currentOutWave.getSampleDuration();

    const int sampleCount = static_cast<int>(newSampleCount - oldSampleCount);
    this->currentOutWave.samples.clear();

    for(int i = 0; i < sampleCount; i++)
    {
        // avg peak sound pressure level amplitude in conversations
        constexpr SimT conversationAmplitude = 0.02f;

        SimT val = std::sin((oldSampleCount + i) / 38.9f) * conversationAmplitude;

        if(oldSampleCount + i < smoothingDuration)
            val *= (oldSampleCount + i) / smoothingDuration;

        val += atmosphericPressure;

        this->currentOutWave.samples.push_back(val);

        /*std::cout << val << std::endl;*/
    }
}


/////////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////////


Pipe::Pipe(Simulation& simulation, Cylinder& cylinder,
    const SimT pipeRadius, const SimT pipeLengthPhysical) :
    pipeLength(pipeLengthPhysical + endCorrectionFactor * pipeRadius),
    pipeLengthPhysical(pipeLengthPhysical),
    pipeRadius(pipeRadius),
    pipeCrossSectionalArea(static_cast<SimT>(std::numbers::pi)* pipeRadius* pipeRadius),
    simulation(simulation),
    cylinder(cylinder)
{
}

Wave Pipe::sumRadiatedWaves(const size_t sampleCount) const
{
    Wave sumWave{this->simulation, 
        Wave::SampleContainer {sampleCount, 
        atmosphericPressure, Wave::SampleContainer::allocator_type()}};

    assert(this->radiatedSampleCount <= sampleCount);

    for(const auto& wave : this->radiatedWaves)
    {
        const size_t requestedGeneratedSampleCountDiff = sampleCount - wave.getSampleCount();
        for(size_t i = 0; i < wave.samples.size(); i++)
        {
            sumWave.samples[requestedGeneratedSampleCountDiff + i] += wave.samples[i];
        }
    }

    return sumWave;
}

void Pipe::clearRadiatedWaves()
{
    this->radiatedWaves.clear();
    this->radiatedSampleCount = 0;
}

void Pipe::progressSimulation(
    const SimT oldSampleCount, const SimT newSampleCount, const SimT deltaSampleCount)
{
    // add the new wave
    Wave newInWave = this->cylinder.currentOutWave;
    this->addPipeWave(std::move(newInWave), this->pipeWaves.begin());

    // single waves longer than pipe length * 2 are not yet supported;
    // TODO: implement it
    {
        const SimT waveLength = this->pipeWaves.begin()->getLength();
        assert(waveLength <= this->pipeLength * 2);
    }

    // handle the pipe exit wave interactions
    for(auto it = this->pipeWaves.begin(); it != this->pipeWaves.end(); it++)
        this->progressPipeWave(it);

    // remove old pipe waves that have negligible pressure variations
    this->prunePipeWaves();
}

void Pipe::progressPipeWave(const std::list<Wave>::iterator waveIt)
{
    assert(waveIt != this->pipeWaves.end());

    if(waveIt->leftToRightDirection)
    {
        // handle open end wave reflection
        const size_t sampleCount = waveIt->getSampleCountForLength(
            waveIt->position + waveIt->getLength() - this->pipeLength);

        // there has to be three or more samples so that the rates of change can be calculated
        // (velocity and acceleration)
        if(sampleCount >= 2)
        {
            const Wave waveToBePartiallyReflected =
                waveIt->cutWaveBySampleCount(sampleCount - 1) +
                waveIt->copyWaveBySampleCount(1);
            auto [radiatedWave, reflectedWave] = 
                this->splitToRadiatedAndReflectedWaves(waveToBePartiallyReflected);

            this->addRadiatedWave(std::move(radiatedWave));
            this->addPipeWave(std::move(reflectedWave), ++std::list<Wave>::iterator{waveIt});
        }
    }
    else
    {
        // note that the zero position is now at the right end of the pipe
        // and grows to the left direction
        const size_t sampleCount = waveIt->getSampleCountForLength(
            waveIt->position + waveIt->getLength() - this->pipeLength);

        // handle reflection
        if(sampleCount >= 2)
        {
            Wave waveToBeReflected = waveIt->cutWaveBySampleCount(sampleCount - 1);
            Wave reflectedWave{this->simulation, std::move(waveToBeReflected.samples)};

            this->addPipeWave(std::move(reflectedWave), ++std::list<Wave>::iterator{waveIt});
        }
    }
}

void Pipe::prunePipeWaves()
{
    // TODO: implement properly

    // TODO: probably the sound wave needs to lose its energy when it bounces in the pipe

    // for now, just remove pipe waves older than 10 iterations
    while(this->pipeWaves.size() > 20)
        this->pipeWaves.pop_back();
}

std::pair<Wave, Wave> Pipe::splitToRadiatedAndReflectedWaves(const Wave& wave) const
{
    assert(wave.getSampleCount() >= 2);

    Wave radiatedWave{this->simulation, {wave.samples.begin(), wave.samples.end() - 1}};
    Wave reflectedWave{this->simulation, {wave.samples.begin(), wave.samples.end() - 1}, false};

    // a change in the pressure of the wave is an approximation of the flow at the end of the
    // open pipe

    for(size_t i = 0; i < wave.samples.size() - 1; i++)
    {
        const SimT pressure1 = wave.samples[i];
        const SimT pressure2 = wave.samples[i + 1];

        // dP = -γ P dy / dt, where γ is the adiabatic factor
        // <=> dy / dt = dP / (-γ P)
        const SimT flowVelocity =
            ((pressure2 - pressure1) / (-airAdiabaticFactor * pressure1)) /
            wave.getSampleDuration();
        const SimT flowAcceleration = (pressure2 - pressure1) / -airDensity;
        const SimT volumeFlow = this->pipeCrossSectionalArea * flowVelocity;

        // F = ma
        // F = air density * A * L * flowAcceleration
        // pA = air density * A * L * flowAcceleration <=>
        // p = (air density * A * L * flowAcceleration) / A <=>
        // p = air density * L * flowAcceleration
        // Zrad = p/U = (air density * L * flowAcceleration) / volumeFlow
        SimT radiationImpedance =
            (airDensity * endCorrectionFactor * this->pipeRadius * flowAcceleration)
            / 
            volumeFlow;
        if(!std::isnormal(radiationImpedance))
            radiationImpedance = 0.0f;

        // p> + p< = prad
        const SimT radiationPressure = radiationImpedance * volumeFlow;
        const SimT reflectionPressure = radiationPressure - pressure1;

        // note: radiation pressure is relative to atmospheric pressure

        radiatedWave.samples[i] = radiationPressure;
        reflectedWave.samples[i] = reflectionPressure;
    }
    
    return {std::move(radiatedWave), std::move(reflectedWave)};
}

void Pipe::addRadiatedWave(Wave&& radiatedWave)
{
    this->radiatedWaves.push_back(std::move(radiatedWave));
    this->radiatedSampleCount =
        std::max(this->radiatedSampleCount, this->radiatedWaves.back().getSampleCount());
}

void Pipe::addPipeWave(Wave&& pipeWave, const std::list<Wave>::iterator pos)
{
    if(pipeWave.samples.empty())
        return;

    if(pos == this->pipeWaves.end())
        this->pipeWaves.insert(pos, std::move(pipeWave));
    else
    {
        assert(pos->leftToRightDirection == pipeWave.leftToRightDirection);
        assert(pos->position == pipeWave.position);
        *pos += pipeWave;
    }
}