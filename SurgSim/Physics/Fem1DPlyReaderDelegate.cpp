// This file is a part of the OpenSurgSim project.
// Copyright 2015, SimQuest Solutions Inc.
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

#include "SurgSim/Math/Valid.h"
#include "SurgSim/Physics/Fem1DElementBeam.h"
#include "SurgSim/Physics/Fem1DPlyReaderDelegate.h"


using SurgSim::DataStructures::PlyReader;

namespace SurgSim
{
namespace Physics
{

Fem1DPlyReaderDelegate::Fem1DPlyReaderDelegate()
{
}

Fem1DPlyReaderDelegate::Fem1DPlyReaderDelegate(std::shared_ptr<Fem1D> mesh) :
	m_mesh(mesh)
{
	SURGSIM_ASSERT(mesh != nullptr) << "The mesh cannot be null.";
	mesh->clear();
}

std::string Fem1DPlyReaderDelegate::getElementName() const
{
	return "1d_element";
}

bool Fem1DPlyReaderDelegate::registerDelegate(PlyReader* reader)
{
	// Vertex processing
	reader->requestElement("vertex",
						   std::bind(&Fem1DPlyReaderDelegate::beginVertices, this,
									 std::placeholders::_1, std::placeholders::_2),
						   std::bind(&Fem1DPlyReaderDelegate::processVertex, this, std::placeholders::_1),
						   std::bind(&Fem1DPlyReaderDelegate::endVertices, this, std::placeholders::_1));
	reader->requestScalarProperty("vertex", "x", PlyReader::TYPE_DOUBLE, offsetof(Vertex6DData, x));
	reader->requestScalarProperty("vertex", "y", PlyReader::TYPE_DOUBLE, offsetof(Vertex6DData, y));
	reader->requestScalarProperty("vertex", "z", PlyReader::TYPE_DOUBLE, offsetof(Vertex6DData, z));

	if (m_hasRotationDOF)
	{
		reader->requestScalarProperty("vertex", "thetaX", PlyReader::TYPE_DOUBLE, offsetof(Vertex6DData, thetaX));
		reader->requestScalarProperty("vertex", "thetaY", PlyReader::TYPE_DOUBLE, offsetof(Vertex6DData, thetaY));
		reader->requestScalarProperty("vertex", "thetaZ", PlyReader::TYPE_DOUBLE, offsetof(Vertex6DData, thetaZ));
	}

	// Radius Processing
	reader->requestElement(
		"radius",
		std::bind(
		&Fem1DPlyReaderDelegate::beginRadius, this, std::placeholders::_1, std::placeholders::_2),
		nullptr,
		std::bind(&Fem1DPlyReaderDelegate::endRadius, this, std::placeholders::_1));
	reader->requestScalarProperty("radius", "value", PlyReader::TYPE_DOUBLE, 0);

	FemPlyReaderDelegate::registerDelegate(reader);

	return true;
}

bool Fem1DPlyReaderDelegate::fileIsAcceptable(const PlyReader& reader)
{
	bool result = FemPlyReaderDelegate::fileIsAcceptable(reader);

	// 6DOF processing
	m_hasRotationDOF = reader.hasProperty("vertex", "thetaX") && reader.hasProperty("vertex", "thetaY") &&
							  reader.hasProperty("vertex", "thetaZ");

	result = result && reader.hasProperty("radius", "value");

	return result;
}

void Fem1DPlyReaderDelegate::endParseFile()
{
	for(auto element : m_mesh->getElements())
	{
		element->radius = m_radius;
		element->enableShear = m_enableShear;
		element->massDensity = m_materialData.massDensity;
		element->poissonRatio = m_materialData.poissonRatio;
		element->youngModulus = m_materialData.youngModulus;
	}
	m_mesh->update();
}

void* Fem1DPlyReaderDelegate::beginVertices(const std::string &elementName, size_t vertexCount)
{
	m_vertexData.overrun1 = 0l;
	m_vertexData.overrun2 = 0l;
	return &m_vertexData;
}

void Fem1DPlyReaderDelegate::processVertex(const std::string& elementName)
{
	FemElementStructs::RotationVectorData data;

	if (m_hasRotationDOF)
	{
		data.thetaX = m_vertexData.thetaX;
		data.thetaY = m_vertexData.thetaY;
		data.thetaZ = m_vertexData.thetaZ;
	}

	Fem1D::VertexType vertex(SurgSim::Math::Vector3d(m_vertexData.x, m_vertexData.y, m_vertexData.z), data);

	m_mesh->addVertex(vertex);
}

void Fem1DPlyReaderDelegate::endVertices(const std::string &elementName)
{
	SURGSIM_ASSERT(m_vertexData.overrun1 == 0l && m_vertexData.overrun2 == 0l) <<
			"There was an overrun while reading the vertex structures, it is likely that data " <<
			"has become corrupted.";
}

void Fem1DPlyReaderDelegate::processFemElement(const std::string& elementName)
{
	SURGSIM_ASSERT(m_elementData.vertexCount == 2) << "Cannot process 1D Element with "
		<< m_elementData.vertexCount << " vertices.";

	auto femElement = std::make_shared<FemElementStructs::FemElement1DParameter>();
	static Fem1DElementBeam beam;
	femElement->type = beam.getClassName();
	femElement->nodeIds.resize(m_elementData.vertexCount);
	std::copy(m_elementData.indices, m_elementData.indices + m_elementData.vertexCount, femElement->nodeIds.data());
	m_mesh->addElement(femElement);
}

void* Fem1DPlyReaderDelegate::beginRadius(const std::string& elementName, size_t radiusCount)
{
	return &m_radius;
}

void Fem1DPlyReaderDelegate::endRadius(const std::string &elementName)
{
	SURGSIM_ASSERT(SurgSim::Math::isValid(m_radius)) << "No radius information processed.";
}

void Fem1DPlyReaderDelegate::processBoundaryCondition(const std::string& elementName)
{
	m_mesh->addBoundaryCondition(static_cast<size_t>(m_boundaryConditionData));
}

} // namespace Physics
} // namespace SurgSim
