#pragma once
#include <vector>
#include <type_traits>

class Simulation;
using SimT = double;

constexpr SimT waveSpeed = 340.6520; // speed of sound in air at 15 c

class Wave
{
public:
    using SampleContainer = std::vector<SimT>;
public:
    // collection of sound pressure samples;
    // duration of a single sample is 1/samplingRate;
    // samples are continuous;
    // denotes the difference to atmospheric pressure
    SampleContainer samples;

    // position of the newest(last) sample
    SimT position = 0.0;
    bool leftToRightDirection;

    Wave(const Simulation& simulation, 
        const SampleContainer& samples = {}, 
        const bool leftToRightDirection = true);
    Wave(const Simulation& simulation, 
        SampleContainer&& samples, 
        const bool leftToRightDirection = true);

    SimT getLength() const;
    static SimT getLength(const SimT sampleCount, const SimT sampleDuration);
    static SimT getLength(const size_t sampleCount, const SimT sampleDuration);
    SimT getSampleDuration() const;
    size_t getSampleCount() const {
        // empty() is checked first because it's guaranteed to be true if the vector has been moved
        return this->samples.empty() ? 0 : this->samples.size();
    }

    void moveByDuration(const SimT duration);

    // returns the amount of samples that are fully included in the duration
    size_t getSampleCountForDuration(const SimT duration) const;
    size_t getSampleCountForLength(const SimT length) const 
    { return this->getSampleCountForDuration(length / waveSpeed); }

    // moves samples from this to the returned wave by sampleCount;
    // cut start position is the oldest(first) sample
    Wave cutWaveBySampleCount(const size_t sampleCount);
    Wave copyWaveBySampleCount(const size_t sampleCount);

    // concatenates the waves
    Wave& operator+(const Wave& rhs) &&;
    // appends to this wave
    Wave& operator+=(const Wave& rhs);

    void print() const;

private:
    const Simulation* simulation;
};

static_assert(std::is_move_constructible_v<Wave>);
static_assert(std::is_move_assignable_v<Wave>);