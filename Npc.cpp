// 인간형 적 구현

#include "stdafx.h"
#include "Npc.h"

// CHumanoidObject 인자 순서: moveSpeed, attackRange, moveStopDistance, tooCloseDistance, fireCooldown, health
CNpc::CNpc()
    : CHumanoidObject(2.5f, 180.0f, 120.0f, 55.0f, 0.8f, 50)
{
    BuildHumanoid();

    SetEnemyColor(XMFLOAT4(1.0f, 0.35f, 0.65f, 1.0f));
}

void CNpc::SetEnemyColor(const XMFLOAT4& color)
{
    SetColor(color);

    if (GetTorso())
    {
        GetTorso()->SetColor(color);
    }

    if (GetHead())
    {
        GetHead()->SetColor(XMFLOAT4(min(color.x * 1.25f, 1.0f), min(color.y * 1.25f, 1.0f), min(color.z * 1.25f, 1.0f), color.w));
    }

    if (GetLeftArm())
    {
        GetLeftArm()->SetColor(XMFLOAT4(color.x * 0.85f, color.y * 0.85f, color.z * 0.85f, color.w));
    }

    if (GetRightArm())
    {
        GetRightArm()->SetColor(XMFLOAT4(color.x * 0.85f, color.y * 0.85f, color.z * 0.85f, color.w));
    }

    if (GetLeftLeg())
    {
        GetLeftLeg()->SetColor(XMFLOAT4(color.x * 0.65f, color.y * 0.65f, color.z * 0.65f, color.w));
    }

    if (GetRightLeg())
    {
        GetRightLeg()->SetColor(XMFLOAT4(color.x * 0.65f, color.y * 0.65f, color.z * 0.65f, color.w));
    }
}
