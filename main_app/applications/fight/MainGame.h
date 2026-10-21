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
    std::string m_strAppName = "Zombie Land";

    /*Textures background layers, drawn from back to front*/
    std::vector<std::shared_ptr<Texture>> m_vecTexturesBackground;

    /*Soldier movement and animations*/
    float m_fSoldierFeetX = 650.0f;
    float m_fSoldierFeetY = 930.0f;
    bool m_bSoldierFacingRight = true;
    bool m_bSoldierMoving = false;
    AnimPlayer m_animSoldierIdle;
    AnimPlayer m_animSoldierWalk;
    std::shared_ptr<Texture> m_textureSoldierShadow;

    /*Horizontal world scroll, used for the parallax backgrounds*/
    float m_fWorldOffsetX = 0.0f;

    void UpdateSoldierMovement();
    void DrawBackgroundLayer(std::size_t unLayerIndex, float fScaleFactor);
    void DrawSoldier();
    void DrawSoldierShadow();

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
