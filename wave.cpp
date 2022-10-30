#include "wave.h"
#include "simulation.h"

#include <iostream>
#include <iomanip>

Wave::Wave(const Simulation& simulation, 
    const SampleContainer& samples, 
    const bool leftToRightDirection) :
    samples(samples),
    leftToRightDirection(leftToRightDirection),
    simulation(&simulation)
{
}

Wave::Wave(const Simulation& simulation, 
    SampleContainer&& samples, 
    const bool leftToRightDirection) :
    samples(std::move(samples)),
    leftToRightDirection(leftToRightDirection),
    simulation(&simulation)
{
}

SimT Wave::getSampleDuration() const
{
    return 1.0 / this->simulation->samplingRate;
}

SimT Wave::getLength() const
{
    return getLength(this->samples.size(), this->getSampleDuration());
}

SimT Wave::getLength(const SimT sampleCount, const SimT sampleDuration)
{
    return sampleCount * waveSpeed * sampleDuration;
}

SimT Wave::getLength(const size_t sampleCount, const SimT sampleDuration)
{
    return sampleCount * waveSpeed * sampleDuration;
}

void Wave::moveByDuration(const SimT duration)
{
    const SimT moveFactor = this->leftToRightDirection ? 1.0 : -1.0;
    this->position += moveFactor * waveSpeed * duration;
}

size_t Wave::getSampleCountForDuration(const SimT duration) const
{
    return std::max(0, 
        std::min(
            static_cast<int>(duration / this->getSampleDuration()),
            static_cast<int>(this->samples.size())));
}

Wave Wave::cutWaveBySampleCount(const size_t sampleCount)
{
    const Wave newWave = this->copyWaveBySampleCount(sampleCount);
    this->samples.erase(this->samples.begin(), this->samples.begin() + sampleCount);

    return newWave;
}

Wave Wave::copyWaveBySampleCount(const size_t sampleCount)
{
    return Wave{*this->simulation, {this->samples.begin(), this->samples.begin() + sampleCount}};
}

Wave& Wave::operator+(const Wave& rhs) &&
{
    *this += rhs;
    return *this;
}

Wave& Wave::operator+=(const Wave& rhs)
{
    this->samples.insert(this->samples.end(), rhs.samples.begin(), rhs.samples.end());
    return *this;
}

void Wave::print() const
{
    for(const auto pressure : this->samples)
        std::cout << std::setprecision(12) << pressure << std::endl;
}