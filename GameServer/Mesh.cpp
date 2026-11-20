// 메시 구현

#include "stdafx.h"
#include "Mesh.h"

CColorCubeMesh::CColorCubeMesh(ID3D12Device* device, ID3D12GraphicsCommandList* commandList, float width, float height, float depth)
{
    float halfWidth = width * 0.5f;
    float halfHeight = height * 0.5f;
    float halfDepth = depth * 0.5f;

    XMFLOAT4 whiteColor(1.0f, 1.0f, 1.0f, 1.0f);

    CColorVertex vertices[8] =
    {
        { XMFLOAT3(-halfWidth, +halfHeight, -halfDepth), whiteColor },
        { XMFLOAT3(+halfWidth, +halfHeight, -halfDepth), whiteColor },
        { XMFLOAT3(+halfWidth, +halfHeight, +halfDepth), whiteColor },
        { XMFLOAT3(-halfWidth, +halfHeight, +halfDepth), whiteColor },

        { XMFLOAT3(-halfWidth, -halfHeight, -halfDepth), whiteColor },
        { XMFLOAT3(+halfWidth, -halfHeight, -halfDepth), whiteColor },
        { XMFLOAT3(+halfWidth, -halfHeight, +halfDepth), whiteColor },
        { XMFLOAT3(-halfWidth, -halfHeight, +halfDepth), whiteColor }
    };

    UINT16 indices[36] =
    {
        0, 1, 2,
        0, 2, 3,

        4, 6, 5,
        4, 7, 6,

        3, 2, 6,
        3, 6, 7,

        0, 5, 1,
        0, 4, 5,

        0, 3, 7,
        0, 7, 4,

        1, 5, 6,
        1, 6, 2
    };

    vertexCount = _countof(vertices);
    vertexStride = sizeof(CColorVertex);
    indexCount = _countof(indices);
    primitiveTopology = D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST;

    vertexBuffer = CreateBufferResource(device, commandList, vertices, sizeof(vertices), D3D12_HEAP_TYPE_DEFAULT, D3D12_RESOURCE_STATE_VERTEX_AND_CONSTANT_BUFFER, &vertexUploadBuffer);

    if (vertexBuffer)
    {
        vertexBufferView.BufferLocation = vertexBuffer->GetGPUVirtualAddress();
        vertexBufferView.StrideInBytes = sizeof(CColorVertex);
        vertexBufferView.SizeInBytes = sizeof(vertices);
    }

    indexBuffer = CreateBufferResource(device, commandList, indices, sizeof(indices), D3D12_HEAP_TYPE_DEFAULT, D3D12_RESOURCE_STATE_INDEX_BUFFER, &indexUploadBuffer);

    if (indexBuffer)
    {
        indexBufferView.BufferLocation = indexBuffer->GetGPUVirtualAddress();
        indexBufferView.Format = DXGI_FORMAT_R16_UINT;
        indexBufferView.SizeInBytes = sizeof(indices);
    }
}

CDirectMesh::~CDirectMesh()
{
    if (indexUploadBuffer)
    {
        indexUploadBuffer->Release();
        indexUploadBuffer = nullptr;
    }

    if (indexBuffer)
    {
        indexBuffer->Release();
        indexBuffer = nullptr;
    }

    if (vertexUploadBuffer)
    {
        vertexUploadBuffer->Release();
        vertexUploadBuffer = nullptr;
    }

    if (vertexBuffer)
    {
        vertexBuffer->Release();
        vertexBuffer = nullptr;
    }
}

void CDirectMesh::ReleaseUploadBuffers()
{
    if (vertexUploadBuffer)
    {
        vertexUploadBuffer->Release();
        vertexUploadBuffer = nullptr;
    }

    if (indexUploadBuffer)
    {
        indexUploadBuffer->Release();
        indexUploadBuffer = nullptr;
    }
}

void CDirectMesh::Render(ID3D12GraphicsCommandList* commandList)
{
    if (!commandList || !vertexBuffer)
    {
        return;
    }

    commandList->IASetPrimitiveTopology(primitiveTopology);
    commandList->IASetVertexBuffers(0, 1, &vertexBufferView);

    if (indexBuffer)
    {
        commandList->IASetIndexBuffer(&indexBufferView);

        commandList->DrawIndexedInstanced(indexCount, 1, 0, 0, 0);
    }
    else
    {
        commandList->DrawInstanced(vertexCount, 1, 0, 0);
    }
}
