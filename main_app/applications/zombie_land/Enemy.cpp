#include "Enemy.h"

#include <algorithm>
#include <cmath>
#include <filesystem>
#include <functional>
#include <random>
#include <utility>

#include <main_app/MainApp.h>
#include <main_app/renderer/Renderer.h>
#include <debug/Logger.h>

namespace
{
constexpr float g_fFpsIdle = 10.0f;
constexpr float g_fFpsRun = 16.0f;
constexpr float g_fFpsAttack = 12.0f;
constexpr float g_fFpsHurt = 12.0f;
constexpr float g_fFpsDead = 10.0f;
constexpr float g_fCorpseHoldSeconds = 3.0f;
constexpr float g_fCorpseFadeSeconds = 0.75f;
constexpr float g_fCorpseJumpVelocityY = -520.0f;
constexpr float g_fCorpseJumpGravity = 1600.0f;
constexpr float g_fScale = 3.333333f;
constexpr float g_fShadowScale = 0.45f;
constexpr float g_fAttackRange = 120.0f;
constexpr float g_fVerticalAttackRange = 20.0f;
constexpr float g_fAttackCooldownSeconds = 0.25f;
constexpr float g_fMinFeetY = 850.0f;
constexpr float g_fMaxFeetY = 1030.0f;

/*Health is rolled per spawn so some zombies need several shots*/
constexpr int g_nMinHealth = 30;
constexpr int g_nMaxHealth = 120;
}

Enemy::Enemy(std::string strResourceFolder, std::string strDisplayName,
             unsigned int unFrameWidth, unsigned int unFrameHeight,
             float fHeadOffsetPixels)
    : m_strResourceFolder(std::move(strResourceFolder)),
      m_strDisplayName(std::move(strDisplayName)),
      m_unFrameWidth(std::max(1u, unFrameWidth)),
      m_unFrameHeight(std::max(1u, unFrameHeight)),
      m_fHeadOffsetPixels(std::max(0.0f, fHeadOffsetPixels))
{
}

void Enemy::SetTarget(const ICharacter *pTarget)
{
    m_pTarget = pTarget;
}

void Enemy::SetSpawnPosition(float fWorldX, float fFeetY)
{
    m_fSpawnWorldX = fWorldX;
    m_fSpawnFeetY = std::clamp(fFeetY, g_fMinFeetY, g_fMaxFeetY);
}

void Enemy::SetSpawnDelay(float fSeconds)
{
    m_fSpawnDelaySeconds = std::max(0.0f, fSeconds);
}

void Enemy::SetRunSpeed(float fSpeed)
{
    m_fRunSpeed = std::max(0.0f, fSpeed);
}

const std::string &Enemy::GetDisplayName() const
{
    return m_strDisplayName;
}

void Enemy::TakeDamage(int nDamage, bool bLaunchCorpse)
{
    if (m_bDead || nDamage <= 0)
    {
        return;
    }

    m_nHealth = std::max(0, m_nHealth - nDamage);

    if (m_nHealth <= 0)
    {
        m_bDead = true;
        m_bAttackHitPending = false;
        m_fDeathTimer = 0.0f;
        m_fCorpseWorldX = m_fFeetWorldX;
        m_fCorpseFeetY = m_fFeetY;
        m_fCorpseJumpOffsetY = 0.0f;
        m_fCorpseJumpVelocityY =
            bLaunchCorpse ? g_fCorpseJumpVelocityY : 0.0f;
        m_bCorpseJumpActive = bLaunchCorpse;
        m_eState = EState::eIdle;
        m_fStateTimer = 0.0f;

        m_animIdle.Stop();
        m_animRun.Stop();
        m_animAttack1.Stop();
        m_animAttack2.Stop();
        m_animAttack3.Stop();
        m_animHurt.Stop();
        m_animDead.Stop();
        m_animDead.Start(g_fFpsDead, EAnimPlayerPlayMode::ePlayOnceHoldLastFrame);

        LOG_INFO("Enemy - \"{0}\" died after taking {1} damage",
                 m_strDisplayName, nDamage);
        return;
    }

    m_bAttackHitPending = false;
    m_eState = EState::eHurt;
    m_fStateTimer = 0.0f;

    m_animIdle.Stop();
    m_animRun.Stop();
    m_animAttack1.Stop();
    m_animAttack2.Stop();
    m_animAttack3.Stop();
    m_animDead.Stop();
    m_animHurt.Stop();
    m_animHurt.Start(g_fFpsHurt, EAnimPlayerPlayMode::ePlayOnce);

    LOG_INFO("Enemy - \"{0}\" took {1} damage, health {2}/{3}",
             m_strDisplayName, nDamage, m_nHealth, m_nMaxHealth);
}

bool Enemy::ConsumeAttackHit()
{
    const bool bPending = m_bAttackHitPending;
    m_bAttackHitPending = false;
    return bPending;
}

bool Enemy::IsDead() const
{
    return m_bDead;
}

bool Enemy::IsCorpseFadedOut() const
{
    return m_bDead &&
           m_fDeathTimer >= g_fCorpseHoldSeconds + g_fCorpseFadeSeconds;
}

int Enemy::GetHealth() const
{
    return m_nHealth;
}

int Enemy::GetMaxHealth() const
{
    return m_nMaxHealth;
}

std::string Enemy::GetAssetPath(const std::string &strFileName) const
{
    return "../src/resources/zombie_land/" + m_strResourceFolder + "/" + strFileName;
}

bool Enemy::Init()
{
    m_textureShadow = Texture::CreateTexture("../src/resources/zombie_land/soldier_1/shadow.png");

    if (!m_textureShadow || !m_textureShadow->Load())
    {
        LOG_ERROR("Enemy - Unable to load shadow for \"{0}\" !", m_strDisplayName);
        return false;
    }

    if (!m_animIdle.InitSpriteSheet(GetAssetPath("Idle.png"),
                                    m_unFrameWidth, m_unFrameHeight, g_fFpsIdle))
    {
        LOG_ERROR("Enemy - Unable to load idle animation for \"{0}\" !", m_strDisplayName);
        return false;
    }

    /*Some imported zombies only provide Walk.png; use it for movement*/
    const std::string strRunPath = GetAssetPath("Run.png");
    const std::string strWalkPath = GetAssetPath("Walk.png");
    const std::string strMovementPath =
        std::filesystem::exists(strRunPath) ? strRunPath : strWalkPath;

    if (!m_animRun.InitSpriteSheet(strMovementPath,
                                   m_unFrameWidth, m_unFrameHeight, g_fFpsRun))
    {
        LOG_ERROR("Enemy - Unable to load movement animation for \"{0}\" !", m_strDisplayName);
        return false;
    }

    /*Some imported zombies provide a single Attack.png instead of Attack_1..3.png*/
    m_bSingleAttack = !std::filesystem::exists(GetAssetPath("Attack_1.png"));

    if (m_bSingleAttack)
    {
        if (!m_animAttack1.InitSpriteSheet(GetAssetPath("Attack.png"),
                                           m_unFrameWidth, m_unFrameHeight, g_fFpsAttack))
        {
            LOG_ERROR("Enemy - Unable to load attack animation for \"{0}\" !", m_strDisplayName);
            return false;
        }
    }
    else
    {
        if (!m_animAttack1.InitSpriteSheet(GetAssetPath("Attack_1.png"),
                                           m_unFrameWidth, m_unFrameHeight, g_fFpsAttack))
        {
            LOG_ERROR("Enemy - Unable to load attack 1 animation for \"{0}\" !", m_strDisplayName);
            return false;
        }

        if (!m_animAttack2.InitSpriteSheet(GetAssetPath("Attack_2.png"),
                                           m_unFrameWidth, m_unFrameHeight, g_fFpsAttack))
        {
            LOG_ERROR("Enemy - Unable to load attack 2 animation for \"{0}\" !", m_strDisplayName);
            return false;
        }

        if (!m_animAttack3.InitSpriteSheet(GetAssetPath("Attack_3.png"),
                                           m_unFrameWidth, m_unFrameHeight, g_fFpsAttack))
        {
            LOG_ERROR("Enemy - Unable to load attack 3 animation for \"{0}\" !", m_strDisplayName);
            return false;
        }
    }

    if (!m_animHurt.InitSpriteSheet(GetAssetPath("Hurt.png"),
                                    m_unFrameWidth, m_unFrameHeight, g_fFpsHurt))
    {
        LOG_ERROR("Enemy - Unable to load hurt animation for \"{0}\" !", m_strDisplayName);
        return false;
    }

    if (!m_animDead.InitSpriteSheet(GetAssetPath("Dead.png"),
                                    m_unFrameWidth, m_unFrameHeight, g_fFpsDead))
    {
        LOG_ERROR("Enemy - Unable to load dead animation for \"{0}\" !", m_strDisplayName);
        return false;
    }

    std::function<void()> onAttackEnd = [this]()
    {
        if (m_eState == EState::eAttacking)
        {
            /*Damage is applied by Zombie Land when it consumes this hit*/
            m_bAttackHitPending = true;
            m_eState = EState::eAttackCooldown;
            m_fStateTimer = 0.0f;
            m_animIdle.Start(g_fFpsIdle, EAnimPlayerPlayMode::ePlayLooped);
        }
    };
    m_animAttack1.SetEndCallback(onAttackEnd);
    m_animAttack2.SetEndCallback(onAttackEnd);
    m_animAttack3.SetEndCallback(onAttackEnd);

    std::function<void()> onHurtEnd = [this]()
    {
        if (!m_bDead && m_eState == EState::eHurt)
        {
            m_eState = EState::eAttackCooldown;
            m_fStateTimer = 0.0f;
            m_animIdle.Start(g_fFpsIdle, EAnimPlayerPlayMode::ePlayLooped);
        }
    };
    m_animHurt.SetEndCallback(onHurtEnd);

    LOG_INFO("Enemy - Initialized \"{0}\" ...", m_strDisplayName);
    return true;
}

void Enemy::Deinit()
{
    m_animIdle.Deinit();
    m_animRun.Deinit();
    m_animAttack1.Deinit();
    m_animAttack2.Deinit();
    m_animAttack3.Deinit();
    m_animHurt.Deinit();
    m_animDead.Deinit();

    if (m_textureShadow)
    {
        m_textureShadow->DeleteTexture();
        m_textureShadow.reset();
    }

    LOG_WARN("Enemy - Deinitialized \"{0}\" ...", m_strDisplayName);
}

void Enemy::Reset()
{
    static std::mt19937 rng(std::random_device{}());
    static std::uniform_int_distribution<int> healthDistribution(g_nMinHealth, g_nMaxHealth);

    m_nMaxHealth = healthDistribution(rng);
    m_nHealth = m_nMaxHealth;
    m_bDead = false;
    m_bAttackHitPending = false;
    m_fDeathTimer = 0.0f;
    m_fCorpseWorldX = 0.0f;
    m_fCorpseFeetY = 0.0f;
    m_fCorpseJumpOffsetY = 0.0f;
    m_fCorpseJumpVelocityY = 0.0f;
    m_bCorpseJumpActive = false;
    m_eState = EState::eIdle;
    m_fFeetWorldX = m_fSpawnWorldX;
    m_fFeetY = m_fSpawnFeetY;
    m_fStateTimer = 0.0f;
    m_bFacingRight = false;
    m_unAttackIndex = 0;

    m_animRun.Stop();
    m_animAttack1.Stop();
    m_animAttack2.Stop();
    m_animAttack3.Stop();
    m_animHurt.Stop();
    m_animDead.Stop();
    m_animIdle.Start(g_fFpsIdle, EAnimPlayerPlayMode::ePlayLooped);

    LOG_INFO("Enemy - \"{0}\" spawned with {1} health ...",
             m_strDisplayName, m_nHealth);
}

void Enemy::Stop()
{
    m_eState = EState::eIdle;
    m_fStateTimer = 0.0f;
    m_bAttackHitPending = false;
    m_fDeathTimer = 0.0f;
    m_fCorpseWorldX = 0.0f;
    m_fCorpseFeetY = 0.0f;
    m_fCorpseJumpOffsetY = 0.0f;
    m_fCorpseJumpVelocityY = 0.0f;
    m_bCorpseJumpActive = false;

    m_animIdle.Stop();
    m_animRun.Stop();
    m_animAttack1.Stop();
    m_animAttack2.Stop();
    m_animAttack3.Stop();
    m_animHurt.Stop();
    m_animDead.Stop();
}

void Enemy::Update(float fDeltaTimeSeconds)
{
    if (m_bDead)
    {
        /*Keep the corpse timer running so it can fade even if combat stops*/
        m_fDeathTimer += fDeltaTimeSeconds;

        if (m_bCorpseJumpActive)
        {
            m_fCorpseJumpVelocityY += g_fCorpseJumpGravity * fDeltaTimeSeconds;
            m_fCorpseJumpOffsetY += m_fCorpseJumpVelocityY * fDeltaTimeSeconds;

            if (m_fCorpseJumpOffsetY >= 0.0f)
            {
                m_fCorpseJumpOffsetY = 0.0f;
                m_fCorpseJumpVelocityY = 0.0f;
                m_bCorpseJumpActive = false;
            }
        }

        return;
    }

    if (!m_pTarget)
    {
        return;
    }

    const float fTargetWorldX = m_pTarget->GetFeetWorldX();
    const float fDeltaX = fTargetWorldX - m_fFeetWorldX;
    const float fDeltaY = m_pTarget->GetFeetY() - m_fFeetY;
    const float fHorizontalDistance = std::fabs(fDeltaX);
    const float fVerticalDistance = std::fabs(fDeltaY);
    const float fDistance = std::sqrt(fDeltaX * fDeltaX + fDeltaY * fDeltaY);

    if (std::fabs(fDeltaX) > 0.001f)
    {
        m_bFacingRight = fDeltaX > 0.0f;
    }

    const bool bHorizontallyInRange =
        fHorizontalDistance <= g_fAttackRange + 0.01f;
    const bool bVerticallyAligned =
        fVerticalDistance <= g_fVerticalAttackRange + 0.01f;
    const bool bInAttackRange = bHorizontallyInRange && bVerticallyAligned;

    switch (m_eState)
    {
        case EState::eIdle:
        {
            m_fStateTimer += fDeltaTimeSeconds;

            if (m_fStateTimer >= m_fSpawnDelaySeconds)
            {
                m_fStateTimer = 0.0f;

                if (bInAttackRange)
                {
                    StartAttacking();
                }
                else
                {
                    StartRunning();
                }
            }

            break;
        }
        case EState::eRunning:
        {
            if (bInAttackRange)
            {
                m_animRun.Stop();
                StartAttacking();
            }
            else
            {
                const float fMaxStep = m_fRunSpeed * fDeltaTimeSeconds;

                if (!bHorizontallyInRange && !bVerticallyAligned &&
                    fDistance > 0.001f)
                {
                    /*Run diagonally towards the target until one axis is close*/
                    const float fStep = std::min(fMaxStep, fDistance);
                    m_fFeetWorldX += (fDeltaX / fDistance) * fStep;
                    m_fFeetY += (fDeltaY / fDistance) * fStep;
                }
                else if (!bHorizontallyInRange)
                {
                    /*Keep the same horizontal stop distance used by the old chase*/
                    const float fStep =
                        std::min(fMaxStep,
                                 fHorizontalDistance - g_fAttackRange);
                    m_fFeetWorldX += fDeltaX > 0.0f ? fStep : -fStep;
                }
                else
                {
                    /*Horizontal range is reached, so run up/down to line up*/
                    const float fStep =
                        std::min(fMaxStep,
                                 fVerticalDistance - g_fVerticalAttackRange);
                    m_fFeetY += fDeltaY > 0.0f ? fStep : -fStep;
                }

                m_fFeetY = std::clamp(m_fFeetY, g_fMinFeetY, g_fMaxFeetY);
            }

            break;
        }
        case EState::eAttacking:
        {
            /*The attack animation's end callback enters the cooldown state*/
            break;
        }
        case EState::eAttackCooldown:
        {
            m_fStateTimer += fDeltaTimeSeconds;

            if (m_fStateTimer >= g_fAttackCooldownSeconds)
            {
                m_fStateTimer = 0.0f;

                if (bInAttackRange)
                {
                    StartAttacking();
                }
                else
                {
                    StartRunning();
                }
            }

            break;
        }
        case EState::eHurt:
        {
            /*The hurt animation's end callback resumes the chase*/
            break;
        }
    }
}

void Enemy::StartRunning()
{
    m_animIdle.Stop();
    m_animAttack1.Stop();
    m_animAttack2.Stop();
    m_animAttack3.Stop();
    m_animHurt.Stop();
    m_animDead.Stop();

    m_bAttackHitPending = false;
    m_eState = EState::eRunning;
    m_fStateTimer = 0.0f;
    m_animRun.Start(g_fFpsRun, EAnimPlayerPlayMode::ePlayLooped);
}

void Enemy::StartAttacking()
{
    if (m_bSingleAttack)
    {
        m_unAttackIndex = 0;
    }
    else
    {
        static std::mt19937 rng(std::random_device{}());
        static std::uniform_int_distribution<unsigned int> attackDistribution(0, 2);
        m_unAttackIndex = attackDistribution(rng);
    }

    m_animIdle.Stop();
    m_animRun.Stop();
    m_animAttack1.Stop();
    m_animAttack2.Stop();
    m_animAttack3.Stop();
    m_animHurt.Stop();
    m_animDead.Stop();

    m_bAttackHitPending = false;
    m_eState = EState::eAttacking;
    m_fStateTimer = 0.0f;
    GetAttackAnimation().Start(g_fFpsAttack, EAnimPlayerPlayMode::ePlayOnce);
}

AnimPlayer &Enemy::GetAttackAnimation()
{
    if (m_bSingleAttack)
    {
        return m_animAttack1;
    }

    switch (m_unAttackIndex)
    {
        case 1:
            return m_animAttack2;
        case 2:
            return m_animAttack3;
        default:
            return m_animAttack1;
    }
}

void Enemy::Draw(float fWorldOffsetX)
{
    const auto &rend = MainApp::GetInstance().ptrRend;
    const float fFrameWidth = static_cast<float>(m_unFrameWidth) * g_fScale;
    const float fFrameHeight = static_cast<float>(m_unFrameHeight) * g_fScale;

    /*Corpses stay at the world position where they died*/
    const float fEntityWorldX = m_bDead ? m_fCorpseWorldX : m_fFeetWorldX;
    const float fEntityFeetY = m_bDead ? m_fCorpseFeetY : m_fFeetY;
    const float fDrawFeetY =
        m_bDead ? m_fCorpseFeetY + m_fCorpseJumpOffsetY : m_fFeetY;
    const float fFeetScreenX = fEntityWorldX - fWorldOffsetX;
    const float fFrameX = fFeetScreenX - fFrameWidth * 0.5f;
    const float fFrameY = fDrawFeetY - fFrameHeight;
    const float fScreenWidth = static_cast<float>(rend->SCREEN_WIDTH);
    const bool bFlipHorizontal = !m_bFacingRight;

    if (fFrameX + fFrameWidth < 0.0f || fFrameX > fScreenWidth)
    {
        return;
    }

    const float fFadeElapsed =
        m_bDead ? m_fDeathTimer - g_fCorpseHoldSeconds : 0.0f;

    if (m_bDead && fFadeElapsed >= g_fCorpseFadeSeconds)
    {
        return;
    }

    if (m_bDead)
    {
        const float fFadeProgress =
            std::clamp(fFadeElapsed / g_fCorpseFadeSeconds, 0.0f, 1.0f);
        rend->SetColor(1.0f, 1.0f, 1.0f, 1.0f - fFadeProgress);
    }

    DrawShadow(fFeetScreenX, fEntityFeetY);

    if (m_bDead)
    {
        m_animDead.Draw(fFrameX, fFrameY, g_fScale, bFlipHorizontal);
        rend->SetColor(1.0f, 1.0f, 1.0f, 1.0f);
        return;
    }

    switch (m_eState)
    {
        case EState::eRunning:
            m_animRun.Draw(fFrameX, fFrameY, g_fScale, bFlipHorizontal);
            break;
        case EState::eAttacking:
            GetAttackAnimation().Draw(fFrameX, fFrameY, g_fScale, bFlipHorizontal);
            break;
        case EState::eHurt:
            m_animHurt.Draw(fFrameX, fFrameY, g_fScale, bFlipHorizontal);
            break;
        case EState::eIdle:
        case EState::eAttackCooldown:
        default:
            m_animIdle.Draw(fFrameX, fFrameY, g_fScale, bFlipHorizontal);
            break;
    }
}

void Enemy::DrawShadow(float fFeetX, float fFeetY)
{
    if (!m_textureShadow)
    {
        return;
    }

    const auto &rend = MainApp::GetInstance().ptrRend;
    const float fShadowWidth =
        static_cast<float>(m_textureShadow->GetWidth()) * g_fShadowScale;
    const float fShadowHeight =
        static_cast<float>(m_textureShadow->GetHeight()) * g_fShadowScale;

    rend->DrawPictureScaled(m_textureShadow,
                            fFeetX - fShadowWidth * 0.5f,
                            fFeetY - fShadowHeight * 0.5f,
                            g_fShadowScale);
}

float Enemy::GetFeetWorldX() const
{
    return m_fFeetWorldX;
}

float Enemy::GetFeetY() const
{
    return m_fFeetY;
}

float Enemy::GetHeadWorldY() const
{
    /*Standing sprites keep the head about 60 source pixels above the feet;
      the crouching wild zombie needs a lower point*/
    return m_fFeetY - m_fHeadOffsetPixels * g_fScale;
}

void Enemy::SetFeetWorldX(float fWorldX)
{
    m_fFeetWorldX = fWorldX;
}

void Enemy::SetFeetY(float fFeetY)
{
    m_fFeetY = std::clamp(fFeetY, g_fMinFeetY, g_fMaxFeetY);
}

bool Enemy::IsFacingRight() const
{
    return m_bFacingRight;
}
