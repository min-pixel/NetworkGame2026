// 시작 화면: 제목 블록 글자를 클릭하면 폭발 연출 후 맵 선택 화면으로 넘어간다.

#pragma once

#include "SceneBase.h"

class CCamera;
class CGameObject;
class CBlockTextObject;
class CDirectMesh;

class CStartScene : public CSceneBase
{
public:
    CStartScene();
    ~CStartScene() override;

    void BuildObjects(int clientWidth, int clientHeight) override;
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
    void BuildStartText();

    // 화면 좌표 기준 클릭 영역 (1280x720 기준)
    bool PickStartText(int mouseX, int mouseY);

    void UpdateCameraShaderVariables(ID3D12GraphicsCommandList* commandList);

    struct FExplosionBlock
    {
        CGameObject* object = nullptr;
        XMFLOAT3 velocity = XMFLOAT3(0.0f, 0.0f, 0.0f);
    };

    std::vector<FExplosionBlock> explosionBlocks;

    bool isExploding = false;
    float explosionTimer = 0.0f;
    float explosionDuration = 0.9f;

    void StartExplosion();
    void UpdateExplosion(float deltaTime);
    void ReleaseExplosionBlocks();

private:
    int clientWidth = 0;
    int clientHeight = 0;

    CCamera* camera = nullptr;
    CBlockTextObject* startText = nullptr;
    CDirectMesh* sharedBlockMesh = nullptr;
};
