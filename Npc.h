// 인간형 적 (Map1)

#pragma once

#include "Object.h"

// 능력치가 정해진 인간형 적. 느리게 움직이고 체력 50
class CNpc : public CHumanoidObject
{
public:
    CNpc();
    ~CNpc() override = default;

    void SetEnemyColor(const XMFLOAT4& color) override;
};
