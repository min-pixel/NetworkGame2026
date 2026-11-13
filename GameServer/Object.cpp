// 게임 오브젝트 기본 클래스와 적 계열 클래스 구현

#include "stdafx.h"
#include "Object.h"
#include "Mesh.h"
#include "Collider.h"

CGameObject::CGameObject()
    : position(0.0f, 0.0f, 0.0f),
    rotation(0.0f, 0.0f, 0.0f),
    scale(1.0f, 1.0f, 1.0f),
    color(1.0f, 1.0f, 1.0f, 1.0f),
    localMatrix(Matrix4x4::Identity()),
    worldMatrix(Matrix4x4::Identity()),
    parent(nullptr),
    directMesh(nullptr),
    collider(nullptr),
    isActive(true)
{
}

CGameObject::~CGameObject()
{
    for (CGameObject* child : children)
    {
        delete child;
    }
    children.clear();

    if (collider)
    {
        delete collider;
        collider = nullptr;
    }

    if (directMesh)
    {
        directMesh->Release();
        directMesh = nullptr;
    }
}

void CGameObject::SetCollider(CCollider* newCollider)
{
    if (collider)
    {
        delete collider;
    }
    collider = newCollider;
}

CCollider* CGameObject::GetCollider() const
{
    return collider;
}

void CGameObject::SetParent(CGameObject* newParent)
{
    parent = newParent;
}

CGameObject* CGameObject::GetParent() const
{
    return parent;
}

void CGameObject::AddChild(CGameObject* child)
{
    if (child == nullptr)
    {
        return;
    }

    child->SetParent(this);
    children.push_back(child);
}

const std::vector<CGameObject*>& CGameObject::GetChildren() const
{
    return children;
}

void CGameObject::SetPosition(float x, float y, float z)
{
    position = XMFLOAT3(x, y, z);
}

void CGameObject::SetRotation(float pitch, float yaw, float roll)
{
    rotation = XMFLOAT3(pitch, yaw, roll);
}

void CGameObject::SetScale(float x, float y, float z)
{
    scale = XMFLOAT3(x, y, z);
}

const XMFLOAT3& CGameObject::GetPosition() const
{
    return position;
}

const XMFLOAT3& CGameObject::GetRotation() const
{
    return rotation;
}

const XMFLOAT3& CGameObject::GetScale() const
{
    return scale;
}

const XMFLOAT4X4& CGameObject::GetLocalMatrix() const
{
    return localMatrix;
}

const XMFLOAT4X4& CGameObject::GetWorldMatrix() const
{
    return worldMatrix;
}

void CGameObject::SetColor(const XMFLOAT4& newColor)
{
    color = newColor;
}

const XMFLOAT4& CGameObject::GetColor() const
{
    return color;
}

void CGameObject::SetActive(bool active)
{
    isActive = active;
}

bool CGameObject::IsActive() const
{
    return isActive;
}

void CGameObject::Update(float deltaTime)
{
    if (!isActive)
    {
        return;
    }

    Animate(deltaTime);
    UpdateTransform();
    UpdateCollider();

    for (CGameObject* child : children)
    {
        if (child)
        {
            child->Update(deltaTime);
        }
    }
}

void CGameObject::Animate(float deltaTime)
{
    UNREFERENCED_PARAMETER(deltaTime);
}

void CGameObject::UpdateTransform()
{
    XMFLOAT4X4 scalingMatrix = Matrix4x4::Scaling(scale.x, scale.y, scale.z);
    XMFLOAT4X4 rotationMatrix = Matrix4x4::RotationRollPitchYaw(rotation.x, rotation.y, rotation.z);
    XMFLOAT4X4 translationMatrix = Matrix4x4::Translation(position.x, position.y, position.z);

    localMatrix = Matrix4x4::Multiply(Matrix4x4::Multiply(scalingMatrix, rotationMatrix), translationMatrix);

    if (parent)
    {
        worldMatrix = Matrix4x4::Multiply(localMatrix, parent->GetWorldMatrix());
    }
    else
    {
        worldMatrix = localMatrix;
    }
}

void CGameObject::UpdateCollider()
{
    if (collider)
    {
        collider->Update(worldMatrix);
    }
}

CBodyPartObject::CBodyPartObject(BodyPartType type)
    : partType(type)
{
}

BodyPartType CBodyPartObject::GetPartType() const
{
    return partType;
}

CHumanoidObject::CHumanoidObject(float moveSpeed, float attackRange, float moveStopDistance, float tooCloseDistance, float fireCooldown, int health)
    : CEnemy(EEnemyType::Humanoid, moveSpeed, attackRange, moveStopDistance, tooCloseDistance, fireCooldown, health),
    torso(nullptr),
    head(nullptr),
    leftArm(nullptr),
    rightArm(nullptr),
    leftLeg(nullptr),
    rightLeg(nullptr),
    walkTime(0.0f)
{
}

CHumanoidObject::~CHumanoidObject()
{
}

void CHumanoidObject::BuildHumanoid()
{
    SetCollider(new COrientedBoxCollider(XMFLOAT3(0.0f, 1.0f, 0.0f), XMFLOAT3(1.25f, 2.0f, 1.25f)));

    torso = new CBodyPartObject(BodyPartType::Torso);
    head = new CBodyPartObject(BodyPartType::Head);
    leftArm = new CBodyPartObject(BodyPartType::LeftArm);
    rightArm = new CBodyPartObject(BodyPartType::RightArm);
    leftLeg = new CBodyPartObject(BodyPartType::LeftLeg);
    rightLeg = new CBodyPartObject(BodyPartType::RightLeg);

    AddChild(torso);

    torso->AddChild(head);
    torso->AddChild(leftArm);
    torso->AddChild(rightArm);
    torso->AddChild(leftLeg);
    torso->AddChild(rightLeg);

    torso->SetPosition(0.0f, 1.0f, 0.0f);
    torso->SetScale(1.5f, 1.5f, 1.5f);

    torso->SetCollider(new COrientedBoxCollider(XMFLOAT3(0.0f, 0.0f, 0.0f), XMFLOAT3(0.5f, 0.5f, 0.5f)));

    head->SetPosition(0.0f, 0.85f, 0.0f);
    head->SetScale(1.0f, 1.0f, 1.0f);
    head->SetCollider(new COrientedBoxCollider(XMFLOAT3(0.0f, -0.25f, 0.0f), XMFLOAT3(0.35f, 0.35f, 0.35f)));

    leftArm->SetPosition(-0.7f, 0.0f, 0.0f);
    leftArm->SetScale(1.0f, 1.0f, 1.0f);
    leftArm->SetCollider(new COrientedBoxCollider(XMFLOAT3(0.0f, -0.6f, 0.0f), XMFLOAT3(0.2f, 0.5f, 0.2f)));

    rightArm->SetPosition(0.7f, 0.0f, 0.0f);
    rightArm->SetScale(1.0f, 1.0f, 1.0f);
    rightArm->SetCollider(new COrientedBoxCollider(XMFLOAT3(0.0f, -0.6f, 0.0f), XMFLOAT3(0.2f, 0.5f, 0.2f)));

    leftLeg->SetPosition(-0.35f, -0.9f, 0.0f);
    leftLeg->SetScale(1.0f, 1.0f, 1.0f);

    leftLeg->SetCollider(new COrientedBoxCollider(XMFLOAT3(0.0f, -0.25f, 0.0f), XMFLOAT3(0.2f, 0.5f, 0.2f)));

    rightLeg->SetPosition(0.35f, -0.9f, 0.0f);
    rightLeg->SetScale(1.0f, 1.0f, 1.0f);
    rightLeg->SetCollider(new COrientedBoxCollider(XMFLOAT3(0.0f, -0.25f, 0.0f), XMFLOAT3(0.2f, 0.5f, 0.2f)));
}

void CHumanoidObject::Update(float deltaTime)
{
    CEnemy::Update(deltaTime);
}

void CHumanoidObject::Animate(float deltaTime)
{
    walkTime += deltaTime;

    float armSwing = std::sin(walkTime * 6.0f) * 30.0f;
    float legSwing = std::sin(walkTime * 6.0f) * 30.0f;

    if (leftArm)
    {
        leftArm->SetRotation(armSwing, 0.0f, 0.0f);
    }

    if (rightArm)
    {
        rightArm->SetRotation(-armSwing, 0.0f, 0.0f);
    }

    if (leftLeg)
    {
        leftLeg->SetRotation(-legSwing, 0.0f, 0.0f);
    }

    if (rightLeg)
    {
        rightLeg->SetRotation(legSwing, 0.0f, 0.0f);
    }
}

CBodyPartObject* CHumanoidObject::GetTorso() const
{
    return torso;
}

CBodyPartObject* CHumanoidObject::GetHead() const
{
    return head;
}

CBodyPartObject* CHumanoidObject::GetLeftArm() const
{
    return leftArm;
}

CBodyPartObject* CHumanoidObject::GetRightArm() const
{
    return rightArm;
}

CBodyPartObject* CHumanoidObject::GetLeftLeg() const
{
    return leftLeg;
}

CBodyPartObject* CHumanoidObject::GetRightLeg() const
{
    return rightLeg;
}

CBodyPartObject* CHumanoidObject::FindBodyPart(BodyPartType type) const
{
    switch (type)
    {
    case BodyPartType::Torso:
        return torso;
    case BodyPartType::Head:
        return head;
    case BodyPartType::LeftArm:
        return leftArm;
    case BodyPartType::RightArm:
        return rightArm;
    case BodyPartType::LeftLeg:
        return leftLeg;
    case BodyPartType::RightLeg:
        return rightLeg;
    default:
        return nullptr;
    }
}

void CGameObject::SetDirectMesh(CDirectMesh* newDirectMesh)
{
    if (directMesh == newDirectMesh)
    {
        return;
    }

    if (directMesh)
    {
        directMesh->Release();
    }

    directMesh = newDirectMesh;

    if (directMesh)
    {
        directMesh->AddRef();
    }
}

CDirectMesh* CGameObject::GetDirectMesh() const
{
    return directMesh;
}

void CGameObject::RenderDirect(ID3D12GraphicsCommandList* commandList)
{
    if (!isActive || !commandList)
    {
        return;
    }

    XMFLOAT4X4 transposedWorldMatrix;
    XMStoreFloat4x4(&transposedWorldMatrix, XMMatrixTranspose(XMLoadFloat4x4(&worldMatrix)));

    commandList->SetGraphicsRoot32BitConstants(0, 16, &transposedWorldMatrix, 0);

    commandList->SetGraphicsRoot32BitConstants(2, 4, &color, 0);

    if (directMesh)
    {
        directMesh->Render(commandList);
    }

    for (CGameObject* child : children)
    {
        if (child)
        {
            child->RenderDirect(commandList);
        }
    }
}

CBlockTextObject::CBlockTextObject()
{
}

void CBlockTextObject::SetBlockMesh(CDirectMesh* mesh)
{
    blockMesh = mesh;
}

void CBlockTextObject::SetBlockSize(float size)
{
    blockSize = size;
}

void CBlockTextObject::SetBlockSpacing(float spacing)
{
    blockSpacing = spacing;
}

void CBlockTextObject::SetBlockColor(const XMFLOAT4& color)
{
    blockColor = color;
}

void CBlockTextObject::AddBlock(float x, float y, float z)
{
    CGameObject* block = new CGameObject();

    block->SetPosition(x * blockSpacing, y * blockSpacing, z * blockSpacing);

    block->SetScale(blockSize, blockSize, blockSize);
    block->SetColor(blockColor);
    block->SetDirectMesh(blockMesh);

    AddChild(block);
}

CEnemy::CEnemy(EEnemyType enemyType, float moveSpeed, float attackRange, float moveStopDistance, float tooCloseDistance, float fireCooldown, int health)
    : enemyType(enemyType),
    moveSpeed(moveSpeed),
    attackRange(attackRange),
    moveStopDistance(moveStopDistance),
    tooCloseDistance(tooCloseDistance),
    fireCooldown(fireCooldown),
    health(health)
{
}

void CEnemy::Update(float deltaTime)
{
    if (fireTimer > 0.0f)
    {
        fireTimer -= deltaTime;

        if (fireTimer < 0.0f)
        {
            fireTimer = 0.0f;
        }
    }

    CGameObject::Update(deltaTime);
}

bool CEnemy::CanFire() const
{
    return fireTimer <= 0.0f;
}

void CEnemy::ResetFireCooldown()
{
    fireTimer = fireCooldown;
}

void CEnemy::ApplyDamage(int amount)
{
    health -= amount;

    if (health < 0)
    {
        health = 0;
    }

    if (health == 0)
    {
        SetActive(false);
    }
}

bool CEnemy::IsDead() const
{
    return health <= 0;
}

int CEnemy::GetHealth() const
{
    return health;
}

EEnemyType CEnemy::GetEnemyType() const
{
    return enemyType;
}

float CEnemy::GetMoveSpeed() const
{
    return moveSpeed;
}

float CEnemy::GetAttackRange() const
{
    return attackRange;
}

float CEnemy::GetMoveStopDistance() const
{
    return moveStopDistance;
}

float CEnemy::GetTooCloseDistance() const
{
    return tooCloseDistance;
}

void CEnemy::SetEnemyColor(const XMFLOAT4& color)
{
    SetColor(color);
}
