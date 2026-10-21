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
    /*Placeholder background until the Fight artwork is created*/
    m_textureBackground = Texture::CreateTexture("../src/resources/kids_fantasy/kids_fantasy_background.png");

    if (!m_textureBackground || !m_textureBackground->Load())
    {
        LOG_ERROR("Fight - Unable to load texture background !");
        return false;
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

    /*Draw placeholder background*/
    rend->DrawPicture(m_textureBackground, 0.0f, 0.0f);

    /*Draw Panel, including the Home button used to exit*/
    MainApp::GetInstance().ptrPanel->OnDraw();
}

void Fight::OnTick(unsigned int unID, unsigned int unTimes)
{
}
