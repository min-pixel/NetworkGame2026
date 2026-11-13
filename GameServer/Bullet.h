// 총알

#pragma once

#include "Object.h"

// 누가 쏜 총알인지 (Player 총알은 적만, Enemy 총알은 Player만 맞힌다)
enum class EBulletOwner
{
    Player,
    Enemy
};

// 총알 풀에서 재사용되는 총알. 속도 180, 사거리 350, 피해 10
class CBullet : public CGameObject
{
public:
    CBullet();
    ~CBullet() override;

    // 비활성 총알을 활성화하고 발사 위치/방향/주인을 정한다.
    void Fire(const XMFLOAT3& startPosition, const XMFLOAT3& direction, EBulletOwner newOwner);
    void Update(float deltaTime) override;

    int GetDamage() const;
    EBulletOwner GetOwner() const;

    // 직전 프레임 위치 (이전~현재 위치 사이로 지형 충돌을 검사할 때 사용)
    const XMFLOAT3& GetPreviousPosition() const;

private:

    XMFLOAT3 previousPosition;
    XMFLOAT3 direction;
    float speed;
    float elapsedTime;
    float maxDistance;
    int damage;
    EBulletOwner owner;
};
