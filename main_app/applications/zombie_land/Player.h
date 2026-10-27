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
    /*Three hearts; each zombie hit removes a quarter heart*/
    float m_fHealth = 3.0f;
    float m_fMaxHealth = 3.0f;
    bool m_bHurt = false;
    bool m_bDead = false;
    unsigned int m_unPendingShotHits = 0;
    float m_fContinuousShotTimer = 0.0f;

    AnimPlayer m_animIdle;
    AnimPlayer m_animWalk;
    AnimPlayer m_animRun;
    AnimPlayer m_animShot;
    AnimPlayer m_animShotContinuous;
    AnimPlayer m_animGrenade;
    AnimPlayer m_animExplosion;
    AnimPlayer m_animHurt;
    AnimPlayer m_animDead;
    std::shared_ptr<Texture> m_textureShadow;
    std::shared_ptr<Texture> m_textureGrenadeSheet;

    bool m_bGrenadeKeyWasDown = false;
    bool m_bGrenadeHeld = false;
    bool m_bGrenadeThrowRequested = false;
    bool m_bGrenadeThrowing = false;
    bool m_bGrenadeInFlight = false;
    bool m_bExplosionActive = false;
    bool m_bExplosionImpactPending = false;
    bool m_bExplosionImpactApplied = false;
    float m_fGrenadeStartWorldX = 0.0f;
    float m_fGrenadeStartWorldY = 0.0f;
    float m_fGrenadeTargetWorldX = 0.0f;
    float m_fGrenadeTargetFeetY = 0.0f;
    float m_fGrenadeProjectileElapsed = 0.0f;
    float m_fGrenadeProjectileDuration = 0.7f;
    float m_fGrenadeArcHeight = 160.0f;
    float m_fExplosionWorldX = 0.0f;
    float m_fExplosionFeetY = 0.0f;

    bool IsShiftDown() const;
    bool IsGrenadeKeyDown() const;
    bool IsGrenadeActionInProgress() const;
    void UpdateGrenadeInput();
    void UpdateGrenade(float fDeltaTimeSeconds);
    void UpdateShooting(float fDeltaTimeSeconds);
    void UpdateMovement(float fDeltaTimeSeconds);
    void CancelShooting();
    void StartGrenadeProjectile();
    void StartExplosion();
    void DrawShadow(float fFeetX, float fFeetY);
    void DrawGrenadeHeld(float fFrameX, float fFrameY, bool bFlipHorizontal);

public:
    bool Init() override;
    void Deinit() override;
    void Reset() override;
    void Respawn();
    void Stop() override;
    void Update(float fDeltaTimeSeconds) override;
    void Draw(float fWorldOffsetX) override;

    /*Zombie Land draws the grenade effects above the foreground background layers*/
    void DrawGrenadeEffects(float fWorldOffsetX);

    /*Zombie Land asks the player to throw after the Ctrl key is released*/
    bool ConsumeGrenadeThrowRequest();
    void ThrowGrenadeAt(float fTargetWorldX, float fTargetFeetY);

    /*Combat hooks used by Zombie Land*/
    void TakeDamage(float fDamage);
    unsigned int ConsumeShotHits();
    bool ConsumeExplosionImpact(float &fWorldX, float &fFeetY);
    bool IsDead() const;
    float GetHealth() const;
    float GetMaxHealth() const;

    float GetFeetWorldX() const override;
    float GetFeetY() const override;
    void SetFeetWorldX(float fWorldX) override;
    void SetFeetY(float fFeetY) override;
    bool IsFacingRight() const override;

    /*Used by the game camera to know when horizontal camera rules should run*/
    bool HasHorizontalInput() const;
    float GetHalfWidth() const;
};
