// 한 프레임 동안의 플레이어 입력 (Scene이 만들고 World가 사용)

#pragma once

struct PlayerInput
{
    // -1 ~ 1 (W/S, D/A)
    float moveForward = 0.0f;
    float moveRight = 0.0f;

    // 마우스 이동량 (카메라 회전용)
    float lookYawDelta = 0.0f;
    float lookPitchDelta = 0.0f;

    // 좌클릭을 누르고 있는지
    bool fire = false;

    // 이번 프레임에 E 키를 눌렀는지 (Shield 토글)
    bool shieldTogglePressed = false;
};
