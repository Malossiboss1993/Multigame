#pragma once

#include <main_app/applications/IApp.h>
#include <main_app/texture/Texture.h>
#include <main_app/button/Button.h>
#include <main_app/applications/kids_fantasy/MainGame.h>
#include <main_app/applications/particle_builder/MainGame.h>
#include <main_app/applications/roulette/MainGame.h>

#include <map>

enum class EApps
{
    eKidsFantasy,
    eParticleBuilder,
    eRoulette,
    eAppSelect,
    eTotalAppsCount
};

enum class EAppSelectStates
{
    eInactive,
    eReadyForSelection,
    eBusyInGame,
    eTotalAppSelectsStates
};

class AppSelect
{

private:
    /*boolean flags button hovered*/
    bool m_bIsKidsFantasyHovered = false;
    bool m_bIsParticleBuilderHovered = false;
    bool m_bIsRouletteHovered = false;

    /*Current active app*/
    EApps m_eCurrentApp = EApps::eAppSelect;

    /*Current AppSelect state*/
    EAppSelectStates m_eState = EAppSelectStates::eInactive;

    /*Container, holding all the registered IApp objects*/
    std::map<EApps, IApp *> m_mapAppClients;

    /*Texture main background*/
    std::shared_ptr<Texture> m_textureBackground;

    /*Kids Fantasy Button*/
    Button m_buttonKidsFantasy;

    /*Particle Builder Button*/
    Button m_buttonParticleBuilder;

    /*Roulette Button*/
    Button m_buttonRoulette;

    /*Kids Fantasy object*/
    KidsFantasy m_KidsFantasy;

    /*Particle Builder object*/
    ParticleBuilder m_ParticleBuilder;

    /*Roulette object;*/
    Roulette m_Roulette;

    /*Particle System Star Effect*/
    ParticleSystem m_particleStarEffect;

    /*Return the registered client for an application, or nullptr if invalid*/
    IApp *GetAppClient(EApps eApp) const;

    /*Enter from App to AppSelect*/
    void OnEnterInAppSelect();

    /*Exit from App Select, enter to App*/
    void OnExitFromAppSelect();

public:
    bool Init();
    bool Deinit();
    bool HandleEvent();
    bool RequestTransition(const EApps eAppToTransition);
    const EAppSelectStates &GetState();
    void RegisterClient(EApps eApp, IApp *client);
    void UnregisterClient(EApps eApp, IApp *client);
    void OnDraw();
};
