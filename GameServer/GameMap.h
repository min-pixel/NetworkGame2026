// 맵 공통 인터페이스

#pragma once

#include "Object.h"

// 맵이 알려주는 적 Spawn 한 건 (useCustomColor가 false면 적 클래스 기본색 사용)
struct FEnemySpawnInfo
{
    EEnemyType enemyType = EEnemyType::Humanoid;
    XMFLOAT3 position = XMFLOAT3(0.0f, 0.0f, 0.0f);

    bool useCustomColor = false;
    XMFLOAT4 color = XMFLOAT4(1.0f, 1.0f, 1.0f, 1.0f);
};

// 맵 공통 인터페이스
// - gameplay: 지형 높이 질의, Player 시작 위치, 적 Spawn 정보
// - 렌더링: DirectX 리소스 생성/그리기 (현재 단계에서는 의도적으로 함께 둠)
class CGameMap
{
public:
    explicit CGameMap(int mapNumber);
    virtual ~CGameMap() = default;

    int GetMapNumber() const;

    // 지형 데이터 읽기 (GPU 없이 동작)
    virtual bool LoadMapData() = 0;

    // Player 시작 위치와 적 Spawn 목록 채우기
    virtual void LoadSpawnPositions() = 0;

    // (x, z) 위치의 지면 높이. 맵 밖이면 false
    virtual bool TryGetGroundYAtPosition(const XMFLOAT3& position, float& outGroundY) const = 0;

    const XMFLOAT3& GetPlayerStartPosition() const;
    const std::vector<FEnemySpawnInfo>& GetEnemySpawns() const;

    virtual bool BuildDirectResources(ID3D12Device* device, ID3D12GraphicsCommandList* commandList) = 0;
    virtual void ReleaseDirectUploadBuffers() = 0;
    virtual void ReleaseObjects() = 0;
    virtual void RenderDirect(ID3D12GraphicsCommandList* commandList) = 0;

protected:
    void AddEnemySpawn(EEnemyType enemyType, const XMFLOAT3& position);
    void AddEnemySpawn(EEnemyType enemyType, const XMFLOAT3& position, const XMFLOAT4& color);

protected:
    int mapNumber = 1;

    XMFLOAT3 playerStartPosition = XMFLOAT3(0.0f, 0.0f, 0.0f);
    std::vector<FEnemySpawnInfo> enemySpawns;
};
