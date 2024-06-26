///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
//
//	UnlimRealms
//	Author: Anatole Kuzub
//
///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

#pragma once

#include "RenderGraph/RenderGraph.h"

namespace UnlimRealms
{

	///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
	// RenderGraph
	///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

	RenderGraph::RenderGraph(Realm &realm) :
		RealmEntity(realm)
	{

	}

	RenderGraph::~RenderGraph()
	{

	}


	///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
	// RenderNode
	///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
	
	RenderNode::RenderNode(RenderGraph &renderGraph) :
		RealmEntity(renderGraph.GetRealm()),
		renderGraph(renderGraph)
	{

	}

	RenderNode::~RenderNode()
	{

	}

} // end namespace UnlimRealms