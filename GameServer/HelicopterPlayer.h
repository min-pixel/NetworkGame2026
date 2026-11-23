// 헬리콥터 기체

#pragma once

#include "Player.h"

class CDirectMesh;

enum class HelicopterPartType
{
    Body,
    Tail,
    RotorBlade1,
    RotorBlade2
};

// 헬리콥터 부위 하나 (몸통/꼬리/프로펠러)
class CHelicopterPartObject : public CGameObject
{
public:
    explicit CHelicopterPartObject(HelicopterPartType type);

    HelicopterPartType GetPartType() const;

private:
    HelicopterPartType partType;
};

// 헬리콥터 기체: 지면보다 높으면 천천히 내려오며, 프로펠러가 계속 회전한다.
class CHelicopterPlayer : public CPlayer
{
public:
    CHelicopterPlayer();
    ~CHelicopterPlayer() override;

    EPlayerType GetPlayerType() const override;

    void ApplyHelicopterDirectMeshes(CDirectMesh* bodyMesh, CDirectMesh* tailMesh, CDirectMesh* rotorMesh);

    // 몸통/꼬리 색 지정 (Shield 표시에 사용)
    void SetHelicopterColor(const XMFLOAT4& bodyColor, const XMFLOAT4& tailColor);

    CHelicopterPartObject* GetHelicopterBody() const;
    CHelicopterPartObject* GetHelicopterTail() const;
    CHelicopterPartObject* GetRotorBlade1() const;
    CHelicopterPartObject* GetRotorBlade2() const;

protected:
    void Animate(float deltaTime) override;

private:
    void BuildHelicopterParts();

private:
    CHelicopterPartObject* helicopterBody = nullptr;
    CHelicopterPartObject* helicopterTail = nullptr;
    CHelicopterPartObject* rotorBlade1 = nullptr;
    CHelicopterPartObject* rotorBlade2 = nullptr;

    float rotorAngle = 0.0f;
    float rotorSpeed = 720.0f;
};
