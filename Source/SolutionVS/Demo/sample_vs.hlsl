
#if (0)

static const uint InstanceCount = 24;
cbuffer Common : register(b0)
{
	float4x4 Transform[InstanceCount];
	float4 Desc;
};

struct VS_INPUT
{
	float3 pos	: POSITION;
	float4 col	: COLOR0;
	float2 uv	: TEXCOORD0;
};

struct PS_INPUT
{
	float4 pos	: SV_POSITION;
	float4 col	: COLOR0;
	float2 uv	: TEXCOORD0;
};

static const uint ColorsCount = 7;
static const float3 Colors[ColorsCount] = {
	{ 0, 0, 1 },
	{ 0, 1, 0 },
	{ 0, 1, 1 },
	{ 1, 0, 0 },
	{ 1, 0, 1 },
	{ 1, 1, 0 },
	{ 1, 1, 1 }
};

PS_INPUT main(VS_INPUT input, uint instanceID : SV_InstanceID, uint vertexID : SV_VertexID)
{
	PS_INPUT output;

	output.pos = mul(Transform[instanceID + uint(Desc.x)], float4(input.pos.xyz, 1.0f));
	output.col = input.col;
	//output.col.xyz = Colors[(instanceID + vertexID) % ColorsCount] * 0.5 + 0.5;
	//output.col.xyz = Colors[uint(Desc.x / (InstanceCount / 3))];
	output.col.xyz = 1.0;
	
	output.uv = input.uv;

	return output;
}

#else

// super simple triangle test

struct VS_INPUT
{
	float3 pos		: POSITION;
	float3 color	: COLOR0;
};

struct PS_INPUT
{
	float4 pos		: SV_POSITION;
	float4 color	: COLOR0;
};

static const float3 TestTrianlePos[3] = {
	{ 0.0, 0.5, 0.0 },
	{ 0.5,-0.5, 0.0 },
	{-0.5,-0.5, 0.0 }
};
static const float3 TestTrianleCol[3] = {
	{ 1, 0, 0 },
	{ 0, 1, 0 },
	{ 0, 0, 1 }
};
static const float3 InstancePos[4] = {
	{-0.5, 0.5, 0.0 },
	{ 0.5, 0.5, 0.0 },
	{-0.5,-0.5, 0.0 },
	{ 0.5,-0.5, 0.0 }
};

PS_INPUT main(VS_INPUT input, uint vertexID : SV_VertexID, uint instanceID : SV_InstanceID)
{
	PS_INPUT output;

#if (0)
	// no VB test
	output.pos.xyz = TestTrianlePos[vertexID % 3].xyz + InstancePos[instanceID % 4].xyz;
	output.color.xyz = TestTrianleCol[vertexID % 3].xyz;
#else
	// read from VB
	output.pos.xyz = input.pos.xyz + InstancePos[instanceID % 4].xyz;
	output.color.xyz = input.color.xyz;
#endif
	output.color.w = 1.0;
	output.pos.w = 1.0;

	return output;
}

#endif