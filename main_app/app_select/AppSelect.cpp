#include "AppSelect.h"

#include <imgui/imgui.h>
#include <imgui/imgui_impl_glfw_gl3.h>

#include <main_app/MainApp.h>
#include <main_app/renderer/Renderer.h>
#include <main_app/audio_player/AudioPlayer.h>
#include <debug/Logger.h>

#include <algorithm>
#include <vector>

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

    m_buttonFight.textureButton = Texture::CreateTexture("../src/resources/fight/fight_button.png");

    if (!m_textureBackground ||
        !m_buttonParticleBuilder.textureButton ||
        !m_buttonKidsFantasy.textureButton ||
        !m_buttonRoulette.textureButton ||
        !m_buttonFight.textureButton)
    {
        LOG_ERROR("AppSelect - Unable to create textures !");
        return false;
    }

    if (!m_textureBackground->Load() ||
        !m_buttonParticleBuilder.textureButton->Load() ||
        !m_buttonKidsFantasy.textureButton->Load() ||
        !m_buttonRoulette.textureButton->Load() ||
        !m_buttonFight.textureButton->Load())
    {
        LOG_ERROR("AppSelect - Unable to load textures !");
        return false;
    }

    /*Keep the existing three apps in the first row and place Fight in the
      second row under Kids Fantasy, based on the current screen size.*/
    Button *firstRowButtons[] = { &m_buttonKidsFantasy, &m_buttonRoulette, &m_buttonParticleBuilder };

    float firstRowWidth = 0.0f;
    float maxButtonHeight = 0.0f;

    for (auto *btn : firstRowButtons)
    {
        firstRowWidth += static_cast<float>(btn->textureButton->GetWidth());
        maxButtonHeight = std::max(maxButtonHeight,
                                   static_cast<float>(btn->textureButton->GetHeight()));
    }

    maxButtonHeight = std::max(maxButtonHeight,
                               static_cast<float>(m_buttonFight.textureButton->GetHeight()));

    const float margin = (screenWidth - firstRowWidth) / 4.0f;
    const float rowGap = 70.0f;
    const float totalHeight = maxButtonHeight * 2.0f + rowGap;
    const float firstRowY = (screenHeight - totalHeight) / 2.0f;

    float currentX = margin;

    for (auto *btn : firstRowButtons)
    {
        const float buttonHeight = static_cast<float>(btn->textureButton->GetHeight());

        btn->fX = currentX;
        btn->fY = firstRowY + (maxButtonHeight - buttonHeight) / 2.0f;
        currentX += static_cast<float>(btn->textureButton->GetWidth()) + margin;
    }

    const float fightButtonWidth = static_cast<float>(m_buttonFight.textureButton->GetWidth());
    const float fightButtonHeight = static_cast<float>(m_buttonFight.textureButton->GetHeight());

    m_buttonFight.fX = m_buttonKidsFantasy.fX +
                       (static_cast<float>(m_buttonKidsFantasy.textureButton->GetWidth()) -
                        fightButtonWidth) / 2.0f;
    m_buttonFight.fY = firstRowY + maxButtonHeight + rowGap +
                       (maxButtonHeight - fightButtonHeight) / 2.0f;

    /*Register Client Kids Fantasy*/
    RegisterClient(EApps::eKidsFantasy, &m_KidsFantasy);

    /*Register Client Roulette*/
    RegisterClient(EApps::eRoulette, &m_Roulette);

    /*Register Client Particle Builder*/
    RegisterClient(EApps::eParticleBuilder, &m_ParticleBuilder);

    /*Register Client Fight*/
    RegisterClient(EApps::eFight, &m_Fight);

    /*Initalize all the applications*/
    std::vector<IApp *> vecInitializedApps;
    vecInitializedApps.reserve(m_mapAppClients.size());

    const auto deinitializeInitializedApps = [&vecInitializedApps]()
    {
        for (auto it = vecInitializedApps.rbegin(); it != vecInitializedApps.rend(); ++it)
        {
            (*it)->Deinit();
        }
    };

    for (const auto &app : m_mapAppClients)
    {
        if (!app.second->Init())
        {
            LOG_ERROR("AppSelect - Cannot initialize client : \"{0}\"", app.second->GetAppName());

            /*Roll back applications initialized before the failure*/
            deinitializeInitializedApps();
            return false;
        }

        vecInitializedApps.emplace_back(app.second);
    }

    auto textureStar = Texture::CreateTexture("../src/resources/particle_sun.png");
    if (!textureStar || !textureStar->Load())
    {
        LOG_ERROR("AppSelect - Unable to load texture particle !");
        deinitializeInitializedApps();
        return false;
    }

    if (!m_particleStarEffect.Init(textureStar, {rend->SCREEN_WIDTH / 2, rend->SCREEN_HEIGHT / 2}))
    {
        LOG_ERROR("AppSelect - Unable to initialize particle star effect !");
        deinitializeInitializedApps();
        return false;
    }

    m_particleStarEffect.SetVelocityVariation({2.5f, 2.0f});
    m_particleStarEffect.SetSizeBegin(0.001f);
    m_particleStarEffect.SetSizeEnd(1.0f);
    m_particleStarEffect.SetLifeTime(400.0f);
    m_particleStarEffect.SetDensity(30.0f);
    m_particleStarEffect.StartEmitting();

    m_eState = EAppSelectStates::eReadyForSelection;

    LOG_INFO("AppSelect - Initialized ...");
    return true;
}

bool AppSelect::Deinit()
{
    /*Stop the particle timer before the applications are deinitialized*/
    m_particleStarEffect.Reset();

    bool bDeinitialized = true;

    /*Deinitialize all the applications*/
    for (const auto &app : m_mapAppClients)
    {
        if (!app.second->Deinit())
        {
            LOG_ERROR("AppSelect - Cannot Deinitialize client : \"{0}\"", app.second->GetAppName());
            bDeinitialized = false;
        }
    }

    m_mapAppClients.clear();
    m_eCurrentApp = EApps::eAppSelect;
    m_eState = EAppSelectStates::eInactive;

    LOG_WARN("AppSelect - Deinitialzied ...");
    return bDeinitialized;
}

bool AppSelect::HandleEvent()
{
    /*Hover on Game Buttons*/
    const int nXMouse = static_cast<int>(ImGui::GetMousePos().x);
    const int nYMouse = static_cast<int>(ImGui::GetMousePos().y);

    switch (m_eState)
    {
    case EAppSelectStates::eReadyForSelection:
    {
        /*Update every hover flag before handling a click, so moving the mouse
          between buttons cannot leave another button highlighted.*/
        m_bIsKidsFantasyHovered = m_buttonKidsFantasy.IsHovered(nXMouse, nYMouse);
        m_bIsParticleBuilderHovered = m_buttonParticleBuilder.IsHovered(nXMouse, nYMouse);
        m_bIsRouletteHovered = m_buttonRoulette.IsHovered(nXMouse, nYMouse);
        m_bIsFightHovered = m_buttonFight.IsHovered(nXMouse, nYMouse);

        bool bAppSelected = false;
        EApps eSelectedApp = EApps::eAppSelect;

        if (m_bIsKidsFantasyHovered && m_buttonKidsFantasy.IsPressed(nXMouse, nYMouse))
        {
            eSelectedApp = EApps::eKidsFantasy;
            bAppSelected = true;
        }
        else if (m_bIsParticleBuilderHovered && m_buttonParticleBuilder.IsPressed(nXMouse, nYMouse))
        {
            eSelectedApp = EApps::eParticleBuilder;
            bAppSelected = true;
        }
        else if (m_bIsRouletteHovered && m_buttonRoulette.IsPressed(nXMouse, nYMouse))
        {
            eSelectedApp = EApps::eRoulette;
            bAppSelected = true;
        }
        else if (m_bIsFightHovered && m_buttonFight.IsPressed(nXMouse, nYMouse))
        {
            eSelectedApp = EApps::eFight;
            bAppSelected = true;
        }

        if (bAppSelected && RequestTransition(eSelectedApp))
        {
            MainApp::GetInstance().ptrAudioPlayer->PlaySound("../src/resources/panel/sounds/enter.wav");
            return true;
        }

        return m_bIsKidsFantasyHovered ||
               m_bIsParticleBuilderHovered ||
               m_bIsRouletteHovered ||
               m_bIsFightHovered;
    }

    case EAppSelectStates::eBusyInGame:
    {
        if (IApp *app = GetAppClient(m_eCurrentApp))
        {
            app->HandleEvent();
        }
    }
    break;

    default:
        break;
    }

    return false;
}

bool AppSelect::RequestTransition(const EApps eAppToTransition)
{
    /*From App Select to an application*/
    if (m_eState == EAppSelectStates::eReadyForSelection)
    {
        if (eAppToTransition == EApps::eAppSelect)
        {
            LOG_ERROR("AppSelect - Cannot transition from App Select to App Select");
            return false;
        }

        IApp *appToEnter = GetAppClient(eAppToTransition);
        if (appToEnter == nullptr)
        {
            return false;
        }

        LOG_INFO("AppSelect - Requesting Transition to App \"{0}\"", appToEnter->GetAppName());

        OnExitFromAppSelect();
        m_eCurrentApp = eAppToTransition;
        m_eState = EAppSelectStates::eBusyInGame;
        appToEnter->OnEnter();

        return true;
    }

    /*From an application back to App Select*/
    if (m_eState == EAppSelectStates::eBusyInGame &&
        eAppToTransition == EApps::eAppSelect)
    {
        IApp *appToExit = GetAppClient(m_eCurrentApp);
        if (appToExit == nullptr)
        {
            return false;
        }

        LOG_INFO("AppSelect - Requesting Transition to App Select");

        appToExit->OnExit();
        m_eCurrentApp = EApps::eAppSelect;
        m_eState = EAppSelectStates::eReadyForSelection;
        OnEnterInAppSelect();

        return true;
    }

    LOG_ERROR("AppSelect - Impossible transition from state \"{0}\" to app \"{1}\"",
              static_cast<int>(m_eState), static_cast<int>(eAppToTransition));
    return false;
}

const EAppSelectStates &AppSelect::GetState()
{
    return m_eState;
}

void AppSelect::OnEnterInAppSelect()
{
    m_bIsKidsFantasyHovered = false;
    m_bIsParticleBuilderHovered = false;
    m_bIsRouletteHovered = false;
    m_bIsFightHovered = false;

    m_particleStarEffect.StartEmitting();

    LOG_INFO("AppSelect - Enter in App Select succeed");
}

void AppSelect::OnExitFromAppSelect()
{
    m_bIsKidsFantasyHovered = false;
    m_bIsParticleBuilderHovered = false;
    m_bIsRouletteHovered = false;
    m_bIsFightHovered = false;

    /*The effect is not drawn while a game is active, so stop its timer instead
      of letting it emit in the background.*/
    m_particleStarEffect.Reset();

    LOG_INFO("AppSelect - Exit from App Select succeed");
}

IApp *AppSelect::GetAppClient(const EApps eApp) const
{
    const auto it = m_mapAppClients.find(eApp);
    if (it == m_mapAppClients.end() || it->second == nullptr)
    {
        LOG_ERROR("AppSelect - No registered client for app \"{0}\"", static_cast<int>(eApp));
        return nullptr;
    }

    return it->second;
}

void AppSelect::RegisterClient(EApps eApp, IApp *client)
{
    if (client == nullptr)
    {
        LOG_ERROR("AppSelect - Cannot register a null client");
        return;
    }

    const auto result = m_mapAppClients.emplace(eApp, client);
    if (!result.second)
    {
        LOG_WARN("AppSelect - Client for app \"{0}\" is already registered",
                 static_cast<int>(eApp));
        return;
    }

    LOG_INFO("AppSelect - Registered client : \"{0}\"", client->GetAppName());
    LOG_INFO("AppSelect - Current size of AppClientsContainer: \"{0}\"", m_mapAppClients.size());
}

void AppSelect::UnregisterClient(EApps eApp, IApp *client)
{
    const auto it = m_mapAppClients.find(eApp);
    if (it == m_mapAppClients.end())
    {
        LOG_WARN("AppSelect - Cannot unregister unknown app \"{0}\"", static_cast<int>(eApp));
        return;
    }

    if (it->second != client)
    {
        LOG_WARN("AppSelect - Cannot unregister client for app \"{0}\": client mismatch",
                 static_cast<int>(eApp));
        return;
    }

    LOG_INFO("AppSelect - Unregistered client : \"{0}\"", it->second->GetAppName());
    m_mapAppClients.erase(it);

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

        const auto drawButton = [&rend](const Button &button, bool bHovered)
        {
            if (bHovered)
            {
                rend->SetColor(1.0f, 1.0f, 1.0f, 0.5f);
            }

            rend->DrawPicture(button.textureButton, button.fX, button.fY);
            rend->SetColor(1.0f, 1.0f, 1.0f, 1.0f);
        };

        drawButton(m_buttonKidsFantasy, m_bIsKidsFantasyHovered);
        drawButton(m_buttonParticleBuilder, m_bIsParticleBuilderHovered);
        drawButton(m_buttonRoulette, m_bIsRouletteHovered);
        drawButton(m_buttonFight, m_bIsFightHovered);
    }
    break;

    case EAppSelectStates::eBusyInGame:
    {
        if (IApp *app = GetAppClient(m_eCurrentApp))
        {
            app->OnDraw();
        }
    }
    break;

    default:
        break;
    }
}
