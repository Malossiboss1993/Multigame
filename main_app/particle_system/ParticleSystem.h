#pragma once

#include <vector>

#include <main_app/texture/Texture.h>
#include <main_app/timer/TimerMgr.h>

#include "Random.h"

struct Particle
{
    glm::vec2 vec2Position;
    glm::vec2 vec2StartPosition;
    glm::vec2 vec2Velocity;
    glm::vec4 vec4Color;
    float fRotation = 0.0f;
    float fSize = 1.0f;
    float fLifeRemaining = 1.0f;
    bool bIsActive = false;
    std::shared_ptr<Texture> textureParticle = nullptr;
};

enum class EParticleStates
{
    eInactive,
    eEmitting,
    eEmittingStopped,
    eTotalStatesCount
};

class ParticleSystem : public ITimer
{

private:

    EParticleStates m_eState = EParticleStates::eInactive;

    /*Max pool size of particles*/
    const unsigned int m_unMaxParticlesAllowed = 1000;

    /*Particle index at start*/
    unsigned int m_unParticleIndex;

    /*Current active particles*/
    unsigned int m_unDensity;

    /*Start position*/
    glm::vec2 m_vec2StartPosition;

    /*Current Postion*/
    glm::vec2 m_vec2CurrentPosition;

    /*Start velocity*/
    glm::vec2 m_vec2Velocity;

    /*Velocity variation, in base of this property the particles will fly in different positions*/
    glm::vec2 m_vec2VelocityVariation;

    /*Start color*/
    glm::vec4 m_vec4ColorStart;

    /*End color*/
    glm::vec4 m_vec4ColorEnd;

    /*Orient to Motion - Rotates the particle in the going direction*/
    bool m_bOrientToMotion = false;

    /*Rotation Begin*/
    float m_fRotationBegin;

    /*Rotation End*/
    float m_fRotationEnd;

    /*Start size*/
    float m_fSizeBegin;

    /*End size*/
    float m_fSizeEnd;

    /*Gravity*/
    float m_fGravity;

    /*Life in steps*/
    float m_fLifeTime;

    /*Container holding all particles*/
    std::vector<Particle>m_vecParticles;

    /*Update method called every cycle*/
    void Update(unsigned int unTimes);  

    /*Method called when particle need to be started*/
    void StartCurrentParticle(Particle& particle);

    /*Process properties of particle*/
    void ProcessCurrentParticle(Particle& particle);

    /*Stopping be active when needed*/
    void StopCurrentParticle(Particle& particle);

public:

    bool Init(std::shared_ptr<Texture>textureParticles, glm::vec2 vec2Position);
    void OnTick(unsigned int unID, unsigned int unTimes) final;
    void Draw();
    void StartEmitting();
    void StopEmitting();
    void DieImmediately();
    void Reset();

    /*Set Properties*/
    void SetPosition(glm::vec2 vec2Postion);
    void SetVelocity(glm::vec2 vec2Velocity);
    void SetVelocityVariation(glm::vec2 vec2VelocityVariation);
    void SetColorStart(glm::vec4 vec4ColorStart);
    void SetColorEnd(glm::vec4 vec4ColorEnd);
    void SetOrientToMotion(bool bOrientToMotion);
    void SetRotationBegin(float fRotationBegin);
    void SetRotationEnd(float fRotationEnd);
    void SetSizeBegin(float fSizeBegin);
    void SetSizeEnd(float fSizeEnd);
    void SetGravity(float fGravity);
    void SetLifeTime(float fLifeTimeSteps);
    void SetDensity(unsigned int unDensity);

    /*Get Properties*/
    inline const glm::vec2& GetPosition(){ return m_vec2CurrentPosition; }
    inline const glm::vec2& GetVelocity(){ return m_vec2Velocity; }
    inline const glm::vec2& GetVelocityVariation(){ return m_vec2VelocityVariation; }
    inline const glm::vec4& GetColorStart(){ return m_vec4ColorStart; }
    inline const glm::vec4& GetColorEnd(){ return m_vec4ColorEnd; }
    inline const bool& GetOrientToMotion(){ return m_bOrientToMotion; }
    inline const float& GetRotationBegin(){ return m_fRotationBegin; }
    inline const float& GetRotationEnd(){ return m_fRotationEnd; }
    inline const float& GetSizeBegin(){ return m_fSizeBegin; }
    inline const float& GetSizeEnd(){ return m_fSizeEnd; }
    inline const float& GetGravity(){ return m_fGravity; }
    inline const float& GetLifeTime(){ return m_fLifeTime; }
    inline const unsigned int& GetDensity(){ return m_unDensity; }
};