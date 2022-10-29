#include "simulation.h"

#include <iostream>

Simulation::Simulation(const SimT samplingRate) : 
    samplingRate(samplingRate), 
    outWave(*this),
    cylinder(*this),
    pipe(*this, this->cylinder)
{
}

const Wave& Simulation::progressSimulation(const SimT sampleCountProgress)
{
    const SimT newSampleCount = this->oldSampleCount + sampleCountProgress;

    this->cylinder.progressSimulation(this->oldSampleCount, newSampleCount);
    this->pipe.progressSimulation(this->oldSampleCount, newSampleCount, sampleCountProgress);

    this->outWave = this->pipe.sumRadiatedWaves(static_cast<size_t>(sampleCountProgress));
    this->pipe.clearRadiatedWaves();

    /*this->outWave = this->cylinder.currentOutWave;*/

    /*system("cls");*/
    /*this->outWave.print();*/

    this->oldSampleCount = newSampleCount;

    return this->outWave;
}