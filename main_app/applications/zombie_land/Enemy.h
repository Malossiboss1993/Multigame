#pragma once

#include <memory>
#include <string>

#include <main_app/anim_player/AnimPlayer.h>
#include <main_app/texture/Texture.h>

#include "ICharacter.h"

class Enemy : public ICharacter
{
private:
    enum class EState
    {
        eIdle,
        eRunning,
        eAttacking,
        eAttackCooldown,
        eHurt
    };

    std::string m_strResourceFolder;
    std::string m_strDisplayName;
    unsigned int m_unFrameWidth = 96;
    unsigned int m_unFrameHeight = 96;
    float m_fHeadOffsetPixels = 60.0f;
    bool m_bSingleAttack = false;

    const ICharacter *m_pTarget = nullptr;

    EState m_eState = EState::eIdle;
    float m_fFeetWorldX = 0.0f;
    float m_fFeetY = 0.0f;
    float m_fStateTimer = 0.0f;
    float m_fSpawnWorldX = 1250.0f;
    float m_fSpawnFeetY = 930.0f;
    float m_fSpawnDelaySeconds = 2.0f;
    float m_fRunSpeed = 500.0f;
    bool m_bFacingRight = false;
    unsigned int m_unAttackIndex = 0;
    int m_nHealth = 0;
    int m_nMaxHealth = 0;
    bool m_bDead = false;
    bool m_bAttackHitPending = false;
    float m_fDeathTimer = 0.0f;
    float m_fCorpseWorldX = 0.0f;
    float m_fCorpseFeetY = 0.0f;
    float m_fCorpseJumpOffsetY = 0.0f;
    float m_fCorpseJumpVelocityY = 0.0f;
    bool m_bCorpseJumpActive = false;

    AnimPlayer m_animIdle;
    AnimPlayer m_animRun;
    AnimPlayer m_animAttack1;
    AnimPlayer m_animAttack2;
    AnimPlayer m_animAttack3;
    AnimPlayer m_animHurt;
    AnimPlayer m_animDead;
    std::shared_ptr<Texture> m_textureShadow;

    void StartRunning();
    void StartAttacking();
    AnimPlayer &GetAttackAnimation();
    void DrawShadow(float fFeetX, float fFeetY);
    std::string GetAssetPath(const std::string &strFileName) const;

public:
    Enemy(std::string strResourceFolder, std::string strDisplayName,
          unsigned int unFrameWidth = 96, unsigned int unFrameHeight = 96,
          float fHeadOffsetPixels = 60.0f);

    void SetTarget(const ICharacter *pTarget);
    void SetSpawnPosition(float fWorldX, float fFeetY);
    void SetSpawnDelay(float fSeconds);
    void SetRunSpeed(float fSpeed);
    const std::string &GetDisplayName() const;
    void TakeDamage(int nDamage, bool bLaunchCorpse = false);
    bool ConsumeAttackHit();
    bool IsDead() const;
    bool IsCorpseFadedOut() const;
    int GetHealth() const;
    int GetMaxHealth() const;

    bool Init() override;
    void Deinit() override;
    void Reset() override;
    void Stop() override;
    void Update(float fDeltaTimeSeconds) override;
    void Draw(float fWorldOffsetX) override;

    float GetFeetWorldX() const override;
    float GetFeetY() const override;
    float GetHeadWorldY() const;
    void SetFeetWorldX(float fWorldX) override;
    void SetFeetY(float fFeetY) override;
    bool IsFacingRight() const override;
};
