// 게임 오브젝트 기본 클래스와 적(Enemy) 계열 클래스

#pragma once

#include "stdafx.h"

class CCollider;
class COrientedBoxCollider;

class CDirectMesh;

// Humanoid 부위 (부위별로 피해량이 다름)
enum class BodyPartType
{
    Torso,
    Head,
    LeftArm,
    RightArm,
    LeftLeg,
    RightLeg
};

enum class EEnemyType
{
    Humanoid,
    Tank
};

// 모든 오브젝트의 기본 클래스
// 위치/회전(도 단위)/크기, 색, 부모-자식 계층, 충돌 박스, 메시를 가진다.
// 자식 오브젝트는 부모가 delete 하며, 메시는 참조 카운트로 공유한다.
class CGameObject
{
public:
    CGameObject();
    virtual ~CGameObject();

    void SetCollider(CCollider* newCollider);

    CCollider* GetCollider() const;

    void SetParent(CGameObject* newParent);
    CGameObject* GetParent() const;

    void AddChild(CGameObject* child);
    const std::vector<CGameObject*>& GetChildren() const;

    void SetPosition(float x, float y, float z);
    void SetRotation(float pitch, float yaw, float roll);
    void SetScale(float x, float y, float z);

    const XMFLOAT3& GetPosition() const;
    const XMFLOAT3& GetRotation() const;
    const XMFLOAT3& GetScale() const;

    const XMFLOAT4X4& GetLocalMatrix() const;
    const XMFLOAT4X4& GetWorldMatrix() const;

    void SetColor(const XMFLOAT4& newColor);
    const XMFLOAT4& GetColor() const;

    void SetActive(bool active);
    bool IsActive() const;

    // Animate -> 행렬 갱신 -> 충돌 박스 갱신 -> 자식 Update 순서 (비활성이면 아무것도 안 함)
    virtual void Update(float deltaTime);

    // 파생 클래스의 움직임 연출용 (기본은 빈 함수)
    virtual void Animate(float deltaTime);
    virtual void UpdateTransform();
    virtual void UpdateCollider();

    void SetDirectMesh(CDirectMesh* newDirectMesh);
    CDirectMesh* GetDirectMesh() const;

    // 월드 행렬(b0)과 색(b2)을 올리고 메시를 그린 뒤 자식도 그린다.
    virtual void RenderDirect(ID3D12GraphicsCommandList* commandList);

protected:
    XMFLOAT3 position;
    XMFLOAT3 rotation;
    XMFLOAT3 scale;

    XMFLOAT4 color;

    XMFLOAT4X4 localMatrix;
    XMFLOAT4X4 worldMatrix;

    CGameObject* parent;
    std::vector<CGameObject*> children;

    CCollider* collider;

    CDirectMesh* directMesh = nullptr;

    bool isActive;
};

// 적 공통 클래스 (Humanoid / Tank)
// 이동 속도, 사거리, 거리 유지 기준, 발사 쿨타임, 체력을 가진다. 체력이 0이 되면 비활성화된다.
class CEnemy : public CGameObject
{
public:
    CEnemy(EEnemyType enemyType, float moveSpeed, float attackRange, float moveStopDistance, float tooCloseDistance, float fireCooldown, int health);

    ~CEnemy() override = default;

    void Update(float deltaTime) override;

    EEnemyType GetEnemyType() const;

    float GetMoveSpeed() const;
    float GetAttackRange() const;

    // 이 거리보다 멀면 접근, TooCloseDistance보다 가까우면 후퇴
    float GetMoveStopDistance() const;
    float GetTooCloseDistance() const;

    bool CanFire() const;
    void ResetFireCooldown();

    void ApplyDamage(int amount);
    bool IsDead() const;
    int GetHealth() const;

    // 적 전체 색 변경 (파생 클래스가 부위별 색까지 처리)
    virtual void SetEnemyColor( const XMFLOAT4& color );

private:
    EEnemyType enemyType;

    float moveSpeed;
    float attackRange;
    float moveStopDistance;
    float tooCloseDistance;

    float fireCooldown;
    float fireTimer = 0.0f;

    int health;
};

// Humanoid의 부위 하나 (부위별 충돌 박스로 피격 부위를 판정)
class CBodyPartObject : public CGameObject
{
public:
    explicit CBodyPartObject(BodyPartType type);

    BodyPartType GetPartType() const;

private:
    BodyPartType partType;
};

// 몸통 아래에 머리/팔/다리가 붙은 인간형 적. 걷는 동안 팔다리를 흔든다.
class CHumanoidObject : public CEnemy
{
public:
    CHumanoidObject(float moveSpeed, float attackRange, float moveStopDistance, float tooCloseDistance, float fireCooldown, int health);
    virtual ~CHumanoidObject();

    // 부위 오브젝트와 충돌 박스 생성
    virtual void BuildHumanoid();
    void Update(float deltaTime) override;
    void Animate(float deltaTime) override;

    CBodyPartObject* GetTorso() const;
    CBodyPartObject* GetHead() const;
    CBodyPartObject* GetLeftArm() const;
    CBodyPartObject* GetRightArm() const;
    CBodyPartObject* GetLeftLeg() const;
    CBodyPartObject* GetRightLeg() const;

    CBodyPartObject* FindBodyPart(BodyPartType type) const;

protected:
    CBodyPartObject* torso;
    CBodyPartObject* head;
    CBodyPartObject* leftArm;
    CBodyPartObject* rightArm;
    CBodyPartObject* leftLeg;
    CBodyPartObject* rightLeg;

    float walkTime;
};

// 작은 큐브 블록을 찍어서 글자를 만드는 UI 오브젝트 (시작/맵 선택/결과 화면)
class CBlockTextObject : public CGameObject
{
public:
    CBlockTextObject();
    ~CBlockTextObject() override = default;

    void SetBlockMesh(CDirectMesh* mesh);
    void SetBlockSize(float size);
    void SetBlockSpacing(float spacing);
    void SetBlockColor(const XMFLOAT4& color);

    // 블록 칸 좌표 (x, y, z)에 블록 하나 추가 (실제 위치 = 칸 좌표 x blockSpacing)
    void AddBlock(float x, float y, float z);

private:
    CDirectMesh* blockMesh = nullptr;
    float blockSize = 1.0f;
    float blockSpacing = 1.2f;
    XMFLOAT4 blockColor = XMFLOAT4(1.0f, 1.0f, 1.0f, 1.0f);
};
