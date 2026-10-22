// 게임 플레이 씬 (클라이언트 / 표현 담당)
// 입력 수집, 3인칭 카메라, 렌더링, 폭발 연출, 승패에 따른 씬 전환을 맡는다.
// 이동/AI/사격/충돌/피해/승패 판정 같은 gameplay는 CGameWorld가 담당한다.

#pragma once

#include "stdafx.h"
#include "Mesh.h"
#include "SceneBase.h"

#include "PlayerInput.h"

#include "ThirdPersonCameraProfile.h"
#include "PlayerType.h"
#include "TankPlayer.h"
#include "GameSessionConfig.h"

#include "GameWorld.h"

class CCamera;
class CPlayer;
class CGameObject;
class CHumanoidObject;
class CGameMap;
class CHelicopterPlayer;
class CEnemy;

// AnimateObjects 한 프레임 흐름:
//   플레이어(이동/사격/Shield) -> 적 AI -> 총알 이동 -> 피격 판정 -> 폭발 연출 -> 승패 확인
class CGameScene : public CSceneBase
{
public:

    CGameScene();

    // config: 맵 번호, 기체 타입, 게임 규칙
    explicit CGameScene(const GameSessionConfig& config);

    ~CGameScene()override;

    void BuildObjects(int clientWidth, int clientHeight)override;
    void ReleaseObjects()override;

    bool ProcessInput(UCHAR* keyBuffer)override;
    void AnimateObjects(float deltaTime)override;

    void OnResize(int width, int height)override;

    bool OnProcessingKeyboardMessage(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam)override;
    bool OnProcessingMouseMessage(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam)override;

    bool BuildDirectResources(ID3D12Device* device, ID3D12GraphicsCommandList* commandList)override;

    void ReleaseDirectUploadBuffers()override;

    void RenderDirect(ID3D12GraphicsCommandList* commandList)override;

private:

    // 충돌 박스가 카메라 시야(frustum) 안에 있는지 (컬링용)
    bool IsVisible(const CGameObject* object, const BoundingFrustum& frustum) const;

    // 여러 오브젝트가 함께 쓰는 공용 메시 (참조 카운트로 관리)
    CDirectMesh* sharedDirectTorsoMesh = nullptr;
    CDirectMesh* sharedDirectHeadMesh = nullptr;
    CDirectMesh* sharedDirectArmMesh = nullptr;
    CDirectMesh* sharedDirectLegMesh = nullptr;
    CDirectMesh* sharedDirectBulletMesh = nullptr;

    void CreateSharedDirectMeshes(ID3D12Device* device, ID3D12GraphicsCommandList* commandList);

    void ReleaseSharedDirectMeshes();

    // Humanoid 각 부위에 메시와 색(기본색 기준 밝기 차이)을 붙인다.
    void ApplyHumanoidDirectMeshes(CHumanoidObject* target);

    // 카메라 뷰/투영 행렬을 루트 상수 b1에 올린다.
    void UpdateCameraShaderVariables(ID3D12GraphicsCommandList* commandList);

private:
    int clientWidth;
    int clientHeight;

    // 마우스 캡처 중에는 매 이동마다 커서를 화면 중앙으로 되돌리고, 중앙과의 차이를 시점 회전에 쓴다.
    POINT mouseCenter;
    bool isMouseCaptured;

    // 커서를 중앙으로 되돌릴 때 생기는 WM_MOUSEMOVE 한 번을 무시하기 위한 플래그
    bool ignoreMouseMove;

    bool isLButtonDown;

    float mouseSensitivity;

    // 프레임워크에서 받은 키 상태 복사본
    UCHAR inputBuffer[256];

    // 이번 프레임에 World로 넘길 입력
    PlayerInput currentPlayerInput;

    // 다음 UpdatePlayerInput까지 쌓아 두는 입력 (마우스 이동량, Shield 키)
    float pendingLookYawDelta = 0.0f;
    float pendingLookPitchDelta = 0.0f;

    bool pendingShieldTogglePressed = false;

    // 쌓아 둔 입력을 PlayerInput으로 묶고, 한 번만 쓰는 값은 비운다.
    void UpdatePlayerInput();

    // 마우스 이동량으로 카메라 yaw/pitch를 바꾼다 (pitch는 프로필 범위로 제한).
    void ApplyThirdPersonLookInput();

    // 대상 뒤쪽(yaw/pitch/거리 기준)에 카메라를 놓고 대상을 바라보게 한다.
    void UpdateThirdPersonCamera(const XMFLOAT3& targetPosition, const ThirdPersonCameraProfile& profile);

    float thirdPersonCameraYaw = 0.0f;
    float thirdPersonCameraPitch = 20.0f;

    // 기체별 3인칭 카메라 설정
    ThirdPersonCameraProfile helicopterCameraProfile;
    ThirdPersonCameraProfile tankCameraProfile;

    CCamera* camera;

    // 아래 Player 포인터들은 모두 gameWorld가 소유한 객체를 가리키는 비소유 포인터 (delete 금지)
    CPlayer* localPlayer = nullptr;

    // gameplay 객체(Player/Enemy/Bullet) 소유 및 시뮬레이션
    CGameWorld gameWorld;

    CHelicopterPlayer* helicopterPlayer = nullptr;

    GameSessionConfig sessionConfig;

    // World에 플레이어 생성을 요청하고, 타입에 맞는 비소유 포인터를 저장한다.
    void CreateLocalPlayer();
    void ApplyLocalPlayerDirectMeshes();
    const ThirdPersonCameraProfile& GetCurrentThirdPersonCameraProfile() const;

    void UpdateMouseCenter();
    void BeginMouseCapture(HWND hWnd);
    void EndMouseCapture(HWND hWnd);
    void RecenterMouse(HWND hWnd);

    CGameMap* gameMap = nullptr;

    CDirectMesh* sharedDirectHelicopterBodyMesh = nullptr;
    CDirectMesh* sharedDirectHelicopterTailMesh = nullptr;
    CDirectMesh* sharedDirectHelicopterRotorMesh = nullptr;

    void ApplyHelicopterDirectMeshes(CHelicopterPlayer* target);

    CTankPlayer* playerTank = nullptr;

    CDirectMesh* sharedDirectTankBodyMesh = nullptr;
    CDirectMesh* sharedDirectTankTurretMesh = nullptr;
    CDirectMesh* sharedDirectTankBarrelMesh = nullptr;
    CDirectMesh* sharedDirectTankTrackMesh = nullptr;

    void ApplyEnemyDirectMeshes();
    void ApplyBulletDirectMeshes();

    // 카메라 회전 -> World 이동/Shield -> World 사격 -> Shield 색 -> 카메라 위치 순서로 처리
    void UpdateLocalPlayerFrame(float deltaTime);

    // Shield 상태를 기체 색으로 표시 (켜지면 두 기체 모두 같은 색)
    void UpdateShieldVisual();

    // 적 사망 시 흩어지는 폭발 블록 (표현 전용, 1초 뒤 삭제)
    struct FExplosionBlock
    {
        CGameObject* object = nullptr;
        XMFLOAT3 velocity = XMFLOAT3(0.0f, 0.0f, 0.0f);
        float lifeTime = 0.0f;
    };

    std::vector<FExplosionBlock> explosionBlocks;

    CDirectMesh* sharedDirectExplosionBlockMesh = nullptr;

    void StartEnemyExplosion(const XMFLOAT3& center);
    void UpdateEnemyExplosions(float deltaTime);
    void ReleaseEnemyExplosions();
};
