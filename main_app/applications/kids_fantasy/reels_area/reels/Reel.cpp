#include "Reel.h"

#include <imgui/imgui.h>
#include <imgui/imgui_impl_glfw_gl3.h>

#include <main_app/MainApp.h>
#include <main_app/renderer/Renderer.h>
#include <main_app/audio_player/AudioPlayer.h>
#include <main_app/applications/kids_fantasy/math_logic/MathLogic.h>
#include <debug/Logger.h>

constexpr unsigned int g_unTimerReeling = 1;
constexpr unsigned int g_unPeriodReeling = 1;
constexpr unsigned int g_unReelingCyclesBeforeBounce = 1;
constexpr float g_fAccelerationStep = 6806.25f;   // pixels/sec^2 (was 0.25f per tick on 165Hz)
constexpr float g_fBouncingStep = -300.0f;        // pixels/sec   (was -1.2f per tick on 165Hz)
constexpr float g_fReelingMaxSpeed = 2805.0f;     // pixels/sec   (was 17.0f per tick on 165Hz)

bool Reel::Init(GameDefs::EReels eIDReel,
                float fXOrgPos,
                float fYOrgPos,
                const std::array<std::shared_ptr<Texture>, GameDefs::eTotalGameFiguresCount> &arrFiguresTexture)
{
    m_eIDReel = eIDReel;
    m_fXOrgPos = fXOrgPos;
    m_fYOrgPos = fYOrgPos;
    m_arrFiguresTexture = arrFiguresTexture;
    m_nReelingCyclesBeforeBounce = g_unReelingCyclesBeforeBounce + m_eIDReel;

    m_fYMinTresholdReelingFigure = m_fYOrgPos - GameDefs::g_fHeightFigurePicture;
    m_fYMaxTresholdReelingFigure = m_fYOrgPos + (GameDefs::g_fHeightFigurePicture * GameDefs::g_unVisibleFiguresPerReel);

    for (unsigned int i = GameDefs::eFirstPositionNONVisible; i < GameDefs::eTotalFigurePositionsPerReel; ++i)
    {
        Figure figureObject;
        figureObject.bWinFigure = false;
        figureObject.textureFigure = m_arrFiguresTexture.at(i);
        figureObject.fXPos = m_fXOrgPos;
        figureObject.fYPos = m_fYOrgPos + (figureObject.textureFigure->GetHeight() * (i - 1));
        if (i == GameDefs::eFirstPositionNONVisible)
        {
            figureObject.fYPos = m_fYMinTresholdReelingFigure;
        }

        figureObject.eGameFigure = GameDefs::EGameFigure::eGameFigureOne;
        figureObject.eFigurePosition = (GameDefs::EFigurePositionPerReel)i;

        m_arrReelFigures.at(i) = figureObject;
    }

    m_eState = EReelState::eStopped;

    LOG_INFO("Reel - Initialized Reel with ID - \"{0}\"", (int)m_eIDReel);
    return true;
}

bool Reel::InitFiguresAnimations(const std::array<AnimPlayer, GameDefs::eTotalGameFiguresCount> &arrFiguresAnimations)
{
    if(arrFiguresAnimations.empty())
    {
        LOG_ERROR("Reel - Reel with ID - \"{0}\", failed to initialize Figures Animations!", (int)m_eIDReel);
        return false;
    }

    m_arrFiguresAnimations = arrFiguresAnimations;
    return true;
}

void Reel::ProcessReeling()
{
    /*Compute delta time in seconds for frame-rate independent movement*/
    auto currentTime = std::chrono::system_clock::now();
    std::chrono::duration<float> deltaTime = currentTime - m_lastProcessTime;
    float dt = deltaTime.count();
    m_lastProcessTime = currentTime;

    /*Safety clamp: if the game lags, don't let the reel jump too far*/
    if (dt > 0.1f)
    {
        dt = 0.1f;
    }

    if (m_eState == EReelState::eAccelerating)
    {
        m_fReelingStep += g_fAccelerationStep * dt;

        /*Max Speed reached, change the state to Reeling*/
        if (m_fReelingStep >= g_fReelingMaxSpeed)
        {
            m_eState = EReelState::eReeling;
            m_fReelingStep = g_fReelingMaxSpeed;
        }
    }
    else if (m_eState == EReelState::eReeling)
    {
        /*If reeling cycled reached (every reel should stop after the previous reel) or fastStopEnabled*/
        if (m_nCounterCycles >= m_nReelingCyclesBeforeBounce)
        {
            /*If main Y reached initial position, change state to Bouncing*/
            if (m_fYFirstVisibleFigure >= m_fYOrgPos)
            {
                m_eState = EReelState::eBouncing;
                MainApp::GetInstance().ptrAudioPlayer->PlaySound("../src/resources/kids_fantasy/sounds/stop_reeling.wav");
            }
        }
    }
    else if (m_eState == EReelState::eBouncing)
    {
        m_fReelingStep = g_fBouncingStep;

        /*Here is already reached the initial Y position, stopped*/
        if (m_fYFirstVisibleFigure <= m_fYOrgPos)
        {
            /*Reset figure positions as initial*/
            for (unsigned int i = GameDefs::eFirstPositionNONVisible; i < GameDefs::eTotalFigurePositionsPerReel; ++i)
            {
                auto &figureObject = m_arrReelFigures.at(i);
                figureObject.fYPos = m_fYOrgPos + (figureObject.textureFigure->GetHeight() * (i - 1));
                if (i == GameDefs::eFirstPositionNONVisible)
                {
                    figureObject.fYPos = m_fYMinTresholdReelingFigure;
                }
            }
            m_eState = EReelState::eStopped;
            MainApp::GetInstance().ptrTimer->StopTimer(this, g_unTimerReeling);
        }
    }

    int nIndexFigure = 0;
    for (auto &figure : m_arrReelFigures)
    {
        if (figure.fYPos > m_fYMaxTresholdReelingFigure)
        {
            int nIndexToBeInsertedAbove = nIndexFigure + 1;

            /*Behavior when we are at the last figure and we want to be inserted above the first*/
            if (nIndexToBeInsertedAbove > GameDefs::eFifthPositionNONVisible)
            {
                nIndexToBeInsertedAbove = GameDefs::eFirstPositionNONVisible;
                ++m_nCounterCycles;
            }

            figure.fYPos = m_arrReelFigures.at(nIndexToBeInsertedAbove).fYPos - GameDefs::g_fHeightFigurePicture;

            /*Generate Fake Reel*/
            const auto &nGeneratedNumber = MathLogic::GetInstance().GenerateRandomNuber(GameDefs::eGameFigureOne, GameDefs::eTotalGameFiguresCount);
            figure.textureFigure = m_arrFiguresTexture.at(nGeneratedNumber);
            figure.eGameFigure = (GameDefs::EGameFigure)nGeneratedNumber;

            /*Insert Results, 1 cycle before stopping*/
            if (m_nCounterCycles >= (m_nReelingCyclesBeforeBounce - 1))
            {
                const auto &nCurrentFigure = MathLogic::GetInstance().GetResults().at(nIndexFigure).at(m_eIDReel);
                figure.textureFigure = m_arrFiguresTexture.at(nCurrentFigure);
                figure.eGameFigure = (GameDefs::EGameFigure)nGeneratedNumber;
            }
        }

        figure.fYPos += m_fReelingStep * dt;

        /*Set every cycle the leading Y position of the first visible figure*/
        if (figure.eFigurePosition == GameDefs::EFigurePositionPerReel::eSecondPositionVisible)
        {
            m_fYFirstVisibleFigure = figure.fYPos;
        }

        if (++nIndexFigure >= m_arrReelFigures.size())
        {
            nIndexFigure = 0;
        }
    }
}

void Reel::Draw()
{
    const auto &rend = MainApp::GetInstance().ptrRend;

    rend->UseShaderCrop(glm::vec4(m_fXOrgPos, m_fYOrgPos,
                                  GameDefs::g_fWidthFigurePicture,
                                  GameDefs::g_fHeightFigurePicture * GameDefs::g_unVisibleFiguresPerReel));

    for (auto &figure : m_arrReelFigures)
    {
        if (figure.bWinFigure)
        {
            figure.animFigure.Draw(figure.fXPos, figure.fYPos);
        }
        else
        {
            rend->DrawPicture(figure.textureFigure, figure.fXPos, figure.fYPos);
        }
    }

    rend->UseShaderPicture();
}

void Reel::StartReeling()
{
    /*Set the state to accelerate and start the timer reeling*/
    StopAnimation();
    m_nCounterCycles = -1;
    m_fReelingStep = 0.0f;
    m_fYFirstVisibleFigure = 0.0f;
    m_bNeedToFastStop = false;
    m_eState = EReelState::eAccelerating;
    m_lastProcessTime = std::chrono::system_clock::now();
    MainApp::GetInstance().ptrTimer->StartTimer(this, g_unTimerReeling, g_unPeriodReeling);
}

void Reel::NeedToFastStop()
{
    m_bNeedToFastStop = true;
}

void Reel::StartAnimation(int nFigurePosition, GameDefs::EGameFigure eWinFigure)
{
    if(m_arrFiguresAnimations.empty())
    {
        LOG_ERROR("Reel - Reel with ID - \"{0}\" cannot start animation due to empty animation container!", (int)m_eIDReel);
        return;
    }

    auto& winFigure = m_arrReelFigures.at(nFigurePosition);
    winFigure.eGameFigure = eWinFigure;
    winFigure.bWinFigure = true;
    winFigure.animFigure = m_arrFiguresAnimations.at(eWinFigure);
    winFigure.animFigure.Start();
}

void Reel::StopAnimation()
{
    for (auto &figure : m_arrReelFigures)
    {
        figure.bWinFigure = false;
        figure.animFigure.Stop();
    }
}

void Reel::OnTick(unsigned int unID, unsigned int unTimes)
{
    if (unID == g_unTimerReeling)
    {
        ProcessReeling();
    }
}

const EReelState &Reel::GetReelState()
{
    return m_eState;
}