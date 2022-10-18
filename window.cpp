#include "window.h"
#include <cassert>
#include <string>

#include <iostream>

#undef max
#undef min

extern CAppModule module_;

// TODO: this is somewhat broken
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
    SimT peakValue = 0.0001;
    const SimT normalizedPeakValue = 0.2;
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
                (wave.samples[frame * simSampleRateRatio]) *
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


LRESULT ControlDlg::OnInitDialog(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled)
{
    this->startButton.Attach(this->GetDlgItem(IDC_STARTBUTTON));
    this->stopButton.Attach(this->GetDlgItem(IDC_STOPBUTTON));
    this->inputSoundFreqSlider.Attach(this->GetDlgItem(IDC_INPUTFREQSLIDER));
    this->echoIterationsSlider.Attach(this->GetDlgItem(IDC_ECHOITERATIONSSLIDER));
    this->pipeLengthSlider.Attach(this->GetDlgItem(IDC_PIPELENGTHSLIDER));
    this->inputSoundFreqEdit.Attach(this->GetDlgItem(IDC_INPUTFREQEDIT));
    this->echoIterationsEdit.Attach(this->GetDlgItem(IDC_ECHOITERATIONSEDIT));
    this->pipeLengthEdit.Attach(this->GetDlgItem(IDC_PIPELENGTHEDIT));

    this->startButton.EnableWindow(FALSE);

    this->inputSoundFreqSlider.SetRangeMin(200);
    this->inputSoundFreqSlider.SetRangeMax(1500);
    this->setInputSoundFreqPos(static_cast<int>(this->inputSoundFrequency));

    this->echoIterationsSlider.SetRangeMin(1);
    this->echoIterationsSlider.SetRangeMax(200);
    this->setEchoIterationsPos(static_cast<int>(this->echoIterations));

    this->pipeLengthSlider.SetRangeMin(1);
    this->pipeLengthSlider.SetRangeMax(2500);
    this->setPipeLengthPos(static_cast<int>(this->pipeLengthCm));

    return TRUE;
}

LRESULT ControlDlg::OnBnClickedStartbutton(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/)
{
    this->generateInputSound = true;

    this->startButton.EnableWindow(FALSE);
    this->stopButton.EnableWindow(TRUE);
    this->stopButton.SetFocus();

    return 0;
}


LRESULT ControlDlg::OnBnClickedStopbutton(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/)
{
    this->generateInputSound = false;

    this->startButton.EnableWindow(TRUE);
    this->startButton.SetFocus();
    this->stopButton.EnableWindow(FALSE);

    return 0;
}

LRESULT ControlDlg::OnTrackBarScroll(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM /*lParam*/, BOOL& /*bHandled*/)
{
    const int pos = this->inputSoundFreqSlider.GetPos();
    const int pos2 = this->echoIterationsSlider.GetPos();
    const int pos3 = this->pipeLengthSlider.GetPos();

    this->setInputSoundFreqPos(pos);
    this->setEchoIterationsPos(pos2);
    this->setPipeLengthPos(pos3);

    return 0;
}

LRESULT ControlDlg::OnEnChangeInputfreqedit(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/)
{
    // TODO:  If this is a RICHEDIT control, the control will not
    // send this notification unless you override the __super::OnInitDialog()
    // function and call CRichEditCtrl().SetEventMask()
    // with the ENM_CHANGE flag ORed into the mask.

    CString str;
    this->inputSoundFreqEdit.GetWindowTextW(str);

    wchar_t* end;
    const int newInputFreq = std::wcstol(str, &end, 10);

    if(newInputFreq != 0)
        this->setInputSoundFreqPos(newInputFreq, false);

    return 0;
}

LRESULT ControlDlg::OnEnChangeEchoiterationsedit(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/)
{
    // TODO:  If this is a RICHEDIT control, the control will not
    // send this notification unless you override the __super::OnInitDialog()
    // function and call CRichEditCtrl().SetEventMask()
    // with the ENM_CHANGE flag ORed into the mask.

    CString str;
    this->echoIterationsEdit.GetWindowTextW(str);

    wchar_t* end;
    const int newEchoIterations = std::wcstol(str, &end, 10);

    if(newEchoIterations != 0)
        this->setEchoIterationsPos(newEchoIterations, false);

    return 0;
}


void ControlDlg::setInputSoundFreqPos(const int newSliderPos, const bool updateText)
{
    this->inputSoundFrequency = newSliderPos;

    this->inputSoundFreqSlider.SetPos(newSliderPos);

    if(updateText)
    {
        const std::wstring posStr = std::to_wstring(newSliderPos);
        this->inputSoundFreqEdit.SetWindowTextW(posStr.c_str());
    }
}

void ControlDlg::setEchoIterationsPos(const int newSliderPos, const bool updateText)
{
    this->echoIterations = newSliderPos;

    this->echoIterationsSlider.SetPos(newSliderPos);

    if(updateText)
    {
        const std::wstring posStr = std::to_wstring(newSliderPos);
        this->echoIterationsEdit.SetWindowTextW(posStr.c_str());
    }
}

void ControlDlg::setPipeLengthPos(const int newSliderPos, const bool updateText)
{
    this->pipeLengthCm = newSliderPos;

    this->pipeLengthSlider.SetPos(newSliderPos);

    if(updateText)
    {
        const std::wstring posStr = std::to_wstring(newSliderPos);
        this->pipeLengthEdit.SetWindowTextW(posStr.c_str());
    }
}


void ControlDlg::startAudioRenderAndSimulationThread()
{
    assert(!this->simulationThread);
    this->simulationThread.reset(new std::thread {&ControlDlg::simulationThreadEntryPoint, this});
}

void ControlDlg::stopAudioRenderAndSimulationThread()
{
    assert(this->simulationThread);

    this->runSimulation = false;
    this->simulationThread->join();
}

void ControlDlg::simulationThreadEntryPoint()
{
    // debug flag
    const DWORD force_silence = 0; AUDCLNT_BUFFERFLAGS_SILENT;
    ///

    HRESULT hr = S_OK;
    CComPtr<IMMDeviceEnumerator> enumerator;
    CComPtr<IMMDevice> device;
    CComPtr<IAudioClient> audioClient;
    WAVEFORMATEX* mixFormat = nullptr;
    UINT32 bufferFrameCount;
    CComPtr<IAudioRenderClient> renderClient;
    BYTE* audioBuffer;

    CHECK_HR(hr = CoInitialize(nullptr));

    // create audio client
    CHECK_HR(hr = CoCreateInstance(
        __uuidof(MMDeviceEnumerator), nullptr,
        CLSCTX_ALL, __uuidof(IMMDeviceEnumerator), (void**)&enumerator));
    CHECK_HR(hr = enumerator->GetDefaultAudioEndpoint(eRender, eConsole, &device));
    CHECK_HR(hr = device->Activate(__uuidof(IAudioClient), CLSCTX_ALL, nullptr, (void**)&audioClient));
    CHECK_HR(hr = audioClient->GetMixFormat(&mixFormat));
    CHECK_HR(hr = audioClient->Initialize(AUDCLNT_SHAREMODE_SHARED, 0, 0, 0, mixFormat, nullptr));
    CHECK_HR(hr = audioClient->GetBufferSize(&bufferFrameCount));

    if(mixFormat->wBitsPerSample != 32)
        CHECK_HR(hr = E_UNEXPECTED);

    CHECK_HR(hr = audioClient->GetService(__uuidof(IAudioRenderClient), (void**)&renderClient));

    // set the initial buffer
    CHECK_HR(hr = renderClient->GetBuffer(bufferFrameCount, &audioBuffer));

    const SimT simSampleRate = static_cast<SimT>(mixFormat->nSamplesPerSec * simSampleRateRatio);
    Simulation simulation{simSampleRate};

    this->checkAndApplyParameters(simulation);
    const Wave& wave = simulation.progressSimulation(
        static_cast<SimT>(bufferFrameCount * simSampleRateRatio));

    const bool silence = populateAudioBuffer(wave, audioBuffer, bufferFrameCount, mixFormat);
    const DWORD flags = silence ? AUDCLNT_BUFFERFLAGS_SILENT : 0;

    CHECK_HR(hr = renderClient->ReleaseBuffer(bufferFrameCount, flags | force_silence));
    CHECK_HR(hr = audioClient->Start());

    while(this->runSimulation)
    {
        UINT32 numFramesPadding;
        CHECK_HR(hr = audioClient->GetCurrentPadding(&numFramesPadding));

        const UINT32 numFramesAvailable = bufferFrameCount - numFramesPadding;
        if(numFramesAvailable)
        {
            this->checkAndApplyParameters(simulation);

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

    CoTaskMemFree(mixFormat);
    CoUninitialize();
}

void ControlDlg::checkAndApplyParameters(Simulation& simulation)
{
    const bool generateInputSound = this->generateInputSound;
    const int inputSoundFrequency = this->inputSoundFrequency;
    const int echoIterations = this->echoIterations;
    const int pipeLengthCm = this->pipeLengthCm;

    if(generateInputSound)
        simulation.cylinder.start();
    else
        simulation.cylinder.stop();

    simulation.cylinder.setFrequency(inputSoundFrequency);
    if(simulation.pipe.getEchoIterations() != echoIterations)
        simulation.pipe.setEchoIterationsAndReset(echoIterations);
    if(std::lround(simulation.pipe.getPipePhysicalLength() * 100.0) != pipeLengthCm)
        simulation.pipe.setPipePhysicalLengthAndReset(pipeLengthCm / 100.0);
}



/////////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////////



BOOL MainWnd::PreTranslateMessage(MSG* pMsg)
{
    return this->IsDialogMessageW(pMsg);
}

int MainWnd::OnCreate(LPCREATESTRUCT /*createstruct*/)
{
    CMessageLoop* loop = module_.GetMessageLoop();
    loop->AddMessageFilter(this);

    SetClassLongPtr(*this, GCLP_HBRBACKGROUND, (LONG_PTR)GetSysColorBrush(COLOR_3DFACE));

    this->controlDlg.Create(*this);
    this->controlDlg.ShowWindow(SW_SHOW);

    // set this window client size to match the dialog client size
    RECT dlgRect;
    this->controlDlg.GetWindowRect(&dlgRect);
    dlgRect.right -= dlgRect.left;
    dlgRect.bottom -= dlgRect.top;
    dlgRect.left = 0;
    dlgRect.top = 0;
    AdjustWindowRect(&dlgRect, this->GetWndStyle(0), FALSE);
    this->SetWindowPos(nullptr, &dlgRect, SWP_NOMOVE);

    this->controlDlg.startAudioRenderAndSimulationThread();

    return 0;
}

void MainWnd::OnDestroy()
{
    this->controlDlg.stopAudioRenderAndSimulationThread();

    CMessageLoop* loop = module_.GetMessageLoop();
    loop->RemoveMessageFilter(this);

    PostQuitMessage(0);
}