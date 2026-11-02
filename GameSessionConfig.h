// 게임 한 판의 설정 (맵 번호, 기체 타입, 규칙). CGameFramework가 정해서 CGameScene에 넘긴다.

#pragma once

#include <array>
#include <cstdint>
#include <DirectXMath.h>

#include "PlayerType.h"
#include "GameMode.h"

struct GameSessionConfig
{
    int mapNumber = 1;

    EPlayerType playerType = EPlayerType::Helicopter;

    EGameMode gameMode = EGameMode::EliminateAll;

    // PlayerId는 0~3, 255는 미배정. 기존 싱글플레이 설정과 독립적으로 사용한다.
    uint8_t myPlayerId = 255;
    uint8_t playerCount = 0;

    // 앞의 playerCount개가 참가자이며, 같은 인덱스의 ID와 시작 위치가 한 쌍이다.
    std::array<uint8_t, 4> playerIds = { 255, 255, 255, 255 };
    std::array<DirectX::XMFLOAT3, 4> playerStartPositions = {};
};
