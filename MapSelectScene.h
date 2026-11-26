// 맵 선택 화면: Start를 누르면 Map1~3 중 하나로 무작위 진입, End를 누르면 종료.
// (기체 타입은 CGameFramework::ChangeScene에서 무작위로 정한다)

#pragma once

#include "SceneBase.h"
#include "Protocol.h"

class CCamera;
class CBlockTextObject;
class CDirectMesh;
class CGameObject;
class CNetworkClient;

class CMapSelectScene : public CSceneBase
{
public:
    CMapSelectScene();
    ~CMapSelectScene() override;

    void BuildObjects(int clientWidth, int clientHeight) override;
    void SetNetworkClient(CNetworkClient* client) override;
    void OnNetEvent(const NetEvent& event) override;
    void SetLobbyState(const SC_LobbyState& state);
    void ReleaseObjects() override;

    bool ProcessInput(UCHAR* keyBuffer) override;
    void AnimateObjects(float deltaTime) override;

    void OnResize(int width, int height) override;

    bool OnProcessingKeyboardMessage(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam) override;

    bool OnProcessingMouseMessage(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam) override;

    bool BuildDirectResources(ID3D12Device* device, ID3D12GraphicsCommandList* commandList) override;

    void ReleaseDirectUploadBuffers() override;

    void RenderDirect(ID3D12GraphicsCommandList* commandList) override;

private:
    // numberOne = Start 버튼 글자, numberTwo = End 버튼 글자
    void BuildNumberOne();
    void BuildNumberTwo();

    // 화면 좌표 기준 클릭 영역 (1280x720 기준 화면 투영 위치에 맞춘 값)
    bool PickNumberOne(int mouseX, int mouseY);
    bool PickNumberTwo(int mouseX, int mouseY);

    void UpdateCameraShaderVariables(ID3D12GraphicsCommandList* commandList);
    void BuildLobbyIndicators();
    void UpdateLobbyIndicators();
    void UpdateReadyButtonColor();

private:
    int clientWidth = 0;
    int clientHeight = 0;

    CCamera* camera = nullptr;

    CBlockTextObject* numberOne = nullptr;
    CBlockTextObject* numberTwo = nullptr;

    CDirectMesh* sharedBlockMesh = nullptr;

    CGameObject* lobbyIndicators[MAX_PLAYERS] = {};

    CNetworkClient* networkClient = nullptr;
    SC_LobbyState lobbyState = {};
    bool hasLobbyState = false;
    bool readySent = false;
    bool readyConfirmed = false;
};

