#pragma once

#include "wtl.h"
#include "wave.h"
#include "simulation.h"
#include "simulators.h"
#include <Audioclient.h>
#include <mmdeviceapi.h>
#include <thread>
#include <memory>
#include <atomic>

class ControlDlg final : public CDialogImpl<ControlDlg>
{
public:
    enum { IDD = IDD_CONTROLDLG };

    BEGIN_MSG_MAP(ControlDlg)
        MESSAGE_HANDLER(WM_INITDIALOG, OnInitDialog)
        MESSAGE_HANDLER(WM_HSCROLL, OnTrackBarScroll)
        COMMAND_HANDLER(IDC_STARTBUTTON, BN_CLICKED, OnBnClickedStartbutton)
        COMMAND_HANDLER(IDC_STOPBUTTON, BN_CLICKED, OnBnClickedStopbutton)

        COMMAND_HANDLER(IDC_INPUTFREQEDIT, EN_CHANGE, OnEnChangeInputfreqedit)
        COMMAND_HANDLER(IDC_ECHOITERATIONSEDIT, EN_CHANGE, OnEnChangeEchoiterationsedit)
    END_MSG_MAP()

    LRESULT OnInitDialog(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled);
    LRESULT OnTrackBarScroll(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM /*lParam*/, BOOL& /*bHandled*/);
    LRESULT OnBnClickedStartbutton(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/);
    LRESULT OnBnClickedStopbutton(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/);
    LRESULT OnEnChangeInputfreqedit(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/);
    LRESULT OnEnChangeEchoiterationsedit(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/);

    void startAudioRenderAndSimulationThread();
    void stopAudioRenderAndSimulationThread();

private:
    CButton startButton, stopButton;
    CTrackBarCtrl inputSoundFreqSlider, echoIterationsSlider, pipeLengthSlider;
    CEdit inputSoundFreqEdit, echoIterationsEdit, pipeLengthEdit;

    void setInputSoundFreqPos(const int newSliderPos, const bool updateText = true);
    void setEchoIterationsPos(const int newSliderPos, const bool updateText = true);
    void setPipeLengthPos(const int newSliderPos, const bool updateText = true);

    // audio/simulation thread stuff below

    std::unique_ptr<std::thread> simulationThread;
    std::atomic_bool runSimulation = true;
    std::atomic_bool generateInputSound = true;
    std::atomic_int inputSoundFrequency = static_cast<int>(Cylinder::startFrequency);
    std::atomic_int echoIterations = static_cast<int>(Pipe::startEchoIterations);
    std::atomic_int pipeLengthCm = static_cast<int>(Pipe::startPipeLengthPhysicalCm);

    // audio/simulation thread context funcs below

    void simulationThreadEntryPoint();
    void checkAndApplyParameters(Simulation&);
};


/////////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////////

static const wchar_t windowTitle[] = L"Audio controller";

class MainWnd final : 
    public CWindowImpl<MainWnd, CWindow, CWinTraits<
    (WS_OVERLAPPEDWINDOW & ~(WS_MAXIMIZEBOX | WS_THICKFRAME)) |
    WS_CLIPCHILDREN | WS_CLIPSIBLINGS, WS_EX_APPWINDOW | WS_EX_WINDOWEDGE>>,
    public CMessageFilter
{
public:
    DECLARE_WND_CLASS(L"Audio controller")

    BOOL PreTranslateMessage(MSG* pMsg);

    BEGIN_MSG_MAP(MainWnd)
        MSG_WM_CREATE(OnCreate)
        MSG_WM_DESTROY(OnDestroy)
    END_MSG_MAP()

    int OnCreate(LPCREATESTRUCT);
    void OnDestroy();
private:
    ControlDlg controlDlg;
};