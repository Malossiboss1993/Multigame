#include "MainGame.h"

#include <iostream>

#include <imgui/imgui.h>
#include <imgui/imgui_impl_glfw_gl3.h>

#include <main_app/MainApp.h>
#include <main_app/renderer/Renderer.h>
#include <main_app/audio_player/AudioPlayer.h>
#include <main_app/panel/Panel.h>
#include <main_app/applications/roulette/math_logic/RouletteMathLogic.h>
#include <debug/Logger.h>

constexpr unsigned int g_unTimerHoldWins = 1;
constexpr unsigned int g_unTimerHoldWinsPeriod = 1000;
constexpr unsigned int g_unDurationHoldWins = 7;

constexpr unsigned int g_unTimerHoldLoses = 2;
constexpr unsigned int g_unTimerHoldLosesPeriod = 1000;
constexpr unsigned int g_unDurationHoldLoses = 3;

Roulette::Roulette()
{
}

bool Roulette::Init()
{

    /*Load textures*/
    m_textureBackground = Texture::CreateTexture("../src/resources/roulette/background.png");

    if (!m_textureBackground->Load())
    {
        LOG_ERROR("Roulette - Unable to load texture background !");
        return false;
    }

    /*Initialize Math Logic*/
    if (!RouletteMathLogic::GetInstance().Init())
    {
        LOG_ERROR("Roulette - Unable to init math logic !");
        return false;
    }

    /*Initialize table area*/
    if (!m_tableArea.Init())
    {
        LOG_ERROR("Roulette - Unable to init table area !");
        return false;
    }

    /*Initialize wheel area*/
    if (!m_wheelArea.Init())
    {
        LOG_ERROR("Roulette - Unable to init wheel area !");
        return false;
    }

    /*Initialize status line*/
    if (!m_statusLine.Init())
    {
        LOG_ERROR("Roulette - Unable to Init StatusLine !");
        return false;
    }

    /*Set callback when finished spinning, invoke it*/
    std::function<void()> afterSpinningStoppedCallback = [this]()
    {
        AfterSpinningStopped();
    };
    m_wheelArea.SetAfterSpinningStoppedCallback(afterSpinningStoppedCallback);

    LOG_INFO("Roulette - Initialized ...");
    return true;
}

bool Roulette::Deinit()
{
    /*Deinit wheel area*/
    m_wheelArea.Deinit();

    /*Deinit table area*/
    m_tableArea.Deinit();

    /*Deinit status line*/
    m_statusLine.Deinit();

    /*Deinitialize math logic*/
    RouletteMathLogic::GetInstance().Deinit();

    LOG_WARN("Roulette - Deinitalized ...");
    return true;
}

bool Roulette::HandleEvent()
{
    const auto &nXMouse = ImGui::GetMousePos().x;
    const auto &nYMouse = ImGui::GetMousePos().y;

    /*Panel Handle Event*/
    if (m_eState == ERouletteStates::eReadyForGame)
    {
        if (MainApp::GetInstance().ptrPanel->HandleEvent())
        {
            return true;
        }
    }

    /*If calculator or volume scene active, no handle event for MainGame*/
    if (MainApp::GetInstance().ptrPanel->GetPanelInfoScene() != EPanelInfoScenes::eNoInfoScene)
    {
        return false;
    }

    /*ENTER button*/
    if (ImGui::IsKeyPressed(ImGui::GetKeyIndex(ImGuiKey_Enter), false))
    {
        LOG_INFO("Roulette - ENTER Button Pressed");
        StartNewGame();
        return true;
    }

    /*Table Area Handle Event*/
    m_tableArea.HandleEvent();

    /*Wheel Area Handle Event*/
    m_wheelArea.HandleEvent();

    return false;
}

const std::string &Roulette::GetAppName()
{
    return m_strAppName;
}

void Roulette::RequestState(ERouletteStates eStateToRequest)
{
    std::string strStateToRequest = "N/A";
    std::string strCurrentState = "N/A";

    /*Current State*/
    switch (m_eState)
    {
    case ERouletteStates::eInactive:
    {
        strCurrentState = "eInactive";
    }
    break;

    case ERouletteStates::eReadyForGame:
    {
        strCurrentState = "eReadyForGame";
    }
    break;

    case ERouletteStates::eSpinning:
    {
        strCurrentState = "eSpinning";
    }
    break;

    case ERouletteStates::eAfterSpinningStopped:
    {
        strCurrentState = "eAfterSpinningStopped";
    }
    break;

    case ERouletteStates::eWinFromGame:
    {
        strCurrentState = "eWinFromGame";
    }
    break;

    case ERouletteStates::eNoWinFromGame:
    {
        strCurrentState = "eNoWinFromGame";
    }
    break;

    default:
        break;
    }

    /*State to Request*/
    switch (eStateToRequest)
    {
    case ERouletteStates::eInactive:
    {
        strStateToRequest = "eInactive";

        if (m_eState == ERouletteStates::eReadyForGame)
        {
            LOG_INFO("Roulette - Possible request state: \"{0}\" -> \"{1}\"", strCurrentState, strStateToRequest);
            m_eState = eStateToRequest;
        }
        else
        {
            LOG_ERROR("Roulette - Impossible request state: \"{0}\" -> \"{1}\"", strCurrentState, strStateToRequest);
        }
    }
    break;

    case ERouletteStates::eReadyForGame:
    {
        strStateToRequest = "eReadyForGame";

        if (m_eState == ERouletteStates::eInactive ||
            m_eState == ERouletteStates::eAfterSpinningStopped ||
            m_eState == ERouletteStates::eWinFromGame ||
            m_eState == ERouletteStates::eNoWinFromGame)
        {
            LOG_INFO("Roulette - Possible request state: \"{0}\" -> \"{1}\"", strCurrentState, strStateToRequest);
            m_eState = eStateToRequest;
        }
        else
        {
            LOG_ERROR("Roulette - Impossible request state: \"{0}\" -> \"{1}\"", strCurrentState, strStateToRequest);
        }
    }
    break;

    case ERouletteStates::eSpinning:
    {
        strStateToRequest = "eSpinning";

        if (m_eState == ERouletteStates::eReadyForGame)
        {
            LOG_INFO("Roulette - Possible request state: \"{0}\" -> \"{1}\"", strCurrentState, strStateToRequest);
            m_eState = eStateToRequest;
        }
        else
        {
            LOG_ERROR("Roulette - Impossible request state: \"{0}\" -> \"{1}\"", strCurrentState, strStateToRequest);
        }
    }
    break;

    case ERouletteStates::eAfterSpinningStopped:
    {
        strStateToRequest = "eAfterSpinningStopped";

        if (m_eState == ERouletteStates::eSpinning)
        {
            LOG_INFO("Roulette - Possible request state: \"{0}\" -> \"{1}\"", strCurrentState, strStateToRequest);
            m_eState = eStateToRequest;
        }
        else
        {
            LOG_ERROR("Roulette - Impossible request state: \"{0}\" -> \"{1}\"", strCurrentState, strStateToRequest);
        }
    }
    break;

    case ERouletteStates::eWinFromGame:
    {
        strStateToRequest = "eWinFromGame";

        if (m_eState == ERouletteStates::eAfterSpinningStopped)
        {
            LOG_INFO("Roulette - Possible request state: \"{0}\" -> \"{1}\"", strCurrentState, strStateToRequest);
            m_eState = eStateToRequest;
        }
        else
        {
            LOG_ERROR("Roulette - Impossible request state: \"{0}\" -> \"{1}\"", strCurrentState, strStateToRequest);
        }
    }
    break;

    case ERouletteStates::eNoWinFromGame:
    {
        strStateToRequest = "eNoWinFromGame";

        if (m_eState == ERouletteStates::eAfterSpinningStopped)
        {
            LOG_INFO("Roulette - Possible request state: \"{0}\" -> \"{1}\"", strCurrentState, strStateToRequest);
            m_eState = eStateToRequest;
        }
        else
        {
            LOG_ERROR("Roulette - Impossible request state: \"{0}\" -> \"{1}\"", strCurrentState, strStateToRequest);
        }
    }
    break;

    default:
        break;
    }
}

void Roulette::StartNewGame(int unDemoSectorNumber)
{
    /*A new spin is only valid after the previous round has fully reset. Ignoring
      the request while the wheel is holding prevents overlapping rounds and keeps
      the state machine in sync when the player presses Enter repeatedly.*/
    if (m_eState != ERouletteStates::eReadyForGame)
    {
        return;
    }

    if (m_wheelArea.StartNewSpin(unDemoSectorNumber))
    {
        RequestState(ERouletteStates::eSpinning);
        m_tableArea.LockBetTable();
        m_statusLine.NeedToShowGoodLuckScenario();
    }
}

void ::Roulette::AfterSpinningStopped()
{
    RequestState(ERouletteStates::eAfterSpinningStopped);

    const auto &unWinFromGame = RouletteMathLogic::GetInstance().GetTotalWinFromGame();
    /*If there is a win, start holding timer*/
    if (unWinFromGame != 0)
    {
        RequestState(ERouletteStates::eWinFromGame);
        MainApp::GetInstance().ptrPanel->StartWinCounting(unWinFromGame);
        MainApp::GetInstance().ptrTimer->StartTimer(this, g_unTimerHoldWins, g_unTimerHoldWinsPeriod);
        m_statusLine.StartWinScenario();
    }
    else /*If there is NO win, reset game elements after timer expires*/
    {
        RequestState(ERouletteStates::eNoWinFromGame);
        MainApp::GetInstance().ptrTimer->StartTimer(this, g_unTimerHoldLoses, g_unTimerHoldLosesPeriod);
        m_statusLine.StartNormalScenario();
    }

    m_tableArea.StartWinAnimations();
    LOG_INFO("Roulette - Start Hold Timer");
}

void Roulette::OnEnter()
{
    RequestState(ERouletteStates::eReadyForGame);
    MainApp::GetInstance().ptrPanel->LockBetButtons();
    m_statusLine.StartNormalScenario();
    LOG_INFO("Roulette - Transition to Application succeed");
}

void Roulette::OnExit()
{
    RequestState(ERouletteStates::eInactive);
    LOG_INFO("Roulette - Exit from Application");
    m_wheelArea.StopRotation();
    m_statusLine.StopNormalScenario();
    MainApp::GetInstance().ptrAudioPlayer->PlaySound("../src/resources/panel/sounds/exit.wav");
}

void Roulette::OnDraw()
{
    const auto &rend = MainApp::GetInstance().ptrRend;

    /*Draw Background*/
    rend->DrawPicture(m_textureBackground, 0.0f, 0.0f);

    /*Draw StatusLine*/
    m_statusLine.Draw();

    /*Table Area Draw*/
    m_tableArea.Draw();

    /*Wheel Area Draw*/
    m_wheelArea.Draw();

    /*Draw Demo Menu*/
    DrawDemoMenu();

    /*Draw Panel*/
    MainApp::GetInstance().ptrPanel->OnDraw();
}

void Roulette::DrawDemoMenu()
{
    ImGui::Begin("Roulette Demo", &m_bDemoMenu);

    for (unsigned int i = GameDefs::eSector32; i <= GameDefs::eSector0; ++i)
    {
        if (i % 3)
        {
            ImGui::SameLine();
        }

        std::string strButtonName = "Sector Number: " + std::to_string(GameDefs::g_arrSectorNumbers[i]);
        if (ImGui::Button(strButtonName.c_str()))
        {
            StartNewGame(i);
            ImGui::SetWindowCollapsed(true);
            break;
        }
    }

    ImGui::End();
}

void Roulette::OnTick(unsigned int unID, unsigned int unTimes)
{
    if (unID == g_unTimerHoldWins)
    {
        if (unTimes >= g_unDurationHoldWins && m_tableArea.IsEndGameScenarioFinished())
        {
            MainApp::GetInstance().ptrTimer->StopTimer(this, g_unTimerHoldWins);
            ResetGameElements();
        }
    }

    if (unID == g_unTimerHoldLoses)
    {
        if (unTimes >= g_unDurationHoldLoses && m_tableArea.IsEndGameScenarioFinished())
        {
            MainApp::GetInstance().ptrTimer->StopTimer(this, g_unTimerHoldLoses);
            ResetGameElements();
        }
    }
}

void Roulette::ResetGameElements()
{
    const auto &unWinFromGame = RouletteMathLogic::GetInstance().GetTotalWinFromGame();
    if (unWinFromGame != 0)
    {
        MainApp::GetInstance().ptrPanel->FastCollectCounting();
        MainApp::GetInstance().ptrPanel->ResetWin();
    }
    m_tableArea.ResetTableElements();
    RouletteMathLogic::GetInstance().ResetValuesToWinSector();
    RequestState(ERouletteStates::eReadyForGame);
    m_tableArea.UnlockBetTable();
    m_statusLine.StopWinScenario();
    m_statusLine.StopNormalScenario();
    m_statusLine.StartNormalScenario();
    LOG_INFO("Roulette - Stop Hold Timer, Reset Containers");
}