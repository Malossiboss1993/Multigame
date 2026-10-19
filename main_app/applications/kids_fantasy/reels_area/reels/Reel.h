#pragma once

#include <main_app/anim_player/AnimPlayer.h>
#include <main_app/texture/Texture.h>
#include <main_app/timer/TimerMgr.h>
#include <main_app/applications/kids_fantasy/GameDefinitions.h>

enum class EReelState
{
    eStopped,
    eAccelerating,
    eReeling,
    eBouncing
};

struct Figure
{
    float fXPos = 0.0f;
    float fYPos = 0.0f;
    bool bWinFigure = false;
    GameDefs::EGameFigure eGameFigure = GameDefs::eGameFigureOne;
    GameDefs::EFigurePositionPerReel eFigurePosition = GameDefs::eFirstPositionNONVisible;
    std::shared_ptr<Texture> textureFigure = nullptr;
    AnimPlayer animFigure;
};

class Reel : public ITimer
{

private:
    /*Reel State*/
    EReelState m_eState = EReelState::eStopped;

    /*ID current reel*/
    GameDefs::EReels m_eIDReel = GameDefs::EReels::eFirstReel;

    /*X Position reel*/
    float m_fXOrgPos = 0;

    /*Y Position reel*/
    float m_fYOrgPos = 0;

    /*Leading Y of first visible figure*/
    float m_fYFirstVisibleFigure = 0.0f;

    /*Min Y Figure value while reeling*/
    float m_fYMinTresholdReelingFigure = 0;

    /*Max Y Figure value while reeling*/
    float m_fYMaxTresholdReelingFigure = 0;

    /*Current reeling step, in base of state will be changed*/
    float m_fReelingStep = 0.0f;

    /*Last time ProcessReeling was called, used for frame-rate independent movement*/
    std::chrono::time_point<std::chrono::system_clock> m_lastProcessTime;

    /*Cycles counter, how many times actual reels*/
    int m_nCounterCycles = 0;

    /*Reeling counter cycles before make the bounce effect and stop*/
    int m_nReelingCyclesBeforeBounce = 0;

    /*Fast Stop Key*/
    bool m_bNeedToFastStop = false;

    /*Array holding reel figures*/
    std::array<Figure, GameDefs::g_unTotalFiguresPerReel> m_arrReelFigures;

    /*Container with figure textures*/
    std::array<std::shared_ptr<Texture>, GameDefs::eTotalGameFiguresCount> m_arrFiguresTexture;

    /*Container with figure animations*/
    std::array<AnimPlayer, GameDefs::eTotalGameFiguresCount> m_arrFiguresAnimations;

    /*Method called every tick of reeling timer*/
    void ProcessReeling();

public:
    const EReelState &GetReelState();
    bool Init(GameDefs::EReels eIDReel,
              float fXOrgPos,
              float fYOrgPos,
              const std::array<std::shared_ptr<Texture>, GameDefs::eTotalGameFiguresCount> &arrFiguresTexture);
    bool InitFiguresAnimations(const std::array<AnimPlayer, GameDefs::eTotalGameFiguresCount> &arrFiguresAnimations);
    void Draw();
    void StartReeling();
    void NeedToFastStop();
    void StartAnimation(int nFigurePosition, GameDefs::EGameFigure eWinFigure);
    void StopAnimation();
    void OnTick(unsigned int unID, unsigned int unTimes) final;
};