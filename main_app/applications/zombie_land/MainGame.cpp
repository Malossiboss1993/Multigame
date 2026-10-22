#include "MainGame.h"

#include <algorithm>
#include <cmath>
#include <cstddef>
#include <functional>

#include <main_app/MainApp.h>
#include <main_app/renderer/Renderer.h>
#include <main_app/panel/Panel.h>
#include <debug/Logger.h>

constexpr float g_fFpsSoldierIdle = 10.0f;
constexpr float g_fFpsSoldierWalk = 10.0f;
constexpr float g_fFpsSoldierShot = 12.0f;
constexpr float g_fFpsSoldierShotContinuous = 24.0f;
constexpr float g_fSoldierMoveSpeed = 300.0f;
constexpr float g_fSoldierVerticalMoveSpeed = 200.0f;

/*The soldier is rooted in place while firing, so he feels focused on shooting*/
constexpr float g_fSoldierShootingMoveSpeedMultiplier = 0.0f;
constexpr unsigned int g_unSoldierFrameWidth = 128;
constexpr unsigned int g_unSoldierFrameHeight = 128;
constexpr float g_fSoldierScale = 3.333333f;
constexpr float g_fSoldierMinFeetY = 850.0f;
constexpr float g_fSoldierMaxFeetY = 1030.0f;
constexpr float g_fSoldierShadowScale = 0.85f;

/*The camera starts scrolling forward once the soldier reaches the middle of the screen*/
constexpr float g_fForwardCameraAnchorScreenRatio = 0.5f;

/*When walking back, the soldier moves across the screen first and the camera
  starts following once he reaches the left edge of the playable area*/
constexpr float g_fBackwardCameraAnchorScreenRatio = 1.0f / 25.0f;

/*Parallax factors for layers 1 to 5: far layers move slower, near layers move faster*/
constexpr float g_arrBackgroundParallaxFactors[] =
{
    0.2f,
    0.4f,
    0.6f,
    0.8f,
    1.0f
};
constexpr std::size_t g_unBackgroundParallaxFactorsCount =
    sizeof(g_arrBackgroundParallaxFactors) / sizeof(g_arrBackgroundParallaxFactors[0]);

/*Draw the soldier behind the foreground cactus layer*/
constexpr std::size_t g_unSoldierBackgroundLayerIndex = 4;

Fight::Fight()
{
}

bool Fight::Init()
{
    /*Load background layers in drawing order: 1 (back) to 5 (front)*/
    const std::vector<std::string> vecBackgroundPaths =
    {
        "../src/resources/zombie_land/background_1/1.png",
        "../src/resources/zombie_land/background_1/2.png",
        "../src/resources/zombie_land/background_1/3.png",
        "../src/resources/zombie_land/background_1/4.png",
        "../src/resources/zombie_land/background_1/5.png"
    };

    m_vecTexturesBackground.reserve(vecBackgroundPaths.size());

    for (const auto &strPath : vecBackgroundPaths)
    {
        auto textureBackground = Texture::CreateTexture(strPath);

        if (!textureBackground || !textureBackground->Load())
        {
            LOG_ERROR("Zombie Land - Unable to load background layer \"{0}\" !", strPath);
            return false;
        }

        /*The background layers are tiled horizontally for the parallax scroll*/
        textureBackground->SetWrapModeS(GL_REPEAT);

        m_vecTexturesBackground.emplace_back(textureBackground);
    }

    m_textureSoldierShadow = Texture::CreateTexture("../src/resources/zombie_land/soldier_1/shadow.png");

    if (!m_textureSoldierShadow || !m_textureSoldierShadow->Load())
    {
        LOG_ERROR("Zombie Land - Unable to load soldier shadow !");
        return false;
    }

    if (!m_animSoldierIdle.InitSpriteSheet("../src/resources/zombie_land/soldier_1/Idle.png",
                                          g_unSoldierFrameWidth,
                                          g_unSoldierFrameHeight,
                                          g_fFpsSoldierIdle))
    {
        LOG_ERROR("Zombie Land - Unable to load soldier idle animation !");
        return false;
    }

    if (!m_animSoldierWalk.InitSpriteSheet("../src/resources/zombie_land/soldier_1/Walk.png",
                                          g_unSoldierFrameWidth,
                                          g_unSoldierFrameHeight,
                                          g_fFpsSoldierWalk))
    {
        LOG_ERROR("Zombie Land - Unable to load soldier walk animation !");
        return false;
    }

    if (!m_animSoldierShot.InitSpriteSheet("../src/resources/zombie_land/soldier_1/Shot_1.png",
                                          g_unSoldierFrameWidth,
                                          g_unSoldierFrameHeight,
                                          g_fFpsSoldierShot))
    {
        LOG_ERROR("Zombie Land - Unable to load soldier shot animation !");
        return false;
    }

    if (!m_animSoldierShotContinuous.InitSpriteSheet("../src/resources/zombie_land/soldier_1/Shot_2.png",
                                                     g_unSoldierFrameWidth,
                                                     g_unSoldierFrameHeight,
                                                     g_fFpsSoldierShotContinuous))
    {
        LOG_ERROR("Zombie Land - Unable to load soldier continuous shot animation !");
        return false;
    }

    std::function<void()> onShotEnd = [this]()
    {
        m_bSoldierShooting = false;
    };
    m_animSoldierShot.SetEndCallback(onShotEnd);

    LOG_INFO("Zombie Land - Initialized ...");
    return true;
}

bool Fight::Deinit()
{
    m_animSoldierIdle.Deinit();
    m_animSoldierWalk.Deinit();
    m_animSoldierShot.Deinit();
    m_animSoldierShotContinuous.Deinit();

    if (m_textureSoldierShadow)
    {
        m_textureSoldierShadow->DeleteTexture();
        m_textureSoldierShadow.reset();
    }

    LOG_WARN("Zombie Land - Deinitialized ...");
    return true;
}

bool Fight::HandleEvent()
{
    UpdateSoldierShooting();
    UpdateSoldierMovement();

    /*The panel remains active so the Home button can return to App Select*/
    return MainApp::GetInstance().ptrPanel->HandleEvent();
}

const std::string &Fight::GetAppName()
{
    return m_strAppName;
}

void Fight::OnEnter()
{
    /*Zombie Land only needs the Home button and the volume knob*/
    MainApp::GetInstance().ptrPanel->HideFields();

    m_bSoldierMoving = false;
    m_bSoldierShooting = false;
    m_bSoldierContinuousShooting = false;
    m_bShootKeyWasDown = false;
    m_animSoldierWalk.Stop();
    m_animSoldierShot.Stop();
    m_animSoldierShotContinuous.Stop();
    m_animSoldierIdle.Start(g_fFpsSoldierIdle, EAnimPlayerPlayMode::ePlayLooped);

    LOG_INFO("Zombie Land - Transition to Application succeed");
}

void Fight::OnExit()
{
    m_animSoldierIdle.Stop();
    m_animSoldierWalk.Stop();
    m_animSoldierShot.Stop();
    m_animSoldierShotContinuous.Stop();
    m_bSoldierMoving = false;
    m_bSoldierShooting = false;
    m_bSoldierContinuousShooting = false;
    m_bShootKeyWasDown = false;

    MainApp::GetInstance().ptrPanel->ShowFields();
    LOG_INFO("Zombie Land - Exit from Application");
}

void Fight::OnDraw()
{
    const auto &rend = MainApp::GetInstance().ptrRend;

    const float fBackgroundScaleFactor =
        m_vecTexturesBackground.empty() ?
        1.0f :
        static_cast<float>(rend->SCREEN_WIDTH) /
            static_cast<float>(m_vecTexturesBackground.front()->GetWidth());

    const std::size_t unSoldierLayerIndex =
        m_vecTexturesBackground.size() > g_unSoldierBackgroundLayerIndex ?
        g_unSoldierBackgroundLayerIndex :
        m_vecTexturesBackground.size();

    /*Draw background layers from back to front, inserting the soldier
      behind the foreground cacti*/
    for (std::size_t unLayerIndex = 0; unLayerIndex < m_vecTexturesBackground.size(); ++unLayerIndex)
    {
        if (unLayerIndex == unSoldierLayerIndex)
        {
            DrawSoldier();
        }

        DrawBackgroundLayer(unLayerIndex, fBackgroundScaleFactor);
    }

    if (unSoldierLayerIndex >= m_vecTexturesBackground.size())
    {
        DrawSoldier();
    }

    /*Draw Panel, including the Home button used to exit*/
    MainApp::GetInstance().ptrPanel->OnDraw();
}

void Fight::DrawBackgroundLayer(std::size_t unLayerIndex, float fScaleFactor)
{
    if (unLayerIndex >= m_vecTexturesBackground.size() ||
        unLayerIndex >= g_unBackgroundParallaxFactorsCount)
    {
        return;
    }

    const auto &rend = MainApp::GetInstance().ptrRend;
    const auto &textureBackground = m_vecTexturesBackground[unLayerIndex];
    const float fLayerWidth = static_cast<float>(textureBackground->GetWidth()) * fScaleFactor;

    if (fLayerWidth <= 0.0f)
    {
        return;
    }

    const float fLayerScroll =
        m_fWorldOffsetX * g_arrBackgroundParallaxFactors[unLayerIndex];
    float fScrollOffset = std::fmod(fLayerScroll, fLayerWidth);

    if (fScrollOffset < 0.0f)
    {
        fScrollOffset += fLayerWidth;
    }

    const float fScreenWidth = static_cast<float>(rend->SCREEN_WIDTH);
    const float fStartX = -fScrollOffset;

    /*A background texture is one screen wide, so repeat it to cover the view*/
    for (float fX = fStartX; fX < fScreenWidth; fX += fLayerWidth)
    {
        rend->DrawPictureScaled(textureBackground, fX, 0.0f, fScaleFactor);
    }
}

void Fight::DrawSoldierShadow()
{
    if (!m_textureSoldierShadow)
    {
        return;
    }

    const auto &rend = MainApp::GetInstance().ptrRend;
    const float fShadowWidth =
        static_cast<float>(m_textureSoldierShadow->GetWidth()) * g_fSoldierShadowScale;
    const float fShadowHeight =
        static_cast<float>(m_textureSoldierShadow->GetHeight()) * g_fSoldierShadowScale;

    rend->DrawPictureScaled(m_textureSoldierShadow,
                            m_fSoldierFeetX - fShadowWidth * 0.5f,
                            m_fSoldierFeetY - fShadowHeight * 0.5f,
                            g_fSoldierShadowScale);
}

void Fight::UpdateSoldierShooting()
{
    const auto &rend = MainApp::GetInstance().ptrRend;
    const bool bShootKeyDown =
        glfwGetKey(rend->GetWindow(), GLFW_KEY_SPACE) == GLFW_PRESS;
    const bool bShiftKeyDown =
        glfwGetKey(rend->GetWindow(), GLFW_KEY_LEFT_SHIFT) == GLFW_PRESS ||
        glfwGetKey(rend->GetWindow(), GLFW_KEY_RIGHT_SHIFT) == GLFW_PRESS;
    const bool bContinuousShootRequested = bShootKeyDown && bShiftKeyDown;

    /*Continuous fire lasts only while both Shift and Space are held*/
    if (m_bSoldierContinuousShooting && !bContinuousShootRequested)
    {
        m_animSoldierShotContinuous.Stop();
        m_bSoldierShooting = false;
        m_bSoldierContinuousShooting = false;
    }

    if (bContinuousShootRequested && !m_bSoldierContinuousShooting)
    {
        /*Switch from the single shot to the looping Shot_2 burst*/
        m_animSoldierShot.Stop();
        m_bSoldierShooting = true;
        m_bSoldierContinuousShooting = true;
        m_animSoldierShotContinuous.Start(g_fFpsSoldierShotContinuous,
                                          EAnimPlayerPlayMode::ePlayLooped);
    }
    else if (bShootKeyDown && !m_bShootKeyWasDown && !m_bSoldierShooting)
    {
        m_bSoldierShooting = true;
        m_animSoldierShot.Start(g_fFpsSoldierShot, EAnimPlayerPlayMode::ePlayOnce);
    }

    m_bShootKeyWasDown = bShootKeyDown;
}

void Fight::UpdateSoldierMovement()
{
    const auto &rend = MainApp::GetInstance().ptrRend;

    const bool bMoveLeft = glfwGetKey(rend->GetWindow(), GLFW_KEY_LEFT) == GLFW_PRESS;
    const bool bMoveRight = glfwGetKey(rend->GetWindow(), GLFW_KEY_RIGHT) == GLFW_PRESS;
    const bool bMoveUp = glfwGetKey(rend->GetWindow(), GLFW_KEY_UP) == GLFW_PRESS;
    const bool bMoveDown = glfwGetKey(rend->GetWindow(), GLFW_KEY_DOWN) == GLFW_PRESS;
    const int nDirection = static_cast<int>(bMoveRight) - static_cast<int>(bMoveLeft);
    const int nVerticalDirection = static_cast<int>(bMoveDown) - static_cast<int>(bMoveUp);
    const bool bMoving = nDirection != 0 || nVerticalDirection != 0;

    if (bMoving)
    {
        if (!m_bSoldierMoving)
        {
            m_animSoldierIdle.Stop();
            m_animSoldierWalk.Start(g_fFpsSoldierWalk, EAnimPlayerPlayMode::ePlayLooped);
            m_bSoldierMoving = true;
        }

        if (nDirection != 0)
        {
            m_bSoldierFacingRight = nDirection > 0;
        }

        const float fDeltaTimeSeconds = MainApp::GetInstance().GetDeltaTime() / 1000.0f;
        const float fMoveSpeedMultiplier =
            m_bSoldierShooting ? g_fSoldierShootingMoveSpeedMultiplier : 1.0f;
        const float fScreenWidth = static_cast<float>(rend->SCREEN_WIDTH);

        if (nDirection != 0)
        {
            const float fMoveDistance =
                g_fSoldierMoveSpeed * fMoveSpeedMultiplier * fDeltaTimeSeconds;
            const float fForwardCameraAnchorX =
                fScreenWidth * g_fForwardCameraAnchorScreenRatio;
            const float fBackwardCameraAnchorX =
                fScreenWidth * g_fBackwardCameraAnchorScreenRatio;

            if (nDirection > 0)
            {
                /*Walk to the middle of the screen first, then scroll the world*/
                const float fDistanceToAnchor =
                    std::max(0.0f, fForwardCameraAnchorX - m_fSoldierFeetX);
                const float fSoldierMoveDistance = std::min(fMoveDistance, fDistanceToAnchor);

                m_fSoldierFeetX += fSoldierMoveDistance;
                m_fWorldOffsetX += fMoveDistance - fSoldierMoveDistance;
            }
            else
            {
                /*Walk back across the screen first, then scroll the world*/
                const float fDistanceToAnchor =
                    std::max(0.0f, m_fSoldierFeetX - fBackwardCameraAnchorX);
                const float fSoldierMoveDistance = std::min(fMoveDistance, fDistanceToAnchor);

                m_fSoldierFeetX -= fSoldierMoveDistance;
                m_fWorldOffsetX -= fMoveDistance - fSoldierMoveDistance;
            }

            const float fHalfSoldierWidth =
                static_cast<float>(g_unSoldierFrameWidth) * g_fSoldierScale * 0.5f;
            const float fMaxFeetX = fScreenWidth - fHalfSoldierWidth;
            const float fMinFeetX = std::min(fBackwardCameraAnchorX, fMaxFeetX);

            m_fSoldierFeetX = std::clamp(m_fSoldierFeetX, fMinFeetX, fMaxFeetX);
            m_fWorldOffsetX = std::max(0.0f, m_fWorldOffsetX);
        }

        if (nVerticalDirection != 0)
        {
            m_fSoldierFeetY += static_cast<float>(nVerticalDirection) *
                               g_fSoldierVerticalMoveSpeed * fMoveSpeedMultiplier *
                               fDeltaTimeSeconds;
            m_fSoldierFeetY =
                std::clamp(m_fSoldierFeetY, g_fSoldierMinFeetY, g_fSoldierMaxFeetY);
        }
    }
    else if (m_bSoldierMoving)
    {
        m_animSoldierWalk.Stop();
        m_animSoldierIdle.Start(g_fFpsSoldierIdle, EAnimPlayerPlayMode::ePlayLooped);
        m_bSoldierMoving = false;
    }
}

void Fight::DrawSoldier()
{
    const float fFrameSize = static_cast<float>(g_unSoldierFrameWidth) * g_fSoldierScale;
    const float fSoldierX = m_fSoldierFeetX - fFrameSize * 0.5f;
    const float fSoldierY = m_fSoldierFeetY - fFrameSize;
    const bool bFlipHorizontal = !m_bSoldierFacingRight;

    /*The shadow is drawn first, so the soldier stands on top of it*/
    DrawSoldierShadow();

    if (m_bSoldierContinuousShooting)
    {
        m_animSoldierShotContinuous.Draw(fSoldierX, fSoldierY,
                                         g_fSoldierScale, bFlipHorizontal);
    }
    else if (m_bSoldierShooting)
    {
        m_animSoldierShot.Draw(fSoldierX, fSoldierY, g_fSoldierScale, bFlipHorizontal);
    }
    else if (m_bSoldierMoving)
    {
        m_animSoldierWalk.Draw(fSoldierX, fSoldierY, g_fSoldierScale, bFlipHorizontal);
    }
    else
    {
        m_animSoldierIdle.Draw(fSoldierX, fSoldierY, g_fSoldierScale, bFlipHorizontal);
    }
}

void Fight::OnTick(unsigned int unID, unsigned int unTimes)
{
}
