
#include "ProceduralGenerationGraph.hlsli"

[Shader("node")]
[NodeIsProgramEntry]
[NodeLaunch("broadcasting")]
[NodeMaxDispatchGrid(16, 1, 1)]
[NumThreads(1, 1, 1)]
void ProceduralEntryNode(
	DispatchNodeInputRecord<ProceduralGenInputRecord> inputData
	)
{
}