// 프로젝트 공통 헤더 (미리 컴파일된 헤더)
// Windows / DirectX12 / DirectXMath 헤더와 벡터·행렬 계산 도우미를 모아 둔다.

#pragma once

#include "targetver.h"
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <stdlib.h>
#include <malloc.h>
#include <memory.h>
#include <tchar.h>

#include <string>
#include <vector>
#include <array>
#include <algorithm>
#include <cmath>
#include <cstdlib>
#include <cstdint>

#include <d3d12.h>
#include <dxgi1_4.h>
#include <D3Dcompiler.h>
#include <DirectXMath.h>
#include <DirectXColors.h>
#include <DirectXCollision.h>
#include <DXGIDebug.h>

#pragma comment(lib, "d3dcompiler.lib")
#pragma comment(lib, "d3d12.lib")
#pragma comment(lib, "dxgi.lib")
#pragma comment(lib, "dxguid.lib")

using namespace DirectX;

// 기본 창(클라이언트 영역) 크기
constexpr int FRAME_BUFFER_WIDTH = 1280;
constexpr int FRAME_BUFFER_HEIGHT = 720;

// CPU 데이터로 GPU 버퍼(정점/인덱스 버퍼 등)를 만든다.
// DEFAULT 힙이면 업로드 버퍼를 거쳐 복사하며, 업로드 버퍼는 ppd3dUploadBuffer로 돌려준다.
// 업로드 버퍼는 GPU 복사가 끝난 뒤(ReleaseUploadBuffers) 해제해야 한다.
extern ID3D12Resource* CreateBufferResource(ID3D12Device* pd3dDevice, ID3D12GraphicsCommandList* pd3dCommandList, void* pData, UINT nBytes, D3D12_HEAP_TYPE d3dHeapType = D3D12_HEAP_TYPE_UPLOAD, D3D12_RESOURCE_STATES d3dResourceStates = D3D12_RESOURCE_STATE_VERTEX_AND_CONSTANT_BUFFER, ID3D12Resource** ppd3dUploadBuffer = NULL);

// XMFLOAT3 계산 도우미
namespace Vector3
{
    inline XMFLOAT3 ToFloat3(FXMVECTOR vector)
    {
        XMFLOAT3 result;
        XMStoreFloat3(&result, vector);
        return result;
    }

    inline XMFLOAT3 Add(const XMFLOAT3& a, const XMFLOAT3& b)
    {
        return ToFloat3(XMLoadFloat3(&a) + XMLoadFloat3(&b));
    }

    inline XMFLOAT3 Add(const XMFLOAT3& a, const XMFLOAT3& b, float scale)
    {
        return ToFloat3(XMLoadFloat3(&a) + XMLoadFloat3(&b) * scale);
    }

    inline XMFLOAT3 Subtract(const XMFLOAT3& a, const XMFLOAT3& b)
    {
        return ToFloat3(XMLoadFloat3(&a) - XMLoadFloat3(&b));
    }

    inline XMFLOAT3 Multiply(const XMFLOAT3& v, float scalar)
    {
        return ToFloat3(XMLoadFloat3(&v) * scalar);
    }

    inline XMFLOAT3 Normalize(const XMFLOAT3& v)
    {
        return ToFloat3(XMVector3Normalize(XMLoadFloat3(&v)));
    }

    inline float Length(const XMFLOAT3& v)
    {
        return XMVectorGetX(XMVector3Length(XMLoadFloat3(&v)));
    }

    inline float Distance(const XMFLOAT3& a, const XMFLOAT3& b)
    {
        return XMVectorGetX(XMVector3Length(XMLoadFloat3(&a) - XMLoadFloat3(&b)));
    }

    inline float Dot(const XMFLOAT3& a, const XMFLOAT3& b)
    {
        return XMVectorGetX(XMVector3Dot(XMLoadFloat3(&a), XMLoadFloat3(&b)));
    }

    inline XMFLOAT3 Cross(const XMFLOAT3& a, const XMFLOAT3& b, bool normalize = true)
    {
        XMVECTOR cross = XMVector3Cross(XMLoadFloat3(&a), XMLoadFloat3(&b));
        if (normalize)
        {
            cross = XMVector3Normalize(cross);
        }
        return ToFloat3(cross);
    }

    inline XMFLOAT3 TransformCoord(const XMFLOAT3& v, CXMMATRIX matrix)
    {
        return ToFloat3(XMVector3TransformCoord(XMLoadFloat3(&v), matrix));
    }

    inline XMFLOAT3 TransformCoord(const XMFLOAT3& v, const XMFLOAT4X4& matrix)
    {
        return TransformCoord(v, XMLoadFloat4x4(&matrix));
    }

    inline XMFLOAT3 TransformNormal(const XMFLOAT3& v, CXMMATRIX matrix)
    {
        return ToFloat3(XMVector3TransformNormal(XMLoadFloat3(&v), matrix));
    }

    inline float Angle(const XMFLOAT3& a, const XMFLOAT3& b)
    {
        XMVECTOR na = XMVector3Normalize(XMLoadFloat3(&a));
        XMVECTOR nb = XMVector3Normalize(XMLoadFloat3(&b));
        float cosValue = XMVectorGetX(XMVector3Dot(na, nb));

        if (cosValue < -1.0f)
        {
            cosValue = -1.0f;
        }
        else if (cosValue > 1.0f)
        {
            cosValue = 1.0f;
        }

        return XMConvertToDegrees(std::acos(cosValue));
    }
}

// XMFLOAT4 계산 도우미
namespace Vector4
{
    inline XMFLOAT4 ToFloat4(FXMVECTOR vector)
    {
        XMFLOAT4 result;
        XMStoreFloat4(&result, vector);
        return result;
    }

    inline XMFLOAT4 Add(const XMFLOAT4& a, const XMFLOAT4& b)
    {
        return ToFloat4(XMLoadFloat4(&a) + XMLoadFloat4(&b));
    }

    inline XMFLOAT4 Multiply(const XMFLOAT4& a, const XMFLOAT4& b)
    {
        return ToFloat4(XMLoadFloat4(&a) * XMLoadFloat4(&b));
    }

    inline XMFLOAT4 Multiply(const XMFLOAT4& v, float scalar)
    {
        return ToFloat4(XMLoadFloat4(&v) * scalar);
    }
}

// XMFLOAT4X4 계산 도우미 (회전 각도와 시야각은 모두 도(degree) 단위)
namespace Matrix4x4
{
    inline XMFLOAT4X4 Identity()
    {
        XMFLOAT4X4 result;
        XMStoreFloat4x4(&result, XMMatrixIdentity());
        return result;
    }

    inline XMFLOAT4X4 Multiply(const XMFLOAT4X4& a, const XMFLOAT4X4& b)
    {
        XMFLOAT4X4 result;
        XMStoreFloat4x4(&result, XMLoadFloat4x4(&a) * XMLoadFloat4x4(&b));
        return result;
    }

    inline XMFLOAT4X4 Inverse(const XMFLOAT4X4& matrix)
    {
        XMFLOAT4X4 result;
        XMStoreFloat4x4(&result, XMMatrixInverse(nullptr, XMLoadFloat4x4(&matrix)));
        return result;
    }

    inline XMFLOAT4X4 Transpose(const XMFLOAT4X4& matrix)
    {
        XMFLOAT4X4 result;
        XMStoreFloat4x4(&result, XMMatrixTranspose(XMLoadFloat4x4(&matrix)));
        return result;
    }

    inline XMFLOAT4X4 Translation(float x, float y, float z)
    {
        XMFLOAT4X4 result;
        XMStoreFloat4x4(&result, XMMatrixTranslation(x, y, z));
        return result;
    }

    inline XMFLOAT4X4 Scaling(float x, float y, float z)
    {
        XMFLOAT4X4 result;
        XMStoreFloat4x4(&result, XMMatrixScaling(x, y, z));
        return result;
    }

    inline XMFLOAT4X4 RotationRollPitchYaw(float pitch, float yaw, float roll)
    {
        XMFLOAT4X4 result;
        XMStoreFloat4x4(&result, XMMatrixRotationRollPitchYaw(XMConvertToRadians(pitch), XMConvertToRadians(yaw), XMConvertToRadians(roll)));
        return result;
    }

    inline XMFLOAT4X4 LookAtLH(const XMFLOAT3& eye, const XMFLOAT3& lookAt, const XMFLOAT3& up)
    {
        XMFLOAT4X4 result;
        XMStoreFloat4x4(&result, XMMatrixLookAtLH(XMLoadFloat3(&eye), XMLoadFloat3(&lookAt), XMLoadFloat3(&up)));
        return result;
    }

    inline XMFLOAT4X4 PerspectiveFovLH(float fovAngleY, float aspectRatio, float nearZ, float farZ)
    {
        XMFLOAT4X4 result;
        XMStoreFloat4x4(&result, XMMatrixPerspectiveFovLH(XMConvertToRadians(fovAngleY), aspectRatio, nearZ, farZ));
        return result;
    }
}
