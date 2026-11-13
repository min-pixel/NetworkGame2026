// 메시 (정점/인덱스 버퍼)

#pragma once

#include "stdafx.h"

// 위치 + 색 정점 (셰이더 입력 레이아웃과 같아야 함)
struct CColorVertex
{
    XMFLOAT3 position;
    XMFLOAT4 color;
};

// GPU 정점/인덱스 버퍼를 가진 메시 기본 클래스
// 여러 오브젝트가 공유하므로 참조 카운트(AddRef/Release)로 관리한다.
class CDirectMesh
{
public:
    CDirectMesh() = default;
    virtual ~CDirectMesh();

    void AddRef() { ++referenceCount; }
    void Release() { if (--referenceCount <= 0) delete this; }

    virtual void ReleaseUploadBuffers();
    virtual void Render(ID3D12GraphicsCommandList* commandList);

protected:
    int referenceCount = 1;

    ID3D12Resource* vertexBuffer = nullptr;
    ID3D12Resource* vertexUploadBuffer = nullptr;
    D3D12_VERTEX_BUFFER_VIEW vertexBufferView = {};

    ID3D12Resource* indexBuffer = nullptr;
    ID3D12Resource* indexUploadBuffer = nullptr;
    D3D12_INDEX_BUFFER_VIEW indexBufferView = {};

    D3D12_PRIMITIVE_TOPOLOGY primitiveTopology = D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST;

    UINT vertexCount = 0;
    UINT vertexStride = 0;
    UINT indexCount = 0;
};

// 가로/세로/깊이를 지정한 흰색 큐브 (실제 색은 오브젝트 색으로 곱해짐)
class CColorCubeMesh : public CDirectMesh
{
public:
    CColorCubeMesh(ID3D12Device* device, ID3D12GraphicsCommandList* commandList, float width, float height, float depth);

    ~CColorCubeMesh() override = default;
};
