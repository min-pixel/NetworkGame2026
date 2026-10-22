// 게임 플레이 씬 구현: 입력, 카메라, 렌더링, 폭발 연출, 씬 전환

#include "stdafx.h"
#include "GameScene.h"

#include "Camera.h"
#include "Mesh.h"
#include "Player.h"
#include "Npc.h"
#include "Collider.h"
#include "Bullet.h"

#include <algorithm>

#include "GameMap.h"

#include "HelicopterPlayer.h"

#include "TerrainGameMap.h"

#include "Tank.h"

namespace
{
    float ClampColor(float value)
    {
        if (value < 0.0f)
        {
            return 0.0f;
        }

        if (value > 1.0f)
        {
            return 1.0f;
        }

        return value;
    }

    XMFLOAT4 AddColor(const XMFLOAT4& color, float r, float g, float b)
    {
        return XMFLOAT4(ClampColor(color.x + r), ClampColor(color.y + g), ClampColor(color.z + b), color.w);
    }

    XMFLOAT4 MultiplyColor(const XMFLOAT4& color, float factor)
    {
        return XMFLOAT4(ClampColor(color.x * factor), ClampColor(color.y * factor), ClampColor(color.z * factor), color.w);
    }
}

CGameScene::CGameScene()
    : CGameScene(GameSessionConfig{})
{
}

CGameScene::CGameScene(const GameSessionConfig& config)
    : clientWidth(0),
    clientHeight(0),
    isLButtonDown(false),

    camera(nullptr),
    localPlayer(nullptr),

    isMouseCaptured(false),
    ignoreMouseMove(false),

    thirdPersonCameraYaw(0.0f),
    thirdPersonCameraPitch(20.0f)
{
    sessionConfig = config;

    mouseCenter.x = 0;
    mouseCenter.y = 0;

    ZeroMemory(inputBuffer, sizeof(inputBuffer));
}

CGameScene::~CGameScene()
{
    ReleaseObjects();
}

void CGameScene::CreateLocalPlayer()
{
    if (gameWorld.GetLocalPlayer() || !gameMap)
    {
        return;
    }

    CPlayer* createdPlayer = gameWorld.SpawnPlayer(sessionConfig.playerType, gameMap->GetPlayerStartPosition(), gameMap);

    if (!createdPlayer)
    {
        return;
    }

    gameWorld.SetLocalPlayer(createdPlayer);

    localPlayer = createdPlayer;

    switch (createdPlayer->GetPlayerType())
    {
    case EPlayerType::Helicopter:
        helicopterPlayer = static_cast<CHelicopterPlayer*>(createdPlayer);
        break;

    case EPlayerType::Tank:
        playerTank = static_cast<CTankPlayer*>(createdPlayer);

        playerTank->SetTankColor(XMFLOAT4(0.20f, 0.65f, 0.30f, 1.0f));
        break;

    default:
        break;
    }
}

void CGameScene::BuildObjects(int width, int height)
{
    clientWidth = width;
    clientHeight = (height <= 0) ? 1 : height;

    mouseCenter = { clientWidth / 2, clientHeight / 2 };

    isLButtonDown = false;

    isMouseCaptured = false;
    ignoreMouseMove = false;

    mouseSensitivity = 0.15f;

    if (!camera)
    {
        camera = new CCamera();
        camera->SetViewport(clientWidth, clientHeight);
        camera->SetPosition(XMFLOAT3(0.0f, 5.0f, -15.0f));
        camera->SetTarget(XMFLOAT3(0.0f, 2.0f, 0.0f));
        camera->SetUp(XMFLOAT3(0.0f, 1.0f, 0.0f));
        camera->SetLens(60.0f, 0.1f, 1000.0f);
    }

    if (!gameMap)
    {
        gameMap = new CTerrainGameMap(sessionConfig.mapNumber);

        if (!gameMap->LoadMapData())
        {
            OutputDebugStringA("[ERROR] LoadMapData failed\n");

            return;
        }

        gameMap->LoadSpawnPositions();
    }

    CreateLocalPlayer();

    gameWorld.SpawnEnemies(gameMap);

    const int bulletCount = 64;

    gameWorld.BuildBulletPool(bulletCount);
}

void CGameScene::ReleaseObjects()
{
    EndMouseCapture(nullptr);

    if (gameMap)
    {
        delete gameMap;
        gameMap = nullptr;
    }

    gameWorld.Clear();

    localPlayer = nullptr;

    helicopterPlayer = nullptr;
    playerTank = nullptr;

    if (camera)
    {
        delete camera;
        camera = nullptr;
    }

    ReleaseEnemyExplosions();

    ReleaseSharedDirectMeshes();
}

bool CGameScene::ProcessInput(UCHAR* keyBuffer)
{
    if (keyBuffer)
    {
        memcpy(inputBuffer, keyBuffer, sizeof(inputBuffer));
    }

    UpdatePlayerInput();

    return true;
}

void CGameScene::UpdatePlayerInput()
{
    currentPlayerInput = PlayerInput{};

    if (inputBuffer['W'] & 0x80)
    {
        currentPlayerInput.moveForward += 1.0f;
    }

    if (inputBuffer['S'] & 0x80)
    {
        currentPlayerInput.moveForward -= 1.0f;
    }

    if (inputBuffer['D'] & 0x80)
    {
        currentPlayerInput.moveRight += 1.0f;
    }

    if (inputBuffer['A'] & 0x80)
    {
        currentPlayerInput.moveRight -= 1.0f;
    }

    currentPlayerInput.lookYawDelta = pendingLookYawDelta;

    currentPlayerInput.lookPitchDelta = pendingLookPitchDelta;

    currentPlayerInput.fire = isLButtonDown;

    currentPlayerInput.shieldTogglePressed = pendingShieldTogglePressed;

    pendingLookYawDelta = 0.0f;
    pendingLookPitchDelta = 0.0f;

    pendingShieldTogglePressed = false;
}

void CGameScene::AnimateObjects(float deltaTime)
{
    UpdateLocalPlayerFrame(deltaTime);

    gameWorld.UpdateEnemies(deltaTime, gameMap);

    gameWorld.UpdateBullets(deltaTime, gameMap);

    std::vector<CEnemy*> deadEnemies;

    gameWorld.CheckBulletHits(deadEnemies);

    for (CEnemy* enemy : deadEnemies)
    {
        if (!enemy)
        {
            continue;
        }

        StartEnemyExplosion(enemy->GetPosition());
    }

    UpdateEnemyExplosions(deltaTime);

    gameWorld.UpdateGameResult(sessionConfig.gameMode);

    EGameResult gameResult = gameWorld.GetGameResult();

    if (gameResult ==
        EGameResult::Victory)
    {
        requestedScene = ESceneType::End;

        return;
    }

    if (gameResult ==
        EGameResult::Defeat)
    {
        requestedScene = ESceneType::Defeat;

        return;
    }
}

void CGameScene::OnResize(int width, int height)
{
    clientWidth = width;
    clientHeight = (height <= 0) ? 1 : height;

    UpdateMouseCenter();

    if (camera)
    {
        camera->SetViewport(clientWidth, clientHeight);
    }
}

bool CGameScene::OnProcessingKeyboardMessage(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam)
{
    UNREFERENCED_PARAMETER(hWnd);
    UNREFERENCED_PARAMETER(message);
    UNREFERENCED_PARAMETER(wParam);
    UNREFERENCED_PARAMETER(lParam);

    if (message == WM_KEYDOWN)
    {
        bool wasKeyDown = (lParam & (1 << 30)) != 0;

        if ((wParam == VK_CONTROL || wParam == VK_LCONTROL || wParam == VK_RCONTROL) && !wasKeyDown)
        {
            EndMouseCapture(hWnd);
            return true;
        }

        if (wParam == VK_ESCAPE && !wasKeyDown)
        {
            EndMouseCapture(hWnd);
            requestedScene = ESceneType::MapSelect;
            return true;
        }

        if (wParam == 'E' && !wasKeyDown)
        {
            pendingShieldTogglePressed = true;
            return true;
        }
    }

    return false;
}

bool CGameScene::OnProcessingMouseMessage(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam)
{
    UNREFERENCED_PARAMETER(wParam);

    POINT currentMousePosition;
    currentMousePosition.x = LOWORD(lParam);
    currentMousePosition.y = HIWORD(lParam);

    switch (message)
    {
    case WM_MOUSEMOVE:
    {
        if (!isMouseCaptured)
        {
            break;
        }

        if (ignoreMouseMove)
        {
            ignoreMouseMove = false;
            break;
        }

        int dx = currentMousePosition.x - mouseCenter.x;
        int dy = currentMousePosition.y - mouseCenter.y;

        if (dx != 0 || dy != 0)
        {
            pendingLookYawDelta +=
                static_cast<float>(dx) * mouseSensitivity;

            pendingLookPitchDelta +=
                static_cast<float>(dy) * mouseSensitivity;
        }

        RecenterMouse(hWnd);
        break;
    }

    case WM_LBUTTONDOWN:

        if (!isMouseCaptured)
        {
            BeginMouseCapture(hWnd);
        }
        isLButtonDown = true;
        break;

    case WM_LBUTTONUP:
        isLButtonDown = false;

        break;

    case WM_RBUTTONDOWN:

        if (!isMouseCaptured)
        {
            BeginMouseCapture(hWnd);
        }

        break;

    case WM_RBUTTONUP:

        break;

    default:
        break;
    }

    return false;
}

bool CGameScene::IsVisible(const CGameObject* object, const BoundingFrustum& frustum) const
{
    if (!object || !object->IsActive())
    {
        return false;
    }

    CCollider* collider = object->GetCollider();
    if (!collider)
    {
        return true;
    }

    return collider->Intersects(frustum);
}

void CGameScene::UpdateMouseCenter()
{
    mouseCenter.x = clientWidth / 2;
    mouseCenter.y = clientHeight / 2;
}

void CGameScene::RecenterMouse(HWND hWnd)
{
    POINT centerScreen = mouseCenter;
    ClientToScreen(hWnd, &centerScreen);

    ignoreMouseMove = true;
    SetCursorPos(centerScreen.x, centerScreen.y);
}

void CGameScene::BeginMouseCapture(HWND hWnd)
{
    if (isMouseCaptured)
    {
        return;
    }

    isMouseCaptured = true;

    RECT rc;
    GetClientRect(hWnd, &rc);

    POINT leftTop = { rc.left, rc.top };
    POINT rightBottom = { rc.right, rc.bottom };

    ClientToScreen(hWnd, &leftTop);
    ClientToScreen(hWnd, &rightBottom);

    RECT clipRect = { leftTop.x, leftTop.y, rightBottom.x, rightBottom.y };

    ClipCursor(&clipRect);
    SetCapture(hWnd);
    ShowCursor(FALSE);

    UpdateMouseCenter();
    RecenterMouse(hWnd);
}

void CGameScene::EndMouseCapture(HWND hWnd)
{
    UNREFERENCED_PARAMETER(hWnd);

    if (!isMouseCaptured)
    {
        return;
    }

    isMouseCaptured = false;
    ignoreMouseMove = false;

    ClipCursor(nullptr);
    ReleaseCapture();
    ShowCursor(TRUE);
}

void CGameScene::CreateSharedDirectMeshes(ID3D12Device* device, ID3D12GraphicsCommandList* commandList)
{
    if (!device || !commandList)
    {
        return;
    }

    if (!sharedDirectTorsoMesh)
    {
        sharedDirectTorsoMesh = new CColorCubeMesh(device, commandList, 1.0f, 1.0f, 1.0f);
    }

    if (!sharedDirectHeadMesh)
    {
        sharedDirectHeadMesh = new CColorCubeMesh(device, commandList, 0.7f, 0.7f, 0.7f);
    }

    if (!sharedDirectArmMesh)
    {
        sharedDirectArmMesh = new CColorCubeMesh(device, commandList, 0.4f, 1.0f, 0.4f);
    }

    if (!sharedDirectLegMesh)
    {
        sharedDirectLegMesh = new CColorCubeMesh(device, commandList, 0.4f, 1.0f, 0.4f);
    }

    if (!sharedDirectHelicopterBodyMesh)
    {
        sharedDirectHelicopterBodyMesh = new CColorCubeMesh(device, commandList, 1.6f, 0.8f, 1.2f);
    }

    if (!sharedDirectHelicopterTailMesh)
    {
        sharedDirectHelicopterTailMesh = new CColorCubeMesh(device, commandList, 0.3f, 0.3f, 2.0f);
    }

    if (!sharedDirectHelicopterRotorMesh)
    {
        sharedDirectHelicopterRotorMesh = new CColorCubeMesh(device, commandList, 2.8f, 0.08f, 0.20f);
    }

    if (!sharedDirectBulletMesh)
    {
        sharedDirectBulletMesh = new CColorCubeMesh(device, commandList, 0.4f, 0.4f, 0.4f);
    }

    if (!sharedDirectTankBodyMesh)
    {
        sharedDirectTankBodyMesh = new CColorCubeMesh(device, commandList, 4.0f, 1.0f, 5.0f);
    }

    if (!sharedDirectTankTurretMesh)
    {
        sharedDirectTankTurretMesh = new CColorCubeMesh(device, commandList, 2.0f, 0.8f, 2.0f);
    }

    if (!sharedDirectTankBarrelMesh)
    {
        sharedDirectTankBarrelMesh = new CColorCubeMesh(device, commandList, 0.4f, 0.4f, 3.5f);
    }

    if (!sharedDirectTankTrackMesh)
    {
        sharedDirectTankTrackMesh = new CColorCubeMesh(device, commandList, 0.7f, 0.6f, 5.2f);
    }

    if (!sharedDirectExplosionBlockMesh)
    {
        sharedDirectExplosionBlockMesh = new CColorCubeMesh(device, commandList, 1.0f, 1.0f, 1.0f);
    }
}

void CGameScene::ReleaseSharedDirectMeshes()
{
    if (sharedDirectTorsoMesh)
    {
        sharedDirectTorsoMesh->Release();
        sharedDirectTorsoMesh = nullptr;
    }

    if (sharedDirectHeadMesh)
    {
        sharedDirectHeadMesh->Release();
        sharedDirectHeadMesh = nullptr;
    }

    if (sharedDirectArmMesh)
    {
        sharedDirectArmMesh->Release();
        sharedDirectArmMesh = nullptr;
    }

    if (sharedDirectLegMesh)
    {
        sharedDirectLegMesh->Release();
        sharedDirectLegMesh = nullptr;
    }

    if (sharedDirectHelicopterBodyMesh)
    {
        sharedDirectHelicopterBodyMesh->Release();
        sharedDirectHelicopterBodyMesh = nullptr;
    }

    if (sharedDirectHelicopterTailMesh)
    {
        sharedDirectHelicopterTailMesh->Release();
        sharedDirectHelicopterTailMesh = nullptr;
    }

    if (sharedDirectHelicopterRotorMesh)
    {
        sharedDirectHelicopterRotorMesh->Release();
        sharedDirectHelicopterRotorMesh = nullptr;
    }

    if (sharedDirectBulletMesh)
    {
        sharedDirectBulletMesh->Release();
        sharedDirectBulletMesh = nullptr;
    }

    if (sharedDirectTankBodyMesh)
    {
        sharedDirectTankBodyMesh->Release();
        sharedDirectTankBodyMesh = nullptr;
    }

    if (sharedDirectTankTurretMesh)
    {
        sharedDirectTankTurretMesh->Release();
        sharedDirectTankTurretMesh = nullptr;
    }

    if (sharedDirectTankBarrelMesh)
    {
        sharedDirectTankBarrelMesh->Release();
        sharedDirectTankBarrelMesh = nullptr;
    }

    if (sharedDirectTankTrackMesh)
    {
        sharedDirectTankTrackMesh->Release();
        sharedDirectTankTrackMesh = nullptr;
    }

    if (sharedDirectExplosionBlockMesh)
    {
        sharedDirectExplosionBlockMesh->Release();
        sharedDirectExplosionBlockMesh = nullptr;
    }
}

void CGameScene::ApplyHumanoidDirectMeshes(CHumanoidObject* target)
{
    if (!target)
    {
        return;
    }

    XMFLOAT4 baseColor = target->GetColor();

    XMFLOAT4 torsoColor = baseColor;
    XMFLOAT4 headColor = MultiplyColor(baseColor, 1.25f);
    XMFLOAT4 armColor = MultiplyColor(baseColor, 0.85f);
    XMFLOAT4 legColor = MultiplyColor(baseColor, 0.65f);

    if (target->GetTorso())
    {
        target->GetTorso()->SetDirectMesh(sharedDirectTorsoMesh);
        target->GetTorso()->SetColor(torsoColor);
    }

    if (target->GetHead())
    {
        target->GetHead()->SetDirectMesh(sharedDirectHeadMesh);
        target->GetHead()->SetColor(headColor);
    }

    if (target->GetLeftArm())
    {
        target->GetLeftArm()->SetDirectMesh(sharedDirectArmMesh);
        target->GetLeftArm()->SetColor(armColor);
    }

    if (target->GetRightArm())
    {
        target->GetRightArm()->SetDirectMesh(sharedDirectArmMesh);
        target->GetRightArm()->SetColor(armColor);
    }

    if (target->GetLeftLeg())
    {
        target->GetLeftLeg()->SetDirectMesh(sharedDirectLegMesh);
        target->GetLeftLeg()->SetColor(legColor);
    }

    if (target->GetRightLeg())
    {
        target->GetRightLeg()->SetDirectMesh(sharedDirectLegMesh);
        target->GetRightLeg()->SetColor(legColor);
    }
}

bool CGameScene::BuildDirectResources(ID3D12Device* device, ID3D12GraphicsCommandList* commandList)
{
    if (!device || !commandList || !gameMap)
    {
        return false;
    }

    CreateSharedDirectMeshes(device, commandList);

    if (!gameMap->BuildDirectResources(device, commandList))
    {
        return false;
    }

    ApplyLocalPlayerDirectMeshes();

    ApplyEnemyDirectMeshes();

    ApplyBulletDirectMeshes();

    return true;
}

void CGameScene::ReleaseDirectUploadBuffers()
{
    if (sharedDirectTorsoMesh)
    {
        sharedDirectTorsoMesh->ReleaseUploadBuffers();
    }

    if (sharedDirectHeadMesh)
    {
        sharedDirectHeadMesh->ReleaseUploadBuffers();
    }

    if (sharedDirectArmMesh)
    {
        sharedDirectArmMesh->ReleaseUploadBuffers();
    }

    if (sharedDirectLegMesh)
    {
        sharedDirectLegMesh->ReleaseUploadBuffers();
    }

    if (sharedDirectHelicopterBodyMesh)
    {
        sharedDirectHelicopterBodyMesh->ReleaseUploadBuffers();
    }

    if (sharedDirectHelicopterTailMesh)
    {
        sharedDirectHelicopterTailMesh->ReleaseUploadBuffers();
    }

    if (sharedDirectHelicopterRotorMesh)
    {
        sharedDirectHelicopterRotorMesh->ReleaseUploadBuffers();
    }

    if (sharedDirectBulletMesh)
    {
        sharedDirectBulletMesh->ReleaseUploadBuffers();
    }

    if (gameMap)
    {
        gameMap->ReleaseDirectUploadBuffers();
    }

    if (sharedDirectTankBodyMesh)
    {
        sharedDirectTankBodyMesh->ReleaseUploadBuffers();
    }

    if (sharedDirectTankTurretMesh)
    {
        sharedDirectTankTurretMesh->ReleaseUploadBuffers();
    }

    if (sharedDirectTankBarrelMesh)
    {
        sharedDirectTankBarrelMesh->ReleaseUploadBuffers();
    }

    if (sharedDirectTankTrackMesh)
    {
        sharedDirectTankTrackMesh->ReleaseUploadBuffers();
    }

    if (sharedDirectExplosionBlockMesh)
    {
        sharedDirectExplosionBlockMesh->ReleaseUploadBuffers();
    }
}

void CGameScene::UpdateCameraShaderVariables(ID3D12GraphicsCommandList* commandList)
{
    if (!commandList || !camera)
    {
        return;
    }

    XMFLOAT4X4 viewMatrix;
    XMFLOAT4X4 projectionMatrix;

    XMStoreFloat4x4(&viewMatrix, XMMatrixTranspose(XMLoadFloat4x4(&camera->GetViewMatrix())));

    XMStoreFloat4x4(&projectionMatrix, XMMatrixTranspose(XMLoadFloat4x4(&camera->GetProjectionMatrix())));

    commandList->SetGraphicsRoot32BitConstants(1, 16, &viewMatrix, 0);

    commandList->SetGraphicsRoot32BitConstants(1, 16, &projectionMatrix, 16);
}

void CGameScene::RenderDirect(ID3D12GraphicsCommandList* commandList)
{
    auto& bullets = gameWorld.GetBullets();

    auto& enemies = gameWorld.GetEnemies();

    if (!commandList || !camera)
    {
        return;
    }

    UpdateCameraShaderVariables(commandList);

    BoundingFrustum worldFrustum = camera->GetWorldFrustum();

    if (gameMap)
    {
        gameMap->RenderDirect(commandList);
    }

    if (IsVisible(localPlayer, worldFrustum))
    {
        localPlayer->RenderDirect(commandList);
    }

    for (CEnemy* enemy : enemies)
    {
        if (enemy &&
            enemy->IsActive() &&
            !enemy->IsDead() &&
            IsVisible(enemy, worldFrustum))
        {
            enemy->RenderDirect(commandList);
        }
    }

    for (CBullet* bullet : bullets)
    {
        if (bullet && bullet->IsActive())
        {
            bullet->RenderDirect(commandList);
        }
    }

    for (FExplosionBlock& block : explosionBlocks)
    {
        if (block.object)
        {
            block.object->RenderDirect(commandList);
        }
    }
}

void CGameScene::ApplyHelicopterDirectMeshes(CHelicopterPlayer* target)
{
    if (!target)
    {
        return;
    }

    target->ApplyHelicopterDirectMeshes(sharedDirectHelicopterBodyMesh, sharedDirectHelicopterTailMesh, sharedDirectHelicopterRotorMesh);
}

void CGameScene::StartEnemyExplosion(const XMFLOAT3& center)
{
    if (!sharedDirectExplosionBlockMesh)
    {
        return;
    }

    const int verticalCount = 4;
    const int horizontalCount = 12;

    const float minPitch = -35.0f;
    const float maxPitch = 70.0f;

    const float speed = 35.0f;
    const float blockScale = 1.2f;

    XMFLOAT3 explosionCenter = center;
    explosionCenter.y += 3.0f;

    for (int y = 0; y < verticalCount; ++y)
    {
        float pitchDegree = minPitch;

        if (verticalCount > 1)
        {
            pitchDegree = minPitch +
                (maxPitch - minPitch) * static_cast<float>(y) /
                static_cast<float>(verticalCount - 1);
        }

        float pitch = XMConvertToRadians(pitchDegree);

        for (int x = 0; x < horizontalCount; ++x)
        {
            float yawDegree = 360.0f * static_cast<float>(x) /
                static_cast<float>(horizontalCount);

            float yaw = XMConvertToRadians(yawDegree);

            XMFLOAT3 direction;
            direction.x = cosf(pitch) * cosf(yaw);
            direction.y = sinf(pitch);
            direction.z = cosf(pitch) * sinf(yaw);

            CGameObject* block = new CGameObject();

            block->SetDirectMesh(sharedDirectExplosionBlockMesh);
            block->SetScale(blockScale, blockScale, blockScale);

            block->SetPosition(explosionCenter.x, explosionCenter.y, explosionCenter.z);

            block->SetColor(XMFLOAT4(1.0f, 0.35f, 0.05f, 1.0f));

            block->Update(0.0f);

            FExplosionBlock explosionBlock;
            explosionBlock.object = block;

            explosionBlock.velocity = XMFLOAT3(direction.x * speed, direction.y * speed, direction.z * speed);

            explosionBlock.lifeTime = 0.0f;

            explosionBlocks.push_back(explosionBlock);
        }
    }
}

void CGameScene::UpdateEnemyExplosions(float deltaTime)
{
    const float explosionDuration = 1.0f;
    const float gravity = 25.0f;

    for (FExplosionBlock& block : explosionBlocks)
    {
        if (!block.object)
        {
            continue;
        }

        block.lifeTime += deltaTime;

        block.velocity.y -= gravity * deltaTime;

        XMFLOAT3 position = block.object->GetPosition();

        position.x += block.velocity.x * deltaTime;
        position.y += block.velocity.y * deltaTime;
        position.z += block.velocity.z * deltaTime;

        block.object->SetPosition(position.x, position.y, position.z);

        block.object->Update(deltaTime);

        if (block.lifeTime >= explosionDuration)
        {
            delete block.object;
            block.object = nullptr;
        }
    }

    auto isFinished = [](const FExplosionBlock& block) { return block.object == nullptr; };

    explosionBlocks.erase(std::remove_if(explosionBlocks.begin(), explosionBlocks.end(), isFinished), explosionBlocks.end());
}

void CGameScene::ReleaseEnemyExplosions()
{
    for (FExplosionBlock& block : explosionBlocks)
    {
        delete block.object;
        block.object = nullptr;
    }

    explosionBlocks.clear();
}

const ThirdPersonCameraProfile& CGameScene::GetCurrentThirdPersonCameraProfile() const
{
    if (gameWorld.GetLocalPlayerType() ==
        EPlayerType::Tank)
    {
        return tankCameraProfile;
    }

    return helicopterCameraProfile;
}

void CGameScene::ApplyThirdPersonLookInput()
{
    const ThirdPersonCameraProfile& profile = GetCurrentThirdPersonCameraProfile();

    thirdPersonCameraYaw +=
        currentPlayerInput.lookYawDelta;

    thirdPersonCameraPitch +=
        currentPlayerInput.lookPitchDelta;

    if (thirdPersonCameraPitch < profile.minPitch)
    {
        thirdPersonCameraPitch = profile.minPitch;
    }
    else if (thirdPersonCameraPitch > profile.maxPitch)
    {
        thirdPersonCameraPitch = profile.maxPitch;
    }
}

void CGameScene::UpdateThirdPersonCamera(const XMFLOAT3& targetPosition, const ThirdPersonCameraProfile& profile)
{
    if (!camera)
    {
        return;
    }

    XMFLOAT3 cameraTarget = Vector3::Add(targetPosition, XMFLOAT3(0.0f, profile.targetHeight, 0.0f));

    float yawRadian = XMConvertToRadians(thirdPersonCameraYaw);

    float pitchRadian = XMConvertToRadians(thirdPersonCameraPitch);

    float horizontalDistance = std::cos(pitchRadian) *
        profile.distance;

    float verticalDistance = std::sin(pitchRadian) *
        profile.distance;

    XMFLOAT3 cameraOffset;

    cameraOffset.x = -std::sin(yawRadian) *
        horizontalDistance;

    cameraOffset.y = verticalDistance;

    cameraOffset.z = -std::cos(yawRadian) *
        horizontalDistance;

    XMFLOAT3 cameraPosition = Vector3::Add(cameraTarget, cameraOffset);

    camera->SetPosition(cameraPosition);

    camera->SetTarget(cameraTarget);

    camera->SetUp(XMFLOAT3(0.0f, 1.0f, 0.0f));
}

void CGameScene::ApplyLocalPlayerDirectMeshes()
{
    if (!localPlayer)
    {
        return;
    }

    switch (gameWorld.GetLocalPlayerType())
    {
    case EPlayerType::Helicopter:
    {
        if (helicopterPlayer)
        {
            ApplyHelicopterDirectMeshes(helicopterPlayer);
        }

        break;
    }

    case EPlayerType::Tank:
    {
        if (playerTank)
        {
            playerTank->ApplyTankDirectMeshes(sharedDirectTankBodyMesh, sharedDirectTankTurretMesh, sharedDirectTankBarrelMesh, sharedDirectTankTrackMesh);
        }

        break;
    }

    default:
        break;
    }
}

void CGameScene::ApplyEnemyDirectMeshes()
{
    auto& enemies = gameWorld.GetEnemies();

    for (CEnemy* enemy : enemies)
    {
        if (!enemy)
        {
            continue;
        }

        switch (enemy->GetEnemyType())
        {
        case EEnemyType::Humanoid:
        {
            CNpc* npc = static_cast<CNpc*>(enemy);

            ApplyHumanoidDirectMeshes(npc);

            break;
        }

        case EEnemyType::Tank:
        {
            CTankObject* enemyTank = static_cast<CTankObject*>(enemy);

            enemyTank->ApplyTankDirectMeshes(sharedDirectTankBodyMesh, sharedDirectTankTurretMesh, sharedDirectTankBarrelMesh, sharedDirectTankTrackMesh);

            break;
        }
        }
    }
}

void CGameScene::ApplyBulletDirectMeshes()
{
    auto& bullets = gameWorld.GetBullets();

    for (CBullet* bullet : bullets)
    {
        if (!bullet)
        {
            continue;
        }

        bullet->SetDirectMesh(sharedDirectBulletMesh);
    }
}

void CGameScene::UpdateLocalPlayerFrame(float deltaTime)
{
    if (!localPlayer)
    {
        return;
    }

    ApplyThirdPersonLookInput();

    gameWorld.UpdateLocalPlayer(deltaTime, currentPlayerInput, thirdPersonCameraYaw, gameMap);

    gameWorld.HandleLocalPlayerFire(currentPlayerInput);

    UpdateShieldVisual();

    if (camera)
    {
        UpdateThirdPersonCamera(localPlayer->GetPosition(), GetCurrentThirdPersonCameraProfile());
    }
}

void CGameScene::UpdateShieldVisual()
{
    if (!localPlayer)
    {
        return;
    }

    const XMFLOAT4 shieldColor(0.75f, 0.95f, 1.0f, 1.0f);
    const XMFLOAT4 shieldTailColor(0.55f, 0.75f, 0.85f, 1.0f);

    const bool shieldEnabled = localPlayer->IsShieldEnabled();

    if (playerTank)
    {
        playerTank->SetTankColor(shieldEnabled ? shieldColor : XMFLOAT4(0.20f, 0.65f, 0.30f, 1.0f));
    }

    if (helicopterPlayer)
    {
        if (shieldEnabled)
        {
            helicopterPlayer->SetHelicopterColor(shieldColor, shieldTailColor);
        }
        else
        {
            helicopterPlayer->SetHelicopterColor(XMFLOAT4(0.15f, 0.55f, 0.95f, 1.0f), XMFLOAT4(0.10f, 0.35f, 0.75f, 1.0f));
        }
    }
}
