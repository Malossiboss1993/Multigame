#pragma once

#include <memory>

#include <main_app/anim_player/AnimPlayer.h>
#include <main_app/texture/Texture.h>

#include "ICharacter.h"

class Player : public ICharacter
{
private:
    float m_fFeetWorldX = 650.0f;
    float m_fFeetY = 930.0f;
    bool m_bFacingRight = true;
    bool m_bMoving = false;
    bool m_bRunning = false;
    bool m_bShooting = false;
    bool m_bContinuousShooting = false;
    bool m_bShootKeyWasDown = false;
    int m_nHorizontalDirection = 0;

    AnimPlayer m_animIdle;
    AnimPlayer m_animWalk;
    AnimPlayer m_animRun;
    AnimPlayer m_animShot;
    AnimPlayer m_animShotContinuous;
    std::shared_ptr<Texture> m_textureShadow;

    bool IsShiftDown() const;
    void UpdateShooting();
    void UpdateMovement(float fDeltaTimeSeconds);
    void DrawShadow(float fFeetX, float fFeetY);

public:
    bool Init() override;
    void Deinit() override;
    void Reset() override;
    void Stop() override;
    void Update(float fDeltaTimeSeconds) override;
    void Draw(float fWorldOffsetX) override;

    float GetFeetWorldX() const override;
    float GetFeetY() const override;
    void SetFeetWorldX(float fWorldX) override;
    void SetFeetY(float fFeetY) override;
    bool IsFacingRight() const override;

    /*Used by the game camera to know when horizontal camera rules should run*/
    bool HasHorizontalInput() const;
    float GetHalfWidth() const;
};
