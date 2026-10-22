// 결과 화면 구현

#include "stdafx.h"
#include "EndScene.h"

#include "Camera.h"
#include "Mesh.h"
#include "Object.h"

CEndScene::CEndScene(bool victory)
    : isVictory(victory)
{
}

CEndScene::~CEndScene()
{
    ReleaseObjects();
}

void CEndScene::BuildObjects(int width, int height)
{
    clientWidth = width;
    clientHeight = (height <= 0) ? 1 : height;

    if (!camera)
    {
        camera = new CCamera();
        camera->SetViewport(clientWidth, clientHeight);
        camera->SetPosition(XMFLOAT3(0.0f, 3.0f, -18.0f));
        camera->SetTarget(XMFLOAT3(0.0f, 2.0f, 0.0f));
        camera->SetUp(XMFLOAT3(0.0f, 1.0f, 0.0f));
        camera->SetLens(60.0f, 0.1f, 1000.0f);
    }
}

void CEndScene::ReleaseObjects()
{
    if (endText)
    {
        delete endText;
        endText = nullptr;
    }

    if (camera)
    {
        delete camera;
        camera = nullptr;
    }

    if (sharedBlockMesh)
    {
        sharedBlockMesh->Release();
        sharedBlockMesh = nullptr;
    }
}

bool CEndScene::ProcessInput(UCHAR* keyBuffer)
{
    UNREFERENCED_PARAMETER(keyBuffer);
    return true;
}

void CEndScene::AnimateObjects(float deltaTime)
{
    if (endText)
    {
        XMFLOAT3 rotation = endText->GetRotation();

        rotation.y += 30.0f * deltaTime;

        endText->SetRotation(rotation.x, rotation.y, rotation.z);

        endText->Update(deltaTime);
    }
}

void CEndScene::OnResize(int width, int height)
{
    clientWidth = width;
    clientHeight = (height <= 0) ? 1 : height;

    if (camera)
    {
        camera->SetViewport(clientWidth, clientHeight);
    }
}

bool CEndScene::OnProcessingKeyboardMessage(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam)
{
    UNREFERENCED_PARAMETER(hWnd);
    UNREFERENCED_PARAMETER(message);
    UNREFERENCED_PARAMETER(wParam);
    UNREFERENCED_PARAMETER(lParam);

    return false;
}

bool CEndScene::OnProcessingMouseMessage(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam)
{
    UNREFERENCED_PARAMETER(hWnd);
    UNREFERENCED_PARAMETER(wParam);

    if (message == WM_LBUTTONDOWN)
    {
        int mouseX = LOWORD(lParam);
        int mouseY = HIWORD(lParam);

        if (PickEndText(mouseX, mouseY))
        {
            requestedScene = ESceneType::MapSelect;
            return true;
        }
    }

    return false;
}

bool CEndScene::BuildDirectResources(ID3D12Device* device, ID3D12GraphicsCommandList* commandList)
{
    if (!device || !commandList)
    {
        return false;
    }

    if (!sharedBlockMesh)
    {
        sharedBlockMesh = new CColorCubeMesh(device, commandList, 1.0f, 1.0f, 1.0f);
    }

    BuildEndText();

    return true;
}

void CEndScene::ReleaseDirectUploadBuffers()
{
    if (sharedBlockMesh)
    {
        sharedBlockMesh->ReleaseUploadBuffers();
    }
}

void CEndScene::RenderDirect(ID3D12GraphicsCommandList* commandList)
{
    if (!commandList || !camera)
    {
        return;
    }

    UpdateCameraShaderVariables(commandList);

    if (endText)
    {
        endText->RenderDirect(commandList);
    }
}

void CEndScene::BuildEndText()
{
    if (endText)
    {
        return;
    }

    endText = new CBlockTextObject();

    endText->SetBlockMesh(sharedBlockMesh);
    endText->SetBlockSize(1.0f);
    endText->SetBlockSpacing(1.2f);

    if (isVictory) {
        endText->SetBlockColor(XMFLOAT4(1.0f, 0.85f, 0.1f, 1.0f));

        endText->SetPosition(0.0f, 2.0f, 10.0f);
        endText->SetScale(0.45f, 0.45f, 0.45f);

        // YOU WIN!
        // Y
        endText->AddBlock(-23.0f, 3.0f, 0.0f);
        endText->AddBlock(-19.0f, 3.0f, 0.0f);
        endText->AddBlock(-22.0f, 2.0f, 0.0f);
        endText->AddBlock(-20.0f, 2.0f, 0.0f);
        endText->AddBlock(-21.0f, 1.0f, 0.0f);
        endText->AddBlock(-21.0f, 0.0f, 0.0f);
        endText->AddBlock(-21.0f, -1.0f, 0.0f);
        endText->AddBlock(-21.0f, -2.0f, 0.0f);
        endText->AddBlock(-21.0f, -3.0f, 0.0f);

        // O
        endText->AddBlock(-16.0f, 3.0f, 0.0f);
        endText->AddBlock(-15.0f, 3.0f, 0.0f);
        endText->AddBlock(-14.0f, 3.0f, 0.0f);
        endText->AddBlock(-17.0f, 2.0f, 0.0f);
        endText->AddBlock(-13.0f, 2.0f, 0.0f);
        endText->AddBlock(-17.0f, 1.0f, 0.0f);
        endText->AddBlock(-13.0f, 1.0f, 0.0f);
        endText->AddBlock(-17.0f, 0.0f, 0.0f);
        endText->AddBlock(-13.0f, 0.0f, 0.0f);
        endText->AddBlock(-17.0f, -1.0f, 0.0f);
        endText->AddBlock(-13.0f, -1.0f, 0.0f);
        endText->AddBlock(-17.0f, -2.0f, 0.0f);
        endText->AddBlock(-13.0f, -2.0f, 0.0f);
        endText->AddBlock(-16.0f, -3.0f, 0.0f);
        endText->AddBlock(-15.0f, -3.0f, 0.0f);
        endText->AddBlock(-14.0f, -3.0f, 0.0f);

        // U
        endText->AddBlock(-10.0f, 3.0f, 0.0f);
        endText->AddBlock(-6.0f, 3.0f, 0.0f);
        endText->AddBlock(-10.0f, 2.0f, 0.0f);
        endText->AddBlock(-6.0f, 2.0f, 0.0f);
        endText->AddBlock(-10.0f, 1.0f, 0.0f);
        endText->AddBlock(-6.0f, 1.0f, 0.0f);
        endText->AddBlock(-10.0f, 0.0f, 0.0f);
        endText->AddBlock(-6.0f, 0.0f, 0.0f);
        endText->AddBlock(-10.0f, -1.0f, 0.0f);
        endText->AddBlock(-6.0f, -1.0f, 0.0f);
        endText->AddBlock(-10.0f, -2.0f, 0.0f);
        endText->AddBlock(-6.0f, -2.0f, 0.0f);
        endText->AddBlock(-9.0f, -3.0f, 0.0f);
        endText->AddBlock(-8.0f, -3.0f, 0.0f);
        endText->AddBlock(-7.0f, -3.0f, 0.0f);

        // W
        endText->AddBlock(0.0f, 3.0f, 0.0f);
        endText->AddBlock(4.0f, 3.0f, 0.0f);
        endText->AddBlock(0.0f, 2.0f, 0.0f);
        endText->AddBlock(4.0f, 2.0f, 0.0f);
        endText->AddBlock(0.0f, 1.0f, 0.0f);
        endText->AddBlock(4.0f, 1.0f, 0.0f);
        endText->AddBlock(0.0f, 0.0f, 0.0f);
        endText->AddBlock(2.0f, 0.0f, 0.0f);
        endText->AddBlock(4.0f, 0.0f, 0.0f);
        endText->AddBlock(0.0f, -1.0f, 0.0f);
        endText->AddBlock(2.0f, -1.0f, 0.0f);
        endText->AddBlock(4.0f, -1.0f, 0.0f);
        endText->AddBlock(0.0f, -2.0f, 0.0f);
        endText->AddBlock(1.0f, -2.0f, 0.0f);
        endText->AddBlock(3.0f, -2.0f, 0.0f);
        endText->AddBlock(4.0f, -2.0f, 0.0f);
        endText->AddBlock(0.0f, -3.0f, 0.0f);
        endText->AddBlock(4.0f, -3.0f, 0.0f);

        // I
        endText->AddBlock(7.0f, 3.0f, 0.0f);
        endText->AddBlock(8.0f, 3.0f, 0.0f);
        endText->AddBlock(9.0f, 3.0f, 0.0f);
        endText->AddBlock(10.0f, 3.0f, 0.0f);
        endText->AddBlock(11.0f, 3.0f, 0.0f);
        endText->AddBlock(9.0f, 2.0f, 0.0f);
        endText->AddBlock(9.0f, 1.0f, 0.0f);
        endText->AddBlock(9.0f, 0.0f, 0.0f);
        endText->AddBlock(9.0f, -1.0f, 0.0f);
        endText->AddBlock(9.0f, -2.0f, 0.0f);
        endText->AddBlock(7.0f, -3.0f, 0.0f);
        endText->AddBlock(8.0f, -3.0f, 0.0f);
        endText->AddBlock(9.0f, -3.0f, 0.0f);
        endText->AddBlock(10.0f, -3.0f, 0.0f);
        endText->AddBlock(11.0f, -3.0f, 0.0f);

        // N
        endText->AddBlock(14.0f, 3.0f, 0.0f);
        endText->AddBlock(18.0f, 3.0f, 0.0f);
        endText->AddBlock(14.0f, 2.0f, 0.0f);
        endText->AddBlock(15.0f, 2.0f, 0.0f);
        endText->AddBlock(18.0f, 2.0f, 0.0f);
        endText->AddBlock(14.0f, 1.0f, 0.0f);
        endText->AddBlock(16.0f, 1.0f, 0.0f);
        endText->AddBlock(18.0f, 1.0f, 0.0f);
        endText->AddBlock(14.0f, 0.0f, 0.0f);
        endText->AddBlock(17.0f, 0.0f, 0.0f);
        endText->AddBlock(18.0f, 0.0f, 0.0f);
        endText->AddBlock(14.0f, -1.0f, 0.0f);
        endText->AddBlock(18.0f, -1.0f, 0.0f);
        endText->AddBlock(14.0f, -2.0f, 0.0f);
        endText->AddBlock(18.0f, -2.0f, 0.0f);
        endText->AddBlock(14.0f, -3.0f, 0.0f);
        endText->AddBlock(18.0f, -3.0f, 0.0f);

        // !
        endText->AddBlock(22.0f, 3.0f, 0.0f);
        endText->AddBlock(22.0f, 2.0f, 0.0f);
        endText->AddBlock(22.0f, 1.0f, 0.0f);
        endText->AddBlock(22.0f, 0.0f, 0.0f);
        endText->AddBlock(22.0f, -1.0f, 0.0f);
        endText->AddBlock(22.0f, -3.0f, 0.0f);

        endText->Update(0.0f);
    }
    else {
        endText->SetBlockColor(XMFLOAT4(1.0f, 0.1f, 0.1f, 1.0f));

        endText->SetPosition(0.0f, 2.0f, 10.0f);
        endText->SetScale(0.45f, 0.45f, 0.45f);

            // LOSE

            // L
            endText->AddBlock(-10.0f, 3.0f, 0.0f);
            endText->AddBlock(-10.0f, 2.0f, 0.0f);
            endText->AddBlock(-10.0f, 1.0f, 0.0f);
            endText->AddBlock(-10.0f, 0.0f, 0.0f);
            endText->AddBlock(-10.0f, -1.0f, 0.0f);
            endText->AddBlock(-9.0f, -1.0f, 0.0f);
            endText->AddBlock(-8.0f, -1.0f, 0.0f);

            // O
            endText->AddBlock(-5.0f, 3.0f, 0.0f);
            endText->AddBlock(-4.0f, 3.0f, 0.0f);
            endText->AddBlock(-3.0f, 3.0f, 0.0f);
            endText->AddBlock(-6.0f, 2.0f, 0.0f);
            endText->AddBlock(-2.0f, 2.0f, 0.0f);
            endText->AddBlock(-6.0f, 1.0f, 0.0f);
            endText->AddBlock(-2.0f, 1.0f, 0.0f);
            endText->AddBlock(-6.0f, 0.0f, 0.0f);
            endText->AddBlock(-2.0f, 0.0f, 0.0f);
            endText->AddBlock(-5.0f, -1.0f, 0.0f);
            endText->AddBlock(-4.0f, -1.0f, 0.0f);
            endText->AddBlock(-3.0f, -1.0f, 0.0f);

            // S
            endText->AddBlock(1.0f, 3.0f, 0.0f);
            endText->AddBlock(2.0f, 3.0f, 0.0f);
            endText->AddBlock(3.0f, 3.0f, 0.0f);
            endText->AddBlock(0.0f, 2.0f, 0.0f);
            endText->AddBlock(1.0f, 1.0f, 0.0f);
            endText->AddBlock(2.0f, 1.0f, 0.0f);
            endText->AddBlock(3.0f, 0.0f, 0.0f);
            endText->AddBlock(0.0f, -1.0f, 0.0f);
            endText->AddBlock(1.0f, -1.0f, 0.0f);
            endText->AddBlock(2.0f, -1.0f, 0.0f);

            // E
            endText->AddBlock(6.0f, 3.0f, 0.0f);
            endText->AddBlock(7.0f, 3.0f, 0.0f);
            endText->AddBlock(8.0f, 3.0f, 0.0f);
            endText->AddBlock(6.0f, 2.0f, 0.0f);
            endText->AddBlock(6.0f, 1.0f, 0.0f);
            endText->AddBlock(7.0f, 1.0f, 0.0f);
            endText->AddBlock(8.0f, 1.0f, 0.0f);
            endText->AddBlock(6.0f, 0.0f, 0.0f);
            endText->AddBlock(6.0f, -1.0f, 0.0f);
            endText->AddBlock(7.0f, -1.0f, 0.0f);
            endText->AddBlock(8.0f, -1.0f, 0.0f);
    }
}

bool CEndScene::PickEndText(int mouseX, int mouseY)
{
    return mouseX >= clientWidth / 2 - 260 &&
        mouseX <= clientWidth / 2 + 260 &&
        mouseY >= clientHeight / 2 - 120 &&
        mouseY <= clientHeight / 2 + 120;
}

void CEndScene::UpdateCameraShaderVariables(ID3D12GraphicsCommandList* commandList)
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
