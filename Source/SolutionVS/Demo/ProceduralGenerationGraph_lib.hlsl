
#include "ProceduralGenerationGraph.hlsli"

[Shader("node")]
[NodeIsProgramEntry]
[NodeLaunch("broadcasting")]
[NodeDispatchGrid(PartitionTetrahedraRootCount, 1, 1)]
[NumThreads(1, 1, 1)]
void PartitionUpdateEntryNode(
	[MaxRecords(2)] NodeOutput<PartitionUpdateRecord> PartitionUpdateNode)
{
	// TODO: split/merge root tetrahedra
}

[Shader("node")]
[NodeLaunch("thread")]
void PartitionUpdateNode(
	ThreadNodeInputRecord<PartitionUpdateRecord> inputData,
	[MaxRecords(2)] NodeOutput<PartitionUpdateRecord> PartitionUpdateNode)
{
}