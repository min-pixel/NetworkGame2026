// 헬리콥터 기체 구현

#include "stdafx.h"
#include "HelicopterPlayer.h"
#include "Collider.h"

CHelicopterPartObject::CHelicopterPartObject(HelicopterPartType type)
    : partType(type)
{
}

HelicopterPartType CHelicopterPartObject::GetPartType() const
{
    return partType;
}

CHelicopterPlayer::CHelicopterPlayer()
    : CPlayer(25.0f, 0.25f, 100)
{
    BuildHelicopterParts();

    SetColor(XMFLOAT4(0.15f, 0.55f, 0.95f, 1.0f));
}

CHelicopterPlayer::~CHelicopterPlayer()
{
}

EPlayerType CHelicopterPlayer::GetPlayerType() const
{
    return EPlayerType::Helicopter;
}

void CHelicopterPlayer::BuildHelicopterParts()
{
    SetCollider(new COrientedBoxCollider(XMFLOAT3(0.0f, 1.2f, -0.45f), XMFLOAT3(1.6f, 0.9f, 2.4f)));

    helicopterBody = new CHelicopterPartObject(HelicopterPartType::Body);
    helicopterTail = new CHelicopterPartObject(HelicopterPartType::Tail);
    rotorBlade1 = new CHelicopterPartObject(HelicopterPartType::RotorBlade1);
    rotorBlade2 = new CHelicopterPartObject(HelicopterPartType::RotorBlade2);

    AddChild(helicopterBody);
    AddChild(helicopterTail);
    AddChild(rotorBlade1);
    AddChild(rotorBlade2);

    helicopterBody->SetPosition(0.0f, 1.2f, 0.0f);
    helicopterBody->SetScale(1.0f, 1.0f, 1.0f);
    helicopterBody->SetColor(XMFLOAT4(0.15f, 0.55f, 0.95f, 1.0f));
    helicopterBody->SetCollider(new COrientedBoxCollider(XMFLOAT3(0.0f, 0.0f, 0.0f), XMFLOAT3(0.8f, 0.4f, 0.6f)));

    helicopterTail->SetPosition(0.0f, 1.2f, -1.55f);
    helicopterTail->SetScale(1.0f, 1.0f, 1.0f);
    helicopterTail->SetColor(XMFLOAT4(0.10f, 0.35f, 0.75f, 1.0f));
    helicopterTail->SetCollider(new COrientedBoxCollider(XMFLOAT3(0.0f, 0.0f, 0.0f), XMFLOAT3(0.15f, 0.15f, 1.0f)));

    rotorBlade1->SetPosition(0.0f, 1.85f, 0.0f);
    rotorBlade1->SetScale(1.0f, 1.0f, 1.0f);
    rotorBlade1->SetColor(XMFLOAT4(0.95f, 0.95f, 0.95f, 1.0f));
    rotorBlade1->SetCollider(new COrientedBoxCollider(XMFLOAT3(0.0f, 0.0f, 0.0f), XMFLOAT3(1.4f, 0.05f, 0.10f)));

    rotorBlade2->SetPosition(0.0f, 1.85f, 0.0f);
    rotorBlade2->SetScale(1.0f, 1.0f, 1.0f);
    rotorBlade2->SetRotation(0.0f, 90.0f, 0.0f);
    rotorBlade2->SetColor(XMFLOAT4(0.95f, 0.95f, 0.95f, 1.0f));
    rotorBlade2->SetCollider(new COrientedBoxCollider(XMFLOAT3(0.0f, 0.0f, 0.0f), XMFLOAT3(1.4f, 0.05f, 0.10f)));
}

void CHelicopterPlayer::ApplyHelicopterDirectMeshes(CDirectMesh* bodyMesh, CDirectMesh* tailMesh, CDirectMesh* rotorMesh)
{
    if (helicopterBody)
    {
        helicopterBody->SetDirectMesh(bodyMesh);
    }

    if (helicopterTail)
    {
        helicopterTail->SetDirectMesh(tailMesh);
    }

    if (rotorBlade1)
    {
        rotorBlade1->SetDirectMesh(rotorMesh);
    }

    if (rotorBlade2)
    {
        rotorBlade2->SetDirectMesh(rotorMesh);
    }
}

void CHelicopterPlayer::SetHelicopterColor(const XMFLOAT4& bodyColor, const XMFLOAT4& tailColor)
{
    SetColor(bodyColor);

    if (helicopterBody)
    {
        helicopterBody->SetColor(bodyColor);
    }

    if (helicopterTail)
    {
        helicopterTail->SetColor(tailColor);
    }
}

void CHelicopterPlayer::Animate(float deltaTime)
{
    rotorAngle += rotorSpeed * deltaTime;

    if (rotorAngle >= 360.0f)
    {
        rotorAngle = fmodf(rotorAngle, 360.0f);
    }

    if (rotorBlade1)
    {
        rotorBlade1->SetRotation(0.0f, rotorAngle, 0.0f);
    }

    if (rotorBlade2)
    {
        rotorBlade2->SetRotation(0.0f, rotorAngle + 90.0f, 0.0f);
    }
}

CHelicopterPartObject* CHelicopterPlayer::GetHelicopterBody() const
{
    return helicopterBody;
}

CHelicopterPartObject* CHelicopterPlayer::GetHelicopterTail() const
{
    return helicopterTail;
}

CHelicopterPartObject* CHelicopterPlayer::GetRotorBlade1() const
{
    return rotorBlade1;
}

CHelicopterPartObject* CHelicopterPlayer::GetRotorBlade2() const
{
    return rotorBlade2;
}
