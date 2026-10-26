#include "Enemy.h"

#include <algorithm>
#include <cmath>
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
constexpr unsigned int g_unFrameWidth = 96;
constexpr unsigned int g_unFrameHeight = 96;
constexpr float g_fScale = 3.333333f;
constexpr float g_fShadowScale = 0.45f;
constexpr float g_fAttackRange = 120.0f;
constexpr float g_fVerticalAttackRange = 20.0f;
constexpr float g_fAttackCooldownSeconds = 0.25f;
constexpr float g_fMinFeetY = 850.0f;
constexpr float g_fMaxFeetY = 1030.0f;
}

Enemy::Enemy(std::string strResourceFolder, std::string strDisplayName)
    : m_strResourceFolder(std::move(strResourceFolder)),
      m_strDisplayName(std::move(strDisplayName))
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
                                    g_unFrameWidth, g_unFrameHeight, g_fFpsIdle))
    {
        LOG_ERROR("Enemy - Unable to load idle animation for \"{0}\" !", m_strDisplayName);
        return false;
    }

    if (!m_animRun.InitSpriteSheet(GetAssetPath("Run.png"),
                                   g_unFrameWidth, g_unFrameHeight, g_fFpsRun))
    {
        LOG_ERROR("Enemy - Unable to load run animation for \"{0}\" !", m_strDisplayName);
        return false;
    }

    if (!m_animAttack1.InitSpriteSheet(GetAssetPath("Attack_1.png"),
                                       g_unFrameWidth, g_unFrameHeight, g_fFpsAttack))
    {
        LOG_ERROR("Enemy - Unable to load attack 1 animation for \"{0}\" !", m_strDisplayName);
        return false;
    }

    if (!m_animAttack2.InitSpriteSheet(GetAssetPath("Attack_2.png"),
                                       g_unFrameWidth, g_unFrameHeight, g_fFpsAttack))
    {
        LOG_ERROR("Enemy - Unable to load attack 2 animation for \"{0}\" !", m_strDisplayName);
        return false;
    }

    if (!m_animAttack3.InitSpriteSheet(GetAssetPath("Attack_3.png"),
                                       g_unFrameWidth, g_unFrameHeight, g_fFpsAttack))
    {
        LOG_ERROR("Enemy - Unable to load attack 3 animation for \"{0}\" !", m_strDisplayName);
        return false;
    }

    std::function<void()> onAttackEnd = [this]()
    {
        if (m_eState == EState::eAttacking)
        {
            m_eState = EState::eAttackCooldown;
            m_fStateTimer = 0.0f;
            m_animIdle.Start(g_fFpsIdle, EAnimPlayerPlayMode::ePlayLooped);
        }
    };
    m_animAttack1.SetEndCallback(onAttackEnd);
    m_animAttack2.SetEndCallback(onAttackEnd);
    m_animAttack3.SetEndCallback(onAttackEnd);

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

    if (m_textureShadow)
    {
        m_textureShadow->DeleteTexture();
        m_textureShadow.reset();
    }

    LOG_WARN("Enemy - Deinitialized \"{0}\" ...", m_strDisplayName);
}

void Enemy::Reset()
{
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
    m_animIdle.Start(g_fFpsIdle, EAnimPlayerPlayMode::ePlayLooped);
}

void Enemy::Stop()
{
    m_eState = EState::eIdle;
    m_fStateTimer = 0.0f;

    m_animIdle.Stop();
    m_animRun.Stop();
    m_animAttack1.Stop();
    m_animAttack2.Stop();
    m_animAttack3.Stop();
}

void Enemy::Update(float fDeltaTimeSeconds)
{
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
    }
}

void Enemy::StartRunning()
{
    m_animIdle.Stop();
    m_animAttack1.Stop();
    m_animAttack2.Stop();
    m_animAttack3.Stop();

    m_eState = EState::eRunning;
    m_fStateTimer = 0.0f;
    m_animRun.Start(g_fFpsRun, EAnimPlayerPlayMode::ePlayLooped);
}

void Enemy::StartAttacking()
{
    static std::mt19937 rng(std::random_device{}());
    static std::uniform_int_distribution<unsigned int> attackDistribution(0, 2);
    m_unAttackIndex = attackDistribution(rng);

    m_animIdle.Stop();
    m_animRun.Stop();
    m_animAttack1.Stop();
    m_animAttack2.Stop();
    m_animAttack3.Stop();

    m_eState = EState::eAttacking;
    m_fStateTimer = 0.0f;
    GetAttackAnimation().Start(g_fFpsAttack, EAnimPlayerPlayMode::ePlayOnce);
}

AnimPlayer &Enemy::GetAttackAnimation()
{
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
    const float fFrameSize = static_cast<float>(g_unFrameWidth) * g_fScale;
    const float fFeetScreenX = m_fFeetWorldX - fWorldOffsetX;
    const float fFrameX = fFeetScreenX - fFrameSize * 0.5f;
    const float fFrameY = m_fFeetY - fFrameSize;
    const float fScreenWidth = static_cast<float>(rend->SCREEN_WIDTH);
    const bool bFlipHorizontal = !m_bFacingRight;

    if (fFrameX + fFrameSize < 0.0f || fFrameX > fScreenWidth)
    {
        return;
    }

    DrawShadow(fFeetScreenX, m_fFeetY);

    switch (m_eState)
    {
        case EState::eRunning:
            m_animRun.Draw(fFrameX, fFrameY, g_fScale, bFlipHorizontal);
            break;
        case EState::eAttacking:
            GetAttackAnimation().Draw(fFrameX, fFrameY, g_fScale, bFlipHorizontal);
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
