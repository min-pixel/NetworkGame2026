// 시작 화면 구현 (블록 글자는 AddBlock 좌표로 한 칸씩 찍는다)

#include "stdafx.h"
#include "StartScene.h"

#include "Camera.h"
#include "Mesh.h"
#include "Object.h"

CStartScene::CStartScene()
{
}

CStartScene::~CStartScene()
{
    ReleaseObjects();
}

void CStartScene::BuildObjects(int width, int height)
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

void CStartScene::ReleaseObjects()
{
    ReleaseExplosionBlocks();

    if (startText)
    {
        delete startText;
        startText = nullptr;
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

bool CStartScene::ProcessInput(UCHAR* keyBuffer)
{
    UNREFERENCED_PARAMETER(keyBuffer);
    return true;
}

void CStartScene::AnimateObjects(float deltaTime)
{
    if (!isExploding)
    {
        if (startText)
        {
            XMFLOAT3 rot = startText->GetRotation();
            rot.y += 30.0f * deltaTime;
            startText->SetRotation(rot.x, rot.y, rot.z);
            startText->Update(deltaTime);
        }

        return;
    }

    UpdateExplosion(deltaTime);
}

void CStartScene::OnResize(int width, int height)
{
    clientWidth = width;
    clientHeight = (height <= 0) ? 1 : height;

    if (camera)
    {
        camera->SetViewport(clientWidth, clientHeight);
    }
}

bool CStartScene::OnProcessingKeyboardMessage(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam)
{
    UNREFERENCED_PARAMETER(hWnd);
    UNREFERENCED_PARAMETER(message);
    UNREFERENCED_PARAMETER(wParam);
    UNREFERENCED_PARAMETER(lParam);

    return false;
}

bool CStartScene::OnProcessingMouseMessage(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam)
{
    UNREFERENCED_PARAMETER(hWnd);
    UNREFERENCED_PARAMETER(wParam);

    if (message == WM_LBUTTONDOWN)
    {
        int mouseX = LOWORD(lParam);
        int mouseY = HIWORD(lParam);

        if (PickStartText(mouseX, mouseY))
        {
            StartExplosion();
            return true;
        }
    }

    return false;
}

bool CStartScene::BuildDirectResources(ID3D12Device* device, ID3D12GraphicsCommandList* commandList)
{
    if (!device || !commandList)
    {
        return false;
    }

    if (!sharedBlockMesh)
    {
        sharedBlockMesh = new CColorCubeMesh(device, commandList, 1.0f, 1.0f, 1.0f);
    }

    BuildStartText();

    return true;
}

void CStartScene::ReleaseDirectUploadBuffers()
{
    if (sharedBlockMesh)
    {
        sharedBlockMesh->ReleaseUploadBuffers();
    }
}

void CStartScene::RenderDirect(ID3D12GraphicsCommandList* commandList)
{
    if (!commandList || !camera)
    {
        return;
    }

    UpdateCameraShaderVariables(commandList);

    if (startText)
    {
        startText->RenderDirect(commandList);
    }

    for (FExplosionBlock& block : explosionBlocks)
    {
        if (block.object)
        {
            block.object->RenderDirect(commandList);
        }
    }
}

void CStartScene::BuildStartText()
{
    if (startText)
    {
        return;
    }

    startText = new CBlockTextObject();

    startText->SetBlockMesh(sharedBlockMesh);
    startText->SetBlockSize(1.0f);
    startText->SetBlockSpacing(1.2f);
    startText->SetBlockColor(XMFLOAT4(1.0f, 0.85f, 0.2f, 1.0f));

    startText->SetPosition(0.0f, 2.0f, 10.0f);

    startText->SetScale(0.5f, 0.5f, 0.5f);

    // 3D게임프로그래밍1

    // 3
    startText->AddBlock(-35.0f, 3.0f, 0.0f);
    startText->AddBlock(-34.0f, 3.0f, 0.0f);
    startText->AddBlock(-33.0f, 3.0f, 0.0f);
    startText->AddBlock(-32.0f, 3.0f, 0.0f);
    startText->AddBlock(-32.0f, 2.0f, 0.0f);
    startText->AddBlock(-32.0f, 1.0f, 0.0f);
    startText->AddBlock(-35.0f, 0.0f, 0.0f);
    startText->AddBlock(-34.0f, 0.0f, 0.0f);
    startText->AddBlock(-33.0f, 0.0f, 0.0f);
    startText->AddBlock(-32.0f, 0.0f, 0.0f);
    startText->AddBlock(-32.0f, -1.0f, 0.0f);
    startText->AddBlock(-32.0f, -2.0f, 0.0f);
    startText->AddBlock(-35.0f, -3.0f, 0.0f);
    startText->AddBlock(-34.0f, -3.0f, 0.0f);
    startText->AddBlock(-33.0f, -3.0f, 0.0f);
    startText->AddBlock(-32.0f, -3.0f, 0.0f);

    // D
    startText->AddBlock(-29.0f, 3.0f, 0.0f);
    startText->AddBlock(-28.0f, 3.0f, 0.0f);
    startText->AddBlock(-27.0f, 3.0f, 0.0f);
    startText->AddBlock(-26.0f, 3.0f, 0.0f);
    startText->AddBlock(-29.0f, 2.0f, 0.0f);
    startText->AddBlock(-25.0f, 2.0f, 0.0f);
    startText->AddBlock(-29.0f, 1.0f, 0.0f);
    startText->AddBlock(-25.0f, 1.0f, 0.0f);
    startText->AddBlock(-29.0f, 0.0f, 0.0f);
    startText->AddBlock(-25.0f, 0.0f, 0.0f);
    startText->AddBlock(-29.0f, -1.0f, 0.0f);
    startText->AddBlock(-25.0f, -1.0f, 0.0f);
    startText->AddBlock(-29.0f, -2.0f, 0.0f);
    startText->AddBlock(-25.0f, -2.0f, 0.0f);
    startText->AddBlock(-29.0f, -3.0f, 0.0f);
    startText->AddBlock(-28.0f, -3.0f, 0.0f);
    startText->AddBlock(-27.0f, -3.0f, 0.0f);
    startText->AddBlock(-26.0f, -3.0f, 0.0f);

    // 게
    startText->AddBlock(-22.0f, 3.0f, 0.0f);
    startText->AddBlock(-21.0f, 3.0f, 0.0f);
    startText->AddBlock(-20.0f, 3.0f, 0.0f);
    startText->AddBlock(-20.0f, 2.0f, 0.0f);
    startText->AddBlock(-20.0f, 1.0f, 0.0f);
    startText->AddBlock(-20.0f, 0.0f, 0.0f);
    startText->AddBlock(-18.0f, 3.0f, 0.0f);
    startText->AddBlock(-18.0f, 2.0f, 0.0f);
    startText->AddBlock(-18.0f, 1.0f, 0.0f);
    startText->AddBlock(-18.0f, 0.0f, 0.0f);
    startText->AddBlock(-18.0f, -1.0f, 0.0f);
    startText->AddBlock(-18.0f, -2.0f, 0.0f);
    startText->AddBlock(-18.0f, -3.0f, 0.0f);
    startText->AddBlock(-16.0f, 3.0f, 0.0f);
    startText->AddBlock(-16.0f, 2.0f, 0.0f);
    startText->AddBlock(-16.0f, 1.0f, 0.0f);
    startText->AddBlock(-16.0f, 0.0f, 0.0f);
    startText->AddBlock(-16.0f, -1.0f, 0.0f);
    startText->AddBlock(-16.0f, -2.0f, 0.0f);
    startText->AddBlock(-16.0f, -3.0f, 0.0f);
    startText->AddBlock(-19.0f, 1.0f, 0.0f);
    startText->AddBlock(-18.0f, 1.0f, 0.0f);
    startText->AddBlock(-16.0f, 1.0f, 0.0f);
    startText->AddBlock(-18.0f, -1.0f, 0.0f);
    startText->AddBlock(-16.0f, -1.0f, 0.0f);

    // 임
    startText->AddBlock(-12.0f, 3.0f, 0.0f);
    startText->AddBlock(-13.0f, 2.0f, 0.0f);
    startText->AddBlock(-11.0f, 2.0f, 0.0f);
    startText->AddBlock(-13.0f, 1.0f, 0.0f);
    startText->AddBlock(-11.0f, 1.0f, 0.0f);
    startText->AddBlock(-12.0f, 0.0f, 0.0f);
    startText->AddBlock(-9.0f, 3.0f, 0.0f);
    startText->AddBlock(-9.0f, 2.0f, 0.0f);
    startText->AddBlock(-9.0f, 1.0f, 0.0f);
    startText->AddBlock(-9.0f, 0.0f, 0.0f);
    startText->AddBlock(-12.0f, -2.0f, 0.0f);
    startText->AddBlock(-11.0f, -2.0f, 0.0f);
    startText->AddBlock(-10.0f, -2.0f, 0.0f);
    startText->AddBlock(-9.0f, -2.0f, 0.0f);
    startText->AddBlock(-12.0f, -3.0f, 0.0f);
    startText->AddBlock(-11.0f, -3.0f, 0.0f);
    startText->AddBlock(-10.0f, -3.0f, 0.0f);
    startText->AddBlock(-9.0f, -3.0f, 0.0f);

    // 프
    startText->AddBlock(-5.0f, 3.0f, 0.0f);
    startText->AddBlock(-4.0f, 3.0f, 0.0f);
    startText->AddBlock(-3.0f, 3.0f, 0.0f);
    startText->AddBlock(-2.0f, 3.0f, 0.0f);
    startText->AddBlock(-1.0f, 3.0f, 0.0f);
    startText->AddBlock(-5.0f, 2.0f, 0.0f);
    startText->AddBlock(-1.0f, 2.0f, 0.0f);
    startText->AddBlock(-5.0f, 1.0f, 0.0f);
    startText->AddBlock(-4.0f, 1.0f, 0.0f);
    startText->AddBlock(-3.0f, 1.0f, 0.0f);
    startText->AddBlock(-2.0f, 1.0f, 0.0f);
    startText->AddBlock(-1.0f, 1.0f, 0.0f);
    startText->AddBlock(-5.0f, 0.0f, 0.0f);
    startText->AddBlock(-1.0f, 0.0f, 0.0f);
    startText->AddBlock(-5.0f, -1.0f, 0.0f);
    startText->AddBlock(-4.0f, -1.0f, 0.0f);
    startText->AddBlock(-3.0f, -1.0f, 0.0f);
    startText->AddBlock(-2.0f, -1.0f, 0.0f);
    startText->AddBlock(-1.0f, -1.0f, 0.0f);
    startText->AddBlock(-5.0f, -3.0f, 0.0f);
    startText->AddBlock(-4.0f, -3.0f, 0.0f);
    startText->AddBlock(-3.0f, -3.0f, 0.0f);
    startText->AddBlock(-2.0f, -3.0f, 0.0f);
    startText->AddBlock(-1.0f, -3.0f, 0.0f);

    // 로
    startText->AddBlock(2.0f, 3.0f, 0.0f);
    startText->AddBlock(3.0f, 3.0f, 0.0f);
    startText->AddBlock(4.0f, 3.0f, 0.0f);
    startText->AddBlock(5.0f, 3.0f, 0.0f);
    startText->AddBlock(5.0f, 2.0f, 0.0f);
    startText->AddBlock(2.0f, 1.0f, 0.0f);
    startText->AddBlock(3.0f, 1.0f, 0.0f);
    startText->AddBlock(4.0f, 1.0f, 0.0f);
    startText->AddBlock(5.0f, 1.0f, 0.0f);
    startText->AddBlock(2.0f, 0.0f, 0.0f);
    startText->AddBlock(2.0f, -1.0f, 0.0f);
    startText->AddBlock(3.0f, -1.0f, 0.0f);
    startText->AddBlock(4.0f, -1.0f, 0.0f);
    startText->AddBlock(5.0f, -1.0f, 0.0f);
    startText->AddBlock(4.0f, -2.0f, 0.0f);
    startText->AddBlock(2.0f, -3.0f, 0.0f);
    startText->AddBlock(3.0f, -3.0f, 0.0f);
    startText->AddBlock(4.0f, -3.0f, 0.0f);
    startText->AddBlock(5.0f, -3.0f, 0.0f);
    startText->AddBlock(6.0f, -3.0f, 0.0f);

    // 그
    startText->AddBlock(9.0f, 3.0f, 0.0f);
    startText->AddBlock(10.0f, 3.0f, 0.0f);
    startText->AddBlock(11.0f, 3.0f, 0.0f);
    startText->AddBlock(12.0f, 3.0f, 0.0f);
    startText->AddBlock(12.0f, 2.0f, 0.0f);
    startText->AddBlock(12.0f, 1.0f, 0.0f);
    startText->AddBlock(12.0f, 0.0f, 0.0f);
    startText->AddBlock(9.0f, -3.0f, 0.0f);
    startText->AddBlock(10.0f, -3.0f, 0.0f);
    startText->AddBlock(11.0f, -3.0f, 0.0f);
    startText->AddBlock(12.0f, -3.0f, 0.0f);
    startText->AddBlock(13.0f, -3.0f, 0.0f);

    // 래
    startText->AddBlock(16.0f, 3.0f, 0.0f);
    startText->AddBlock(17.0f, 3.0f, 0.0f);
    startText->AddBlock(18.0f, 3.0f, 0.0f);
    startText->AddBlock(18.0f, 2.0f, 0.0f);
    startText->AddBlock(16.0f, 1.0f, 0.0f);
    startText->AddBlock(17.0f, 1.0f, 0.0f);
    startText->AddBlock(18.0f, 1.0f, 0.0f);
    startText->AddBlock(16.0f, 0.0f, 0.0f);
    startText->AddBlock(16.0f, -1.0f, 0.0f);
    startText->AddBlock(17.0f, -1.0f, 0.0f);
    startText->AddBlock(18.0f, -1.0f, 0.0f);
    startText->AddBlock(20.0f, 3.0f, 0.0f);
    startText->AddBlock(20.0f, 2.0f, 0.0f);
    startText->AddBlock(20.0f, 1.0f, 0.0f);
    startText->AddBlock(20.0f, 0.0f, 0.0f);
    startText->AddBlock(20.0f, -1.0f, 0.0f);
    startText->AddBlock(20.0f, -2.0f, 0.0f);
    startText->AddBlock(20.0f, -3.0f, 0.0f);
    startText->AddBlock(22.0f, 3.0f, 0.0f);
    startText->AddBlock(22.0f, 2.0f, 0.0f);
    startText->AddBlock(22.0f, 1.0f, 0.0f);
    startText->AddBlock(22.0f, 0.0f, 0.0f);
    startText->AddBlock(22.0f, -1.0f, 0.0f);
    startText->AddBlock(22.0f, -2.0f, 0.0f);
    startText->AddBlock(22.0f, -3.0f, 0.0f);
    startText->AddBlock(21.0f, 0.0f, 0.0f);

    // 밍
    startText->AddBlock(25.0f, 3.0f, 0.0f);
    startText->AddBlock(26.0f, 3.0f, 0.0f);
    startText->AddBlock(27.0f, 3.0f, 0.0f);
    startText->AddBlock(25.0f, 2.0f, 0.0f);
    startText->AddBlock(27.0f, 2.0f, 0.0f);
    startText->AddBlock(25.0f, 1.0f, 0.0f);
    startText->AddBlock(27.0f, 1.0f, 0.0f);
    startText->AddBlock(25.0f, 0.0f, 0.0f);
    startText->AddBlock(26.0f, 0.0f, 0.0f);
    startText->AddBlock(27.0f, 0.0f, 0.0f);
    startText->AddBlock(29.0f, 3.0f, 0.0f);
    startText->AddBlock(29.0f, 2.0f, 0.0f);
    startText->AddBlock(29.0f, 1.0f, 0.0f);
    startText->AddBlock(29.0f, 0.0f, 0.0f);
    startText->AddBlock(26.0f, -2.0f, 0.0f);
    startText->AddBlock(27.0f, -2.0f, 0.0f);
    startText->AddBlock(28.0f, -2.0f, 0.0f);
    startText->AddBlock(29.0f, -2.0f, 0.0f);
    startText->AddBlock(26.0f, -3.0f, 0.0f);
    startText->AddBlock(27.0f, -3.0f, 0.0f);
    startText->AddBlock(28.0f, -3.0f, 0.0f);
    startText->AddBlock(29.0f, -3.0f, 0.0f);

    // 1
    startText->AddBlock(34.0f, 3.0f, 0.0f);
    startText->AddBlock(33.0f, 2.0f, 0.0f);
    startText->AddBlock(34.0f, 2.0f, 0.0f);
    startText->AddBlock(34.0f, 1.0f, 0.0f);
    startText->AddBlock(34.0f, 0.0f, 0.0f);
    startText->AddBlock(34.0f, -1.0f, 0.0f);
    startText->AddBlock(34.0f, -2.0f, 0.0f);
    startText->AddBlock(33.0f, -3.0f, 0.0f);
    startText->AddBlock(34.0f, -3.0f, 0.0f);
    startText->AddBlock(35.0f, -3.0f, 0.0f);

    // 윤민혁

    // 윤
    // ㅇ
    startText->AddBlock(-11.0f, -8.0f, 0.0f);
    startText->AddBlock(-10.0f, -8.0f, 0.0f);
    startText->AddBlock(-9.0f, -8.0f, 0.0f);
    startText->AddBlock(-12.0f, -9.0f, 0.0f);
    startText->AddBlock(-8.0f, -9.0f, 0.0f);
    startText->AddBlock(-12.0f, -10.0f, 0.0f);
    startText->AddBlock(-8.0f, -10.0f, 0.0f);
    startText->AddBlock(-11.0f, -11.0f, 0.0f);
    startText->AddBlock(-10.0f, -11.0f, 0.0f);
    startText->AddBlock(-9.0f, -11.0f, 0.0f);

    // ㅠ
    startText->AddBlock(-12.0f, -12.0f, 0.0f);
    startText->AddBlock(-11.0f, -12.0f, 0.0f);
    startText->AddBlock(-10.0f, -12.0f, 0.0f);
    startText->AddBlock(-9.0f, -12.0f, 0.0f);
    startText->AddBlock(-8.0f, -12.0f, 0.0f);
    startText->AddBlock(-11.0f, -13.0f, 0.0f);
    startText->AddBlock(-11.0f, -14.0f, 0.0f);
    startText->AddBlock(-9.0f, -13.0f, 0.0f);
    startText->AddBlock(-9.0f, -14.0f, 0.0f);

    // ㄴ
    startText->AddBlock(-12.0f, -15.0f, 0.0f);
    startText->AddBlock(-12.0f, -16.0f, 0.0f);
    startText->AddBlock(-11.0f, -16.0f, 0.0f);
    startText->AddBlock(-10.0f, -16.0f, 0.0f);
    startText->AddBlock(-9.0f, -16.0f, 0.0f);
    startText->AddBlock(-8.0f, -16.0f, 0.0f);

    // 민
    // ㅁ
    startText->AddBlock(-3.0f, -8.0f, 0.0f);
    startText->AddBlock(-2.0f, -8.0f, 0.0f);
    startText->AddBlock(-1.0f, -8.0f, 0.0f);
    startText->AddBlock(-3.0f, -9.0f, 0.0f);
    startText->AddBlock(-1.0f, -9.0f, 0.0f);
    startText->AddBlock(-3.0f, -10.0f, 0.0f);
    startText->AddBlock(-1.0f, -10.0f, 0.0f);
    startText->AddBlock(-3.0f, -11.0f, 0.0f);
    startText->AddBlock(-2.0f, -11.0f, 0.0f);
    startText->AddBlock(-1.0f, -11.0f, 0.0f);

    // ㅣ
    startText->AddBlock(1.0f, -8.0f, 0.0f);
    startText->AddBlock(1.0f, -9.0f, 0.0f);
    startText->AddBlock(1.0f, -10.0f, 0.0f);
    startText->AddBlock(1.0f, -11.0f, 0.0f);

    // ㄴ
    startText->AddBlock(-3.0f, -14.0f, 0.0f);
    startText->AddBlock(-3.0f, -15.0f, 0.0f);
    startText->AddBlock(-2.0f, -15.0f, 0.0f);
    startText->AddBlock(-1.0f, -15.0f, 0.0f);
    startText->AddBlock(0.0f, -15.0f, 0.0f);
    startText->AddBlock(1.0f, -15.0f, 0.0f);

    // 혁
    // ㅎ
    startText->AddBlock(7.0f, -8.0f, 0.0f);
    startText->AddBlock(8.0f, -8.0f, 0.0f);
    startText->AddBlock(9.0f, -8.0f, 0.0f);
    startText->AddBlock(7.0f, -10.0f, 0.0f);
    startText->AddBlock(8.0f, -10.0f, 0.0f);
    startText->AddBlock(9.0f, -10.0f, 0.0f);
    startText->AddBlock(7.0f, -11.0f, 0.0f);
    startText->AddBlock(9.0f, -11.0f, 0.0f);
    startText->AddBlock(7.0f, -12.0f, 0.0f);
    startText->AddBlock(9.0f, -12.0f, 0.0f);
    startText->AddBlock(8.0f, -13.0f, 0.0f);

    // ㅕ
    startText->AddBlock(13.0f, -8.0f, 0.0f);
    startText->AddBlock(13.0f, -9.0f, 0.0f);
    startText->AddBlock(13.0f, -10.0f, 0.0f);
    startText->AddBlock(13.0f, -11.0f, 0.0f);
    startText->AddBlock(13.0f, -12.0f, 0.0f);
    startText->AddBlock(13.0f, -13.0f, 0.0f);
    startText->AddBlock(11.0f, -9.0f, 0.0f);
    startText->AddBlock(12.0f, -9.0f, 0.0f);
    startText->AddBlock(11.0f, -11.0f, 0.0f);
    startText->AddBlock(12.0f, -11.0f, 0.0f);

    // ㄱ
    startText->AddBlock(7.0f, -15.0f, 0.0f);
    startText->AddBlock(8.0f, -15.0f, 0.0f);
    startText->AddBlock(9.0f, -15.0f, 0.0f);
    startText->AddBlock(10.0f, -15.0f, 0.0f);
    startText->AddBlock(11.0f, -15.0f, 0.0f);
    startText->AddBlock(12.0f, -15.0f, 0.0f);
    startText->AddBlock(13.0f, -15.0f, 0.0f);
    startText->AddBlock(13.0f, -16.0f, 0.0f);

    startText->Update(0.0f);
}

bool CStartScene::PickStartText(int mouseX, int mouseY)
{
    return mouseX >= clientWidth / 2 - 210 &&
        mouseX <= clientWidth / 2 + 210 &&
        mouseY >= clientHeight / 2 + 80 &&
        mouseY <= clientHeight / 2 + 250;
}

void CStartScene::UpdateCameraShaderVariables(ID3D12GraphicsCommandList* commandList)
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

void CStartScene::StartExplosion()
{
    if (isExploding)
    {
        return;
    }

    if (!sharedBlockMesh)
    {
        requestedScene = ESceneType::MapSelect;
        return;
    }

    isExploding = true;
    explosionTimer = 0.0f;

    if (startText)
    {
        startText->SetActive(false);
    }

    ReleaseExplosionBlocks();

    const int verticalCount = 18;
    const int horizontalCount = 36;

    const float minPitch = -70.0f;
    const float maxPitch = 70.0f;

    const float speed = 18.0f;
    const float blockScale = 0.18f;

    XMFLOAT3 center(0.0f, 1.5f, 10.0f);

    if (startText)
    {
        center = startText->GetPosition();
    }

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

            block->SetDirectMesh(sharedBlockMesh);
            block->SetScale(blockScale, blockScale, blockScale);

            block->SetPosition(center.x, center.y, center.z);

            block->SetColor(XMFLOAT4(1.0f, 0.45f, 0.1f, 1.0f));

            block->Update(0.0f);

            FExplosionBlock explosionBlock;
            explosionBlock.object = block;

            explosionBlock.velocity = XMFLOAT3(direction.x * speed, direction.y * speed, direction.z * speed);

            explosionBlocks.push_back(explosionBlock);
        }
    }
}

void CStartScene::UpdateExplosion(float deltaTime)
{
    explosionTimer += deltaTime;

    for (FExplosionBlock& block : explosionBlocks)
    {
        if (!block.object)
        {
            continue;
        }

        XMFLOAT3 position = block.object->GetPosition();

        position.x += block.velocity.x * deltaTime;
        position.y += block.velocity.y * deltaTime;
        position.z += block.velocity.z * deltaTime;

        block.object->SetPosition(position.x, position.y, position.z);
        block.object->Update(deltaTime);
    }

    if (explosionTimer >= explosionDuration)
    {
        requestedScene = ESceneType::MapSelect;
    }
}

void CStartScene::ReleaseExplosionBlocks()
{
    for (FExplosionBlock& block : explosionBlocks)
    {
        delete block.object;
        block.object = nullptr;
    }

    explosionBlocks.clear();
}
