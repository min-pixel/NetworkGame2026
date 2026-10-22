// 게임 프레임워크
// DirectX12 장치/스왑체인/파이프라인을 만들고, 현재 씬을 소유하며 매 프레임을 진행한다.

#pragma once

#include "stdafx.h"
#include "Timer.h"
#include "SceneBase.h"

// 한 프레임(FrameAdvance) 순서:
//   타이머 갱신 -> 씬 입력 처리 -> 씬 로직 갱신 -> 씬 전환 확인 -> 렌더링 -> Present
class CGameFramework
{
public:
    CGameFramework();
    ~CGameFramework();

    void OnCreate(HINSTANCE instance, HWND windowHandle);
    void OnDestroy();

    // 게임을 한 프레임 진행한다. 메시지 루프에서 메시지가 없을 때마다 호출된다.
    void FrameAdvance();

    // 키 상태(keyBuffer)를 갱신하고, 입력 메시지를 현재 씬에 전달한다.
    void OnProcessingWindowMessage(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam);

private:
    void BuildObjects();
    void ReleaseObjects();
    void ProcessInput();
    void AnimateObjects();

    // 현재 씬을 지우고 새 씬을 만든다. GameMap1~3은 이때 기체 타입을 무작위로 정한다.
    void ChangeScene(ESceneType nextScene, bool buildDirectResources);

    // 씬이 요청한 전환(requestedScene)이 있으면 ChangeScene을 호출한다.
    void CheckSceneChange();

private:
    HINSTANCE instanceHandle;
    HWND windowHandle;

    int clientWidth;
    int clientHeight;

    CSceneBase* scene;
    ESceneType currentSceneType = ESceneType::None;

    CGameTimer gameTimer;

    // 가상 키 코드별 눌림 상태 (0x80 = 눌림)
    UCHAR keyBuffer[256];
    bool isPaused;

    TCHAR frameRateText[128];

private:
    static constexpr UINT swapChainBufferCount = 2;

    IDXGIFactory4* dxgiFactory = nullptr;
    IDXGISwapChain3* swapChain = nullptr;
    ID3D12Device* device = nullptr;

    ID3D12CommandQueue* commandQueue = nullptr;
    ID3D12CommandAllocator* commandAllocator = nullptr;
    ID3D12GraphicsCommandList* commandList = nullptr;

    ID3D12DescriptorHeap* renderTargetViewHeap = nullptr;
    ID3D12DescriptorHeap* depthStencilViewHeap = nullptr;

    ID3D12Resource* swapChainBackBuffers[swapChainBufferCount] = {};
    ID3D12Resource* depthStencilBuffer = nullptr;

    ID3D12Fence* fence = nullptr;
    HANDLE fenceEvent = nullptr;

    UINT renderTargetViewDescriptorSize = 0;
    UINT depthStencilViewDescriptorSize = 0;
    UINT currentBackBufferIndex = 0;

    UINT64 fenceValues[swapChainBufferCount] = {};

private:
    bool CreateDirect3DDevice();
    bool CreateCommandObjects();
    bool CreateDescriptorHeaps();
    bool CreateSwapChain();
    bool CreateRenderTargetViews();
    bool CreateDepthStencilView();

    // GPU가 지금까지 받은 명령을 모두 끝낼 때까지 기다린다.
    void WaitForGpuComplete();

    // Present 후 다음 백버퍼로 넘어가며, 그 프레임의 GPU 작업이 끝날 때까지 기다린다.
    void MoveToNextFrame();

    D3D12_CPU_DESCRIPTOR_HANDLE GetCurrentRenderTargetView() const;
    D3D12_CPU_DESCRIPTOR_HANDLE GetDepthStencilView() const;

    D3D12_VIEWPORT viewport = {};
    D3D12_RECT scissorRect = {};

    void CreateViewportAndScissorRect();

    ID3D12RootSignature* rootSignature = nullptr;

    // 루트 상수 3개: b0 월드 행렬(16), b1 뷰+투영 행렬(32), b2 오브젝트 색(4)
    bool CreateRootSignature();

    ID3D12PipelineState* pipelineState = nullptr;
    bool CreatePipelineState();

    D3D12_SHADER_BYTECODE CompileShaderFromFile(const WCHAR* fileName, LPCSTR shaderName, LPCSTR shaderProfile, ID3DBlob** shaderBlob);

    // 현재 씬의 메시를 GPU에 올리고, 업로드가 끝나면 업로드 버퍼를 해제한다.
    bool CreateSceneDirectResources();
};
