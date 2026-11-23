// 탱크형 적 (Map2, Map3)

#pragma once

#include "Object.h"

class CDirectMesh;

// 차체/포탑/포신/궤도로 이루어진 탱크형 적. 체력 100
class CTankObject : public CEnemy
{
public:
    CTankObject();
    ~CTankObject() override = default;

    void ApplyTankDirectMeshes(CDirectMesh* bodyMesh, CDirectMesh* turretMesh, CDirectMesh* barrelMesh, CDirectMesh* trackMesh);

    void SetTankColor(const XMFLOAT4& baseColor);

    XMFLOAT3 GetForward() const;

    void SetEnemyColor(        const XMFLOAT4& color    ) override;

private:
    void BuildTankParts();

private:
    CGameObject* body = nullptr;
    CGameObject* turret = nullptr;
    CGameObject* barrel = nullptr;
    CGameObject* leftTrack = nullptr;
    CGameObject* rightTrack = nullptr;
};
