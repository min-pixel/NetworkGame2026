// 플레이어 기체 공통 클래스 구현

#include "stdafx.h"
#include "Player.h"

CPlayer::CPlayer(float moveSpeed, float fireCooldown, int health)
    : forward(0.0f, 0.0f, 1.0f),
    right(1.0f, 0.0f, 0.0f),
    moveSpeed(moveSpeed),
    fireCooldown(fireCooldown),
    fireTimer(0.0f),
    health(health)
{
    SetColor(XMFLOAT4(0.2f, 0.4f, 1.0f, 1.0f));
}

CPlayer::~CPlayer()
{
}

void CPlayer::Update(float deltaTime)
{
    if (fireTimer > 0.0f)
    {
        fireTimer -= deltaTime;
        if (fireTimer < 0.0f)
        {
            fireTimer = 0.0f;
        }
    }

    UpdateShieldTimer(deltaTime);

    UpdateDirectionVectors();

    CGameObject::Update(deltaTime);
}

bool CPlayer::CanFire() const
{
    return fireTimer <= 0.0f;
}

void CPlayer::ResetFireCooldown()
{
    fireTimer = fireCooldown;
}

EPlayerType CPlayer::GetPlayerType() const
{
    return EPlayerType::None;
}

void CPlayer::ApplyDamage(int amount)
{
    health -= amount;
    if (health < 0)
    {
        health = 0;
    }

    if (IsDead())
    {
        SetActive(false);
    }
}

int CPlayer::GetHealth() const
{
    return health;
}

XMFLOAT3 CPlayer::GetForward() const
{
    return forward;
}

XMFLOAT3 CPlayer::GetRight() const
{
    return right;
}

void CPlayer::UpdateDirectionVectors()
{
    XMFLOAT4X4 rotationMatrix = Matrix4x4::RotationRollPitchYaw(0.0f, rotation.y, 0.0f);
    XMMATRIX xmRotation = XMLoadFloat4x4(&rotationMatrix);

    forward = Vector3::TransformNormal(XMFLOAT3(0.0f, 0.0f, 1.0f), xmRotation);
    right = Vector3::TransformNormal(XMFLOAT3(1.0f, 0.0f, 0.0f), xmRotation);

    forward = Vector3::Normalize(forward);
    right = Vector3::Normalize(right);
}

void CPlayer::MoveByDirection(const XMFLOAT3& moveDirection, float deltaTime)
{
    if (Vector3::Length(moveDirection) <= 0.0f)
    {
        return;
    }

    XMFLOAT3 direction = Vector3::Normalize(moveDirection);

    XMFLOAT3 deltaMove = Vector3::Multiply(direction, moveSpeed * deltaTime);

    position = Vector3::Add(position, deltaMove);

    rotation.y = XMConvertToDegrees(std::atan2(direction.x, direction.z));
}

bool CPlayer::IsDead() const
{
    return health <= 0;
}

XMFLOAT3 CPlayer::GetMuzzlePosition() const
{
    XMFLOAT3 muzzlePosition = Vector3::Add(position, XMFLOAT3(0.0f, muzzleHeight, 0.0f));

    return Vector3::Add(muzzlePosition, Vector3::Multiply(forward, muzzleForwardDistance));
}

void CPlayer::SetMuzzleOffset(float height, float forwardDistance)
{
    muzzleHeight = height;
    muzzleForwardDistance = forwardDistance;
}

void CPlayer::ToggleShield()
{
    if (shieldEnabled)
    {
        StartShieldCooldown();
        return;
    }

    if (shieldCooldownTimer > 0.0f)
    {
        return;
    }

    shieldEnabled = true;
    shieldTimer = shieldDuration;
}

bool CPlayer::IsShieldEnabled() const
{
    return shieldEnabled;
}

void CPlayer::UpdateShieldTimer(float deltaTime)
{
    if (shieldEnabled)
    {
        shieldTimer -= deltaTime;

        if (shieldTimer <= 0.0f)
        {
            StartShieldCooldown();
        }

        return;
    }

    if (shieldCooldownTimer > 0.0f)
    {
        shieldCooldownTimer -= deltaTime;

        if (shieldCooldownTimer < 0.0f)
        {
            shieldCooldownTimer = 0.0f;
        }
    }
}

void CPlayer::StartShieldCooldown()
{
    shieldEnabled = false;
    shieldTimer = 0.0f;
    shieldCooldownTimer = shieldCooldown;
}
