// 플레이어 기체 공통 클래스

#pragma once

#include "Object.h"
#include "PlayerType.h"

// 플레이어 기체 공통 클래스 (Tank / Helicopter)
// 이동, 발사 쿨타임, 체력, Shield, 총구 위치를 가진다. 체력이 0이 되면 비활성화된다.
class CPlayer : public CGameObject
{
public:
    CPlayer(float moveSpeed = 25.0f, float fireCooldown = 0.25f, int health = 100);

    ~CPlayer() override;

    void Update(float deltaTime) override;

    // 파생 클래스가 자신의 기체 타입을 돌려준다.
    virtual EPlayerType GetPlayerType() const;

    // 주어진 방향으로 이동하고 그 방향을 바라보게 회전한다.
    void MoveByDirection(const XMFLOAT3& moveDirection, float deltaTime);

    bool CanFire() const;
    void ResetFireCooldown();

    bool IsDead() const;

    void ApplyDamage(int amount);
    int GetHealth() const;

    // 기체가 바라보는 방향 (Update 때 회전값으로 갱신)
    XMFLOAT3 GetForward() const;
    XMFLOAT3 GetRight() const;

    // 총알이 나가는 위치 = 위치 + 위로 muzzleHeight + 정면으로 muzzleForwardDistance
    XMFLOAT3 GetMuzzlePosition() const;

    // Shield: 켜져 있으면 모든 총알 피해 0. shieldDuration초 뒤 자동으로 꺼지고,
    // 꺼진 뒤 shieldCooldown초 동안은 다시 켤 수 없다. 켜져 있을 때 누르면 바로 꺼진다(재사용 대기 시작).
    void ToggleShield();
    bool IsShieldEnabled() const;

protected:
    // 기체별 총구 위치 지정 (파생 클래스 생성자에서 호출)
    void SetMuzzleOffset(float height, float forwardDistance);

private:
    void UpdateDirectionVectors();
    void UpdateShieldTimer(float deltaTime);
    void StartShieldCooldown();

private:
    XMFLOAT3 forward;
    XMFLOAT3 right;

    float moveSpeed;

    float fireCooldown;
    float fireTimer;

    int health;

    // Shield 시간 설정 (초)
    float shieldDuration = 3.0f;
    float shieldCooldown = 5.0f;

    bool shieldEnabled = false;
    float shieldTimer = 0.0f;
    float shieldCooldownTimer = 0.0f;

    float muzzleHeight = 1.6f;
    float muzzleForwardDistance = 1.2f;
};
