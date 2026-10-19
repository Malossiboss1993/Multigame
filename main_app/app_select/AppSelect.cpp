#include "AppSelect.h"

#include <imgui/imgui.h>
#include <imgui/imgui_impl_glfw_gl3.h>

#include <main_app/MainApp.h>
#include <main_app/renderer/Renderer.h>
#include <main_app/audio_player/AudioPlayer.h>
#include <debug/Logger.h>

bool AppSelect::Init()
{
    const auto &rend = MainApp::GetInstance().ptrRend;
    const float screenWidth = static_cast<float>(rend->SCREEN_WIDTH);
    const float screenHeight = static_cast<float>(rend->SCREEN_HEIGHT);

    /*Load textures*/
    m_textureBackground = Texture::CreateTexture("../src/resources/app_select_background.jpg");

    m_buttonKidsFantasy.textureButton = Texture::CreateTexture("../src/resources/kids_fantasy/kids_fantasy_button_bgr_round.png");

    m_buttonParticleBuilder.textureButton = Texture::CreateTexture("../src/resources/particle_builder/particle_button.png");

    m_buttonRoulette.textureButton = Texture::CreateTexture("../src/resources/roulette/roulette_button.png");

    if (!m_textureBackground->Load() ||
        !m_buttonParticleBuilder.textureButton->Load() ||
        !m_buttonKidsFantasy.textureButton->Load() ||
        !m_buttonRoulette.textureButton->Load())
    {
        LOG_ERROR("AppSelect - Unable to load textures !");
        return false;
    }

    /*Position the 3 app buttons evenly distributed in one row across the screen,
      based on the current screen size.*/
    Button *buttons[] = { &m_buttonKidsFantasy, &m_buttonRoulette, &m_buttonParticleBuilder };

    float totalWidth = 0.0f;
    for (auto *btn : buttons)
    {
        totalWidth += static_cast<float>(btn->textureButton->GetWidth());
    }

    const float margin = (screenWidth - totalWidth) / 4.0f;
    float currentX = margin;

    for (auto *btn : buttons)
    {
        btn->fX = currentX;
        btn->fY = (screenHeight - static_cast<float>(btn->textureButton->GetHeight())) / 2.0f;
        currentX += static_cast<float>(btn->textureButton->GetWidth()) + margin;
    }

    /*Register Client Kids Fantasy*/
    RegisterClient(EApps::eKidsFantasy, &m_KidsFantasy);

    /*Register Client Roulette*/
    RegisterClient(EApps::eRoulette, &m_Roulette);

    /*Register Client Particle Builder*/
    RegisterClient(EApps::eParticleBuilder, &m_ParticleBuilder);

    /*Initalize all the applications*/
    for (const auto &app : m_mapAppClients)
    {
        if (!app.second->Init())
        {
            LOG_ERROR("AppSelect - Cannot initialize client : \"{0}\"", app.second->GetAppName());
            return false;
        }
    }

    m_eState = EAppSelectStates::eReadyForSelection;

    auto textureStar = Texture::CreateTexture("../src/resources/particle_sun.png");
    if (!textureStar->Load())
    {
        LOG_ERROR("AppSelect - Unable to load texture particle !");
        return false;
    }

    m_particleStarEffect.Init(textureStar, {rend->SCREEN_WIDTH / 2, rend->SCREEN_HEIGHT / 2});
    m_particleStarEffect.SetVelocityVariation({2.5f, 2.0f});
    m_particleStarEffect.SetSizeBegin(0.001f);
    m_particleStarEffect.SetSizeEnd(1.0f);
    m_particleStarEffect.SetLifeTime(400.0f);
    m_particleStarEffect.SetDensity(30.0f);
    m_particleStarEffect.StartEmitting();

    LOG_INFO("AppSelect - Initialized ...");
    return true;
}

bool AppSelect::Deinit()
{
    /*Deinitialize all the applications*/
    for (const auto &app : m_mapAppClients)
    {
        if (!app.second->Deinit())
        {
            LOG_ERROR("AppSelect - Cannot Deinitialize client : \"{0}\"", app.second->GetAppName());
            break;
        }
    }

    LOG_WARN("AppSelect - Deinitialzied ...");
    return true;
}

bool AppSelect::HandleEvent()
{
    /*Hover on Game Buttons*/
    const auto &nXMouse = ImGui::GetMousePos().x;
    const auto &nYMouse = ImGui::GetMousePos().y;

    switch (m_eState)
    {
    case EAppSelectStates::eReadyForSelection:
    {
        m_bIsKidsFantasyHovered = m_buttonKidsFantasy.IsHovered(nXMouse, nYMouse);
        if (m_bIsKidsFantasyHovered)
        {
            /*Select Kids Fantasy*/
            if (m_buttonKidsFantasy.IsPressed(nXMouse, nYMouse))
            {
                RequestTransition(EApps::eKidsFantasy);
                MainApp::GetInstance().ptrAudioPlayer->PlaySound("../src/resources/panel/sounds/enter.wav");
                return true;
            }

            return true;
        }

        m_bIsParticleBuilder = m_buttonParticleBuilder.IsHovered(nXMouse, nYMouse);
        if (m_bIsParticleBuilder)
        {
            /*Select Particle Builder*/
            if (m_buttonParticleBuilder.IsPressed(nXMouse, nYMouse))
            {
                RequestTransition(EApps::eParticleBuilder);
                return true;
            }

            return true;
        }

        m_bIsRouletteHovered = m_buttonRoulette.IsHovered(nXMouse, nYMouse);
        if (m_bIsRouletteHovered)
        {
            /*Select Roulette*/
            if (m_buttonRoulette.IsPressed(nXMouse, nYMouse))
            {
                RequestTransition(EApps::eRoulette);
                MainApp::GetInstance().ptrAudioPlayer->PlaySound("../src/resources/panel/sounds/enter.wav");
                return true;
            }
            return true;
        }
    }
    break;

    case EAppSelectStates::eBusyInGame:
    {
        m_mapAppClients[m_eCurrentApp]->HandleEvent();
    }
    break;

    default:
        break;
    }

    return false;
}

bool AppSelect::RequestTransition(const EApps eAppToTransition)
{
    if (m_eState == EAppSelectStates::eReadyForSelection)
    {
        /*From App Select to Kids Fantasy*/
        if (eAppToTransition == EApps::eKidsFantasy)
        {
            LOG_INFO("AppSelect - Requesting Transition to App Kids Fantasy");
            OnExitFromAppSelect();
            m_eCurrentApp = EApps::eKidsFantasy;
            m_eState = EAppSelectStates::eBusyInGame;
            m_mapAppClients[m_eCurrentApp]->OnEnter();

            return true;
        }

        /*From App Select to Particle Builder*/
        if (eAppToTransition == EApps::eParticleBuilder)
        {
            LOG_INFO("AppSelect - Requesting Transition to App Particle Builder");
            OnExitFromAppSelect();
            m_eCurrentApp = EApps::eParticleBuilder;
            m_eState = EAppSelectStates::eBusyInGame;
            m_mapAppClients[m_eCurrentApp]->OnEnter();

            return true;
        }
        /*From App Select to Roulette*/
        else if (eAppToTransition == EApps::eRoulette)
        {
            LOG_INFO("AppSelect - Requesting Transition to App Roulette");
            OnExitFromAppSelect();
            m_eCurrentApp = EApps::eRoulette;
            m_eState = EAppSelectStates::eBusyInGame;
            m_mapAppClients[m_eCurrentApp]->OnEnter();

            return true;
        }
    }
    /*From App to App Select*/
    else if (m_eState == EAppSelectStates::eBusyInGame)
    {
        /*From Kids Fantasy to App Select*/
        if (eAppToTransition == EApps::eAppSelect &&
            m_eCurrentApp == EApps::eKidsFantasy)
        {
            LOG_INFO("AppSelect - Requesting Transition to App Select");
            m_mapAppClients[m_eCurrentApp]->OnExit();
            m_eCurrentApp = EApps::eAppSelect;
            m_eState = EAppSelectStates::eReadyForSelection;
            OnEnterInAppSelect();

            return true;
        }
        /*From Particle Builder to App Select*/
        if (eAppToTransition == EApps::eAppSelect &&
            m_eCurrentApp == EApps::eParticleBuilder)
        {
            LOG_INFO("AppSelect - Requesting Transition to App Select");
            m_mapAppClients[m_eCurrentApp]->OnExit();
            m_eCurrentApp = EApps::eAppSelect;
            m_eState = EAppSelectStates::eReadyForSelection;
            OnEnterInAppSelect();

            return true;
        }
        /*From Roulette to App Select*/
        else if (eAppToTransition == EApps::eAppSelect &&
                 m_eCurrentApp == EApps::eRoulette)
        {
            LOG_INFO("AppSelect - Requesting Transition to App Select");
            m_mapAppClients[m_eCurrentApp]->OnExit();
            m_eCurrentApp = EApps::eAppSelect;
            m_eState = EAppSelectStates::eReadyForSelection;
            OnEnterInAppSelect();

            return true;
        }
    }

    LOG_ERROR("AppSelect - Impossible Transition to App");
    return false;
}

const EAppSelectStates &AppSelect::GetState()
{
    return m_eState;
}

void AppSelect::OnEnterInAppSelect()
{
    LOG_INFO("AppSelect - Enter in App Select succeed");
}

void AppSelect::OnExitFromAppSelect()
{
    LOG_INFO("AppSelect - Exit from App Select succeed");
}

void AppSelect::RegisterClient(EApps eApp, IApp *client)
{
    m_mapAppClients.emplace(std::make_pair(eApp, client));

    LOG_INFO("AppSelect - Registered client : \"{0}\"", client->GetAppName());
    LOG_INFO("AppSelect - Current size of AppClientsContainer: \"{0}\"", m_mapAppClients.size());
}

void AppSelect::UnregisterClient(EApps eApp, IApp *client)
{
    const auto &it = m_mapAppClients.find(eApp);
    if (it != m_mapAppClients.end())
    {
        LOG_INFO("AppSelect - Unregistered client : \"{0}\"", it->second->GetAppName());
        m_mapAppClients.erase(eApp);
    }

    LOG_INFO("AppSelect - Current size of AppClientsContainer: \"{0}\"", m_mapAppClients.size());
}

void AppSelect::OnDraw()
{
    const auto &rend = MainApp::GetInstance().ptrRend;

    switch (m_eState)
    {
    case EAppSelectStates::eReadyForSelection:
    {
        /*Draw Background*/
        rend->DrawPicture(m_textureBackground, 0.0f, 0.0f);

        /*Draw Particle Star*/
        m_particleStarEffect.Draw();

        /*Draw Button Kids Fantasy*/
        if (m_bIsKidsFantasyHovered)
        {
            rend->SetColor(1.0f, 1.0f, 1.0f, 0.5f);
        }

        rend->DrawPicture(m_buttonKidsFantasy.textureButton, m_buttonKidsFantasy.fX, m_buttonKidsFantasy.fY);
        rend->SetColor(1.0f, 1.0f, 1.0f, 1.0f);

        /*Draw Button Particle Builder*/
        if (m_bIsParticleBuilder)
        {
            rend->SetColor(1.0f, 1.0f, 1.0f, 0.5f);
        }

        rend->DrawPicture(m_buttonParticleBuilder.textureButton, m_buttonParticleBuilder.fX, m_buttonParticleBuilder.fY);
        rend->SetColor(1.0f, 1.0f, 1.0f, 1.0f);

        /*Draw Button Roulette*/
        if (m_bIsRouletteHovered)
        {
            rend->SetColor(1.0f, 1.0f, 1.0f, 0.5f);
        }

        rend->DrawPicture(m_buttonRoulette.textureButton, m_buttonRoulette.fX, m_buttonRoulette.fY);
        rend->SetColor(1.0f, 1.0f, 1.0f, 1.0f);
    }
    break;

    case EAppSelectStates::eBusyInGame:
    {
        m_mapAppClients[m_eCurrentApp]->OnDraw();
    }
    break;

    default:
        break;
    }
}
