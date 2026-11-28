// 맵 선택 화면 구현

#include "stdafx.h"
#include "MapSelectScene.h"

#include "Camera.h"
#include "Mesh.h"
#include "Object.h"
#include "NetworkClient.h"

#include <random>

CMapSelectScene::CMapSelectScene()
{
}

CMapSelectScene::~CMapSelectScene()
{
    ReleaseObjects();
}

void CMapSelectScene::BuildObjects(int width, int height)
{
    clientWidth = width;
    clientHeight = (height <= 0) ? 1 : height;

    if (!camera)
    {
        camera = new CCamera();
        camera->SetViewport(clientWidth, clientHeight);
        camera->SetPosition(XMFLOAT3(0.0f, 3.0f, -15.0f));
        camera->SetTarget(XMFLOAT3(0.0f, 2.0f, 0.0f));
        camera->SetUp(XMFLOAT3(0.0f, 1.0f, 0.0f));
        camera->SetLens(60.0f, 0.1f, 1000.0f);
    }
}

void CMapSelectScene::SetNetworkClient(CNetworkClient* client)
{
    networkClient = client;
}

void CMapSelectScene::OnNetEvent(const NetEvent& event)
{
    if (event.type == EPacketType::SC_LobbyState)
    {
        SetLobbyState(event.lobbyState);
    }
}

void CMapSelectScene::SetLobbyState(const SC_LobbyState& state)
{
    if (state.playerCount > MAX_PLAYERS)
    {
        return;
    }

    lobbyState = state;
    hasLobbyState = true;

    bool myPlayerFound = false;
    bool myPlayerReady = false;

    if (networkClient != nullptr)
    {
        uint8_t myId = networkClient->GetMyPlayerId();

        for (int i = 0; i < lobbyState.playerCount; ++i)
        {
            if (lobbyState.players[i].playerId == myId)
            {
                myPlayerFound = true;
                myPlayerReady = lobbyState.players[i].ready != 0;
                break;
            }
        }
    }

    if (myPlayerReady)
    {
        readySent = true;
        readyConfirmed = true;
    }
    else if (!myPlayerFound || readyConfirmed)
    {
        readySent = false;
        readyConfirmed = false;
    }

    UpdateLobbyIndicators();
    UpdateReadyButtonColor();
}

void CMapSelectScene::ReleaseObjects()
{
    if (numberOne)
    {
        delete numberOne;
        numberOne = nullptr;
    }

    if (numberTwo)
    {
        delete numberTwo;
        numberTwo = nullptr;
    }

    if (camera)
    {
        delete camera;
        camera = nullptr;
    }

    for (int i = 0; i < MAX_PLAYERS; ++i)
    {
        delete lobbyIndicators[i];
        lobbyIndicators[i] = nullptr;
    }

    if (sharedBlockMesh)
    {
        sharedBlockMesh->Release();
        sharedBlockMesh = nullptr;
    }
}

bool CMapSelectScene::ProcessInput(UCHAR* keyBuffer)
{
    UNREFERENCED_PARAMETER(keyBuffer);
    return true;
}

void CMapSelectScene::AnimateObjects(float deltaTime)
{
    if (numberOne)
    {
        XMFLOAT3 rot = numberOne->GetRotation();
        rot.y += 30.0f * deltaTime;
        numberOne->SetRotation(rot.x, rot.y, rot.z);
        numberOne->Update(deltaTime);
    }

    if (numberTwo)
    {
        XMFLOAT3 rot = numberTwo->GetRotation();
        rot.y -= 30.0f * deltaTime;
        numberTwo->SetRotation(rot.x, rot.y, rot.z);
        numberTwo->Update(deltaTime);
    }
}

void CMapSelectScene::OnResize(int width, int height)
{
    clientWidth = width;
    clientHeight = (height <= 0) ? 1 : height;

    if (camera)
    {
        camera->SetViewport(clientWidth, clientHeight);
    }
}

bool CMapSelectScene::OnProcessingKeyboardMessage(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam)
{
    UNREFERENCED_PARAMETER(hWnd);
    UNREFERENCED_PARAMETER(message);
    UNREFERENCED_PARAMETER(wParam);
    UNREFERENCED_PARAMETER(lParam);

    return false;
}

bool CMapSelectScene::OnProcessingMouseMessage(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam)
{
    UNREFERENCED_PARAMETER(hWnd);
    UNREFERENCED_PARAMETER(wParam);

    if (message == WM_LBUTTONDOWN)
    {
        int mouseX = LOWORD(lParam);
        int mouseY = HIWORD(lParam);

        if (PickNumberOne(mouseX, mouseY))
        {
            if (!readySent && networkClient != nullptr && networkClient->SendReady())
            {
                readySent = true;
                UpdateReadyButtonColor();
                OutputDebugStringA("[CLIENT] CS_Ready sent\n");
            }

            return true;
        }

        if (PickNumberTwo(mouseX, mouseY))
        {
            requestedScene = ESceneType::Exit;
            return true;
        }
    }

    return false;
}

bool CMapSelectScene::BuildDirectResources(ID3D12Device* device, ID3D12GraphicsCommandList* commandList)
{
    if (!device || !commandList)
    {
        return false;
    }

    if (!sharedBlockMesh)
    {
        sharedBlockMesh = new CColorCubeMesh(device, commandList, 1.0f, 1.0f, 1.0f);
    }

    BuildNumberOne();
    BuildNumberTwo();
    BuildLobbyIndicators();
    UpdateReadyButtonColor();

    return true;
}

void CMapSelectScene::ReleaseDirectUploadBuffers()
{
    if (sharedBlockMesh)
    {
        sharedBlockMesh->ReleaseUploadBuffers();
    }
}

void CMapSelectScene::RenderDirect(ID3D12GraphicsCommandList* commandList)
{
    if (!commandList || !camera)
    {
        return;
    }

    UpdateCameraShaderVariables(commandList);

    if (numberOne)
    {
        numberOne->RenderDirect(commandList);
    }

    if (numberTwo)
    {
        numberTwo->RenderDirect(commandList);
    }

    for (int i = 0; i < MAX_PLAYERS; ++i)
    {
        if (lobbyIndicators[i])
        {
            lobbyIndicators[i]->RenderDirect(commandList);
        }
    }
}

void CMapSelectScene::BuildLobbyIndicators()
{
    for (int i = 0; i < MAX_PLAYERS; ++i)
    {
        if (!lobbyIndicators[i])
        {
            lobbyIndicators[i] = new CGameObject();
            lobbyIndicators[i]->SetDirectMesh(sharedBlockMesh);
            lobbyIndicators[i]->SetScale(0.7f, 0.7f, 0.7f);
            lobbyIndicators[i]->SetPosition(-3.0f + 2.0f * i, 4.0f, 10.0f);
            lobbyIndicators[i]->Update(0.0f);
        }
    }

    UpdateLobbyIndicators();
}

void CMapSelectScene::UpdateLobbyIndicators()
{
    for (int i = 0; i < MAX_PLAYERS; ++i)
    {
        if (!lobbyIndicators[i])
        {
            continue;
        }

        XMFLOAT4 color(0.3f, 0.3f, 0.3f, 1.0f);

        if (hasLobbyState)
        {
            for (int j = 0; j < lobbyState.playerCount; ++j)
            {
                if (lobbyState.players[j].playerId == i)
                {
                    color = lobbyState.players[j].ready != 0
                        ? XMFLOAT4(0.2f, 0.9f, 0.3f, 1.0f)
                        : XMFLOAT4(1.0f, 0.7f, 0.1f, 1.0f);
                    break;
                }
            }
        }

        lobbyIndicators[i]->SetColor(color);
    }
}

void CMapSelectScene::UpdateReadyButtonColor()
{
    if (!numberOne)
    {
        return;
    }

    XMFLOAT4 color = readySent
        ? XMFLOAT4(0.2f, 0.9f, 0.3f, 1.0f)
        : XMFLOAT4(0.3f, 0.8f, 1.0f, 1.0f);

    for (CGameObject* block : numberOne->GetChildren())
    {
        if (block)
        {
            block->SetColor(color);
        }
    }
}

void CMapSelectScene::BuildNumberOne()
{
    if (numberOne)
    {
        return;
    }

    numberOne = new CBlockTextObject();
    numberOne->SetBlockMesh(sharedBlockMesh);
    numberOne->SetBlockSize(1.0f);
    numberOne->SetBlockSpacing(1.2f);
    numberOne->SetBlockColor(XMFLOAT4(0.3f, 0.8f, 1.0f, 1.0f));

    numberOne->SetPosition(-4.0f, 2.0f, 10.0f);
    numberOne->SetScale(0.13f, 0.13f, 0.13f);

    // Start
    numberOne->AddBlock(-9.0f, -20.0f, 0.0f);
    numberOne->AddBlock(-8.0f, -20.0f, 0.0f);
    numberOne->AddBlock(-7.0f, -20.0f, 0.0f);
    numberOne->AddBlock(-9.0f, -21.0f, 0.0f);
    numberOne->AddBlock(-7.0f, -21.0f, 0.0f);
    numberOne->AddBlock(-9.0f, -22.0f, 0.0f);
    numberOne->AddBlock(-8.0f, -22.0f, 0.0f);
    numberOne->AddBlock(-7.0f, -22.0f, 0.0f);
    numberOne->AddBlock(-9.0f, -23.0f, 0.0f);
    numberOne->AddBlock(-8.0f, -23.0f, 0.0f);
    numberOne->AddBlock(-9.0f, -24.0f, 0.0f);
    numberOne->AddBlock(-7.0f, -24.0f, 0.0f);

    numberOne->AddBlock(-5.0f, -20.0f, 0.0f);
    numberOne->AddBlock(-4.0f, -20.0f, 0.0f);
    numberOne->AddBlock(-3.0f, -20.0f, 0.0f);
    numberOne->AddBlock(-5.0f, -21.0f, 0.0f);
    numberOne->AddBlock(-5.0f, -22.0f, 0.0f);
    numberOne->AddBlock(-4.0f, -22.0f, 0.0f);
    numberOne->AddBlock(-5.0f, -23.0f, 0.0f);
    numberOne->AddBlock(-5.0f, -24.0f, 0.0f);
    numberOne->AddBlock(-4.0f, -24.0f, 0.0f);
    numberOne->AddBlock(-3.0f, -24.0f, 0.0f);

    numberOne->AddBlock(0.0f, -20.0f, 0.0f);
    numberOne->AddBlock(-1.0f, -21.0f, 0.0f);
    numberOne->AddBlock(1.0f, -21.0f, 0.0f);
    numberOne->AddBlock(-1.0f, -22.0f, 0.0f);
    numberOne->AddBlock(0.0f, -22.0f, 0.0f);
    numberOne->AddBlock(1.0f, -22.0f, 0.0f);
    numberOne->AddBlock(-1.0f, -23.0f, 0.0f);
    numberOne->AddBlock(1.0f, -23.0f, 0.0f);
    numberOne->AddBlock(-1.0f, -24.0f, 0.0f);
    numberOne->AddBlock(1.0f, -24.0f, 0.0f);

    numberOne->AddBlock(3.0f, -20.0f, 0.0f);
    numberOne->AddBlock(4.0f, -20.0f, 0.0f);
    numberOne->AddBlock(3.0f, -21.0f, 0.0f);
    numberOne->AddBlock(5.0f, -21.0f, 0.0f);
    numberOne->AddBlock(3.0f, -22.0f, 0.0f);
    numberOne->AddBlock(5.0f, -22.0f, 0.0f);
    numberOne->AddBlock(3.0f, -23.0f, 0.0f);
    numberOne->AddBlock(5.0f, -23.0f, 0.0f);
    numberOne->AddBlock(3.0f, -24.0f, 0.0f);
    numberOne->AddBlock(4.0f, -24.0f, 0.0f);

    numberOne->AddBlock(7.0f, -20.0f, 0.0f);
    numberOne->AddBlock(9.0f, -20.0f, 0.0f);
    numberOne->AddBlock(7.0f, -21.0f, 0.0f);
    numberOne->AddBlock(9.0f, -21.0f, 0.0f);
    numberOne->AddBlock(8.0f, -22.0f, 0.0f);
    numberOne->AddBlock(8.0f, -23.0f, 0.0f);
    numberOne->AddBlock(8.0f, -24.0f, 0.0f);

    numberOne->Update(0.0f);
}

void CMapSelectScene::BuildNumberTwo()
{
    if (numberTwo)
    {
        return;
    }

    numberTwo = new CBlockTextObject();
    numberTwo->SetBlockMesh(sharedBlockMesh);
    numberTwo->SetBlockSize(1.0f);
    numberTwo->SetBlockSpacing(1.2f);
    numberTwo->SetBlockColor(XMFLOAT4(1.0f, 0.5f, 0.3f, 1.0f));

    numberTwo->SetPosition(4.0f, 2.0f, 10.0f);
    numberTwo->SetScale(0.5f, 0.5f, 0.5f);

    // End
    // E
    numberTwo->AddBlock(-5.0f, -4.0f, 0.0f);
    numberTwo->AddBlock(-4.0f, -4.0f, 0.0f);
    numberTwo->AddBlock(-3.0f, -4.0f, 0.0f);
    numberTwo->AddBlock(-5.0f, -5.0f, 0.0f);
    numberTwo->AddBlock(-5.0f, -6.0f, 0.0f);
    numberTwo->AddBlock(-4.0f, -6.0f, 0.0f);
    numberTwo->AddBlock(-3.0f, -6.0f, 0.0f);
    numberTwo->AddBlock(-5.0f, -7.0f, 0.0f);
    numberTwo->AddBlock(-5.0f, -8.0f, 0.0f);
    numberTwo->AddBlock(-4.0f, -8.0f, 0.0f);
    numberTwo->AddBlock(-3.0f, -8.0f, 0.0f);

    // n
    numberTwo->AddBlock(0.0f, -6.0f, 0.0f);
    numberTwo->AddBlock(-1.0f, -7.0f, 0.0f);
    numberTwo->AddBlock(1.0f, -7.0f, 0.0f);
    numberTwo->AddBlock(-1.0f, -8.0f, 0.0f);
    numberTwo->AddBlock(1.0f, -8.0f, 0.0f);

    // D
    numberTwo->AddBlock(3.0f, -4.0f, 0.0f);
    numberTwo->AddBlock(4.0f, -4.0f, 0.0f);
    numberTwo->AddBlock(5.0f, -4.0f, 0.0f);
    numberTwo->AddBlock(3.0f, -5.0f, 0.0f);
    numberTwo->AddBlock(6.0f, -5.0f, 0.0f);
    numberTwo->AddBlock(3.0f, -6.0f, 0.0f);
    numberTwo->AddBlock(6.0f, -6.0f, 0.0f);
    numberTwo->AddBlock(3.0f, -7.0f, 0.0f);
    numberTwo->AddBlock(6.0f, -7.0f, 0.0f);
    numberTwo->AddBlock(3.0f, -8.0f, 0.0f);
    numberTwo->AddBlock(4.0f, -8.0f, 0.0f);
    numberTwo->AddBlock(5.0f, -8.0f, 0.0f);

    numberTwo->Update(0.0f);
}

bool CMapSelectScene::PickNumberOne(int mouseX, int mouseY)
{
    return mouseX >= clientWidth / 2 - 170 &&
        mouseX <= clientWidth / 2 - 35 &&
        mouseY >= clientHeight / 2 + 50 &&
        mouseY <= clientHeight / 2 + 105;
}

bool CMapSelectScene::PickNumberTwo(int mouseX, int mouseY)
{
    return mouseX >= clientWidth / 2 + 10 &&
        mouseX <= clientWidth / 2 + 205 &&
        mouseY >= clientHeight / 2 + 30 &&
        mouseY <= clientHeight / 2 + 115;
}

void CMapSelectScene::UpdateCameraShaderVariables(ID3D12GraphicsCommandList* commandList)
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
