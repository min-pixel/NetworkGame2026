// 3인칭 카메라 설정 (기체별로 하나씩)

#pragma once

struct ThirdPersonCameraProfile
{
    // 기체 위치에서 카메라가 바라볼 높이
    float targetHeight = 2.0f;

    // 바라보는 점에서 카메라까지 거리
    float distance = 15.0f;

    // 카메라 위아래 회전 제한 (도 단위)
    float minPitch = -20.0f;
    float maxPitch = 60.0f;
};
