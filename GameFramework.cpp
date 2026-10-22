// 게임 프레임워크 구현: DirectX12 초기화, 프레임 진행, 씬 전환

#include "stdafx.h"
#include "GameFramework.h"

#include "GameScene.h"
#include "StartScene.h"
#include "MapSelectScene.h"
#include "EndScene.h"

#include "GameSessionConfig.h"

#include <random>

namespace
{
    EPlayerType GetRandomPlayerType()
    {
        static std::mt19937 randomGenerator(std::random_device{}());

        std::uniform_int_distribution<int> playerTypeDistribution(0, 1);

        int randomValue = playerTypeDistribution(randomGenerator);

        if (randomValue == 0)
        {
            return EPlayerType::Helicopter;
        }

        return EPlayerType::Tank;
    }
}

CGameFramework::CGameFramework()
    : instanceHandle(nullptr),
    windowHandle(nullptr),
    clientWidth(0),
    clientHeight(0),
    scene(nullptr),
    isPaused(false)
{
    ZeroMemory(keyBuffer, sizeof(keyBuffer));
    ZeroMemory(frameRateText, sizeof(frameRateText));

    for (UINT i = 0; i < swapChainBufferCount; ++i)
    {
        swapChainBackBuffers[i] = nullptr;
        fenceValues[i] = 0;
    }
}

CGameFramework::~CGameFramework()
{
}

void CGameFramework::OnCreate(HINSTANCE instance, HWND hWnd)
{
    instanceHandle = instance;
    windowHandle = hWnd;

    RECT clientRect = {};
    GetClientRect(windowHandle, &clientRect);

    clientWidth = clientRect.right - clientRect.left;
    clientHeight = clientRect.bottom - clientRect.top;

    if (clientWidth < 1) clientWidth = 1;
    if (clientHeight < 1) clientHeight = 1;

    ZeroMemory(keyBuffer, sizeof(keyBuffer));

    CreateDirect3DDevice();
    CreateCommandObjects();
    CreateDescriptorHeaps();
    CreateSwapChain();
    CreateRenderTargetViews();
    CreateDepthStencilView();

    CreateViewportAndScissorRect();
    CreateRootSignature();
    CreatePipelineState();

    BuildObjects();

    if (!CreateSceneDirectResources())
    {
        OutputDebugStringA("[ERROR] CreateSceneDirectResources failed\n");
        return;
    }

    gameTimer.Reset();
}

void CGameFramework::OnDestroy()
{
    WaitForGpuComplete();

    ReleaseObjects();

    if (depthStencilBuffer)
    {
        depthStencilBuffer->Release();
        depthStencilBuffer = nullptr;
    }

    if (depthStencilViewHeap)
    {
        depthStencilViewHeap->Release();
        depthStencilViewHeap = nullptr;
    }

    for (UINT i = 0; i < swapChainBufferCount; ++i)
    {
        if (swapChainBackBuffers[i])
        {
            swapChainBackBuffers[i]->Release();
            swapChainBackBuffers[i] = nullptr;
        }
    }

    if (renderTargetViewHeap)
    {
        renderTargetViewHeap->Release();
        renderTargetViewHeap = nullptr;
    }

    if (commandList)
    {
        commandList->Release();
        commandList = nullptr;
    }

    if (commandAllocator)
    {
        commandAllocator->Release();
        commandAllocator = nullptr;
    }

    if (commandQueue)
    {
        commandQueue->Release();
        commandQueue = nullptr;
    }

    if (fence)
    {
        fence->Release();
        fence = nullptr;
    }

    if (fenceEvent)
    {
        CloseHandle(fenceEvent);
        fenceEvent = nullptr;
    }

    if (pipelineState)
    {
        pipelineState->Release();
        pipelineState = nullptr;
    }

    if (rootSignature)
    {
        rootSignature->Release();
        rootSignature = nullptr;
    }

    if (swapChain)
    {
        swapChain->SetFullscreenState(FALSE, nullptr);
        swapChain->Release();
        swapChain = nullptr;
    }

    if (device)
    {
        device->Release();
        device = nullptr;
    }

    if (dxgiFactory)
    {
        dxgiFactory->Release();
        dxgiFactory = nullptr;
    }

#if defined(_DEBUG)
    IDXGIDebug1* dxgiDebug = nullptr;

    if (SUCCEEDED(DXGIGetDebugInterface1(0, __uuidof(IDXGIDebug1), reinterpret_cast<void**>(&dxgiDebug))))
    {
        dxgiDebug->ReportLiveObjects(DXGI_DEBUG_ALL, DXGI_DEBUG_RLO_DETAIL);
        dxgiDebug->Release();
        dxgiDebug = nullptr;
    }
#endif
}

void CGameFramework::BuildObjects()
{
    if (scene != nullptr)
    {
        return;
    }

    ChangeScene(ESceneType::Start, false);
}

void CGameFramework::ReleaseObjects()
{
    if (scene)
    {
        scene->ReleaseObjects();
        delete scene;
        scene = nullptr;
    }
}

void CGameFramework::ProcessInput()
{
    if (scene)
    {
        scene->ProcessInput(keyBuffer);
    }
}

void CGameFramework::AnimateObjects()
{
    if (scene)
    {
        scene->AnimateObjects(gameTimer.GetTimeElapsed());
    }
}

void CGameFramework::FrameAdvance()
{
    if (isPaused)
    {
        return;
    }

    gameTimer.Tick(0.0f);

    ProcessInput();
    AnimateObjects();

    CheckSceneChange();

    WaitForGpuComplete();

    HRESULT result = commandAllocator->Reset();
    if (FAILED(result))
    {
        return;
    }

    result = commandList->Reset(commandAllocator, nullptr);
    if (FAILED(result))
    {
        return;
    }

    commandList->RSSetViewports(1, &viewport);
    commandList->RSSetScissorRects(1, &scissorRect);
    commandList->SetGraphicsRootSignature(rootSignature);

    commandList->SetPipelineState(pipelineState);

    D3D12_RESOURCE_BARRIER barrier = {};
    barrier.Type = D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;
    barrier.Flags = D3D12_RESOURCE_BARRIER_FLAG_NONE;
    barrier.Transition.pResource = swapChainBackBuffers[currentBackBufferIndex];
    barrier.Transition.StateBefore = D3D12_RESOURCE_STATE_PRESENT;
    barrier.Transition.StateAfter = D3D12_RESOURCE_STATE_RENDER_TARGET;
    barrier.Transition.Subresource = D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES;

    commandList->ResourceBarrier(1, &barrier);

    D3D12_CPU_DESCRIPTOR_HANDLE renderTargetView = GetCurrentRenderTargetView();
    D3D12_CPU_DESCRIPTOR_HANDLE depthStencilView = GetDepthStencilView();

    commandList->OMSetRenderTargets(1, &renderTargetView, TRUE, &depthStencilView);

    const float clearColor[4] = { 0.05f, 0.05f, 0.08f, 1.0f };

    commandList->ClearRenderTargetView(renderTargetView, clearColor, 0, nullptr);

    commandList->ClearDepthStencilView(depthStencilView, D3D12_CLEAR_FLAG_DEPTH | D3D12_CLEAR_FLAG_STENCIL, 1.0f, 0, 0, nullptr);

    if (scene)
    {
        scene->RenderDirect(commandList);
    }

    barrier.Transition.StateBefore = D3D12_RESOURCE_STATE_RENDER_TARGET;
    barrier.Transition.StateAfter = D3D12_RESOURCE_STATE_PRESENT;

    commandList->ResourceBarrier(1, &barrier);

    result = commandList->Close();
    if (FAILED(result))
    {
        return;
    }

    ID3D12CommandList* commandLists[] = { commandList };

    commandQueue->ExecuteCommandLists(_countof(commandLists), commandLists);

    swapChain->Present(0, 0);

    MoveToNextFrame();

    gameTimer.GetFrameRate(frameRateText, _countof(frameRateText));

    if (windowHandle && _tcslen(frameRateText) > 0)
    {
        SetWindowText(windowHandle, frameRateText);
    }
}

void CGameFramework::OnProcessingWindowMessage(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam)
{
    switch (message)
    {
    case WM_SIZE:
    {
        RECT clientRect = {};
        GetClientRect(hWnd, &clientRect);
        clientWidth = clientRect.right - clientRect.left;
        clientHeight = clientRect.bottom - clientRect.top;

        if (clientWidth < 1) clientWidth = 1;
        if (clientHeight < 1) clientHeight = 1;

        if (scene)
        {
            scene->OnResize(clientWidth, clientHeight);
        }
        break;
    }

    case WM_KEYDOWN:
    {
        if (wParam < 256)
        {
            keyBuffer[wParam] = 0x80;
        }

        if (scene)
        {
            scene->OnProcessingKeyboardMessage(hWnd, message, wParam, lParam);
        }
        break;
    }

    case WM_KEYUP:
    {
        if (wParam < 256)
        {
            keyBuffer[wParam] = 0x00;
        }

        if (scene)
        {
            scene->OnProcessingKeyboardMessage(hWnd, message, wParam, lParam);
        }
        break;
    }

    case WM_LBUTTONDOWN:
    case WM_LBUTTONUP:
    case WM_RBUTTONDOWN:
    case WM_RBUTTONUP:
    case WM_MOUSEMOVE:
    {
        if (scene)
        {
            scene->OnProcessingMouseMessage(hWnd, message, wParam, lParam);
        }
        break;
    }

    default:
        break;
    }
}

bool CGameFramework::CreateDirect3DDevice()
{
    HRESULT result = CreateDXGIFactory2(0, __uuidof(IDXGIFactory4), reinterpret_cast<void**>(&dxgiFactory));

    if (FAILED(result))
    {
        return false;
    }

    IDXGIAdapter1* adapter = nullptr;

    for (UINT adapterIndex = 0;
        dxgiFactory->EnumAdapters1(adapterIndex, &adapter) != DXGI_ERROR_NOT_FOUND;
        ++adapterIndex)
    {
        DXGI_ADAPTER_DESC1 adapterDesc = {};
        adapter->GetDesc1(&adapterDesc);

        if (adapterDesc.Flags & DXGI_ADAPTER_FLAG_SOFTWARE)
        {
            adapter->Release();
            adapter = nullptr;
            continue;
        }

        result = D3D12CreateDevice(adapter, D3D_FEATURE_LEVEL_12_0, __uuidof(ID3D12Device), reinterpret_cast<void**>(&device));

        if (SUCCEEDED(result))
        {
            break;
        }

        adapter->Release();
        adapter = nullptr;
    }

    if (!device)
    {
        result = dxgiFactory->EnumWarpAdapter(__uuidof(IDXGIAdapter1), reinterpret_cast<void**>(&adapter));

        if (FAILED(result))
        {
            return false;
        }

        result = D3D12CreateDevice(adapter, D3D_FEATURE_LEVEL_11_0, __uuidof(ID3D12Device), reinterpret_cast<void**>(&device));

        if (FAILED(result))
        {
            adapter->Release();
            adapter = nullptr;
            return false;
        }
    }

    if (adapter)
    {
        adapter->Release();
        adapter = nullptr;
    }

    result = device->CreateFence(0, D3D12_FENCE_FLAG_NONE, __uuidof(ID3D12Fence), reinterpret_cast<void**>(&fence));

    if (FAILED(result))
    {
        return false;
    }

    fenceEvent = CreateEvent(nullptr, FALSE, FALSE, nullptr);

    if (!fenceEvent)
    {
        return false;
    }

    return true;
}

bool CGameFramework::CreateCommandObjects()
{
    D3D12_COMMAND_QUEUE_DESC queueDesc = {};
    queueDesc.Type = D3D12_COMMAND_LIST_TYPE_DIRECT;
    queueDesc.Flags = D3D12_COMMAND_QUEUE_FLAG_NONE;

    HRESULT result = device->CreateCommandQueue(&queueDesc, __uuidof(ID3D12CommandQueue), reinterpret_cast<void**>(&commandQueue));

    if (FAILED(result))
    {
        return false;
    }

    result = device->CreateCommandAllocator(D3D12_COMMAND_LIST_TYPE_DIRECT, __uuidof(ID3D12CommandAllocator), reinterpret_cast<void**>(&commandAllocator));

    if (FAILED(result))
    {
        return false;
    }

    result = device->CreateCommandList(0, D3D12_COMMAND_LIST_TYPE_DIRECT, commandAllocator, nullptr, __uuidof(ID3D12GraphicsCommandList), reinterpret_cast<void**>(&commandList));

    if (FAILED(result))
    {
        return false;
    }

    commandList->Close();

    return true;
}

bool CGameFramework::CreateDescriptorHeaps()
{
    D3D12_DESCRIPTOR_HEAP_DESC renderTargetViewHeapDesc = {};
    renderTargetViewHeapDesc.NumDescriptors = swapChainBufferCount;
    renderTargetViewHeapDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_RTV;
    renderTargetViewHeapDesc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_NONE;
    renderTargetViewHeapDesc.NodeMask = 0;

    HRESULT result = device->CreateDescriptorHeap(&renderTargetViewHeapDesc, __uuidof(ID3D12DescriptorHeap), reinterpret_cast<void**>(&renderTargetViewHeap));

    if (FAILED(result))
    {
        return false;
    }

    renderTargetViewDescriptorSize = device->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_RTV);

    D3D12_DESCRIPTOR_HEAP_DESC depthStencilViewHeapDesc = {};
    depthStencilViewHeapDesc.NumDescriptors = 1;
    depthStencilViewHeapDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_DSV;
    depthStencilViewHeapDesc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_NONE;
    depthStencilViewHeapDesc.NodeMask = 0;

    result = device->CreateDescriptorHeap(&depthStencilViewHeapDesc, __uuidof(ID3D12DescriptorHeap), reinterpret_cast<void**>(&depthStencilViewHeap));

    if (FAILED(result))
    {
        return false;
    }

    depthStencilViewDescriptorSize = device->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_DSV);

    return true;
}

bool CGameFramework::CreateSwapChain()
{
    if (!dxgiFactory || !commandQueue || !windowHandle)
    {
        return false;
    }

    RECT clientRect = {};
    GetClientRect(windowHandle, &clientRect);

    clientWidth = clientRect.right - clientRect.left;
    clientHeight = clientRect.bottom - clientRect.top;

    if (clientWidth < 1) clientWidth = 1;
    if (clientHeight < 1) clientHeight = 1;

    DXGI_SWAP_CHAIN_DESC swapChainDesc = {};
    swapChainDesc.BufferCount = swapChainBufferCount;
    swapChainDesc.BufferDesc.Width = clientWidth;
    swapChainDesc.BufferDesc.Height = clientHeight;
    swapChainDesc.BufferDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
    swapChainDesc.BufferDesc.RefreshRate.Numerator = 60;
    swapChainDesc.BufferDesc.RefreshRate.Denominator = 1;
    swapChainDesc.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
    swapChainDesc.OutputWindow = windowHandle;
    swapChainDesc.SampleDesc.Count = 1;
    swapChainDesc.SampleDesc.Quality = 0;
    swapChainDesc.Windowed = TRUE;
    swapChainDesc.SwapEffect = DXGI_SWAP_EFFECT_FLIP_DISCARD;
    swapChainDesc.Flags = DXGI_SWAP_CHAIN_FLAG_ALLOW_MODE_SWITCH;

    IDXGISwapChain* baseSwapChain = nullptr;

    HRESULT result = dxgiFactory->CreateSwapChain(commandQueue, &swapChainDesc, &baseSwapChain);

    if (FAILED(result))
    {
        return false;
    }

    result = baseSwapChain->QueryInterface(__uuidof(IDXGISwapChain3), reinterpret_cast<void**>(&swapChain));

    baseSwapChain->Release();
    baseSwapChain = nullptr;

    if (FAILED(result))
    {
        return false;
    }

    currentBackBufferIndex = swapChain->GetCurrentBackBufferIndex();

    dxgiFactory->MakeWindowAssociation(windowHandle, DXGI_MWA_NO_ALT_ENTER);

    return true;
}

bool CGameFramework::CreateRenderTargetViews()
{
    if (!device || !swapChain || !renderTargetViewHeap)
    {
        return false;
    }

    D3D12_CPU_DESCRIPTOR_HANDLE renderTargetViewHandle = renderTargetViewHeap->GetCPUDescriptorHandleForHeapStart();

    for (UINT i = 0; i < swapChainBufferCount; ++i)
    {
        if (swapChainBackBuffers[i])
        {
            swapChainBackBuffers[i]->Release();
            swapChainBackBuffers[i] = nullptr;
        }

        HRESULT result = swapChain->GetBuffer(i, __uuidof(ID3D12Resource), reinterpret_cast<void**>(&swapChainBackBuffers[i]));

        if (FAILED(result))
        {
            return false;
        }

        device->CreateRenderTargetView(swapChainBackBuffers[i], nullptr, renderTargetViewHandle);

        renderTargetViewHandle.ptr += renderTargetViewDescriptorSize;
    }

    return true;
}

bool CGameFramework::CreateDepthStencilView()
{
    if (!device || !depthStencilViewHeap)
    {
        return false;
    }

    if (depthStencilBuffer)
    {
        depthStencilBuffer->Release();
        depthStencilBuffer = nullptr;
    }

    D3D12_RESOURCE_DESC depthStencilDesc = {};
    depthStencilDesc.Dimension = D3D12_RESOURCE_DIMENSION_TEXTURE2D;
    depthStencilDesc.Alignment = 0;
    depthStencilDesc.Width = clientWidth;
    depthStencilDesc.Height = clientHeight;
    depthStencilDesc.DepthOrArraySize = 1;
    depthStencilDesc.MipLevels = 1;
    depthStencilDesc.Format = DXGI_FORMAT_D24_UNORM_S8_UINT;
    depthStencilDesc.SampleDesc.Count = 1;
    depthStencilDesc.SampleDesc.Quality = 0;
    depthStencilDesc.Layout = D3D12_TEXTURE_LAYOUT_UNKNOWN;
    depthStencilDesc.Flags = D3D12_RESOURCE_FLAG_ALLOW_DEPTH_STENCIL;

    D3D12_HEAP_PROPERTIES heapProperties = {};
    heapProperties.Type = D3D12_HEAP_TYPE_DEFAULT;
    heapProperties.CPUPageProperty = D3D12_CPU_PAGE_PROPERTY_UNKNOWN;
    heapProperties.MemoryPoolPreference = D3D12_MEMORY_POOL_UNKNOWN;
    heapProperties.CreationNodeMask = 1;
    heapProperties.VisibleNodeMask = 1;

    D3D12_CLEAR_VALUE clearValue = {};
    clearValue.Format = DXGI_FORMAT_D24_UNORM_S8_UINT;
    clearValue.DepthStencil.Depth = 1.0f;
    clearValue.DepthStencil.Stencil = 0;

    HRESULT result = device->CreateCommittedResource(&heapProperties, D3D12_HEAP_FLAG_NONE, &depthStencilDesc, D3D12_RESOURCE_STATE_DEPTH_WRITE, &clearValue, __uuidof(ID3D12Resource), reinterpret_cast<void**>(&depthStencilBuffer));

    if (FAILED(result))
    {
        return false;
    }

    D3D12_CPU_DESCRIPTOR_HANDLE depthStencilViewHandle = depthStencilViewHeap->GetCPUDescriptorHandleForHeapStart();

    device->CreateDepthStencilView(depthStencilBuffer, nullptr, depthStencilViewHandle);

    return true;
}

D3D12_CPU_DESCRIPTOR_HANDLE CGameFramework::GetCurrentRenderTargetView() const
{
    D3D12_CPU_DESCRIPTOR_HANDLE handle = renderTargetViewHeap->GetCPUDescriptorHandleForHeapStart();

    handle.ptr += currentBackBufferIndex * renderTargetViewDescriptorSize;

    return handle;
}

D3D12_CPU_DESCRIPTOR_HANDLE CGameFramework::GetDepthStencilView() const
{
    return depthStencilViewHeap->GetCPUDescriptorHandleForHeapStart();
}

void CGameFramework::MoveToNextFrame()
{
    const UINT64 fenceValue = ++fenceValues[currentBackBufferIndex];

    HRESULT result = commandQueue->Signal(fence, fenceValue);
    if (FAILED(result))
    {
        return;
    }

    if (fence->GetCompletedValue() < fenceValue)
    {
        result = fence->SetEventOnCompletion(fenceValue, fenceEvent);

        if (SUCCEEDED(result))
        {
            WaitForSingleObject(fenceEvent, INFINITE);
        }
    }

    currentBackBufferIndex = swapChain->GetCurrentBackBufferIndex();
}

void CGameFramework::WaitForGpuComplete()
{
    if (!commandQueue || !fence || !fenceEvent)
    {
        return;
    }

    const UINT64 fenceValue = ++fenceValues[currentBackBufferIndex];

    HRESULT result = commandQueue->Signal(fence, fenceValue);
    if (FAILED(result))
    {
        return;
    }

    if (fence->GetCompletedValue() < fenceValue)
    {
        result = fence->SetEventOnCompletion(fenceValue, fenceEvent);

        if (SUCCEEDED(result))
        {
            WaitForSingleObject(fenceEvent, INFINITE);
        }
    }
}

void CGameFramework::CreateViewportAndScissorRect()
{
    viewport.TopLeftX = 0.0f;
    viewport.TopLeftY = 0.0f;
    viewport.Width = static_cast<float>(clientWidth);
    viewport.Height = static_cast<float>(clientHeight);
    viewport.MinDepth = 0.0f;
    viewport.MaxDepth = 1.0f;

    scissorRect.left = 0;
    scissorRect.top = 0;
    scissorRect.right = clientWidth;
    scissorRect.bottom = clientHeight;
}

bool CGameFramework::CreateRootSignature()
{
    D3D12_ROOT_PARAMETER rootParameters[3] = {};

    rootParameters[0].ParameterType = D3D12_ROOT_PARAMETER_TYPE_32BIT_CONSTANTS;
    rootParameters[0].Constants.Num32BitValues = 16;
    rootParameters[0].Constants.ShaderRegister = 0;
    rootParameters[0].Constants.RegisterSpace = 0;
    rootParameters[0].ShaderVisibility = D3D12_SHADER_VISIBILITY_VERTEX;

    rootParameters[1].ParameterType = D3D12_ROOT_PARAMETER_TYPE_32BIT_CONSTANTS;
    rootParameters[1].Constants.Num32BitValues = 32;
    rootParameters[1].Constants.ShaderRegister = 1;
    rootParameters[1].Constants.RegisterSpace = 0;
    rootParameters[1].ShaderVisibility = D3D12_SHADER_VISIBILITY_VERTEX;

    rootParameters[2].ParameterType = D3D12_ROOT_PARAMETER_TYPE_32BIT_CONSTANTS;
    rootParameters[2].Constants.Num32BitValues = 4;
    rootParameters[2].Constants.ShaderRegister = 2;
    rootParameters[2].Constants.RegisterSpace = 0;
    rootParameters[2].ShaderVisibility = D3D12_SHADER_VISIBILITY_PIXEL;

    D3D12_ROOT_SIGNATURE_DESC rootSignatureDesc = {};
    rootSignatureDesc.NumParameters = _countof(rootParameters);
    rootSignatureDesc.pParameters = rootParameters;
    rootSignatureDesc.NumStaticSamplers = 0;
    rootSignatureDesc.pStaticSamplers = nullptr;
    rootSignatureDesc.Flags = D3D12_ROOT_SIGNATURE_FLAG_ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT;

    ID3DBlob* signatureBlob = nullptr;
    ID3DBlob* errorBlob = nullptr;

    HRESULT result = D3D12SerializeRootSignature(&rootSignatureDesc, D3D_ROOT_SIGNATURE_VERSION_1, &signatureBlob, &errorBlob);

    if (FAILED(result))
    {
        if (errorBlob)
        {
            OutputDebugStringA(static_cast<const char*>(errorBlob->GetBufferPointer()));

            errorBlob->Release();
            errorBlob = nullptr;
        }

        return false;
    }

    result = device->CreateRootSignature(0, signatureBlob->GetBufferPointer(), signatureBlob->GetBufferSize(), __uuidof(ID3D12RootSignature), reinterpret_cast<void**>(&rootSignature));

    signatureBlob->Release();
    signatureBlob = nullptr;

    if (errorBlob)
    {
        errorBlob->Release();
        errorBlob = nullptr;
    }

    if (FAILED(result))
    {
        return false;
    }

    return true;
}

D3D12_SHADER_BYTECODE CGameFramework::CompileShaderFromFile(const WCHAR* fileName, LPCSTR shaderName, LPCSTR shaderProfile, ID3DBlob** shaderBlob)
{
    UINT compileFlags = 0;

#if defined(_DEBUG)
    compileFlags = D3DCOMPILE_DEBUG | D3DCOMPILE_SKIP_OPTIMIZATION;
#endif

    ID3DBlob* errorBlob = nullptr;

    HRESULT result = D3DCompileFromFile(fileName, nullptr, nullptr, shaderName, shaderProfile, compileFlags, 0, shaderBlob, &errorBlob);

    if (FAILED(result))
    {
        if (errorBlob)
        {
            OutputDebugStringA(static_cast<const char*>(errorBlob->GetBufferPointer()));

            errorBlob->Release();
            errorBlob = nullptr;
        }

        D3D12_SHADER_BYTECODE emptyBytecode = {};
        return emptyBytecode;
    }

    if (errorBlob)
    {
        errorBlob->Release();
        errorBlob = nullptr;
    }

    D3D12_SHADER_BYTECODE shaderBytecode = {};
    shaderBytecode.pShaderBytecode = (*shaderBlob)->GetBufferPointer();
    shaderBytecode.BytecodeLength = (*shaderBlob)->GetBufferSize();

    return shaderBytecode;
}

bool CGameFramework::CreatePipelineState()
{
    if (!device || !rootSignature)
    {
        return false;
    }

    ID3DBlob* vertexShaderBlob = nullptr;
    ID3DBlob* pixelShaderBlob = nullptr;

    D3D12_SHADER_BYTECODE vertexShaderBytecode = CompileShaderFromFile(L"Shaders.hlsl", "VSMain", "vs_5_1", &vertexShaderBlob);

    D3D12_SHADER_BYTECODE pixelShaderBytecode = CompileShaderFromFile(L"Shaders.hlsl", "PSMain", "ps_5_1", &pixelShaderBlob);

    if (!vertexShaderBlob || !pixelShaderBlob)
    {
        if (vertexShaderBlob)
        {
            vertexShaderBlob->Release();
            vertexShaderBlob = nullptr;
        }

        if (pixelShaderBlob)
        {
            pixelShaderBlob->Release();
            pixelShaderBlob = nullptr;
        }

        return false;
    }

    D3D12_INPUT_ELEMENT_DESC inputElementDescs[2] = {};

    inputElementDescs[0].SemanticName = "POSITION";
    inputElementDescs[0].SemanticIndex = 0;
    inputElementDescs[0].Format = DXGI_FORMAT_R32G32B32_FLOAT;
    inputElementDescs[0].InputSlot = 0;
    inputElementDescs[0].AlignedByteOffset = 0;
    inputElementDescs[0].InputSlotClass = D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA;
    inputElementDescs[0].InstanceDataStepRate = 0;

    inputElementDescs[1].SemanticName = "COLOR";
    inputElementDescs[1].SemanticIndex = 0;
    inputElementDescs[1].Format = DXGI_FORMAT_R32G32B32A32_FLOAT;
    inputElementDescs[1].InputSlot = 0;
    inputElementDescs[1].AlignedByteOffset = D3D12_APPEND_ALIGNED_ELEMENT;
    inputElementDescs[1].InputSlotClass = D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA;
    inputElementDescs[1].InstanceDataStepRate = 0;

    D3D12_INPUT_LAYOUT_DESC inputLayoutDesc = {};
    inputLayoutDesc.pInputElementDescs = inputElementDescs;
    inputLayoutDesc.NumElements = _countof(inputElementDescs);

    D3D12_RASTERIZER_DESC rasterizerDesc = {};
    rasterizerDesc.FillMode = D3D12_FILL_MODE_SOLID;
    rasterizerDesc.CullMode = D3D12_CULL_MODE_BACK;
    rasterizerDesc.FrontCounterClockwise = FALSE;
    rasterizerDesc.DepthBias = D3D12_DEFAULT_DEPTH_BIAS;
    rasterizerDesc.DepthBiasClamp = D3D12_DEFAULT_DEPTH_BIAS_CLAMP;
    rasterizerDesc.SlopeScaledDepthBias = D3D12_DEFAULT_SLOPE_SCALED_DEPTH_BIAS;
    rasterizerDesc.DepthClipEnable = TRUE;
    rasterizerDesc.MultisampleEnable = FALSE;
    rasterizerDesc.AntialiasedLineEnable = FALSE;
    rasterizerDesc.ForcedSampleCount = 0;
    rasterizerDesc.ConservativeRaster = D3D12_CONSERVATIVE_RASTERIZATION_MODE_OFF;

    D3D12_BLEND_DESC blendDesc = {};
    blendDesc.AlphaToCoverageEnable = FALSE;
    blendDesc.IndependentBlendEnable = FALSE;
    blendDesc.RenderTarget[0].BlendEnable = FALSE;
    blendDesc.RenderTarget[0].LogicOpEnable = FALSE;
    blendDesc.RenderTarget[0].SrcBlend = D3D12_BLEND_ONE;
    blendDesc.RenderTarget[0].DestBlend = D3D12_BLEND_ZERO;
    blendDesc.RenderTarget[0].BlendOp = D3D12_BLEND_OP_ADD;
    blendDesc.RenderTarget[0].SrcBlendAlpha = D3D12_BLEND_ONE;
    blendDesc.RenderTarget[0].DestBlendAlpha = D3D12_BLEND_ZERO;
    blendDesc.RenderTarget[0].BlendOpAlpha = D3D12_BLEND_OP_ADD;
    blendDesc.RenderTarget[0].LogicOp = D3D12_LOGIC_OP_NOOP;
    blendDesc.RenderTarget[0].RenderTargetWriteMask = D3D12_COLOR_WRITE_ENABLE_ALL;

    D3D12_DEPTH_STENCIL_DESC depthStencilDesc = {};
    depthStencilDesc.DepthEnable = TRUE;
    depthStencilDesc.DepthWriteMask = D3D12_DEPTH_WRITE_MASK_ALL;
    depthStencilDesc.DepthFunc = D3D12_COMPARISON_FUNC_LESS;
    depthStencilDesc.StencilEnable = FALSE;
    depthStencilDesc.StencilReadMask = D3D12_DEFAULT_STENCIL_READ_MASK;
    depthStencilDesc.StencilWriteMask = D3D12_DEFAULT_STENCIL_WRITE_MASK;

    D3D12_GRAPHICS_PIPELINE_STATE_DESC pipelineStateDesc = {};
    pipelineStateDesc.pRootSignature = rootSignature;
    pipelineStateDesc.VS = vertexShaderBytecode;
    pipelineStateDesc.PS = pixelShaderBytecode;
    pipelineStateDesc.BlendState = blendDesc;
    pipelineStateDesc.SampleMask = UINT_MAX;
    pipelineStateDesc.RasterizerState = rasterizerDesc;
    pipelineStateDesc.DepthStencilState = depthStencilDesc;
    pipelineStateDesc.InputLayout = inputLayoutDesc;
    pipelineStateDesc.PrimitiveTopologyType = D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE;
    pipelineStateDesc.NumRenderTargets = 1;
    pipelineStateDesc.RTVFormats[0] = DXGI_FORMAT_R8G8B8A8_UNORM;
    pipelineStateDesc.DSVFormat = DXGI_FORMAT_D24_UNORM_S8_UINT;
    pipelineStateDesc.SampleDesc.Count = 1;
    pipelineStateDesc.SampleDesc.Quality = 0;

    HRESULT result = device->CreateGraphicsPipelineState(&pipelineStateDesc, __uuidof(ID3D12PipelineState), reinterpret_cast<void**>(&pipelineState));

    vertexShaderBlob->Release();
    vertexShaderBlob = nullptr;

    pixelShaderBlob->Release();
    pixelShaderBlob = nullptr;

    if (FAILED(result))
    {
        return false;
    }

    return true;
}

bool CGameFramework::CreateSceneDirectResources()
{
    if (!scene)
    {
        return false;
    }

    WaitForGpuComplete();

    HRESULT result = commandAllocator->Reset();
    if (FAILED(result))
    {
        return false;
    }

    result = commandList->Reset(commandAllocator, nullptr);
    if (FAILED(result))
    {
        return false;
    }

    if (!scene->BuildDirectResources(device, commandList))
    {
        return false;
    }

    result = commandList->Close();
    if (FAILED(result))
    {
        return false;
    }

    ID3D12CommandList* commandLists[] = { commandList };
    commandQueue->ExecuteCommandLists(_countof(commandLists), commandLists);

    WaitForGpuComplete();

    scene->ReleaseDirectUploadBuffers();

    return true;
}

void CGameFramework::ChangeScene(ESceneType nextScene, bool buildDirectResources)
{
    WaitForGpuComplete();

    if (scene)
    {
        scene->ReleaseObjects();
        delete scene;
        scene = nullptr;
    }

    switch (nextScene)
    {
    case ESceneType::Start:
        scene = new CStartScene();
        break;

    case ESceneType::MapSelect:
        scene = new CMapSelectScene();
        break;

    case ESceneType::GameMap1:
    {
        GameSessionConfig config;
        config.mapNumber = 1;
        config.playerType = GetRandomPlayerType();
        scene = new CGameScene(config);
        break;
    }

    case ESceneType::GameMap2:
    {
        GameSessionConfig config;
        config.mapNumber = 2;
        config.playerType = GetRandomPlayerType();
        scene = new CGameScene(config);
        break;
    }

    case ESceneType::GameMap3:
    {
        GameSessionConfig config;
        config.mapNumber = 3;
        config.playerType = GetRandomPlayerType();
        scene = new CGameScene(config);
        break;
    }

    case ESceneType::End:
        scene = new CEndScene(true);
        break;

    case ESceneType::Defeat:
        scene = new CEndScene(false);
        break;

    case ESceneType::Exit:
        PostQuitMessage(0);
        return;

    default:
        return;
    }

    currentSceneType = nextScene;

    if (scene)
    {
        scene->BuildObjects(clientWidth, clientHeight);
    }

    if (buildDirectResources)
    {
        if (!CreateSceneDirectResources())
        {
            OutputDebugStringA("[ERROR] ChangeScene CreateSceneDirectResources failed\n");
        }
    }
}

void CGameFramework::CheckSceneChange()
{
    if (!scene)
    {
        return;
    }

    ESceneType requested = scene->GetRequestedScene();

    if (requested == ESceneType::None)
    {
        return;
    }

    scene->ClearRequestedScene();

    ChangeScene(requested, true);
}
