#include "wave.h"
#include "simulation.h"
#include "simulators.h"

#include <limits>
#include <iostream>
#include <Audioclient.h>
#include <mmdeviceapi.h>
#include <atlbase.h>
#include <cassert>

#undef max
#undef min

static_assert(std::numeric_limits<SimT>::is_iec559);

const int simSampleRateRatio = 1;

inline void CHECK_HR(const HRESULT hr) 
{
    if(FAILED(hr)) 
    {
        std::cout << std::hex << "0x" << hr << std::endl; 
        system("pause"); 
        abort();
    }
}

// returns if the buffer is just silence
bool populateAudioBuffer(
    const Wave& wave,
    BYTE* const audioBuffer,
    const UINT32 bufferFrameCount,
    const WAVEFORMATEX* const mixFormat)
{
    assert(wave.samples.size() == bufferFrameCount * simSampleRateRatio);

    // find the peak value and normalize in relation to that
    SimT peakValue = 0.00005f;
    const SimT normalizedPeakValue = 0.1f;
    /*for(auto val : wave.samples)
    {
        val -= atmosphericPressure;
        peakValue = std::max(peakValue, std::abs(val));
    }*/
    const SimT normalizationFactor = normalizedPeakValue / peakValue;

    for(UINT32 frame = 0; frame < bufferFrameCount; frame++)
    {
        float* const samplePtr =
            reinterpret_cast<float* const>(audioBuffer + frame * mixFormat->nBlockAlign);
        for(WORD channel = 0; channel < mixFormat->nChannels; channel++)
        {
            samplePtr[channel] = static_cast<float>(
                (wave.samples[frame * simSampleRateRatio] - atmosphericPressure) * 
                normalizationFactor);
            
            if(samplePtr[channel] > normalizedPeakValue)
                samplePtr[channel] = static_cast<float>(normalizedPeakValue);
            if(samplePtr[channel] < -normalizedPeakValue)
                samplePtr[channel] = -static_cast<float>(normalizedPeakValue);
        }
        /*std::cout << samplePtr[0] << std::endl;*/
    }

    return !std::isnormal(normalizationFactor);
}

int main()
{
    // debug flag
    const DWORD force_silence = 0; AUDCLNT_BUFFERFLAGS_SILENT;
    ///

    HRESULT hr = S_OK;
    CComPtr<IMMDeviceEnumerator> enumerator;
    CComPtr<IMMDevice> device;
    CComPtr<IAudioClient> audioClient;
    CComPtr<IAudioRenderClient> renderClient;
    WAVEFORMATEX* mixFormat = nullptr;
    UINT32 bufferFrameCount;
    BYTE* audioBuffer;

    CHECK_HR(hr = CoInitialize(nullptr));
    CHECK_HR(hr = CoCreateInstance(
        __uuidof(MMDeviceEnumerator), nullptr,
        CLSCTX_ALL, __uuidof(IMMDeviceEnumerator), (void**)&enumerator));
    CHECK_HR(hr = enumerator->GetDefaultAudioEndpoint(eRender, eConsole, &device));
    CHECK_HR(hr = device->Activate(__uuidof(IAudioClient), CLSCTX_ALL, nullptr, (void**)&audioClient));
    CHECK_HR(hr = audioClient->GetMixFormat(&mixFormat));
    CHECK_HR(hr = audioClient->Initialize(AUDCLNT_SHAREMODE_SHARED, 0, 0, 0, mixFormat, nullptr));
    CHECK_HR(hr = audioClient->GetBufferSize(&bufferFrameCount));
    CHECK_HR(hr = audioClient->GetService(__uuidof(IAudioRenderClient), (void**)&renderClient));

    if(mixFormat->wBitsPerSample != 32)
        abort();

    // set the initial buffer
    CHECK_HR(hr = renderClient->GetBuffer(bufferFrameCount, &audioBuffer));

    const SimT simSampleRate = static_cast<SimT>(mixFormat->nSamplesPerSec * simSampleRateRatio);
    Simulation simulation(simSampleRate);
    const Wave& wave = simulation.progressSimulation(
        static_cast<SimT>(bufferFrameCount * simSampleRateRatio));

    const bool silence = populateAudioBuffer(wave, audioBuffer, bufferFrameCount, mixFormat);
    const DWORD flags = silence ? AUDCLNT_BUFFERFLAGS_SILENT : 0;

    CHECK_HR(hr = renderClient->ReleaseBuffer(bufferFrameCount, flags | force_silence));
    CHECK_HR(hr = audioClient->Start());

    for(;;)
    {
        UINT32 numFramesPadding;
        CHECK_HR(hr = audioClient->GetCurrentPadding(&numFramesPadding));

        const UINT32 numFramesAvailable = bufferFrameCount - numFramesPadding;
        if(numFramesAvailable)
        {
            const Wave& wave = simulation.progressSimulation(
                static_cast<SimT>(numFramesAvailable * simSampleRateRatio));

            CHECK_HR(hr = renderClient->GetBuffer(numFramesAvailable, &audioBuffer));

            const bool silence = 
                populateAudioBuffer(wave, audioBuffer, numFramesAvailable, mixFormat);
            const DWORD flags = silence ? AUDCLNT_BUFFERFLAGS_SILENT : 0;

            CHECK_HR(hr = renderClient->ReleaseBuffer(numFramesAvailable, flags | force_silence));
        }
        else
        {
            // yield
            Sleep(0);
        }
    }

//done:
    CoTaskMemFree(mixFormat);
//    if(FAILED(hr))
//    {
//        std::cerr << "failed: " << hr << std::endl;
//        system("pause");
//    }

    return 0;
}