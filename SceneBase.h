// 모든 씬의 공통 인터페이스와 씬 종류

#pragma once

#include "stdafx.h"

// 씬 종류. End = 승리 화면, Defeat = 패배 화면, Exit = 프로그램 종료
enum class ESceneType
{
    None,
    Start,
    MapSelect,
    GameMap1,
    GameMap2,
    GameMap3,
    End,
    Defeat,
    Exit
};

// 씬 인터페이스. CGameFramework가 매 프레임 아래 순서로 호출한다.
//   ProcessInput -> AnimateObjects -> RenderDirect
// 씬을 바꾸고 싶으면 requestedScene에 다음 씬을 넣는다 (실제 전환은 프레임워크가 처리).
class CSceneBase
{
public:
    virtual ~CSceneBase() = default;

    // CPU 쪽 객체 생성 (GPU 리소스 생성은 BuildDirectResources)
    virtual void BuildObjects(int clientWidth, int clientHeight) = 0;
    virtual void ReleaseObjects() = 0;

    virtual bool ProcessInput(UCHAR* keyBuffer) = 0;
    virtual void AnimateObjects(float deltaTime) = 0;

    virtual void OnResize(int width, int height) = 0;

    virtual bool OnProcessingKeyboardMessage(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam) = 0;

    virtual bool OnProcessingMouseMessage(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam) = 0;

    // 메시 등 GPU 리소스 생성. 명령 리스트 기록만 하고, 실행은 프레임워크가 한다.
    virtual bool BuildDirectResources(ID3D12Device* device, ID3D12GraphicsCommandList* commandList) = 0;

    // GPU 업로드가 끝난 뒤 업로드용 임시 버퍼 해제
    virtual void ReleaseDirectUploadBuffers() = 0;
    virtual void RenderDirect(ID3D12GraphicsCommandList* commandList) = 0;

    ESceneType GetRequestedScene() const
    {
        return requestedScene;
    }

    void ClearRequestedScene()
    {
        requestedScene = ESceneType::None;
    }

protected:
    ESceneType requestedScene = ESceneType::None;
};
