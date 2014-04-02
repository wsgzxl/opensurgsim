// This file is a part of the OpenSurgSim project.
// Copyright 2013, SimQuest Solutions Inc.
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
//     http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.

#include "SurgSim/DataStructures/AabbTree.h"
#include "SurgSim/DataStructures/AabbTreeNode.h"

#include <memory>

namespace SurgSim
{
namespace DataStructures
{

AabbTree::AabbTree() :
	m_maxObjectsPerNode(3)
{

}

AabbTree::AabbTree(size_t maxObjectsPerNode) :
	m_maxObjectsPerNode(maxObjectsPerNode)
{

}

AabbTree::~AabbTree()
{

}

void AabbTree::add(const SurgSim::Math::Aabbd& aabb, size_t objectId)
{
	if (getRoot() == nullptr)
	{
		m_typedRoot = std::make_shared<AabbTreeNode>();
		setRoot(std::make_shared<AabbTreeNode>());
	}
	m_typedRoot->addData(aabb, objectId, m_maxObjectsPerNode);
}

std::vector<size_t> AabbTree::getIntersections(const SurgSim::Math::Aabbd& aabb)
{
	std::vector<size_t> result;
	// return m_typedRoot->getIntersections(aabb);
	return std::move(result);
}

size_t AabbTree::getMaxObjectsPerNode() const
{
	return m_maxObjectsPerNode;
}

}
}

