#include "MainGame.h"

#include <cstddef>

#include <main_app/MainApp.h>
#include <main_app/renderer/Renderer.h>
#include <main_app/panel/Panel.h>
#include <debug/Logger.h>

constexpr float g_fFpsSoldierIdle = 10.0f;
constexpr unsigned int g_unSoldierFrameWidth = 128;
constexpr unsigned int g_unSoldierFrameHeight = 128;
constexpr float g_fSoldierScale = 3.333333f;
constexpr float g_fSoldierFeetX = 650.0f;
constexpr float g_fSoldierFeetY = 930.0f;
constexpr float g_fSoldierShadowScale = 0.85f;

/*Draw the soldier after the buildings and before the foreground cacti*/
constexpr std::size_t g_unSoldierBackgroundLayerIndex = 4;

Fight::Fight()
{
}

bool Fight::Init()
{
    /*Load background layers in drawing order: 1 (back) to 5 (front)*/
    const std::vector<std::string> vecBackgroundPaths =
    {
        "../src/resources/fight/background_1/1.png",
        "../src/resources/fight/background_1/2.png",
        "../src/resources/fight/background_1/3.png",
        "../src/resources/fight/background_1/4.png",
        "../src/resources/fight/background_1/5.png"
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

        m_vecTexturesBackground.emplace_back(textureBackground);
    }

    m_textureSoldierShadow = Texture::CreateTexture("../src/resources/fight/soldier_1/shadow.png");

    if (!m_textureSoldierShadow || !m_textureSoldierShadow->Load())
    {
        LOG_ERROR("Zombie Land - Unable to load soldier shadow !");
        return false;
    }

    if (!m_animSoldierIdle.InitSpriteSheet("../src/resources/fight/soldier_1/Idle.png",
                                          g_unSoldierFrameWidth,
                                          g_unSoldierFrameHeight,
                                          g_fFpsSoldierIdle))
    {
        LOG_ERROR("Zombie Land - Unable to load soldier idle animation !");
        return false;
    }

    LOG_INFO("Zombie Land - Initialized ...");
    return true;
}

bool Fight::Deinit()
{
    m_animSoldierIdle.Deinit();

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

    m_animSoldierIdle.Start(g_fFpsSoldierIdle, EAnimPlayerPlayMode::ePlayLooped);

    LOG_INFO("Zombie Land - Transition to Application succeed");
}

void Fight::OnExit()
{
    m_animSoldierIdle.Stop();

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
      between the buildings and the foreground cacti*/
    for (std::size_t unLayerIndex = 0; unLayerIndex < m_vecTexturesBackground.size(); ++unLayerIndex)
    {
        if (unLayerIndex == unSoldierLayerIndex)
        {
            DrawSoldierIdle();
        }

        rend->DrawPictureScaled(m_vecTexturesBackground[unLayerIndex], 0.0f, 0.0f, fBackgroundScaleFactor);
    }

    if (unSoldierLayerIndex >= m_vecTexturesBackground.size())
    {
        DrawSoldierIdle();
    }

    /*Draw Panel, including the Home button used to exit*/
    MainApp::GetInstance().ptrPanel->OnDraw();
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
                            g_fSoldierFeetX - fShadowWidth * 0.5f,
                            g_fSoldierFeetY - fShadowHeight * 0.5f,
                            g_fSoldierShadowScale);
}

void Fight::DrawSoldierIdle()
{
    const float fFrameSize = static_cast<float>(g_unSoldierFrameWidth) * g_fSoldierScale;

    /*The shadow is drawn first, so the soldier stands on top of it*/
    DrawSoldierShadow();

    m_animSoldierIdle.Draw(g_fSoldierFeetX - fFrameSize * 0.5f,
                           g_fSoldierFeetY - fFrameSize,
                           g_fSoldierScale);
}

void Fight::OnTick(unsigned int unID, unsigned int unTimes)
{
}
