// 카메라 구현

#include "stdafx.h"
#include "Camera.h"

CCamera::CCamera()
    : position(0.0f, 0.0f, -10.0f),
    target(0.0f, 0.0f, 0.0f),
    up(0.0f, 1.0f, 0.0f),
    fovAngleY(60.0f),
    nearZ(0.1f),
    farZ(1000.0f),
    aspectRatio(static_cast<float>(FRAME_BUFFER_WIDTH) / static_cast<float>(FRAME_BUFFER_HEIGHT)),
    viewportWidth(FRAME_BUFFER_WIDTH),
    viewportHeight(FRAME_BUFFER_HEIGHT),
    viewMatrix(Matrix4x4::Identity()),
    projectionMatrix(Matrix4x4::Identity())
{
    UpdateMatrices();
}

CCamera::~CCamera()
{
}

void CCamera::SetViewport(int width, int height)
{
    viewportWidth = width;
    viewportHeight = (height <= 0) ? 1 : height;
    aspectRatio = static_cast<float>(viewportWidth) / static_cast<float>(viewportHeight);

    UpdateProjectionMatrix();
}

void CCamera::SetPosition(const XMFLOAT3& newPosition)
{
    position = newPosition;
    UpdateViewMatrix();
}

void CCamera::SetTarget(const XMFLOAT3& newTarget)
{
    target = newTarget;
    UpdateViewMatrix();
}

void CCamera::SetUp(const XMFLOAT3& newUp)
{
    up = newUp;
    UpdateViewMatrix();
}

void CCamera::SetLens(float newFovAngleY, float newNearZ, float newFarZ)
{
    fovAngleY = newFovAngleY;
    nearZ = newNearZ;
    farZ = newFarZ;

    UpdateProjectionMatrix();
}

void CCamera::UpdateViewMatrix()
{
    viewMatrix = Matrix4x4::LookAtLH(position, target, up);
}

void CCamera::UpdateProjectionMatrix()
{
    projectionMatrix = Matrix4x4::PerspectiveFovLH(fovAngleY, aspectRatio, nearZ, farZ);
}

void CCamera::UpdateMatrices()
{
    UpdateViewMatrix();
    UpdateProjectionMatrix();
}

XMFLOAT3 CCamera::WorldToView(const XMFLOAT3& worldPoint) const
{
    return Vector3::TransformCoord(worldPoint, viewMatrix);
}

XMFLOAT3 CCamera::ViewToProjection(const XMFLOAT3& viewPoint) const
{
    return Vector3::TransformCoord(viewPoint, projectionMatrix);
}

XMFLOAT3 CCamera::WorldToProjection(const XMFLOAT3& worldPoint) const
{
    XMFLOAT3 viewPoint = WorldToView(worldPoint);
    return ViewToProjection(viewPoint);
}

BoundingFrustum CCamera::GetWorldFrustum() const
{
    BoundingFrustum viewFrustum;
    BoundingFrustum::CreateFromMatrix(viewFrustum, XMLoadFloat4x4(&projectionMatrix), false);

    BoundingFrustum worldFrustum;
    XMMATRIX invView = XMMatrixInverse(nullptr, XMLoadFloat4x4(&viewMatrix));
    viewFrustum.Transform(worldFrustum, invView);

    return worldFrustum;
}

const XMFLOAT3& CCamera::GetPosition() const
{
    return position;
}

const XMFLOAT3& CCamera::GetTarget() const
{
    return target;
}

const XMFLOAT3& CCamera::GetUp() const
{
    return up;
}

const XMFLOAT4X4& CCamera::GetViewMatrix() const
{
    return viewMatrix;
}

const XMFLOAT4X4& CCamera::GetProjectionMatrix() const
{
    return projectionMatrix;
}

int CCamera::GetViewportWidth() const
{
    return viewportWidth;
}

int CCamera::GetViewportHeight() const
{
    return viewportHeight;
}
