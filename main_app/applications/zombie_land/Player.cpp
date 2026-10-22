#include "Player.h"

#include <algorithm>
#include <functional>

#include <main_app/MainApp.h>
#include <main_app/renderer/Renderer.h>
#include <debug/Logger.h>

namespace
{
constexpr float g_fFpsIdle = 10.0f;
constexpr float g_fFpsWalk = 10.0f;
constexpr float g_fFpsShot = 12.0f;
constexpr float g_fFpsShotContinuous = 24.0f;
constexpr float g_fMoveSpeed = 300.0f;
constexpr float g_fVerticalMoveSpeed = 200.0f;

/*The soldier is rooted in place while firing, so he feels focused on shooting*/
constexpr float g_fShootingMoveSpeedMultiplier = 0.0f;
constexpr unsigned int g_unFrameWidth = 128;
constexpr unsigned int g_unFrameHeight = 128;
constexpr float g_fScale = 3.333333f;
constexpr float g_fMinFeetY = 850.0f;
constexpr float g_fMaxFeetY = 1030.0f;
constexpr float g_fShadowScale = 0.85f;
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

    std::function<void()> onShotEnd = [this]()
    {
        m_bShooting = false;
    };
    m_animShot.SetEndCallback(onShotEnd);

    LOG_INFO("Player - Initialized ...");
    return true;
}

void Player::Deinit()
{
    m_animIdle.Deinit();
    m_animWalk.Deinit();
    m_animShot.Deinit();
    m_animShotContinuous.Deinit();

    if (m_textureShadow)
    {
        m_textureShadow->DeleteTexture();
        m_textureShadow.reset();
    }

    LOG_WARN("Player - Deinitialized ...");
}

void Player::Reset()
{
    /*Keep the player's position between app entries; only clear runtime state*/
    m_bMoving = false;
    m_bShooting = false;
    m_bContinuousShooting = false;
    m_bShootKeyWasDown = false;
    m_nHorizontalDirection = 0;

    m_animWalk.Stop();
    m_animShot.Stop();
    m_animShotContinuous.Stop();
    m_animIdle.Start(g_fFpsIdle, EAnimPlayerPlayMode::ePlayLooped);
}

void Player::Stop()
{
    m_bMoving = false;
    m_bShooting = false;
    m_bContinuousShooting = false;
    m_bShootKeyWasDown = false;
    m_nHorizontalDirection = 0;

    m_animIdle.Stop();
    m_animWalk.Stop();
    m_animShot.Stop();
    m_animShotContinuous.Stop();
}

void Player::Update(float fDeltaTimeSeconds)
{
    UpdateShooting();
    UpdateMovement(fDeltaTimeSeconds);
}

void Player::UpdateShooting()
{
    const auto &rend = MainApp::GetInstance().ptrRend;
    const bool bShootKeyDown =
        glfwGetKey(rend->GetWindow(), GLFW_KEY_SPACE) == GLFW_PRESS;
    const bool bShiftKeyDown =
        glfwGetKey(rend->GetWindow(), GLFW_KEY_LEFT_SHIFT) == GLFW_PRESS ||
        glfwGetKey(rend->GetWindow(), GLFW_KEY_RIGHT_SHIFT) == GLFW_PRESS;
    const bool bContinuousShootRequested = bShootKeyDown && bShiftKeyDown;

    /*Continuous fire lasts only while both Shift and Space are held*/
    if (m_bContinuousShooting && !bContinuousShootRequested)
    {
        m_animShotContinuous.Stop();
        m_bShooting = false;
        m_bContinuousShooting = false;
    }

    if (bContinuousShootRequested && !m_bContinuousShooting)
    {
        /*Switch from the single shot to the looping Shot_2 burst*/
        m_animShot.Stop();
        m_bShooting = true;
        m_bContinuousShooting = true;
        m_animShotContinuous.Start(g_fFpsShotContinuous,
                                   EAnimPlayerPlayMode::ePlayLooped);
    }
    else if (bShootKeyDown && !m_bShootKeyWasDown && !m_bShooting)
    {
        m_bShooting = true;
        m_animShot.Start(g_fFpsShot, EAnimPlayerPlayMode::ePlayOnce);
    }

    m_bShootKeyWasDown = bShootKeyDown;
}

void Player::UpdateMovement(float fDeltaTimeSeconds)
{
    const auto &rend = MainApp::GetInstance().ptrRend;

    const bool bMoveLeft = glfwGetKey(rend->GetWindow(), GLFW_KEY_LEFT) == GLFW_PRESS;
    const bool bMoveRight = glfwGetKey(rend->GetWindow(), GLFW_KEY_RIGHT) == GLFW_PRESS;
    const bool bMoveUp = glfwGetKey(rend->GetWindow(), GLFW_KEY_UP) == GLFW_PRESS;
    const bool bMoveDown = glfwGetKey(rend->GetWindow(), GLFW_KEY_DOWN) == GLFW_PRESS;
    const int nDirection = static_cast<int>(bMoveRight) - static_cast<int>(bMoveLeft);
    const int nVerticalDirection = static_cast<int>(bMoveDown) - static_cast<int>(bMoveUp);
    const bool bMoving = nDirection != 0 || nVerticalDirection != 0;

    m_nHorizontalDirection = nDirection;

    if (bMoving)
    {
        if (!m_bMoving)
        {
            m_animIdle.Stop();
            m_animWalk.Start(g_fFpsWalk, EAnimPlayerPlayMode::ePlayLooped);
            m_bMoving = true;
        }

        if (nDirection != 0)
        {
            m_bFacingRight = nDirection > 0;
        }

        const float fMoveSpeedMultiplier =
            m_bShooting ? g_fShootingMoveSpeedMultiplier : 1.0f;

        if (nDirection != 0)
        {
            m_fFeetWorldX += static_cast<float>(nDirection) * g_fMoveSpeed *
                             fMoveSpeedMultiplier * fDeltaTimeSeconds;
        }

        if (nVerticalDirection != 0)
        {
            m_fFeetY += static_cast<float>(nVerticalDirection) *
                        g_fVerticalMoveSpeed * fMoveSpeedMultiplier *
                        fDeltaTimeSeconds;
            m_fFeetY = std::clamp(m_fFeetY, g_fMinFeetY, g_fMaxFeetY);
        }
    }
    else if (m_bMoving)
    {
        m_animWalk.Stop();
        m_animIdle.Start(g_fFpsIdle, EAnimPlayerPlayMode::ePlayLooped);
        m_bMoving = false;
    }
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

    if (fFrameX + fFrameSize < 0.0f || fFrameX > fScreenWidth)
    {
        return;
    }

    DrawShadow(fScreenX, m_fFeetY);

    if (m_bContinuousShooting)
    {
        m_animShotContinuous.Draw(fFrameX, fFrameY, g_fScale, bFlipHorizontal);
    }
    else if (m_bShooting)
    {
        m_animShot.Draw(fFrameX, fFrameY, g_fScale, bFlipHorizontal);
    }
    else if (m_bMoving)
    {
        m_animWalk.Draw(fFrameX, fFrameY, g_fScale, bFlipHorizontal);
    }
    else
    {
        m_animIdle.Draw(fFrameX, fFrameY, g_fScale, bFlipHorizontal);
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
