#include "MainGame.h"

#include <iostream>

#include <imgui/imgui.h>
#include <imgui/imgui_impl_glfw_gl3.h>

#include <main_app/MainApp.h>
#include <main_app/renderer/Renderer.h>
#include <main_app/audio_player/AudioPlayer.h>
#include <main_app/panel/Panel.h>
#include <main_app/applications/kids_fantasy/math_logic/MathLogic.h>
#include <debug/Logger.h>

constexpr unsigned int g_unTimerDemoMode = 1;
constexpr unsigned int g_unTimerDemoModePeriod = 1000;

constexpr unsigned int g_unTimerResourcesLoading = 2;
constexpr unsigned int g_unTimerResourcesLoadingPeriod = 1;

KidsFantasy::KidsFantasy()
{
}

bool KidsFantasy::Init()
{

    /*Load textures*/
    m_textureBackground = Texture::CreateTexture("../src/resources/kids_fantasy/kids_fantasy_background.png");

    if (!m_textureBackground->Load())
    {
        LOG_ERROR("KidsFantasy - Unable to load texture background !");
        return false;
    }

    /*MathLogic Init*/
    if (!MathLogic::GetInstance().Init())
    {
        LOG_ERROR("Kids Fantasy - Unable to Init Math Logic !");
        return false;
    }

    /*Reels Area Init*/
    if (!m_reelsArea.Init())
    {
        LOG_ERROR("Kids Fantasy - Unable to Init Reels Area !");
        return false;
    }

    if (!m_statusLine.Init())
    {
        LOG_ERROR("Kids Fantasy - Unable to Init StatusLine !");
        return false;
    }

    if (!m_lines.Init())
    {
        LOG_ERROR("Kids Fantasy - Unable to Init LinesMgr !");
        return false;
    }

    /*Set after reeling stopped callback*/
    std::function<void()> afterReelingStopped = [this]()
    {
        AfterReelingStopped();
    };
    m_reelsArea.SetAfterReelingStoppedCallback(afterReelingStopped);

    auto textureParticle = Texture::CreateTexture("../src/resources/kids_fantasy/reels_area/particle_sun.png");

    textureParticle->Load();

    m_particle.Init(textureParticle, {500.0f, 500.0f});

    LOG_INFO("Kids Fantasy - Initialized ...");
    return true;
}

bool KidsFantasy::Deinit()
{
    if (m_threadAnimationLoading.joinable())
    {
        m_threadAnimationLoading.join();
    }

    /*Reels Area Deinit*/
    m_reelsArea.Deinit();

    /*StatusLine Deinit*/
    m_statusLine.Deinit();

    /*LinesMgr Deinit*/
    m_lines.Deinit();

    /*Math Logic Deinit*/
    MathLogic::GetInstance().Deinit();

    LOG_WARN("Kids Fantasy - Deinitialized ...");
    return true;
}

bool KidsFantasy::HandleEvent()
{
    const auto &nXMouse = ImGui::GetMousePos().x;
    const auto &nYMouse = ImGui::GetMousePos().y;

    // m_particle.SetPosition({nXMouse, nYMouse});

    // if (ImGui::IsMouseDown(0))
    // {
    //     m_particle.StartEmitting();
    // }
    // else
    // {
    //     m_particle.StopEmitting();
    // }

    /*Panel Handle Event*/
    if (m_eState == EKidsFantasyStates::eReadyForGame)
    {
        MainApp::GetInstance().ptrPanel->HandleEvent();
    }

    /*If calculator or volume scene active, no handle event for MainGame*/
    if (MainApp::GetInstance().ptrPanel->GetPanelInfoScene() != EPanelInfoScenes::eNoInfoScene)
    {
        return false;
    }

    /*Enter Button*/
    if (ImGui::IsKeyPressed(ImGui::GetKeyIndex(ImGuiKey_Enter), false))
    {
        LOG_INFO("Kids Fantasy - ENTER Button Pressed");

        InvokeStartButton();
        return true;
    }

    /*Reels Area Handle Event*/
    m_reelsArea.HandleEvent();

    return false;
}

void KidsFantasy::InvokeStartButton(bool bDemoMode)
{
    /*Take money counting and request ready for game*/
    if (m_eState == EKidsFantasyStates::eWinFromGame)
    {
        MainApp::GetInstance().ptrPanel->FastCollectCounting();
        m_statusLine.StopWinScenario();
        m_lines.Stop();
        RequestState(EKidsFantasyStates::eReadyForGame);

        /*Collecting a win must not also start a new game in the same press.*/
        return;
    }

    /*Start new Game*/
    if (m_eState == EKidsFantasyStates::eReadyForGame &&
        m_reelsArea.StartNewGame(bDemoMode))
    {
        RequestState(EKidsFantasyStates::eReeling);
        m_statusLine.NeedToShowGoodLuckScenario();
        MainApp::GetInstance().ptrAudioPlayer->PlaySound("../src/resources/kids_fantasy/sounds/press_start.wav");
    }
}

const std::string &KidsFantasy::GetAppName()
{
    return m_strAppName;
}

void KidsFantasy::RequestState(EKidsFantasyStates eStateToRequest)
{
    std::string strStateToRequest = "N/A";
    std::string strCurrentState = "N/A";

    /*Current State*/
    switch (m_eState)
    {
    case EKidsFantasyStates::eInactive:
    {
        strCurrentState = "eInactive";
    }
    break;

    case EKidsFantasyStates::eReadyForGame:
    {
        strCurrentState = "eReadyForGame";
    }
    break;

    case EKidsFantasyStates::eReeling:
    {
        strCurrentState = "eReeling";
    }
    break;

    case EKidsFantasyStates::eAfterReelingStopped:
    {
        strCurrentState = "eAfterReelingStopped";
    }
    break;

    case EKidsFantasyStates::eWinFromGame:
    {
        strCurrentState = "eWinFromGame";
    }
    break;

    default:
        break;
    }

    /*State to Request*/
    switch (eStateToRequest)
    {
    case EKidsFantasyStates::eInactive:
    {
        strStateToRequest = "eInactive";

        if (m_eState == EKidsFantasyStates::eReadyForGame)
        {
            LOG_INFO("Kids Fantasy - Possible request state: \"{0}\" -> \"{1}\"", strCurrentState, strStateToRequest);
            m_eState = eStateToRequest;
        }
        else
        {
            LOG_ERROR("Kids Fantasy - Impossible request state: \"{0}\" -> \"{1}\"", strCurrentState, strStateToRequest);
        }
    }
    break;

    case EKidsFantasyStates::eReadyForGame:
    {
        strStateToRequest = "eReadyForGame";

        if (m_eState == EKidsFantasyStates::eInactive ||
            m_eState == EKidsFantasyStates::eAfterReelingStopped ||
            m_eState == EKidsFantasyStates::eWinFromGame)
        {
            LOG_INFO("Kids Fantasy - Possible request state: \"{0}\" -> \"{1}\"", strCurrentState, strStateToRequest);
            m_eState = eStateToRequest;
        }
        else
        {
            LOG_ERROR("Kids Fantasy - Impossible request state: \"{0}\" -> \"{1}\"", strCurrentState, strStateToRequest);
        }
    }
    break;

    case EKidsFantasyStates::eReeling:
    {
        strStateToRequest = "eReeling";

        if (m_eState == EKidsFantasyStates::eReadyForGame)
        {
            LOG_INFO("Kids Fantasy - Possible request state: \"{0}\" -> \"{1}\"", strCurrentState, strStateToRequest);
            m_eState = eStateToRequest;
        }
        else
        {
            LOG_ERROR("Kids Fantasy - Impossible request state: \"{0}\" -> \"{1}\"", strCurrentState, strStateToRequest);
        }
    }
    break;

    case EKidsFantasyStates::eAfterReelingStopped:
    {
        strStateToRequest = "eAfterReelingStopped";

        if (m_eState == EKidsFantasyStates::eReeling)
        {
            LOG_INFO("Kids Fantasy - Possible request state: \"{0}\" -> \"{1}\"", strCurrentState, strStateToRequest);
            m_eState = eStateToRequest;
        }
        else
        {
            LOG_ERROR("Kids Fantasy - Impossible request state: \"{0}\" -> \"{1}\"", strCurrentState, strStateToRequest);
        }
    }
    break;

    case EKidsFantasyStates::eWinFromGame:
    {
        strStateToRequest = "eWinFromGame";

        if (m_eState == EKidsFantasyStates::eAfterReelingStopped)
        {
            LOG_INFO("Kids Fantasy - Possible request state: \"{0}\" -> \"{1}\"", strCurrentState, strStateToRequest);
            m_eState = eStateToRequest;
        }
        else
        {
            LOG_ERROR("Kids Fantasy - Impossible request state: \"{0}\" -> \"{1}\"", strCurrentState, strStateToRequest);
        }
    }
    break;

    default:
        break;
    }
}

void KidsFantasy::OnEnter()
{
    if (m_threadAnimationLoading.joinable())
    {
        m_threadAnimationLoading.join();
    }

    std::function<void()> threadCallable = [this]()
    {
        m_reelsArea.LoadAnimationSurfaces();
        m_bHasFinishedLoadingOfSurfaces = true;
        LOG_INFO("Kids Fantasy - Ready with loading of all Animation Surfaces");
    };

    m_bHasFinishedLoadingOfSurfaces = false;
    m_threadAnimationLoading = std::thread(threadCallable);
    MainApp::GetInstance().ptrTimer->StartTimer(this, g_unTimerResourcesLoading, g_unTimerResourcesLoadingPeriod);

    LOG_INFO("Kids Fantasy - Transition to Application succeed");

    RequestState(EKidsFantasyStates::eReadyForGame);
    m_statusLine.StartNormalScenario();
    MainApp::GetInstance().ptrPanel->UnlockBetButtons();
}

void KidsFantasy::OnExit()
{
    MainApp::GetInstance().ptrTimer->StopTimer(this, g_unTimerResourcesLoading);
    m_statusLine.StopNormalScenario();
    RequestState(EKidsFantasyStates::eInactive);

    if (m_threadAnimationLoading.joinable())
    {
        m_threadAnimationLoading.join();
    }

    m_reelsArea.UnloadResources();
    MainApp::GetInstance().ptrAudioPlayer->PlaySound("../src/resources/panel/sounds/exit.wav");

    LOG_INFO("Kids Fantasy - Exit from Application");
}

void KidsFantasy::OnDraw()
{
    const auto &rend = MainApp::GetInstance().ptrRend;

    /*Draw Background*/
    rend->DrawPicture(m_textureBackground, 0.0f, 0.0f);

    /*Draw ReelsArea*/
    m_reelsArea.Draw();

    /*Draw StatusLine*/
    m_statusLine.Draw();

    /*Draw LinesMgr*/
    m_lines.Draw();

    /*Draw Panel*/
    MainApp::GetInstance().ptrPanel->OnDraw();

    /*Demo*/
    DrawDemo();

    /*Test Particle*/
    m_particle.Draw();
}

void KidsFantasy::DrawDemo()
{
    ImGui::Begin("Kids Fantasy Demo", &m_bDemoMenu);

    ImGui::Text("Win Figures");
    if (ImGui::Button("Win Figure 1"))
    {
        Matrix2DResults results =
            {
                {{GameDefs::eGameFigureOne, GameDefs::eGameFigureTwo, GameDefs::eGameFigureThree, GameDefs::eGameFigureFour, GameDefs::eGameFigureFive},
                 {GameDefs::eGameFigureOne, GameDefs::eGameFigureTwo, GameDefs::eGameFigureThree, GameDefs::eGameFigureFour, GameDefs::eGameFigureFive},
                 {GameDefs::eGameFigureOne, GameDefs::eGameFigureOne, GameDefs::eGameFigureOne, GameDefs::eGameFigureOne, GameDefs::eGameFigureOne},
                 {GameDefs::eGameFigureOne, GameDefs::eGameFigureTwo, GameDefs::eGameFigureThree, GameDefs::eGameFigureFour, GameDefs::eGameFigureFive},
                 {GameDefs::eGameFigureOne, GameDefs::eGameFigureTwo, GameDefs::eGameFigureThree, GameDefs::eGameFigureFour, GameDefs::eGameFigureFive}}};

        MathLogic::GetInstance().GenerateDemoResults(results);
        InvokeStartButton(true);

        ImGui::SetWindowCollapsed(true);
    }

    ImGui::SameLine();
    if (ImGui::Button("Win Figure 2"))
    {
        Matrix2DResults results =
            {
                {{GameDefs::eGameFigureOne, GameDefs::eGameFigureTwo, GameDefs::eGameFigureThree, GameDefs::eGameFigureFour, GameDefs::eGameFigureFive},
                 {GameDefs::eGameFigureOne, GameDefs::eGameFigureTwo, GameDefs::eGameFigureThree, GameDefs::eGameFigureFour, GameDefs::eGameFigureFive},
                 {GameDefs::eGameFigureTwo, GameDefs::eGameFigureTwo, GameDefs::eGameFigureTwo, GameDefs::eGameFigureTwo, GameDefs::eGameFigureTwo},
                 {GameDefs::eGameFigureOne, GameDefs::eGameFigureTwo, GameDefs::eGameFigureThree, GameDefs::eGameFigureFour, GameDefs::eGameFigureFive},
                 {GameDefs::eGameFigureOne, GameDefs::eGameFigureTwo, GameDefs::eGameFigureThree, GameDefs::eGameFigureFour, GameDefs::eGameFigureFive}}};

        MathLogic::GetInstance().GenerateDemoResults(results);
        InvokeStartButton(true);

        ImGui::SetWindowCollapsed(true);
    }

    ImGui::SameLine();
    if (ImGui::Button("Win Figure 3"))
    {
        Matrix2DResults results =
            {
                {{GameDefs::eGameFigureOne, GameDefs::eGameFigureTwo, GameDefs::eGameFigureThree, GameDefs::eGameFigureFour, GameDefs::eGameFigureFive},
                 {GameDefs::eGameFigureOne, GameDefs::eGameFigureTwo, GameDefs::eGameFigureThree, GameDefs::eGameFigureFour, GameDefs::eGameFigureFive},
                 {GameDefs::eGameFigureThree, GameDefs::eGameFigureThree, GameDefs::eGameFigureThree, GameDefs::eGameFigureThree, GameDefs::eGameFigureThree},
                 {GameDefs::eGameFigureOne, GameDefs::eGameFigureTwo, GameDefs::eGameFigureThree, GameDefs::eGameFigureFour, GameDefs::eGameFigureFive},
                 {GameDefs::eGameFigureOne, GameDefs::eGameFigureTwo, GameDefs::eGameFigureThree, GameDefs::eGameFigureFour, GameDefs::eGameFigureFive}}};

        MathLogic::GetInstance().GenerateDemoResults(results);
        InvokeStartButton(true);

        ImGui::SetWindowCollapsed(true);
    }

    ImGui::SameLine();
    if (ImGui::Button("Win Figure 4"))
    {
        Matrix2DResults results =
            {
                {{GameDefs::eGameFigureOne, GameDefs::eGameFigureTwo, GameDefs::eGameFigureThree, GameDefs::eGameFigureFour, GameDefs::eGameFigureFive},
                 {GameDefs::eGameFigureOne, GameDefs::eGameFigureTwo, GameDefs::eGameFigureThree, GameDefs::eGameFigureFour, GameDefs::eGameFigureFive},
                 {GameDefs::eGameFigureFour, GameDefs::eGameFigureFour, GameDefs::eGameFigureFour, GameDefs::eGameFigureFour, GameDefs::eGameFigureFour},
                 {GameDefs::eGameFigureOne, GameDefs::eGameFigureTwo, GameDefs::eGameFigureThree, GameDefs::eGameFigureFour, GameDefs::eGameFigureFive},
                 {GameDefs::eGameFigureOne, GameDefs::eGameFigureTwo, GameDefs::eGameFigureThree, GameDefs::eGameFigureFour, GameDefs::eGameFigureFive}}};

        MathLogic::GetInstance().GenerateDemoResults(results);
        InvokeStartButton(true);

        ImGui::SetWindowCollapsed(true);
    }

    ImGui::SameLine();
    if (ImGui::Button("Win Figure 5"))
    {
        Matrix2DResults results =
            {
                {{GameDefs::eGameFigureOne, GameDefs::eGameFigureTwo, GameDefs::eGameFigureThree, GameDefs::eGameFigureFour, GameDefs::eGameFigureFive},
                 {GameDefs::eGameFigureOne, GameDefs::eGameFigureTwo, GameDefs::eGameFigureThree, GameDefs::eGameFigureFour, GameDefs::eGameFigureFive},
                 {GameDefs::eGameFigureFive, GameDefs::eGameFigureFive, GameDefs::eGameFigureFive, GameDefs::eGameFigureFive, GameDefs::eGameFigureFive},
                 {GameDefs::eGameFigureOne, GameDefs::eGameFigureTwo, GameDefs::eGameFigureThree, GameDefs::eGameFigureFour, GameDefs::eGameFigureFive},
                 {GameDefs::eGameFigureOne, GameDefs::eGameFigureTwo, GameDefs::eGameFigureThree, GameDefs::eGameFigureFour, GameDefs::eGameFigureFive}}};

        MathLogic::GetInstance().GenerateDemoResults(results);
        InvokeStartButton(true);

        ImGui::SetWindowCollapsed(true);
    }

    ImGui::SameLine();
    if (ImGui::Button("Win Figure 6"))
    {
        Matrix2DResults results =
            {
                {{GameDefs::eGameFigureOne, GameDefs::eGameFigureTwo, GameDefs::eGameFigureThree, GameDefs::eGameFigureFour, GameDefs::eGameFigureFive},
                 {GameDefs::eGameFigureOne, GameDefs::eGameFigureTwo, GameDefs::eGameFigureThree, GameDefs::eGameFigureFour, GameDefs::eGameFigureFive},
                 {GameDefs::eGameFigureSix, GameDefs::eGameFigureSix, GameDefs::eGameFigureSix, GameDefs::eGameFigureSix, GameDefs::eGameFigureSix},
                 {GameDefs::eGameFigureOne, GameDefs::eGameFigureTwo, GameDefs::eGameFigureThree, GameDefs::eGameFigureFour, GameDefs::eGameFigureFive},
                 {GameDefs::eGameFigureOne, GameDefs::eGameFigureTwo, GameDefs::eGameFigureThree, GameDefs::eGameFigureFour, GameDefs::eGameFigureFive}}};

        MathLogic::GetInstance().GenerateDemoResults(results);
        InvokeStartButton(true);

        ImGui::SetWindowCollapsed(true);
    }

    ImGui::Text("Big Win");
    if (ImGui::Button("Big Win Figure 1"))
    {
        Matrix2DResults results =
            {
                {{GameDefs::eGameFigureOne, GameDefs::eGameFigureTwo, GameDefs::eGameFigureThree, GameDefs::eGameFigureFour, GameDefs::eGameFigureFive},
                 {GameDefs::eGameFigureOne, GameDefs::eGameFigureOne, GameDefs::eGameFigureOne, GameDefs::eGameFigureOne, GameDefs::eGameFigureOne},
                 {GameDefs::eGameFigureOne, GameDefs::eGameFigureOne, GameDefs::eGameFigureOne, GameDefs::eGameFigureOne, GameDefs::eGameFigureOne},
                 {GameDefs::eGameFigureOne, GameDefs::eGameFigureOne, GameDefs::eGameFigureOne, GameDefs::eGameFigureOne, GameDefs::eGameFigureOne},
                 {GameDefs::eGameFigureOne, GameDefs::eGameFigureTwo, GameDefs::eGameFigureThree, GameDefs::eGameFigureFour, GameDefs::eGameFigureFive}}};

        MathLogic::GetInstance().GenerateDemoResults(results);
        InvokeStartButton(true);

        ImGui::SetWindowCollapsed(true);
    }

    ImGui::SameLine();
    if (ImGui::Button("Big Win Figure 3"))
    {
        Matrix2DResults results =
            {
                {{GameDefs::eGameFigureOne, GameDefs::eGameFigureTwo, GameDefs::eGameFigureThree, GameDefs::eGameFigureFour, GameDefs::eGameFigureFive},
                 {GameDefs::eGameFigureThree, GameDefs::eGameFigureThree, GameDefs::eGameFigureThree, GameDefs::eGameFigureThree, GameDefs::eGameFigureThree},
                 {GameDefs::eGameFigureThree, GameDefs::eGameFigureThree, GameDefs::eGameFigureThree, GameDefs::eGameFigureThree, GameDefs::eGameFigureThree},
                 {GameDefs::eGameFigureThree, GameDefs::eGameFigureThree, GameDefs::eGameFigureThree, GameDefs::eGameFigureThree, GameDefs::eGameFigureThree},
                 {GameDefs::eGameFigureOne, GameDefs::eGameFigureTwo, GameDefs::eGameFigureThree, GameDefs::eGameFigureFour, GameDefs::eGameFigureFive}}};

        MathLogic::GetInstance().GenerateDemoResults(results);
        InvokeStartButton(true);

        ImGui::SetWindowCollapsed(true);
    }

    ImGui::SameLine();
    if (ImGui::Button("Big Win Figure 6"))
    {
        Matrix2DResults results =
            {
                {{GameDefs::eGameFigureOne, GameDefs::eGameFigureTwo, GameDefs::eGameFigureThree, GameDefs::eGameFigureFour, GameDefs::eGameFigureFive},
                 {GameDefs::eGameFigureSix, GameDefs::eGameFigureSix, GameDefs::eGameFigureSix, GameDefs::eGameFigureSix, GameDefs::eGameFigureSix},
                 {GameDefs::eGameFigureSix, GameDefs::eGameFigureSix, GameDefs::eGameFigureSix, GameDefs::eGameFigureSix, GameDefs::eGameFigureSix},
                 {GameDefs::eGameFigureSix, GameDefs::eGameFigureSix, GameDefs::eGameFigureSix, GameDefs::eGameFigureSix, GameDefs::eGameFigureSix},
                 {GameDefs::eGameFigureOne, GameDefs::eGameFigureTwo, GameDefs::eGameFigureThree, GameDefs::eGameFigureFour, GameDefs::eGameFigureFive}}};

        MathLogic::GetInstance().GenerateDemoResults(results);
        InvokeStartButton(true);

        ImGui::SetWindowCollapsed(true);
    }

    ImGui::Text("Demo Mode");
    if (ImGui::Button("Demo Mode"))
    {
        m_bDemoModeActive = !m_bDemoModeActive;

        if (m_bDemoModeActive)
        {
            MainApp::GetInstance().ptrTimer->StartTimer(this, g_unTimerDemoMode, g_unTimerDemoModePeriod);
            LOG_INFO("Kids Fantasy - Auto Demo Mode Started");
        }
        else
        {
            MainApp::GetInstance().ptrTimer->StopTimer(this, g_unTimerDemoMode);
            LOG_INFO("Kids Fantasy - Auto Demo Mode Stopped");
        }

        ImGui::SetWindowCollapsed(true);
    }

    ImGui::End();
}

void KidsFantasy::AfterReelingStopped()
{
    RequestState(EKidsFantasyStates::eAfterReelingStopped);

    /*After the reeling stops, check if there is a win*/
    if (MathLogic::GetInstance().ThereIsAWin())
    {
        const auto &fWinToReach = MathLogic::GetInstance().GetWinFromCurrentGame();

        RequestState(EKidsFantasyStates::eWinFromGame);
        m_statusLine.StartWinScenario();
        m_lines.Start();
        MainApp::GetInstance().ptrPanel->ResetWin();
        MainApp::GetInstance().ptrPanel->StartWinCounting(fWinToReach);
    }
    else
    {
        RequestState(EKidsFantasyStates::eReadyForGame);
        m_statusLine.StartNormalScenario();
        MainApp::GetInstance().ptrPanel->ResetWin();
    }
}

void KidsFantasy::OnTick(unsigned int unID, unsigned int unTimes)
{
    if (unID == g_unTimerDemoMode)
    {
        if (!MainApp::GetInstance().ptrPanel->CanStartNewGame())
        {
            m_bDemoModeActive = false;
            MainApp::GetInstance().ptrTimer->StopTimer(this, g_unTimerDemoMode);
            return;
        }

        if (m_eState == EKidsFantasyStates::eReadyForGame)
        {
            m_unCounterSecondsDemoMode = 0;
            InvokeStartButton();
        }
        else if (m_eState == EKidsFantasyStates::eWinFromGame)
        {
            const unsigned int unSecondsBeforeStart = 10;
            if (++m_unCounterSecondsDemoMode >= unSecondsBeforeStart)
            {
                m_unCounterSecondsDemoMode = 0;
                InvokeStartButton();
            }
        }
    }

    if (unID == g_unTimerResourcesLoading)
    {
        /*If jobs finished*/
        if (m_bHasFinishedLoadingOfSurfaces)
        {
            if (m_threadAnimationLoading.joinable())
            {
                m_threadAnimationLoading.join();
            }

            m_reelsArea.LoadTexturesFromSurfaces();
            MainApp::GetInstance().ptrTimer->StopTimer(this, g_unTimerResourcesLoading);
            m_bHasFinishedLoadingOfSurfaces = false;
        }
    }
}