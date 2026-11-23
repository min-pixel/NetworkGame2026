// 탱크 기체

#pragma once

#include "Player.h"

class CDirectMesh;

// 탱크 기체: 빠르고(35) 튼튼한 몸체, 지형에 바로 붙어 움직이며 포신 끝에서 발사한다.
class CTankPlayer : public CPlayer
{
public:
    CTankPlayer();
    ~CTankPlayer() override = default;

    void ApplyTankDirectMeshes(CDirectMesh* bodyMesh, CDirectMesh* turretMesh, CDirectMesh* barrelMesh, CDirectMesh* trackMesh);

    // 기본색 기준으로 차체/포탑/포신 색 지정 (궤도는 검정 고정)
    void SetTankColor(const XMFLOAT4& baseColor);

    EPlayerType GetPlayerType() const override;

private:
    void BuildTankParts();

private:
    CGameObject* body = nullptr;
    CGameObject* turret = nullptr;
    CGameObject* barrel = nullptr;
    CGameObject* leftTrack = nullptr;
    CGameObject* rightTrack = nullptr;
};
