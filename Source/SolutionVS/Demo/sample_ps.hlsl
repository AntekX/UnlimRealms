struct PS_INPUT
{
	float4 pos	: SV_POSITION;
	float4 col	: COLOR0;
	float2 uv	: TEXCOORD0;
};
//sampler sampler0	: register(s0);
//Texture2D texture0	: register(t0);

float4 main(PS_INPUT input) : SV_Target
{
	/*float4 out_col = input.col * texture0.Sample(sampler0, input.uv);
	return (out_col - 0.5) * 2.0;*/
	return float4(1.0, 0.0, 0.0, 1.0);
}