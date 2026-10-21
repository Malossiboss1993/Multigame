#include "MainGame.h"

#include <main_app/MainApp.h>
#include <main_app/renderer/Renderer.h>
#include <main_app/panel/Panel.h>
#include <debug/Logger.h>

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
            LOG_ERROR("Fight - Unable to load background layer \"{0}\" !", strPath);
            return false;
        }

        m_vecTexturesBackground.emplace_back(textureBackground);
    }

    LOG_INFO("Fight - Initialized ...");
    return true;
}

bool Fight::Deinit()
{
    LOG_WARN("Fight - Deinitialized ...");
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
    /*Fight only needs the Home button and the volume knob*/
    MainApp::GetInstance().ptrPanel->HideFields();
    LOG_INFO("Fight - Transition to Application succeed");
}

void Fight::OnExit()
{
    MainApp::GetInstance().ptrPanel->ShowFields();
    LOG_INFO("Fight - Exit from Application");
}

void Fight::OnDraw()
{
    const auto &rend = MainApp::GetInstance().ptrRend;

    /*Draw background layers from back to front*/
    if (!m_vecTexturesBackground.empty())
    {
        const float fScaleFactor =
            static_cast<float>(rend->SCREEN_WIDTH) /
            static_cast<float>(m_vecTexturesBackground.front()->GetWidth());

        for (const auto &textureBackground : m_vecTexturesBackground)
        {
            rend->DrawPictureScaled(textureBackground, 0.0f, 0.0f, fScaleFactor);
        }
    }

    /*Draw Panel, including the Home button used to exit*/
    MainApp::GetInstance().ptrPanel->OnDraw();
}

void Fight::OnTick(unsigned int unID, unsigned int unTimes)
{
}
