///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
//
//	UnlimRealms
//	Author: Anatole Kuzub
//
///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

#include "Generic.hlsli"

static const float2 QuadPos[4] = {
	{-1.0, 1.0 }, { 1.0, 1.0 },
	{-1.0,-1.0 }, { 1.0,-1.0 }
};

static const float2 QuadUV[4] = {
	{ 0.0, 0.0 }, { 1.0, 0.0 },
	{ 0.0, 1.0 }, { 1.0, 1.0 }
};

GenericQuadVertex main(uint vertexID : SV_VertexID)
{
	GenericQuadVertex output;

	output.pos = float4(QuadPos[vertexID].xy, 0.0, 1.0);
	output.uv = QuadUV[vertexID].xy;
	output.col = float4(1.0, 1.0, 1.0, 1.0);

	return output;
}