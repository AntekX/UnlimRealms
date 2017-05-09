///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
//
//	UnlimRealms
//	Author: Anatole Kuzub
//
///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

// Logarithmic Depth
// http://outerra.blogspot.com.by/2009/08/logarithmic-z-buffer.html

#define LOG_DEPTH_ENABLED 1

float LogDepth(float4 projPos)
{
#if LOG_DEPTH_ENABLED
	const float C = 0.001;
	const float Far = 1.0e+6;
	return log(C * projPos.w + 1.0) / log(C * Far + 1.0) * projPos.w;
#else
	return projPos.z;
#endif
}
float4 LogDepthPos(float4 projPos)
{
	projPos.z = LogDepth(projPos);
	return projPos;
}