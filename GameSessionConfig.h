// 게임 한 판의 설정 (맵 번호, 기체 타입, 규칙). CGameFramework가 정해서 CGameScene에 넘긴다.

#pragma once

#include "PlayerType.h"
#include "GameMode.h"

struct GameSessionConfig
{
    int mapNumber = 1;

    EPlayerType playerType = EPlayerType::Helicopter;

    EGameMode gameMode = EGameMode::EliminateAll;
};
