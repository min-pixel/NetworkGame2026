// 기본 셰이더: 정점 색 x 오브젝트 색으로 칠한다.
// 루트 상수 b0 = 월드 행렬, b1 = 뷰/투영 행렬, b2 = 오브젝트 색 (CGameFramework::CreateRootSignature)

cbuffer GameObjectInfo : register(b0)
{
    matrix worldMatrix;
};

cbuffer CameraInfo : register(b1)
{
    matrix viewMatrix;
    matrix projectionMatrix;
};

cbuffer ObjectColorInfo : register(b2)
{
    float4 objectColor;
};

struct VS_INPUT
{
    float3 position : POSITION;
    float4 color : COLOR;
};

struct VS_OUTPUT
{
    float4 position : SV_POSITION;
    float4 color : COLOR;
};

VS_OUTPUT VSMain(VS_INPUT input)
{
    VS_OUTPUT output;

    float4 worldPosition = mul(float4(input.position, 1.0f), worldMatrix);
    float4 viewPosition = mul(worldPosition, viewMatrix);
    output.position = mul(viewPosition, projectionMatrix);

    output.color = input.color;

    return output;
}

float4 PSMain(VS_OUTPUT input) : SV_TARGET
{
    return input.color * objectColor;
}
