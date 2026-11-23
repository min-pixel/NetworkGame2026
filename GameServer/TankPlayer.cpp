// 탱크 기체 구현

#include "stdafx.h"
#include "TankPlayer.h"
#include "Collider.h"

CTankPlayer::CTankPlayer()
    : CPlayer(35.0f, 0.8f, 100)
{
    BuildTankParts();

    SetColor(XMFLOAT4(0.20f, 0.60f, 0.25f, 1.0f));

    SetCollider(new COrientedBoxCollider(XMFLOAT3(0.0f, 1.0f, 0.0f), XMFLOAT3(2.4f, 1.2f, 3.0f)));

    SetMuzzleOffset(1.65f, 4.2f);
}

void CTankPlayer::BuildTankParts()
{
    body = new CGameObject();
    turret = new CGameObject();
    barrel = new CGameObject();
    leftTrack = new CGameObject();
    rightTrack = new CGameObject();

    AddChild(body);
    AddChild(turret);
    AddChild(barrel);
    AddChild(leftTrack);
    AddChild(rightTrack);

    body->SetPosition(0.0f, 0.9f, 0.0f);
    body->SetColor(XMFLOAT4(0.20f, 0.55f, 0.25f, 1.0f));

    turret->SetPosition(0.0f, 1.65f, 0.3f);
    turret->SetColor(XMFLOAT4(0.25f, 0.70f, 0.30f, 1.0f));

    barrel->SetPosition(0.0f, 1.65f, 2.45f);
    barrel->SetColor(XMFLOAT4(0.15f, 0.35f, 0.15f, 1.0f));

    leftTrack->SetPosition(-2.0f, 0.55f, 0.0f);
    leftTrack->SetColor(XMFLOAT4(0.08f, 0.08f, 0.08f, 1.0f));

    rightTrack->SetPosition(2.0f, 0.55f, 0.0f);
    rightTrack->SetColor(XMFLOAT4(0.08f, 0.08f, 0.08f, 1.0f));
}

void CTankPlayer::ApplyTankDirectMeshes(CDirectMesh* bodyMesh, CDirectMesh* turretMesh, CDirectMesh* barrelMesh, CDirectMesh* trackMesh)
{
    if (body)
    {
        body->SetDirectMesh(bodyMesh);
    }

    if (turret)
    {
        turret->SetDirectMesh(turretMesh);
    }

    if (barrel)
    {
        barrel->SetDirectMesh(barrelMesh);
    }

    if (leftTrack)
    {
        leftTrack->SetDirectMesh(trackMesh);
    }

    if (rightTrack)
    {
        rightTrack->SetDirectMesh(trackMesh);
    }
}

void CTankPlayer::SetTankColor(const XMFLOAT4& baseColor)
{
    SetColor(baseColor);

    if (body)
    {
        body->SetColor(baseColor);
    }

    if (turret)
    {
        turret->SetColor(XMFLOAT4(min(baseColor.x + 0.15f, 1.0f), min(baseColor.y + 0.15f, 1.0f), min(baseColor.z + 0.15f, 1.0f), 1.0f));
    }

    if (barrel)
    {
        barrel->SetColor(baseColor);
    }

    if (leftTrack)
    {
        leftTrack->SetColor(XMFLOAT4(0.05f, 0.05f, 0.05f, 1.0f));
    }

    if (rightTrack)
    {
        rightTrack->SetColor(XMFLOAT4(0.05f, 0.05f, 0.05f, 1.0f));
    }
}

EPlayerType CTankPlayer::GetPlayerType() const
{
    return EPlayerType::Tank;
}
