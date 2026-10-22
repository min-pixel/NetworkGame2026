// 결과 화면: 승리(YOU WIN!) 또는 패배(LOSE) 글자를 보여주고, 클릭하면 맵 선택 화면으로 돌아간다.

#pragma once

#include "SceneBase.h"

class CCamera;
class CBlockTextObject;
class CDirectMesh;

class CEndScene : public CSceneBase
{
public:
    // victory: true = 승리 화면, false = 패배 화면
    CEndScene(bool victory = true);
    ~CEndScene() override;

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
    void BuildEndText();

    // 화면 좌표 기준 클릭 영역 (1280x720 기준)
    bool PickEndText(int mouseX, int mouseY);

    void UpdateCameraShaderVariables(ID3D12GraphicsCommandList* commandList);

private:
    int clientWidth = 0;
    int clientHeight = 0;

    CCamera* camera = nullptr;
    CBlockTextObject* endText = nullptr;
    CDirectMesh* sharedBlockMesh = nullptr;

    bool isVictory = true;
};
