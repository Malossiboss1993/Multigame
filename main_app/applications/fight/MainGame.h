#pragma once

#include <string>

#include <main_app/applications/IApp.h>
#include <main_app/texture/Texture.h>
#include <main_app/timer/TimerMgr.h>

class Fight : public IApp, public ITimer
{

private:
    std::string m_strAppName = "Fight";

    /*Texture main background*/
    std::shared_ptr<Texture> m_textureBackground;

public:
    Fight();

    bool Init() override;
    bool Deinit() override;
    bool HandleEvent() override;
    const std::string &GetAppName() override;
    void OnEnter() override;
    void OnExit() override;
    void OnDraw() override;
    void OnTick(unsigned int unID, unsigned int unTimes) final;
};
