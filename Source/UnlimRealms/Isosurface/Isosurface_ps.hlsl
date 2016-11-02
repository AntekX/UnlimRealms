struct PS_INPUT
{
	float4 pos	: SV_POSITION;
	float4 col	: COLOR0;
	float3 wpos : TEXCOORD0;
};

float4 main(PS_INPUT input) : SV_Target
{
	float3 wpos_dx = ddx(input.wpos);
	float3 wpos_dy = ddy(input.wpos);
	float3 n = normalize(cross(wpos_dx, wpos_dy));
	n.xyz = n.xzy;
	float4 color = float4((n + 1.0) * 0.5, 1.0f);
	return input.col;
}