#include "Player.h"

#include <algorithm>
#include <cmath>
#include <functional>

#include <main_app/MainApp.h>
#include <main_app/renderer/Renderer.h>
#include <debug/Logger.h>

namespace
{
constexpr float g_fFpsIdle = 10.0f;
constexpr float g_fFpsWalk = 10.0f;
constexpr float g_fFpsRun = 16.0f;
constexpr float g_fFpsShot = 12.0f;
constexpr float g_fFpsShotContinuous = 24.0f;
constexpr float g_fFpsGrenade = 14.0f;
constexpr float g_fFpsExplosion = 14.0f;
constexpr float g_fFpsHurt = 12.0f;
constexpr float g_fFpsDead = 10.0f;
constexpr float g_fContinuousShotIntervalSeconds = 4.0f / 24.0f;
constexpr float g_fMoveSpeed = 300.0f;
constexpr float g_fVerticalMoveSpeed = 200.0f;
constexpr float g_fRunSpeed = 600.0f;
constexpr float g_fRunVerticalSpeed = 400.0f;

/*The soldier dies after four zombie attacks (100 / 25 damage)*/
constexpr int g_nMaxHealth = 100;

/*The soldier is rooted in place while firing, so he feels focused on shooting*/
constexpr float g_fShootingMoveSpeedMultiplier = 0.0f;
constexpr unsigned int g_unFrameWidth = 128;
constexpr unsigned int g_unFrameHeight = 128;
constexpr float g_fScale = 3.333333f;
constexpr float g_fMinFeetY = 850.0f;
constexpr float g_fMaxFeetY = 1030.0f;
constexpr float g_fShadowScale = 0.85f;

/*Grenade pose and projectile*/
/*Approximate hand position in the 128x128 soldier frame*/
constexpr float g_fGrenadeHandOffsetX = 84.0f;
constexpr float g_fGrenadeHandOffsetY = 235.0f;
constexpr float g_fGrenadeFlightSpeed = 1200.0f;
constexpr float g_fGrenadeMinFlightSeconds = 0.35f;
constexpr float g_fGrenadeMaxFlightSeconds = 0.85f;
constexpr float g_fGrenadeArcHeightRatio = 0.25f;
constexpr float g_fGrenadeMinArcHeight = 90.0f;
constexpr float g_fGrenadeMaxArcHeight = 260.0f;
/*The raised hand in Grenade frame 6 contains the small projectile sprite*/
constexpr unsigned int g_unGrenadeProjectileSourceX = 6 * g_unFrameWidth + 47;
constexpr unsigned int g_unGrenadeProjectileSourceY = 55;
constexpr unsigned int g_unGrenadeProjectileWidth = 14;
constexpr unsigned int g_unGrenadeProjectileHeight = 15;
constexpr float g_fGrenadeProjectileScale = 2.8f;
constexpr float g_fExplosionScale = 3.6f;
/*Frames 0-4 are only the igniting spark; frame 5 is the first full fireball*/
constexpr unsigned int g_unExplosionImpactFrame = 5;
}

bool Player::Init()
{
    m_textureShadow = Texture::CreateTexture("../src/resources/zombie_land/soldier_1/shadow.png");

    if (!m_textureShadow || !m_textureShadow->Load())
    {
        LOG_ERROR("Player - Unable to load shadow !");
        return false;
    }

    if (!m_animIdle.InitSpriteSheet("../src/resources/zombie_land/soldier_1/Idle.png",
                                   g_unFrameWidth, g_unFrameHeight, g_fFpsIdle))
    {
        LOG_ERROR("Player - Unable to load idle animation !");
        return false;
    }

    if (!m_animWalk.InitSpriteSheet("../src/resources/zombie_land/soldier_1/Walk.png",
                                   g_unFrameWidth, g_unFrameHeight, g_fFpsWalk))
    {
        LOG_ERROR("Player - Unable to load walk animation !");
        return false;
    }

    if (!m_animRun.InitSpriteSheet("../src/resources/zombie_land/soldier_1/Run.png",
                                  g_unFrameWidth, g_unFrameHeight, g_fFpsRun))
    {
        LOG_ERROR("Player - Unable to load run animation !");
        return false;
    }

    if (!m_animShot.InitSpriteSheet("../src/resources/zombie_land/soldier_1/Shot_1.png",
                                   g_unFrameWidth, g_unFrameHeight, g_fFpsShot))
    {
        LOG_ERROR("Player - Unable to load shot animation !");
        return false;
    }

    if (!m_animShotContinuous.InitSpriteSheet("../src/resources/zombie_land/soldier_1/Shot_2.png",
                                              g_unFrameWidth, g_unFrameHeight,
                                              g_fFpsShotContinuous))
    {
        LOG_ERROR("Player - Unable to load continuous shot animation !");
        return false;
    }

    if (!m_animGrenade.InitSpriteSheet("../src/resources/zombie_land/soldier_1/Grenade.png",
                                       g_unFrameWidth, g_unFrameHeight, g_fFpsGrenade))
    {
        LOG_ERROR("Player - Unable to load grenade animation !");
        return false;
    }

    if (!m_animExplosion.InitSpriteSheet("../src/resources/zombie_land/soldier_1/Explosion.png",
                                         g_unFrameWidth, g_unFrameHeight, g_fFpsExplosion))
    {
        LOG_ERROR("Player - Unable to load explosion animation !");
        return false;
    }

    if (!m_animHurt.InitSpriteSheet("../src/resources/zombie_land/soldier_1/Hurt.png",
                                    g_unFrameWidth, g_unFrameHeight, g_fFpsHurt))
    {
        LOG_ERROR("Player - Unable to load hurt animation !");
        return false;
    }

    if (!m_animDead.InitSpriteSheet("../src/resources/zombie_land/soldier_1/Dead.png",
                                    g_unFrameWidth, g_unFrameHeight, g_fFpsDead))
    {
        LOG_ERROR("Player - Unable to load dead animation !");
        return false;
    }

    m_textureGrenadeSheet =
        Texture::CreateTexture("../src/resources/zombie_land/soldier_1/Grenade.png");

    if (!m_textureGrenadeSheet || !m_textureGrenadeSheet->Load())
    {
        LOG_ERROR("Player - Unable to load grenade sprite sheet !");
        return false;
    }

    std::function<void()> onShotEnd = [this]()
    {
        m_bShooting = false;
    };
    m_animShot.SetEndCallback(onShotEnd);

    std::function<void(unsigned int)> onGrenadeFrame = [this](unsigned int unCurrentFrame)
    {
        if (m_bGrenadeThrowing && unCurrentFrame >= 8 && !m_bGrenadeInFlight)
        {
            StartGrenadeProjectile();
        }
    };
    m_animGrenade.SetFrameCallback(onGrenadeFrame);

    std::function<void()> onGrenadeEnd = [this]()
    {
        m_bGrenadeThrowing = false;
    };
    m_animGrenade.SetEndCallback(onGrenadeEnd);

    std::function<void(unsigned int)> onExplosionFrame = [this](unsigned int unCurrentFrame)
    {
        /*Match the damage to the first visible fireball frame*/
        if (m_bExplosionActive && !m_bExplosionImpactApplied &&
            unCurrentFrame >= g_unExplosionImpactFrame)
        {
            m_bExplosionImpactPending = true;
            m_bExplosionImpactApplied = true;
        }
    };
    m_animExplosion.SetFrameCallback(onExplosionFrame);

    std::function<void()> onExplosionEnd = [this]()
    {
        m_bExplosionActive = false;
    };
    m_animExplosion.SetEndCallback(onExplosionEnd);

    std::function<void()> onHurtEnd = [this]()
    {
        m_bHurt = false;
    };
    m_animHurt.SetEndCallback(onHurtEnd);

    LOG_INFO("Player - Initialized ...");
    return true;
}

void Player::Deinit()
{
    m_animIdle.Deinit();
    m_animWalk.Deinit();
    m_animRun.Deinit();
    m_animShot.Deinit();
    m_animShotContinuous.Deinit();
    m_animGrenade.Deinit();
    m_animExplosion.Deinit();
    m_animHurt.Deinit();
    m_animDead.Deinit();

    if (m_textureShadow)
    {
        m_textureShadow->DeleteTexture();
        m_textureShadow.reset();
    }

    if (m_textureGrenadeSheet)
    {
        m_textureGrenadeSheet->DeleteTexture();
        m_textureGrenadeSheet.reset();
    }

    LOG_WARN("Player - Deinitialized ...");
}

void Player::Reset()
{
    /*Keep the player's position between app entries; only clear runtime state*/
    m_bMoving = false;
    m_bRunning = false;
    m_bShooting = false;
    m_bContinuousShooting = false;
    m_bShootKeyWasDown = false;
    m_nHorizontalDirection = 0;
    m_nHealth = g_nMaxHealth;
    m_nMaxHealth = g_nMaxHealth;
    m_bHurt = false;
    m_bDead = false;
    m_unPendingShotHits = 0;
    m_fContinuousShotTimer = 0.0f;

    m_bGrenadeKeyWasDown = false;
    m_bGrenadeHeld = false;
    m_bGrenadeThrowRequested = false;
    m_bGrenadeThrowing = false;
    m_bGrenadeInFlight = false;
    m_bExplosionActive = false;
    m_bExplosionImpactPending = false;
    m_bExplosionImpactApplied = false;
    m_fGrenadeProjectileElapsed = 0.0f;

    m_animWalk.Stop();
    m_animRun.Stop();
    m_animShot.Stop();
    m_animShotContinuous.Stop();
    m_animGrenade.Stop();
    m_animExplosion.Stop();
    m_animHurt.Stop();
    m_animDead.Stop();
    m_animIdle.Start(g_fFpsIdle, EAnimPlayerPlayMode::ePlayLooped);
}

void Player::Stop()
{
    m_bMoving = false;
    m_bRunning = false;
    m_bShooting = false;
    m_bContinuousShooting = false;
    m_bShootKeyWasDown = false;
    m_nHorizontalDirection = 0;
    m_bHurt = false;
    m_unPendingShotHits = 0;
    m_fContinuousShotTimer = 0.0f;

    m_bGrenadeKeyWasDown = false;
    m_bGrenadeHeld = false;
    m_bGrenadeThrowRequested = false;
    m_bGrenadeThrowing = false;
    m_bGrenadeInFlight = false;
    m_bExplosionActive = false;
    m_bExplosionImpactPending = false;
    m_bExplosionImpactApplied = false;
    m_fGrenadeProjectileElapsed = 0.0f;

    m_animIdle.Stop();
    m_animWalk.Stop();
    m_animRun.Stop();
    m_animShot.Stop();
    m_animShotContinuous.Stop();
    m_animGrenade.Stop();
    m_animExplosion.Stop();
    m_animHurt.Stop();
    m_animDead.Stop();
}

void Player::Update(float fDeltaTimeSeconds)
{
    if (m_bDead)
    {
        return;
    }

    UpdateGrenadeInput();
    UpdateShooting(fDeltaTimeSeconds);
    UpdateMovement(fDeltaTimeSeconds);
    UpdateGrenade(fDeltaTimeSeconds);
}

bool Player::IsShiftDown() const
{
    const auto &rend = MainApp::GetInstance().ptrRend;

    return glfwGetKey(rend->GetWindow(), GLFW_KEY_LEFT_SHIFT) == GLFW_PRESS ||
           glfwGetKey(rend->GetWindow(), GLFW_KEY_RIGHT_SHIFT) == GLFW_PRESS;
}

bool Player::IsGrenadeKeyDown() const
{
    const auto &rend = MainApp::GetInstance().ptrRend;

    /*Some Linux keyboard layouts swap Ctrl and Super. Accept either so the
      physical Ctrl key still selects the grenade on those systems.*/
    return glfwGetKey(rend->GetWindow(), GLFW_KEY_LEFT_CONTROL) == GLFW_PRESS ||
           glfwGetKey(rend->GetWindow(), GLFW_KEY_RIGHT_CONTROL) == GLFW_PRESS ||
           glfwGetKey(rend->GetWindow(), GLFW_KEY_LEFT_SUPER) == GLFW_PRESS ||
           glfwGetKey(rend->GetWindow(), GLFW_KEY_RIGHT_SUPER) == GLFW_PRESS;
}

bool Player::IsGrenadeActionInProgress() const
{
    return m_bGrenadeThrowing || m_bGrenadeInFlight || m_bExplosionActive;
}

void Player::UpdateGrenadeInput()
{
    const bool bGrenadeKeyDown = IsGrenadeKeyDown();

    if (bGrenadeKeyDown && !m_bGrenadeKeyWasDown &&
        !m_bGrenadeHeld && !IsGrenadeActionInProgress())
    {
        m_bGrenadeHeld = true;
        CancelShooting();

        m_animWalk.Stop();
        m_animRun.Stop();
        m_animIdle.Stop();
        m_animIdle.Start(g_fFpsIdle, EAnimPlayerPlayMode::ePlayLooped);
        m_bMoving = false;
        m_bRunning = false;
        m_nHorizontalDirection = 0;

        LOG_INFO("Player - Grenade selected ...");
    }
    else if (!bGrenadeKeyDown && m_bGrenadeKeyWasDown && m_bGrenadeHeld)
    {
        m_bGrenadeHeld = false;
        m_bGrenadeThrowRequested = true;

        LOG_INFO("Player - Grenade throw requested ...");
    }

    m_bGrenadeKeyWasDown = bGrenadeKeyDown;
}

void Player::UpdateShooting(float fDeltaTimeSeconds)
{
    const auto &rend = MainApp::GetInstance().ptrRend;
    const bool bShootKeyDown =
        glfwGetKey(rend->GetWindow(), GLFW_KEY_SPACE) == GLFW_PRESS;
    const bool bShiftKeyDown = IsShiftDown();
    const bool bContinuousShootRequested = bShootKeyDown && bShiftKeyDown;
    const bool bGrenadeBusy =
        m_bGrenadeHeld || m_bGrenadeThrowing || m_bGrenadeThrowRequested;

    if (bGrenadeBusy)
    {
        if (m_bContinuousShooting)
        {
            m_animShotContinuous.Stop();
            m_bShooting = false;
            m_bContinuousShooting = false;
            m_fContinuousShotTimer = 0.0f;
        }

        m_bShootKeyWasDown = bShootKeyDown;
        return;
    }

    /*Continuous fire lasts only while both Shift and Space are held*/
    if (m_bContinuousShooting && !bContinuousShootRequested)
    {
        m_animShotContinuous.Stop();
        m_bShooting = false;
        m_bContinuousShooting = false;
        m_fContinuousShotTimer = 0.0f;
    }

    if (bContinuousShootRequested && !m_bContinuousShooting)
    {
        /*Switch from the single shot to the looping Shot_2 burst*/
        m_animShot.Stop();
        m_bShooting = true;
        m_bContinuousShooting = true;
        m_fContinuousShotTimer = 0.0f;
        ++m_unPendingShotHits;
        m_animShotContinuous.Start(g_fFpsShotContinuous,
                                   EAnimPlayerPlayMode::ePlayLooped);
    }
    else if (bContinuousShootRequested)
    {
        /*One bullet per Shot_2 animation cycle*/
        m_fContinuousShotTimer += fDeltaTimeSeconds;

        while (m_fContinuousShotTimer >= g_fContinuousShotIntervalSeconds)
        {
            m_fContinuousShotTimer -= g_fContinuousShotIntervalSeconds;
            ++m_unPendingShotHits;
        }
    }
    else if (bShootKeyDown && !m_bShootKeyWasDown && !m_bShooting)
    {
        m_bShooting = true;
        ++m_unPendingShotHits;
        m_animShot.Start(g_fFpsShot, EAnimPlayerPlayMode::ePlayOnce);
    }

    m_bShootKeyWasDown = bShootKeyDown;
}

void Player::CancelShooting()
{
    m_animShot.Stop();
    m_animShotContinuous.Stop();
    m_bShooting = false;
    m_bContinuousShooting = false;
    m_fContinuousShotTimer = 0.0f;
}

void Player::UpdateMovement(float fDeltaTimeSeconds)
{
    const auto &rend = MainApp::GetInstance().ptrRend;

    if (m_bGrenadeHeld || m_bGrenadeThrowing || m_bGrenadeThrowRequested)
    {
        if (m_bMoving)
        {
            m_animWalk.Stop();
            m_animRun.Stop();
            m_bMoving = false;
            m_bRunning = false;
        }

        m_nHorizontalDirection = 0;
        return;
    }

    const bool bMoveLeft = glfwGetKey(rend->GetWindow(), GLFW_KEY_LEFT) == GLFW_PRESS;
    const bool bMoveRight = glfwGetKey(rend->GetWindow(), GLFW_KEY_RIGHT) == GLFW_PRESS;
    const bool bMoveUp = glfwGetKey(rend->GetWindow(), GLFW_KEY_UP) == GLFW_PRESS;
    const bool bMoveDown = glfwGetKey(rend->GetWindow(), GLFW_KEY_DOWN) == GLFW_PRESS;
    const int nDirection = static_cast<int>(bMoveRight) - static_cast<int>(bMoveLeft);
    const int nVerticalDirection = static_cast<int>(bMoveDown) - static_cast<int>(bMoveUp);
    const bool bMoving = nDirection != 0 || nVerticalDirection != 0;

    /*Hold Shift with any arrow to run instead of walking*/
    const bool bRunning = bMoving && IsShiftDown();

    m_nHorizontalDirection = nDirection;

    if (bMoving)
    {
        if (!m_bMoving || m_bRunning != bRunning)
        {
            if (m_bMoving)
            {
                m_animWalk.Stop();
                m_animRun.Stop();
            }
            else
            {
                m_animIdle.Stop();
            }

            if (bRunning)
            {
                m_animRun.Start(g_fFpsRun, EAnimPlayerPlayMode::ePlayLooped);
            }
            else
            {
                m_animWalk.Start(g_fFpsWalk, EAnimPlayerPlayMode::ePlayLooped);
            }

            m_bMoving = true;
            m_bRunning = bRunning;
        }

        if (nDirection != 0)
        {
            m_bFacingRight = nDirection > 0;
        }

        const float fMoveSpeedMultiplier =
            m_bShooting ? g_fShootingMoveSpeedMultiplier : 1.0f;
        const float fHorizontalSpeed = m_bRunning ? g_fRunSpeed : g_fMoveSpeed;
        const float fVerticalSpeed = m_bRunning ? g_fRunVerticalSpeed : g_fVerticalMoveSpeed;

        if (nDirection != 0)
        {
            m_fFeetWorldX += static_cast<float>(nDirection) * fHorizontalSpeed *
                             fMoveSpeedMultiplier * fDeltaTimeSeconds;
        }

        if (nVerticalDirection != 0)
        {
            m_fFeetY += static_cast<float>(nVerticalDirection) * fVerticalSpeed *
                        fMoveSpeedMultiplier * fDeltaTimeSeconds;
            m_fFeetY = std::clamp(m_fFeetY, g_fMinFeetY, g_fMaxFeetY);
        }
    }
    else if (m_bMoving)
    {
        m_animWalk.Stop();
        m_animRun.Stop();
        m_animIdle.Start(g_fFpsIdle, EAnimPlayerPlayMode::ePlayLooped);
        m_bMoving = false;
        m_bRunning = false;
    }
}

void Player::UpdateGrenade(float fDeltaTimeSeconds)
{
    if (!m_bGrenadeInFlight)
    {
        return;
    }

    m_fGrenadeProjectileElapsed += fDeltaTimeSeconds;

    if (m_fGrenadeProjectileElapsed >= m_fGrenadeProjectileDuration)
    {
        StartExplosion();
    }
}

void Player::StartGrenadeProjectile()
{
    if (!m_bGrenadeThrowing || m_bGrenadeInFlight)
    {
        return;
    }

    const float fHandOffsetX =
        m_bFacingRight ? g_fGrenadeHandOffsetX : -g_fGrenadeHandOffsetX;
    m_fGrenadeStartWorldX = m_fFeetWorldX + fHandOffsetX;
    m_fGrenadeStartWorldY = m_fFeetY - g_fGrenadeHandOffsetY;

    const float fDeltaX = m_fGrenadeTargetWorldX - m_fGrenadeStartWorldX;
    const float fDeltaY = m_fGrenadeTargetFeetY - m_fGrenadeStartWorldY;
    const float fDistance = std::sqrt(fDeltaX * fDeltaX + fDeltaY * fDeltaY);

    m_fGrenadeArcHeight = std::clamp(fDistance * g_fGrenadeArcHeightRatio,
                                     g_fGrenadeMinArcHeight,
                                     g_fGrenadeMaxArcHeight);
    m_fGrenadeProjectileDuration = std::clamp(fDistance / g_fGrenadeFlightSpeed,
                                              g_fGrenadeMinFlightSeconds,
                                              g_fGrenadeMaxFlightSeconds);
    m_fGrenadeProjectileElapsed = 0.0f;
    m_bGrenadeInFlight = true;

    LOG_INFO("Player - Grenade released, flight time {0}s, arc {1}",
             m_fGrenadeProjectileDuration, m_fGrenadeArcHeight);
}

void Player::StartExplosion()
{
    m_bGrenadeInFlight = false;
    m_bExplosionActive = true;
    m_bExplosionImpactPending = false;
    m_bExplosionImpactApplied = false;
    m_fExplosionWorldX = m_fGrenadeTargetWorldX;
    m_fExplosionFeetY = m_fGrenadeTargetFeetY;

    m_animExplosion.Stop();
    m_animExplosion.Start(g_fFpsExplosion, EAnimPlayerPlayMode::ePlayOnce);

    LOG_INFO("Player - Grenade exploded at world X {0}, feet Y {1}",
             m_fExplosionWorldX, m_fExplosionFeetY);
}

bool Player::ConsumeGrenadeThrowRequest()
{
    const bool bRequested = m_bGrenadeThrowRequested;
    m_bGrenadeThrowRequested = false;
    return bRequested;
}

void Player::ThrowGrenadeAt(float fTargetWorldX, float fTargetFeetY)
{
    if (IsGrenadeActionInProgress())
    {
        return;
    }

    m_bGrenadeHeld = false;
    m_bGrenadeThrowRequested = false;
    m_bGrenadeThrowing = true;
    m_bGrenadeInFlight = false;
    m_bExplosionActive = false;
    m_bExplosionImpactPending = false;
    m_bExplosionImpactApplied = false;
    m_fGrenadeProjectileElapsed = 0.0f;
    m_fGrenadeTargetWorldX = fTargetWorldX;
    m_fGrenadeTargetFeetY = fTargetFeetY;
    m_bFacingRight = fTargetWorldX >= m_fFeetWorldX;
    m_bMoving = false;
    m_bRunning = false;
    m_nHorizontalDirection = 0;

    m_animExplosion.Stop();
    m_animGrenade.Stop();
    m_animGrenade.Start(g_fFpsGrenade, EAnimPlayerPlayMode::ePlayOnce);

    LOG_INFO("Player - Throwing grenade at world X {0}, feet Y {1}",
             fTargetWorldX, fTargetFeetY);
}

void Player::TakeDamage(int nDamage)
{
    if (m_bDead || nDamage <= 0)
    {
        return;
    }

    m_nHealth = std::max(0, m_nHealth - nDamage);

    if (m_nHealth <= 0)
    {
        m_bDead = true;
        m_bHurt = false;
        m_bMoving = false;
        m_bRunning = false;
        m_bShooting = false;
        m_bContinuousShooting = false;
        m_bShootKeyWasDown = false;
        m_nHorizontalDirection = 0;
        m_unPendingShotHits = 0;
        m_fContinuousShotTimer = 0.0f;
        m_bGrenadeKeyWasDown = false;
        m_bGrenadeHeld = false;
        m_bGrenadeThrowRequested = false;
        m_bGrenadeThrowing = false;
        m_bGrenadeInFlight = false;
        m_bExplosionActive = false;
        m_bExplosionImpactPending = false;
        m_bExplosionImpactApplied = false;

        m_animIdle.Stop();
        m_animWalk.Stop();
        m_animRun.Stop();
        m_animShot.Stop();
        m_animShotContinuous.Stop();
        m_animGrenade.Stop();
        m_animExplosion.Stop();
        m_animHurt.Stop();
        m_animDead.Stop();
        m_animDead.Start(g_fFpsDead, EAnimPlayerPlayMode::ePlayOnceHoldLastFrame);

        LOG_INFO("Player - Died after taking {0} damage", nDamage);
        return;
    }

    m_bHurt = true;
    m_animHurt.Stop();
    m_animHurt.Start(g_fFpsHurt, EAnimPlayerPlayMode::ePlayOnce);

    LOG_INFO("Player - Hurt for {0} damage, health {1}/{2}",
             nDamage, m_nHealth, m_nMaxHealth);
}

unsigned int Player::ConsumeShotHits()
{
    const unsigned int unShotHits = m_unPendingShotHits;
    m_unPendingShotHits = 0;
    return unShotHits;
}

bool Player::ConsumeExplosionImpact(float &fWorldX, float &fFeetY)
{
    if (!m_bExplosionImpactPending)
    {
        return false;
    }

    m_bExplosionImpactPending = false;
    fWorldX = m_fExplosionWorldX;
    fFeetY = m_fExplosionFeetY;
    return true;
}

bool Player::IsDead() const
{
    return m_bDead;
}

int Player::GetHealth() const
{
    return m_nHealth;
}

int Player::GetMaxHealth() const
{
    return m_nMaxHealth;
}

void Player::Draw(float fWorldOffsetX)
{
    const auto &rend = MainApp::GetInstance().ptrRend;
    const float fFrameSize = static_cast<float>(g_unFrameWidth) * g_fScale;
    const float fScreenX = m_fFeetWorldX - fWorldOffsetX;
    const float fFrameX = fScreenX - fFrameSize * 0.5f;
    const float fFrameY = m_fFeetY - fFrameSize;
    const float fScreenWidth = static_cast<float>(rend->SCREEN_WIDTH);
    const bool bFlipHorizontal = !m_bFacingRight;
    const bool bPlayerVisible =
        !(fFrameX + fFrameSize < 0.0f || fFrameX > fScreenWidth);

    if (bPlayerVisible)
    {
        DrawShadow(fScreenX, m_fFeetY);

        if (m_bDead)
        {
            m_animDead.Draw(fFrameX, fFrameY, g_fScale, bFlipHorizontal);
        }
        else if (m_bHurt)
        {
            m_animHurt.Draw(fFrameX, fFrameY, g_fScale, bFlipHorizontal);
        }
        else if (m_bGrenadeHeld)
        {
            DrawGrenadeHeld(fFrameX, fFrameY, bFlipHorizontal);
        }
        else if (m_bGrenadeThrowing)
        {
            m_animGrenade.Draw(fFrameX, fFrameY, g_fScale, bFlipHorizontal);
        }
        else if (m_bContinuousShooting)
        {
            m_animShotContinuous.Draw(fFrameX, fFrameY, g_fScale, bFlipHorizontal);
        }
        else if (m_bShooting)
        {
            m_animShot.Draw(fFrameX, fFrameY, g_fScale, bFlipHorizontal);
        }
        else if (m_bMoving)
        {
            if (m_bRunning)
            {
                m_animRun.Draw(fFrameX, fFrameY, g_fScale, bFlipHorizontal);
            }
            else
            {
                m_animWalk.Draw(fFrameX, fFrameY, g_fScale, bFlipHorizontal);
            }
        }
        else
        {
            m_animIdle.Draw(fFrameX, fFrameY, g_fScale, bFlipHorizontal);
        }
    }
}

void Player::DrawGrenadeHeld(float fFrameX, float fFrameY, bool bFlipHorizontal)
{
    if (!m_textureGrenadeSheet)
    {
        return;
    }

    const auto &rend = MainApp::GetInstance().ptrRend;
    rend->DrawPicturePart(m_textureGrenadeSheet, fFrameX, fFrameY,
                          0, 0, g_unFrameWidth, g_unFrameHeight,
                          g_fScale, bFlipHorizontal);
}

void Player::DrawGrenadeEffects(float fWorldOffsetX)
{
    const auto &rend = MainApp::GetInstance().ptrRend;
    const float fScreenWidth = static_cast<float>(rend->SCREEN_WIDTH);

    if (m_bGrenadeInFlight && m_textureGrenadeSheet &&
        m_fGrenadeProjectileDuration > 0.0f)
    {
        const float fProgress = std::clamp(
            m_fGrenadeProjectileElapsed / m_fGrenadeProjectileDuration, 0.0f, 1.0f);
        const float fWorldX =
            m_fGrenadeStartWorldX +
            (m_fGrenadeTargetWorldX - m_fGrenadeStartWorldX) * fProgress;
        const float fWorldY =
            m_fGrenadeStartWorldY +
            (m_fGrenadeTargetFeetY - m_fGrenadeStartWorldY) * fProgress -
            4.0f * m_fGrenadeArcHeight * fProgress * (1.0f - fProgress);
        const float fScreenX = fWorldX - fWorldOffsetX;
        const float fProjectileWidth =
            static_cast<float>(g_unGrenadeProjectileWidth) * g_fGrenadeProjectileScale;
        const float fProjectileHeight =
            static_cast<float>(g_unGrenadeProjectileHeight) * g_fGrenadeProjectileScale;

        if (fScreenX + fProjectileWidth * 0.5f >= 0.0f &&
            fScreenX - fProjectileWidth * 0.5f <= fScreenWidth)
        {
            rend->DrawPicturePart(m_textureGrenadeSheet,
                                  fScreenX - fProjectileWidth * 0.5f,
                                  fWorldY - fProjectileHeight * 0.5f,
                                  g_unGrenadeProjectileSourceX,
                                  g_unGrenadeProjectileSourceY,
                                  g_unGrenadeProjectileWidth,
                                  g_unGrenadeProjectileHeight,
                                  g_fGrenadeProjectileScale, false);
        }
    }

    if (m_bExplosionActive)
    {
        const float fExplosionSize =
            static_cast<float>(g_unFrameWidth) * g_fExplosionScale;
        const float fScreenX = m_fExplosionWorldX - fWorldOffsetX;

        if (fScreenX + fExplosionSize * 0.5f >= 0.0f &&
            fScreenX - fExplosionSize * 0.5f <= fScreenWidth)
        {
            m_animExplosion.Draw(fScreenX - fExplosionSize * 0.5f,
                                 m_fExplosionFeetY - fExplosionSize,
                                 g_fExplosionScale, false);
        }
    }
}

void Player::DrawShadow(float fFeetX, float fFeetY)
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

float Player::GetFeetWorldX() const
{
    return m_fFeetWorldX;
}

float Player::GetFeetY() const
{
    return m_fFeetY;
}

void Player::SetFeetWorldX(float fWorldX)
{
    m_fFeetWorldX = fWorldX;
}

void Player::SetFeetY(float fFeetY)
{
    m_fFeetY = std::clamp(fFeetY, g_fMinFeetY, g_fMaxFeetY);
}

bool Player::IsFacingRight() const
{
    return m_bFacingRight;
}

bool Player::HasHorizontalInput() const
{
    return m_nHorizontalDirection != 0;
}

float Player::GetHalfWidth() const
{
    return static_cast<float>(g_unFrameWidth) * g_fScale * 0.5f;
}
