// 카메라

#pragma once

#include "stdafx.h"

// 위치/바라보는 점/위쪽 방향과 시야각으로 뷰/투영 행렬을 만드는 카메라 (왼손 좌표계)
class CCamera
{
public:
    CCamera();
    ~CCamera();

    void SetViewport(int width, int height);

    void SetPosition(const XMFLOAT3& newPosition);
    void SetTarget(const XMFLOAT3& newTarget);
    void SetUp(const XMFLOAT3& newUp);

    void SetLens(float fovAngleY, float nearZ, float farZ);

    void UpdateViewMatrix();
    void UpdateProjectionMatrix();
    void UpdateMatrices();

    XMFLOAT3 WorldToView(const XMFLOAT3& worldPoint) const;
    XMFLOAT3 ViewToProjection(const XMFLOAT3& viewPoint) const;
    XMFLOAT3 WorldToProjection(const XMFLOAT3& worldPoint) const;

    // 월드 좌표의 시야 영역 (컬링용)
    BoundingFrustum GetWorldFrustum() const;

    const XMFLOAT3& GetPosition() const;
    const XMFLOAT3& GetTarget() const;
    const XMFLOAT3& GetUp() const;

    const XMFLOAT4X4& GetViewMatrix() const;
    const XMFLOAT4X4& GetProjectionMatrix() const;

    int GetViewportWidth() const;
    int GetViewportHeight() const;

private:
    XMFLOAT3 position;
    XMFLOAT3 target;
    XMFLOAT3 up;

    float fovAngleY;
    float nearZ;
    float farZ;
    float aspectRatio;

    int viewportWidth;
    int viewportHeight;

    XMFLOAT4X4 viewMatrix;
    XMFLOAT4X4 projectionMatrix;
};
