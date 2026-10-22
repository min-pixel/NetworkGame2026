// 높이맵(RAW) 지형 맵

#pragma once

#include "GameMap.h"
#include "Mesh.h"

class CHeightMapTerrainMesh;

// RAW 높이맵 데이터 (CPU 전용). 맵 중앙이 월드 원점이며,
// 높이 = (원본 값 - 최소값) x heightScale, 칸 간격 = cellSpacing
class CHeightMapTerrain
{
public:
    CHeightMapTerrain();
   ~CHeightMapTerrain();

    bool LoadRawHeightMap(const char* fileName, int mapWidth, int mapLength);

    // 월드 좌표의 높이를 주변 4점 보간으로 구한다. 맵 밖이면 false
    bool GetHeight(float worldX, float worldZ, float& outHeight) const;

    const std::vector<BYTE>& GetHeightData() const;

    int GetWidth() const;
    int GetLength() const;

    BYTE GetMinRawHeight() const;
    BYTE GetMaxRawHeight() const;

    float GetCellSpacing() const;
    float GetHeightScale() const;

private:
    float GetScaledHeightByIndex(int x, int z) const;

private:
    std::vector<BYTE> heightData;

    int width = 0;
    int length = 0;

    BYTE minRawHeight = 0;
    BYTE maxRawHeight = 255;

    float cellSpacing = 2.0f;
    float heightScale = 0.35f;
};

// 높이맵으로 만든 지형 메시 (높이에 따라 정점 색이 달라짐)
class CHeightMapTerrainMesh : public CDirectMesh
{
public:
    CHeightMapTerrainMesh(ID3D12Device* device, ID3D12GraphicsCommandList* commandList, const std::vector<BYTE>& heightData, int width, int length, BYTE minRawHeight, BYTE maxRawHeight, float cellSpacing, float heightScale);

    ~CHeightMapTerrainMesh() override = default;
};

// CGameMap 구현. 맵 번호별로 HeightMap.raw / HeightMap2.raw / HeightMap3.raw (257x257)를 쓰고,
// 맵 번호별 적 배치(Map1: Humanoid, Map2/Map3: Tank)를 LoadSpawnPositions에서 정한다.
class CTerrainGameMap : public CGameMap
{
public:
    explicit CTerrainGameMap(int mapNumber);
    ~CTerrainGameMap() override;

    bool LoadMapData() override;

    bool BuildDirectResources(ID3D12Device* device, ID3D12GraphicsCommandList* commandList) override;

    void ReleaseDirectUploadBuffers() override;
    void ReleaseObjects() override;

    void RenderDirect(ID3D12GraphicsCommandList* commandList) override;

    void LoadSpawnPositions() override;

    bool TryGetGroundYAtPosition(const XMFLOAT3& position, float& outGroundY) const override;

private:
    const char* GetHeightMapFileName() const;

private:
    CHeightMapTerrain* terrain = nullptr;
    CHeightMapTerrainMesh* terrainMesh = nullptr;
};
