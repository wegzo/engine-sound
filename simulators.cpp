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
{
}

void Cylinder::start()
{
    this->running = true;
}

void Cylinder::stop()
{
    this->running = false;
}

void Cylinder::restart()
{
    this->_restart = true;
}

void Cylinder::progressSimulation(SimT oldSampleCount, SimT newSampleCount)
{
    if(!this->running || this->_restart)
    {
        this->sampleCountStartPosition = oldSampleCount;
        this->_restart = false;
    }
    else if(this->running)
    {
        this->sampleCountStopPosition = oldSampleCount;
    }

    const SimT smoothingDuration = 0.1 / this->currentOutWave.getSampleDuration();
    const int sampleCount = static_cast<int>(newSampleCount - oldSampleCount);

    this->currentOutWave.samples.clear();

    for(int i = 0; i < sampleCount; i++)
    {
        this->counter += (this->frequency * 2 * std::numbers::pi) / this->simulation.samplingRate;

        // avg peak sound pressure level amplitude in conversations
        constexpr SimT conversationAmplitude = 0.02f;

        SimT val = std::sin(this->counter) * conversationAmplitude;

        // start&stop linear smoothing
        if(this->running && oldSampleCount - this->sampleCountStartPosition + i < smoothingDuration)
            val *= (oldSampleCount - this->sampleCountStartPosition + i) / smoothingDuration;
        else if(!this->running && oldSampleCount - this->sampleCountStopPosition + i < smoothingDuration)
            val *= 1.0 - (oldSampleCount - this->sampleCountStopPosition + i) / smoothingDuration;
        else if(!this->running)
            val = 0.0;

        this->currentOutWave.samples.push_back(val);
    }
}


/////////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////////


Pipe::Pipe(Simulation& simulation, Cylinder& cylinder) :
    simulation(simulation),
    cylinder(cylinder),
    pipeLengthPhysical(startPipeLengthPhysicalCm / 100.0),
    pipeRadius(startPipeRadiusCm / 100.0)
{
    // pipe length depends on pipe radius so the order is important
    this->setPipeRadiusAndReset(startPipeRadiusCm / 100.0);
    this->setPipePhysicalLengthAndReset(startPipeLengthPhysicalCm / 100.0);
}

Wave Pipe::sumRadiatedWaves(const size_t sampleCount) const
{
    Wave sumWave{this->simulation, 
        Wave::SampleContainer {sampleCount, 
        0.0, Wave::SampleContainer::allocator_type()}};

    assert(this->radiatedSampleCount <= sampleCount);

    for(const auto& wave : this->radiatedWaves)
    {
        const size_t requestedGeneratedSampleCountDiff = sampleCount - wave.getSampleCount();
        for(size_t i = 0; i < wave.samples.size(); i++)
        {
            sumWave.samples[requestedGeneratedSampleCountDiff + i] += wave.samples[i];
        }
    }

    /*std::cout << this->radiatedWaves.size() << std::endl;*/

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

    // handle the pipe exit wave interactions
    {
        size_t i = 0;
        for(auto it = this->pipeWaves.begin();
            it != this->pipeWaves.end() && i < this->echoIterations;
            it++, i++)
        {
            this->progressPipeWave(it);
        }
    }

    // remove old pipe waves that have negligible pressure variations
    this->prunePipeWaves();
}

void Pipe::progressPipeWave(const std::list<Wave>::iterator waveIt)
{
    assert(waveIt != this->pipeWaves.end());

    const SimT exceedingLength = waveIt->position + waveIt->getLength() - this->pipeLength;
    const size_t sampleCount = waveIt->getSampleCountForLength(exceedingLength);
    if(waveIt->leftToRightDirection && sampleCount >= 2)
    {
        // handle open end wave reflection;
        // there has to be two or more samples so that the rate of change can be calculated
        // (velocity)
        const Wave waveToBePartiallyReflected =
            waveIt->cutWaveBySampleCount(sampleCount - 1) +
            waveIt->copyWaveBySampleCount(1);
        auto [radiatedWave, reflectedWave] =
            this->splitToRadiatedAndReflectedWaves(waveToBePartiallyReflected);

        const SimT straddlingLength = exceedingLength -
            Wave::getLength(sampleCount, 1.0 / this->simulation.samplingRate);
        assert(straddlingLength < Wave::getLength(1.0, 1.0 / this->simulation.samplingRate));
        assert(straddlingLength >= 0);

        reflectedWave.position = straddlingLength;

        this->addRadiatedWave(std::move(radiatedWave));
        this->addPipeWave(std::move(reflectedWave), ++std::list<Wave>::iterator{waveIt});
    }
    else if(!waveIt->leftToRightDirection && sampleCount >= 2)
    {
        // note that the zero position is now at the right end of the pipe
        // and grows to the left direction;
        // handle reflection
        Wave waveToBeReflected = waveIt->cutWaveBySampleCount(sampleCount - 1);
        Wave reflectedWave{this->simulation, std::move(waveToBeReflected.samples)};

        const SimT straddlingLength = exceedingLength -
            Wave::getLength(sampleCount, 1.0 / this->simulation.samplingRate);
        assert(straddlingLength < Wave::getLength(1.0, 1.0 / this->simulation.samplingRate));
        assert(straddlingLength >= 0);

        reflectedWave.position = straddlingLength;

        this->addPipeWave(std::move(reflectedWave), ++std::list<Wave>::iterator{waveIt});
    }
}

void Pipe::prunePipeWaves()
{
    // TODO: implement properly

    // TODO: probably the sound wave needs to lose its energy when it bounces in the pipe

    // for now, just remove pipe waves at a certain threshold
    while(this->pipeWaves.size() > this->echoIterations)
        this->pipeWaves.pop_back();
}

std::pair<Wave, Wave> Pipe::splitToRadiatedAndReflectedWaves(const Wave& wave) const
{
    assert(wave.getSampleCount() >= 2);

    Wave radiatedWave{this->simulation, Wave::SampleContainer {wave.getSampleCount() - 1,
        Wave::SampleContainer::allocator_type()}};
    Wave reflectedWave{this->simulation, Wave::SampleContainer {wave.getSampleCount() - 1,
        Wave::SampleContainer::allocator_type()}, false};

    // a change in the pressure of the wave is an approximation of the flow at the end of the
    // open pipe

    for(size_t i = 0; i < wave.getSampleCount() - 1; i++)
    {
        const SimT pressure1 = wave.samples[i];
        const SimT pressure2 = wave.samples[i + 1];

        // dP = -γ P dy / dt, where γ is the adiabatic factor
        // <=> dy / dt = dP / (-γ P)
        /*const SimT flowVelocity =
            ((pressure2 - pressure1) / (-airAdiabaticFactor * pressure1)) /
            wave.getSampleDuration();*/
        // F = ma <=> a = F/m
        const SimT flowAcceleration = 
            (pressure2 - pressure1) 
            / 
            (Wave::getLength(1.0, wave.getSampleDuration()) * -airDensity);
        /*const SimT volumeFlow = this->pipeCrossSectionalArea * flowVelocity;*/

        // F = ma
        // F = air density * A * L * flowAcceleration
        // pA = air density * A * L * flowAcceleration <=>
        // p = (air density * A * L * flowAcceleration) / A <=>
        // p = air density * L * flowAcceleration, L = endcorrectionfactor * pipe radius
        // Zrad = p/U = (air density * L * flowAcceleration) / volumeFlow
        // radP = Zrad*U

        // p> + p< = prad
        const SimT radiationPressure = 
            airDensity * endCorrectionFactor * this->pipeRadius * flowAcceleration;
        const SimT reflectionPressure = radiationPressure - pressure1;

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

        pos->position = pipeWave.position;
        *pos += pipeWave;
    }
}

void Pipe::reset()
{
    this->clearRadiatedWaves();
    this->pipeWaves.clear();
}

void Pipe::setEchoIterationsAndReset(const size_t echoIterations)
{
    this->echoIterations = echoIterations;
    this->reset();
}

void Pipe::setPipePhysicalLengthAndReset(const SimT pipeLengthPhysical)
{
    assert(pipeLengthPhysical > 0.0);
    assert(this->pipeRadius > 0.0);

    this->pipeLengthPhysical = pipeLengthPhysical;
    this->pipeLength = 
        this->pipeLengthPhysical + endCorrectionFactor * this->pipeRadius - 
        Wave::getLength(1.0, 1.0 / this->simulation.samplingRate);

    this->reset();
}

void Pipe::setPipeRadiusAndReset(const SimT pipeRadius)
{
    assert(pipeRadius > 0.0);

    this->pipeRadius = pipeRadius;
    this->setPipePhysicalLengthAndReset(this->pipeLengthPhysical);
}