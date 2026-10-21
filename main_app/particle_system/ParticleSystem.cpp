#include "ParticleSystem.h"

#include <main_app/MainApp.h>
#include <renderer/Renderer.h>
#include <glm/gtx/compatibility.hpp>

constexpr unsigned int g_unTimerUpdate = 1;
constexpr unsigned int g_unTimerUpdatePeriod = 1;

bool ParticleSystem::Init(std::shared_ptr<Texture> textureParticles, glm::vec2 vec2Position)
{
    m_vecParticles.resize(m_unMaxParticlesAllowed);

    m_unParticleIndex = 0;
    m_unDensity = 200;
    m_vec2StartPosition = vec2Position;
    m_vec2CurrentPosition = vec2Position;
    m_vec2Velocity = glm::vec2(0.0f, 0.0f);
    m_vec2VelocityVariation = glm::vec2(1.0f, 1.0f);
    m_vec4ColorStart = glm::vec4(1.0f, 1.0f, 1.0f, 1.0f);
    m_vec4ColorEnd = glm::vec4(1.0f, 1.0f, 1.0f, 0.0f);
    m_fRotationBegin = 0.0f;
    m_fRotationEnd = 0.0f;
    m_fSizeBegin = 1.0f;
    m_fSizeEnd = 1.0f;
    m_fGravity = 0.0f;
    m_fLifeTime = 100.0f;

    for (auto &particle : m_vecParticles)
    {
        particle.textureParticle = textureParticles;
        particle.vec2Position = vec2Position;
        particle.vec2StartPosition = vec2Position;
        particle.fSize = m_fSizeBegin;
    }

    return true;
}

void ParticleSystem::OnTick(unsigned int unID, unsigned int unTimes)
{
    if (unID == g_unTimerUpdate)
    {
        Update(unTimes);
    }
}

void ParticleSystem::Draw()
{
    const auto &rend = MainApp::GetInstance().ptrRend;

    /*Traversing reverse, due to visibility issues when starting*/
    for (auto it = m_vecParticles.rbegin(); it != m_vecParticles.rend(); ++it)
    {
        auto &particle = *it;
        if (!particle.bIsActive)
        {
            continue;
        }

        /*Is dead, need to be alive again*/
        if (particle.fLifeRemaining <= 0.0f && m_eState == EParticleStates::eEmitting)
        {
            StartCurrentParticle(particle);
        }
        /*Is dead, and stopped*/
        else if (particle.fLifeRemaining <= 0.0f && m_eState == EParticleStates::eEmittingStopped)
        {
            StopCurrentParticle(particle);
            continue;
        }

        /*Process every cycle particle properties*/
        ProcessCurrentParticle(particle);

        rend->SetColor(particle.vec4Color.r, particle.vec4Color.g, particle.vec4Color.b, particle.vec4Color.a);
        rend->DrawPictureRotated(particle.textureParticle,
                                 particle.vec2Position.x - (particle.fSize * (particle.textureParticle->GetWidth() / 2.0f)),
                                 particle.vec2Position.y - (particle.fSize * (particle.textureParticle->GetHeight() / 2.0f)),
                                 particle.fRotation,
                                 particle.fSize);
    }

    rend->SetColor(1.0f, 1.0f, 1.0f, 1.0f);
}

void ParticleSystem::StartCurrentParticle(Particle &particle)
{
    /*Needs to be alive*/
    particle.bIsActive = true;

    /*Position*/
    particle.vec2Position = m_vec2CurrentPosition;
    particle.vec2StartPosition = m_vec2CurrentPosition;
    m_vec2StartPosition = m_vec2CurrentPosition;

    /*Rotation*/
    particle.fRotation = m_fRotationBegin;

    /*Velocity*/
    particle.vec2Velocity = m_vec2Velocity;
    particle.vec2Velocity.x += m_vec2VelocityVariation.x * (Random::GetRandomNumber(-1.0f, 1.0f));
    particle.vec2Velocity.y += m_vec2VelocityVariation.y * (Random::GetRandomNumber(-1.0f, 1.0f));

    /*Color*/
    particle.vec4Color = m_vec4ColorStart;

    /*Life*/
    particle.fLifeRemaining = m_fLifeTime * Random::GetRandomNumber(1.0f, 1.5f);

    /*Size*/
    particle.fSize = m_fSizeBegin;
}

void ParticleSystem::ProcessCurrentParticle(Particle &particle)
{
    float fLife = particle.fLifeRemaining / m_fLifeTime;

    /*Decrement life*/
    particle.fLifeRemaining -= 1.0f;

    /*Set current position*/
    particle.vec2Position += particle.vec2Velocity * (Random::GetRandomNumber(1.0f, 1.5f));

    /*Gravity*/
    particle.vec2Position.y += glm::lerp(m_fGravity, 0.0f, fLife);

    /*Set current rotation*/
    if (m_bOrientToMotion)
    {
        particle.fRotation = glm::degrees(atan2(particle.vec2Position.y - particle.vec2StartPosition.y,
                                                particle.vec2Position.x - particle.vec2StartPosition.x));
    }
    else
    {
        particle.fRotation = glm::lerp(m_fRotationEnd, m_fRotationBegin, fLife);
    }

    /*Set current color*/
    particle.vec4Color = glm::lerp(m_vec4ColorEnd, m_vec4ColorStart, fLife);

    /*Set current size*/
    particle.fSize = glm::lerp(m_fSizeEnd, m_fSizeBegin, fLife);
    if (particle.fSize < 0.0f)
    {
        particle.fSize = 0.0f;
    }
}

void ParticleSystem::StopCurrentParticle(Particle &particle)
{
    particle.fLifeRemaining = 0.0f;
    particle.bIsActive = false;
}

void ParticleSystem::Update(unsigned int unTimes)
{

    /*Start emitting*/
    if (m_eState == EParticleStates::eEmitting)
    {
        /*Increment Density*/
        if (m_unParticleIndex < m_unDensity)
        {
            auto &particle = m_vecParticles[m_unParticleIndex];

            StartCurrentParticle(particle);

            if (++m_unParticleIndex >= m_unDensity)
            {
                m_unParticleIndex = m_unDensity;
            }
        }
        /*Decrement Density*/
        else if(m_unParticleIndex > m_unDensity)
        {
            auto &particle = m_vecParticles[m_unParticleIndex];

            StopCurrentParticle(particle);

            if (--m_unParticleIndex <= m_unDensity)
            {
                m_unParticleIndex = m_unDensity;
            }
        }
    }

    /*Continue emitting*/
    bool m_bNeedToStopUpdate = true;
    for (auto &particle : m_vecParticles)
    {
        if (!particle.bIsActive)
        {
            continue;
        }

        m_bNeedToStopUpdate = false;
    }

    if (m_bNeedToStopUpdate)
    {
        m_eState = EParticleStates::eInactive;
        MainApp::GetInstance().ptrTimer->StopTimer(this, g_unTimerUpdate);
    }
}

void ParticleSystem::StartEmitting()
{

    if (m_eState == EParticleStates::eInactive)
    {
        m_unParticleIndex = 0;
        m_eState = EParticleStates::eEmitting;
        MainApp::GetInstance().ptrTimer->StartTimer(this, g_unTimerUpdate, g_unTimerUpdatePeriod);
    }
}

void ParticleSystem::StopEmitting()
{
    if (m_eState == EParticleStates::eInactive)
    {
        return;
    }

    m_unParticleIndex = 0;
    m_eState = EParticleStates::eEmittingStopped;
}

void ParticleSystem::DieImmediately()
{
    Reset();
}

void ParticleSystem::Reset()
{
    m_eState = EParticleStates::eInactive;
    m_unParticleIndex = 0;

    for (auto &particle : m_vecParticles)
    {
        particle.bIsActive = false;
        particle.fLifeRemaining = 0.0f;
    }

    MainApp::GetInstance().ptrTimer->StopTimer(this, g_unTimerUpdate);
    MainApp::GetInstance().ptrTimer->EraseClient(this);
}

void ParticleSystem::SetPosition(glm::vec2 vec2Postion)
{
    m_vec2CurrentPosition = vec2Postion;
}

void ParticleSystem::SetVelocity(glm::vec2 vec2Velocity)
{
    m_vec2Velocity = vec2Velocity;
}

void ParticleSystem::SetVelocityVariation(glm::vec2 vec2VelocityVariation)
{
    m_vec2VelocityVariation = vec2VelocityVariation;
}

void ParticleSystem::SetColorStart(glm::vec4 vec4ColorStart)
{
    m_vec4ColorStart = vec4ColorStart;
}

void ParticleSystem::SetColorEnd(glm::vec4 vec4ColorEnd)
{
    m_vec4ColorEnd = vec4ColorEnd;
}

void ParticleSystem::SetOrientToMotion(bool bOrientToMotion)
{
    m_bOrientToMotion = bOrientToMotion;
}

void ParticleSystem::SetRotationBegin(float fRotationBegin)
{
    m_fRotationBegin = fRotationBegin;
}

void ParticleSystem::SetRotationEnd(float fRotationEnd)
{
    m_fRotationEnd = fRotationEnd;
}

void ParticleSystem::SetSizeBegin(float fSizeBegin)
{
    m_fSizeBegin = fSizeBegin;
}

void ParticleSystem::SetSizeEnd(float fSizeEnd)
{
    m_fSizeEnd = fSizeEnd;
}

void ParticleSystem::SetGravity(float fGravity)
{
    m_fGravity = fGravity;
}

void ParticleSystem::SetLifeTime(float fLifeTimeSteps)
{
    m_fLifeTime = fLifeTimeSteps;
}

void ParticleSystem::SetDensity(unsigned int unDensity)
{
    if (unDensity > m_unMaxParticlesAllowed)
    {
        return;
    }

    m_unDensity = unDensity;
}
