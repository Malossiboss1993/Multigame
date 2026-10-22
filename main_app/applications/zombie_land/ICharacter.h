#pragma once

class ICharacter
{
public:
    virtual ~ICharacter() = default;

    virtual bool Init() = 0;
    virtual void Deinit() = 0;
    virtual void Reset() = 0;
    virtual void Stop() = 0;
    virtual void Update(float fDeltaTimeSeconds) = 0;
    virtual void Draw(float fWorldOffsetX) = 0;

    virtual float GetFeetWorldX() const = 0;
    virtual float GetFeetY() const = 0;
    virtual void SetFeetWorldX(float fWorldX) = 0;
    virtual void SetFeetY(float fFeetY) = 0;
    virtual bool IsFacingRight() const = 0;
};
