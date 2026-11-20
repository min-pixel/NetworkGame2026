// 충돌 박스 (DirectXCollision 래퍼)

#pragma once

#include "stdafx.h"

enum class ColliderType
{
    None,
    OrientedBox,
    AABB
};

// 충돌 박스 공통 인터페이스
// 로컬 박스를 들고 있다가 Update(월드 행렬) 때 월드 박스로 변환한다.
class CCollider
{
public:
    virtual ~CCollider() = default;

    ColliderType GetType() const;
    virtual void Update(const XMFLOAT4X4& worldMatrix) = 0;
    virtual bool Intersects(const CCollider* other) const = 0;
    virtual bool Intersects(const BoundingFrustum& frustum) const = 0;

protected:
    ColliderType type = ColliderType::None;
};

// 회전하는 박스 (OBB). 캐릭터/기체/부위에 사용
class COrientedBoxCollider : public CCollider
{
public:
    COrientedBoxCollider(const XMFLOAT3& center, const XMFLOAT3& extents);

    void Update(const XMFLOAT4X4& worldMatrix) override;
    bool Intersects(const CCollider* other) const override;
    bool Intersects(const BoundingFrustum& frustum) const override;

    const BoundingOrientedBox& GetLocalShape() const;
    const BoundingOrientedBox& GetWorldShape() const;

private:
    BoundingOrientedBox localShape;
    BoundingOrientedBox worldShape;
};

// 축 정렬 박스 (AABB). 총알에 사용
class CAABBCollider : public CCollider
{
public:
    CAABBCollider(const XMFLOAT3& center, const XMFLOAT3& extents);

    void Update(const XMFLOAT4X4& worldMatrix) override;
    bool Intersects(const CCollider* other) const override;
    bool Intersects(const BoundingFrustum& frustum) const override;

    const BoundingBox& GetLocalShape() const;
    const BoundingBox& GetWorldShape() const;

private:
    BoundingBox localShape;
    BoundingBox worldShape;
};
