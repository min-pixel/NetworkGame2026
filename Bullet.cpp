// 총알 구현

#include "stdafx.h"
#include "Bullet.h"
#include "Collider.h"

CBullet::CBullet()
    : previousPosition(0.0f, 0.0f, 0.0f),
    direction(0.0f, 0.0f, 1.0f),
    speed(180.0f),
    elapsedTime(0.0f),
    maxDistance(350.0f),
    damage(10),
    owner(EBulletOwner::Player)
{
    SetActive(false);
    SetColor(XMFLOAT4(1.0f, 1.0f, 0.2f, 1.0f));
    SetScale(1.0f, 1.0f, 1.0f);
    SetCollider(new CAABBCollider(XMFLOAT3(0.0f, 0.0f, 0.0f), XMFLOAT3(0.2f, 0.2f, 0.2f)));
}

CBullet::~CBullet()
{
}

void CBullet::Fire(const XMFLOAT3& startPosition, const XMFLOAT3& fireDirection, EBulletOwner newOwner)
{
    previousPosition = startPosition;
    position = startPosition;
    direction = Vector3::Normalize(fireDirection);
    owner = newOwner;

    elapsedTime = 0.0f;
    SetActive(true);
}

void CBullet::Update(float deltaTime)
{
    if (!IsActive())
    {
        return;
    }

    previousPosition = position;
    elapsedTime += deltaTime;

    XMFLOAT3 deltaMove = Vector3::Multiply(direction, speed * deltaTime);
    position = Vector3::Add(position, deltaMove);

    CGameObject::Update(deltaTime);

    if (elapsedTime * speed >= maxDistance)
    {
        SetActive(false);
    }
}

int CBullet::GetDamage() const
{
    return damage;
}

EBulletOwner CBullet::GetOwner() const
{
    return owner;
}

const XMFLOAT3& CBullet::GetPreviousPosition() const
{
    return previousPosition;
}
