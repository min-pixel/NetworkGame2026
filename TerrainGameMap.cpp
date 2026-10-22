// 높이맵 지형 맵 구현

#include "stdafx.h"
#include "TerrainGameMap.h"

#include <cstdio>
#include <algorithm>

CHeightMapTerrain::CHeightMapTerrain()
{
}

CHeightMapTerrain::~CHeightMapTerrain()
{
}

bool CHeightMapTerrain::LoadRawHeightMap(const char* fileName, int mapWidth, int mapLength)
{
    if (!fileName || mapWidth <= 0 || mapLength <= 0)
    {
        return false;
    }

    width = mapWidth;
    length = mapLength;

    const size_t dataSize = static_cast<size_t>(width) *
        static_cast<size_t>(length);

    heightData.resize(dataSize);

    FILE* file = nullptr;
    errno_t error = fopen_s(&file, fileName, "rb");

    if (error != 0 || !file)
    {
        return false;
    }

    size_t readSize = fread(heightData.data(), sizeof(BYTE), dataSize, file);

    fclose(file);

    if (readSize != dataSize)
    {
        heightData.clear();
        return false;
    }

    auto minMax = std::minmax_element(heightData.begin(), heightData.end());

    minRawHeight = *minMax.first;
    maxRawHeight = *minMax.second;

    return true;
}

float CHeightMapTerrain::GetScaledHeightByIndex(int x, int z) const
{
    if (x < 0 || x >= width || z < 0 || z >= length)
    {
        return 0.0f;
    }

    int index = z * width + x;

    BYTE rawHeight = heightData[index];

    return static_cast<float>(rawHeight - minRawHeight) * heightScale;
}

bool CHeightMapTerrain::GetHeight(float worldX, float worldZ, float& outHeight) const
{
    if (heightData.empty() || width <= 1 || length <= 1)
    {
        return false;
    }

    float startX = -static_cast<float>(width - 1) * 0.5f * cellSpacing;

    float startZ = +static_cast<float>(length - 1) * 0.5f * cellSpacing;

    float mapX = (worldX - startX) / cellSpacing;
    float mapZ = (startZ - worldZ) / cellSpacing;

    if (mapX < 0.0f || mapZ < 0.0f ||
        mapX >= static_cast<float>(width - 1) ||
        mapZ >= static_cast<float>(length - 1))
    {
        return false;
    }

    int x0 = static_cast<int>(floorf(mapX));
    int z0 = static_cast<int>(floorf(mapZ));

    int x1 = x0 + 1;
    int z1 = z0 + 1;

    float tx = mapX - static_cast<float>(x0);
    float tz = mapZ - static_cast<float>(z0);

    float h00 = GetScaledHeightByIndex(x0, z0);
    float h10 = GetScaledHeightByIndex(x1, z0);
    float h01 = GetScaledHeightByIndex(x0, z1);
    float h11 = GetScaledHeightByIndex(x1, z1);

    float h0 = h00 * (1.0f - tx) + h10 * tx;
    float h1 = h01 * (1.0f - tx) + h11 * tx;

    outHeight = h0 * (1.0f - tz) + h1 * tz;

    return true;
}

CHeightMapTerrainMesh::CHeightMapTerrainMesh(ID3D12Device* device, ID3D12GraphicsCommandList* commandList, const std::vector<BYTE>& heightData, int width, int length, BYTE minRawHeight, BYTE maxRawHeight, float cellSpacing, float heightScale)
{
    if (!device || !commandList || heightData.empty() ||
        width <= 1 || length <= 1)
    {
        return;
    }

    const int vertexTotalCount = width * length;

    std::vector<CColorVertex> vertices;
    vertices.resize(vertexTotalCount);

    float startX = -static_cast<float>(width - 1) * 0.5f * cellSpacing;

    float startZ = +static_cast<float>(length - 1) * 0.5f * cellSpacing;

    float heightRange = static_cast<float>(maxRawHeight - minRawHeight);

    if (heightRange <= 0.0f)
    {
        heightRange = 1.0f;
    }

    for (int z = 0; z < length; ++z)
    {
        for (int x = 0; x < width; ++x)
        {
            int index = z * width + x;

            BYTE rawHeight = heightData[index];

            float height = static_cast<float>(rawHeight - minRawHeight) *
                heightScale;

            float worldX = startX + static_cast<float>(x) * cellSpacing;

            float worldZ = startZ - static_cast<float>(z) * cellSpacing;

            float t = static_cast<float>(rawHeight - minRawHeight) /
                heightRange;

            XMFLOAT4 color;

            if (t < 0.1f)
            {
                color = XMFLOAT4(0.0f, 0.0f, 0.0f, 1.0f);
            }
            else if (t < 0.25f)
            {
                color = XMFLOAT4(0.25f, 0.55f, 0.18f, 1.0f);
            }
            else if (t < 0.4f) {
                color = XMFLOAT4(0.4f, 0.75f, 0.26f, 1.0f);
            }
            else if (t < 0.6f) {
                color = XMFLOAT4(0.55f, 0.95f, 0.32f, 1.0f);
            }
            else if (t < 0.7f) {
                color = XMFLOAT4(0.3f, 0.18f, 0.17f, 1.0f);
            }
            else
            {
                color = XMFLOAT4(1.0f, 1.0f, 1.0f, 1.0f);
            }

            vertices[index] = CColorVertex{
                XMFLOAT3(worldX, height, worldZ),
                color
            };
        }
    }

    std::vector<UINT> indices;
    indices.reserve(static_cast<size_t>(width - 1) * static_cast<size_t>(length - 1) * 6);

    for (int z = 0; z < length - 1; ++z)
    {
        for (int x = 0; x < width - 1; ++x)
        {
            UINT topLeft = static_cast<UINT>(z * width + x);

            UINT topRight = static_cast<UINT>(z * width + x + 1);

            UINT bottomLeft = static_cast<UINT>((z + 1) * width + x);

            UINT bottomRight = static_cast<UINT>((z + 1) * width + x + 1);

            indices.push_back(topLeft);
            indices.push_back(topRight);
            indices.push_back(bottomLeft);

            indices.push_back(bottomLeft);
            indices.push_back(topRight);
            indices.push_back(bottomRight);
        }
    }

    vertexCount = static_cast<UINT>(vertices.size());
    vertexStride = sizeof(CColorVertex);
    indexCount = static_cast<UINT>(indices.size());
    primitiveTopology = D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST;

    vertexBuffer = CreateBufferResource(device, commandList, vertices.data(), static_cast<UINT>(vertices.size() * sizeof(CColorVertex)), D3D12_HEAP_TYPE_DEFAULT, D3D12_RESOURCE_STATE_VERTEX_AND_CONSTANT_BUFFER, &vertexUploadBuffer);

    if (vertexBuffer)
    {
        vertexBufferView.BufferLocation = vertexBuffer->GetGPUVirtualAddress();

        vertexBufferView.StrideInBytes = sizeof(CColorVertex);

        vertexBufferView.SizeInBytes = static_cast<UINT>(vertices.size() * sizeof(CColorVertex));
    }

    indexBuffer = CreateBufferResource(device, commandList, indices.data(), static_cast<UINT>(indices.size() * sizeof(UINT)), D3D12_HEAP_TYPE_DEFAULT, D3D12_RESOURCE_STATE_INDEX_BUFFER, &indexUploadBuffer);

    if (indexBuffer)
    {
        indexBufferView.BufferLocation = indexBuffer->GetGPUVirtualAddress();

        indexBufferView.Format = DXGI_FORMAT_R32_UINT;

        indexBufferView.SizeInBytes = static_cast<UINT>(indices.size() * sizeof(UINT));
    }
}

CTerrainGameMap::CTerrainGameMap(int mapNumber)
    : CGameMap(mapNumber)
{
}

CTerrainGameMap::~CTerrainGameMap()
{
    ReleaseObjects();
}

bool CTerrainGameMap::LoadMapData()
{
    if (terrain)
    {
        return true;
    }

    terrain = new CHeightMapTerrain();

    if (!terrain->LoadRawHeightMap(GetHeightMapFileName(), 257, 257))
    {
        delete terrain;
        terrain = nullptr;

        return false;
    }

    return true;
}

bool CTerrainGameMap::BuildDirectResources(ID3D12Device* device, ID3D12GraphicsCommandList* commandList)
{
    if (!device ||
        !commandList ||
        !terrain)
    {
        return false;
    }

    if (!terrainMesh)
    {
        terrainMesh = new CHeightMapTerrainMesh(device, commandList, terrain->GetHeightData(), terrain->GetWidth(), terrain->GetLength(), terrain->GetMinRawHeight(), terrain->GetMaxRawHeight(), terrain->GetCellSpacing(), terrain->GetHeightScale());
    }

    return terrainMesh != nullptr;
}

void CTerrainGameMap::ReleaseDirectUploadBuffers()
{
    if (terrainMesh)
    {
        terrainMesh->ReleaseUploadBuffers();
    }
}

void CTerrainGameMap::ReleaseObjects()
{
    if (terrainMesh)
    {
        terrainMesh->Release();
        terrainMesh = nullptr;
    }

    if (terrain)
    {
        delete terrain;
        terrain = nullptr;
    }

    enemySpawns.clear();
}

void CTerrainGameMap::RenderDirect(ID3D12GraphicsCommandList* commandList)
{
    if (!commandList ||
        !terrainMesh)
    {
        return;
    }

    XMFLOAT4X4 worldMatrix = Matrix4x4::Identity();

    XMFLOAT4X4 transposedWorldMatrix;

    XMStoreFloat4x4(&transposedWorldMatrix, XMMatrixTranspose(XMLoadFloat4x4(&worldMatrix)));

    commandList->SetGraphicsRoot32BitConstants(0, 16, &transposedWorldMatrix, 0);

    XMFLOAT4 objectColor(1.0f, 1.0f, 1.0f, 1.0f);

    commandList->SetGraphicsRoot32BitConstants(2, 4, &objectColor, 0);

    terrainMesh->Render(commandList);
}

void CTerrainGameMap::LoadSpawnPositions()
{
    playerStartPosition = XMFLOAT3(0.0f, 0.0f, 0.0f);

    enemySpawns.clear();

    const XMFLOAT3 baseSpawnPositions[] =
    {
        XMFLOAT3(40.0f, 0.0f, 40.0f),
        XMFLOAT3(-60.0f, 0.0f, 80.0f),
        XMFLOAT3(90.0f, 0.0f, -50.0f)
    };

    switch (mapNumber)
    {
    case 2:
    {
        const XMFLOAT3 tankPositions[] =
        {
            XMFLOAT3(80.0f, 0.0f, 80.0f),
            XMFLOAT3(-80.0f, 0.0f, 80.0f),
            XMFLOAT3(120.0f, 0.0f, -60.0f),
            XMFLOAT3(-120.0f, 0.0f, -60.0f),
            XMFLOAT3(40.0f, 0.0f, 140.0f),
            XMFLOAT3(-40.0f, 0.0f, 140.0f),
            XMFLOAT3(160.0f, 0.0f, 20.0f),
            XMFLOAT3(-160.0f, 0.0f, 20.0f),
            XMFLOAT3(70.0f, 0.0f, -150.0f),
            XMFLOAT3(-70.0f, 0.0f, -150.0f)
        };

        const XMFLOAT4 tankColor(0.35f, 0.75f, 0.35f, 1.0f);

        for (const XMFLOAT3& position : tankPositions)
        {
            AddEnemySpawn(EEnemyType::Tank, position, tankColor);
        }

        break;
    }

    case 3:
    {
        const XMFLOAT4 tankColor(0.70f, 0.45f, 0.15f, 1.0f);

        for (const XMFLOAT3& position : baseSpawnPositions)
        {
            AddEnemySpawn(EEnemyType::Tank, position, tankColor);
        }

        break;
    }

    default:
    {
        const float spread = 35.0f;

        for (const XMFLOAT3& position : baseSpawnPositions)
        {
            AddEnemySpawn(EEnemyType::Humanoid, position);
            AddEnemySpawn(EEnemyType::Humanoid, XMFLOAT3(position.x + spread, position.y, position.z));
            AddEnemySpawn(EEnemyType::Humanoid, XMFLOAT3(position.x - spread, position.y, position.z));
            AddEnemySpawn(EEnemyType::Humanoid, XMFLOAT3(position.x, position.y, position.z + spread));
            AddEnemySpawn(EEnemyType::Humanoid, XMFLOAT3(position.x, position.y, position.z - spread));
        }

        break;
    }
    }
}

bool CTerrainGameMap::TryGetGroundYAtPosition(const XMFLOAT3& position, float& outGroundY) const
{
    if (!terrain)
    {
        return false;
    }

    return terrain->GetHeight(position.x, position.z, outGroundY);
}

const char* CTerrainGameMap::GetHeightMapFileName() const
{
    if (mapNumber == 2)
    {
        return "HeightMap2.raw";
    }
    else if (mapNumber == 3)
    {
        return "HeightMap3.raw";
    }

    return "HeightMap.raw";
}

const std::vector<BYTE>& CHeightMapTerrain::GetHeightData() const
{
    return heightData;
}

int CHeightMapTerrain::GetWidth() const
{
    return width;
}

int CHeightMapTerrain::GetLength() const
{
    return length;
}

BYTE CHeightMapTerrain::GetMinRawHeight() const
{
    return minRawHeight;
}

BYTE CHeightMapTerrain::GetMaxRawHeight() const
{
    return maxRawHeight;
}

float CHeightMapTerrain::GetCellSpacing() const
{
    return cellSpacing;
}

float CHeightMapTerrain::GetHeightScale() const
{
    return heightScale;
}
