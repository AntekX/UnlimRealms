struct PS_INPUT
{
	float4 pos	: SV_POSITION;
	float3 norm : NORMAL;
	float4 col	: COLOR0;
	float3 wpos : TEXCOORD0;
};

float4 main(PS_INPUT input) : SV_Target
{
	return float4(1.0, 1.0, 1.0, 1.0);
}