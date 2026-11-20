// 충돌 박스 구현

#include "stdafx.h"
#include "Collider.h"

ColliderType CCollider::GetType() const
{
    return type;
}

COrientedBoxCollider::COrientedBoxCollider(const XMFLOAT3& center, const XMFLOAT3& extents)
    : localShape(center, extents, XMFLOAT4(0.0f, 0.0f, 0.0f, 1.0f)),
    worldShape(center, extents, XMFLOAT4(0.0f, 0.0f, 0.0f, 1.0f))
{
    type = ColliderType::OrientedBox;
}

void COrientedBoxCollider::Update(const XMFLOAT4X4& worldMatrix)
{
    localShape.Transform(worldShape, XMLoadFloat4x4(&worldMatrix));
}

bool COrientedBoxCollider::Intersects(const CCollider* other) const
{
    if (other == nullptr)
    {
        return false;
    }

    switch (other->GetType())
    {
    case ColliderType::OrientedBox:
    {
        const auto* box = static_cast<const COrientedBoxCollider*>(other);
        return worldShape.Intersects(box->GetWorldShape());
    }

    default:
        return false;
    }
}

bool COrientedBoxCollider::Intersects(const BoundingFrustum& frustum) const
{
    return worldShape.Intersects(frustum);
}

const BoundingOrientedBox& COrientedBoxCollider::GetLocalShape() const
{
    return localShape;
}

const BoundingOrientedBox& COrientedBoxCollider::GetWorldShape() const
{
    return worldShape;
}

CAABBCollider::CAABBCollider(const XMFLOAT3& center, const XMFLOAT3& extents)
    : localShape(center, extents),
    worldShape(center, extents)
{
    type = ColliderType::AABB;
}

void CAABBCollider::Update(const XMFLOAT4X4& worldMatrix)
{
    localShape.Transform(worldShape, XMLoadFloat4x4(&worldMatrix));
}

bool CAABBCollider::Intersects(const CCollider* other) const
{
    if (other == nullptr)
    {
        return false;
    }

    switch (other->GetType())
    {
    case ColliderType::OrientedBox:
    {
        const auto* box = static_cast<const COrientedBoxCollider*>(other);
        return worldShape.Intersects(box->GetWorldShape());
    }

    case ColliderType::AABB:
    {
        const auto* box = static_cast<const CAABBCollider*>(other);
        return worldShape.Intersects(box->GetWorldShape());
    }

    default:
        return false;
    }
}

bool CAABBCollider::Intersects(const BoundingFrustum& frustum) const
{
    return worldShape.Intersects(frustum);
}

const BoundingBox& CAABBCollider::GetLocalShape() const
{
    return localShape;
}

const BoundingBox& CAABBCollider::GetWorldShape() const
{
    return worldShape;
}
