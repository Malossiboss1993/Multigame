#pragma once

#include <cstddef>
#include <string>
#include <vector>

#include <main_app/applications/IApp.h>
#include <main_app/anim_player/AnimPlayer.h>
#include <main_app/texture/Texture.h>
#include <main_app/timer/TimerMgr.h>

class Fight : public IApp, public ITimer
{

private:
    enum class EZombieWomanState
    {
        eIdle,
        eRunning,
        eAttacking,
        eAttackCooldown
    };

    std::string m_strAppName = "Zombie Land";

    /*Textures background layers, drawn from back to front*/
    std::vector<std::shared_ptr<Texture>> m_vecTexturesBackground;

    /*Soldier movement and animations*/
    float m_fSoldierFeetX = 650.0f;
    float m_fSoldierFeetY = 930.0f;
    bool m_bSoldierFacingRight = true;
    bool m_bSoldierMoving = false;
    bool m_bSoldierShooting = false;
    bool m_bSoldierContinuousShooting = false;
    bool m_bShootKeyWasDown = false;
    AnimPlayer m_animSoldierIdle;
    AnimPlayer m_animSoldierWalk;
    AnimPlayer m_animSoldierShot;
    AnimPlayer m_animSoldierShotContinuous;
    std::shared_ptr<Texture> m_textureSoldierShadow;

    /*Zombie woman AI: wait, run to the soldier, then attack repeatedly*/
    EZombieWomanState m_eZombieWomanState = EZombieWomanState::eIdle;
    float m_fZombieWomanWorldX = 0.0f;
    float m_fZombieWomanFeetY = 0.0f;
    float m_fZombieWomanStateTimer = 0.0f;
    bool m_bZombieWomanFacingRight = false;
    unsigned int m_unZombieWomanAttackIndex = 0;
    AnimPlayer m_animZombieWomanIdle;
    AnimPlayer m_animZombieWomanRun;
    AnimPlayer m_animZombieWomanAttack1;
    AnimPlayer m_animZombieWomanAttack2;
    AnimPlayer m_animZombieWomanAttack3;

    /*Horizontal world scroll, used for the parallax backgrounds*/
    float m_fWorldOffsetX = 0.0f;

    void UpdateSoldierShooting();
    void UpdateSoldierMovement();
    void UpdateZombieWoman();
    void StartZombieWomanRun();
    void StartZombieWomanAttack();
    AnimPlayer &GetZombieWomanAttackAnimation();
    void DrawBackgroundLayer(std::size_t unLayerIndex, float fScaleFactor);
    void DrawSoldier();
    void DrawZombieWoman();
    void DrawShadow(float fFeetX, float fFeetY, float fScale);

public:
    Fight();

    bool Init() override;
    bool Deinit() override;
    bool HandleEvent() override;
    const std::string &GetAppName() override;
    void OnEnter() override;
    void OnExit() override;
    void OnDraw() override;
    void OnTick(unsigned int unID, unsigned int unTimes) final;
};
