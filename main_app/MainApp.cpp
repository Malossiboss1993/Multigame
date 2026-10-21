#include "MainApp.h"

#include <iostream>

#include <imgui/imgui.h>
#include <imgui/imgui_impl_glfw_gl3.h>
#include <debug/Logger.h>
#include <app_select/AppSelect.h>
#include <audio_player/AudioPlayer.h>
#include <renderer/Renderer.h>
#include <timer/TimerMgr.h>
#include <panel/Panel.h>


MainApp::MainApp() : ptrLogger(new Debug::Logger),
                     ptrRend(new Renderer),
                     ptrAppSelect(new AppSelect),
                     ptrAudioPlayer(new AudioPlayer),
                     ptrTimer(new TimerMgr),
                     ptrPanel(new Panel)
{

}

MainApp::~MainApp()
{

}

bool MainApp::Init()
{
    if(!ptrLogger->Init("Multigame"))
    {
        std::cout << "MainApp - Failed to initialize Logger !" << std::endl;
        return false;
    }

    if(!ptrRend->InitSystem("Multigame",
                                1920,
                                1080,
                                100))
    {
        LOG_ERROR("MainApp - Failed to initialize Renderer !");
        return false;
    }
    ptrRend->CreateAndFill2DBuffers();
    ptrRend->CreateAndFill3DBuffers();

    InitImgui();

    if(!ptrAudioPlayer->Init())
    {
        LOG_ERROR("MainApp - Failed to init AudioPlayer");
        return false;
    }

    if(!ptrAppSelect->Init())
    {
        LOG_ERROR("MainApp - Failed to init AppSelect");
        return false;
    }

    if(!ptrPanel->Init())
    {
        LOG_ERROR("MainApp - Failed to init Panel");
        return false;
    }

    LOG_INFO("MainApp - Initialized ...");
    return true;
}

void MainApp::InitImgui()
{
    ImGui::CreateContext();
    ImGui_ImplGlfwGL3_Init(ptrRend->GetWindow(), true);
    ImGui::StyleColorsDark();
    ImGui::GetStyle().FrameRounding = 5.0f;
    ImGui::GetStyle().GrabRounding = 5.0f;
}

void MainApp::Run()
{
    while(!glfwWindowShouldClose(ptrRend->GetWindow()))
    {
        /*Delta Time Measure*/
        m_frameBeginTime = std::chrono::system_clock::now();

        /*Toggle fullscreen with F11*/
        const bool bFullscreenKeyPressed =
            glfwGetKey(ptrRend->GetWindow(), GLFW_KEY_F11) == GLFW_PRESS;

        if (bFullscreenKeyPressed && !m_bFullscreenKeyPressed)
        {
            ptrRend->ToggleFullscreen();
        }

        m_bFullscreenKeyPressed = bFullscreenKeyPressed;

        /*Keep ImGui in the same logical coordinate space as the game. The
          renderer scales the fixed-resolution scene to the window and
          letterboxes it when necessary.*/
        float fLogicalOffsetX = 0.0f;
        float fLogicalOffsetY = 0.0f;
        float fLogicalScale = 1.0f;
        ptrRend->GetWindowToLogicalTransform(fLogicalOffsetX, fLogicalOffsetY, fLogicalScale);

        ImGui_ImplGlfwGL3_SetDisplayTransform(static_cast<float>(ptrRend->SCREEN_WIDTH),
                                              static_cast<float>(ptrRend->SCREEN_HEIGHT),
                                              fLogicalOffsetX,
                                              fLogicalOffsetY,
                                              fLogicalScale);

        /* Debug New frame calculation */
        ImGui_ImplGlfwGL3_NewFrame();

        /* Handle Event */
        ptrAppSelect->HandleEvent();

        /*Process Timing*/
        ptrTimer->Process();

        /*Render the scene into the fixed-resolution framebuffer*/
        ptrRend->BeginFrame();

        /*Main Draw*/
        ptrAppSelect->OnDraw();

        /*ImGui HELP*/
        // if (m_bShowImguiHelp)
        // {
        //     ImGui::SetNextWindowPos(ImVec2(650, 20), ImGuiCond_FirstUseEver);
        //     ImGui::ShowDemoWindow(&m_bShowImguiHelp);
        // }

        /*Draw Imgui*/
        ImGui::Render();
        ImGui_ImplGlfwGL3_RenderDrawData(ImGui::GetDrawData());

        /*Scale the fixed-resolution frame to the window and draw the bars*/
        ptrRend->EndFrame();

        /* SwapBuffers */
        ptrRend->CallEventsAndSwapBuffers();

        m_frameEndTime = std::chrono::system_clock::now();
        m_fDeltaTimeInMicroSeconds = std::chrono::duration_cast<std::chrono::microseconds>(m_frameEndTime - m_frameBeginTime).count();
    }
}

bool MainApp::Deinit()
{
    ImGui::DestroyContext();
    ptrAppSelect->Deinit();
    ptrPanel->Deinit();
    ptrAudioPlayer->Deinit();
    ptrRend->Deinit();

    LOG_WARN("MainApp - Deinitialzied ...");
    return true;
}

const float& MainApp::GetDeltaTime()
{
    m_fDeltaTimeInMilliSeconds = m_fDeltaTimeInMicroSeconds / 1000.0f;
    return m_fDeltaTimeInMilliSeconds;
}

const float& MainApp::GetDeltaTimeInMicroSeconds()
{
    return m_fDeltaTimeInMicroSeconds;
}

const unsigned int MainApp::GeTimeStampInMicroSeconds()
{
    return std::chrono::duration_cast<std::chrono::microseconds>(m_frameBeginTime.time_since_epoch()).count();
}
