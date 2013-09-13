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

#include <SurgSim/Physics/MassSpringRepresentation.h>
#include <SurgSim/DataStructures/Vertex.h>
#include <SurgSim/DataStructures/MeshElement.h>

using SurgSim::DataStructures::Vertex;
using SurgSim::DataStructures::MeshElement;

namespace SurgSim
{

namespace Physics
{

MassSpringRepresentation::MassSpringRepresentation(const std::string& name) : Representation(name)
{
	m_identityPose.setIdentity();
	m_initialPose.setIdentity();
}

MassSpringRepresentation::~MassSpringRepresentation()
{

}

unsigned int MassSpringRepresentation::getNumMasses(void) const
{
	return m_finalState.getNumVertices();
}

unsigned int MassSpringRepresentation::getNumSprings(void) const
{
	return m_finalState.getNumEdges();
}

const MassParameter& MassSpringRepresentation::getMassParameter(unsigned int nodeId) const
{
	SURGSIM_ASSERT(nodeId < getNumMasses()) << "Invalid node id to request a mass from";
	return m_finalState.getVertex(nodeId).data;
}

const LinearSpringParameter& MassSpringRepresentation::getSpringParameter(unsigned int springId) const
{
	SURGSIM_ASSERT(springId < getNumSprings()) << "Invalid spring id";
	return m_finalState.getEdge(springId).data;
}

double MassSpringRepresentation::getTotalMass(void) const
{
	double mass = 0.0;
	const std::vector<Vertex<MassParameter>> &vertices = m_finalState.getVertices();
	for (std::vector<Vertex<MassParameter>>::const_iterator it = vertices.begin(); it != vertices.end(); it++)
	{
		mass += it->data.getMass();
	}
	return mass;
}

double MassSpringRepresentation::getRayleighDampingStiffness(void) const
{
	return m_rayleighDamping.stiffnessCoefficient;
}

double MassSpringRepresentation::getRayleighDampingMass(void) const
{
	return m_rayleighDamping.massCoefficient;
}

void MassSpringRepresentation::setRayleighDampingStiffness(double stiffnessCoef)
{
	m_rayleighDamping.stiffnessCoefficient = stiffnessCoef;
}

void MassSpringRepresentation::setRayleighDampingMass(double massCoef)
{
	m_rayleighDamping.massCoefficient = massCoef;
}

void MassSpringRepresentation::addBoundaryCondition(int nodeId)
{
	m_boundaryConditions.push_back(nodeId);
}

int MassSpringRepresentation::getBoundaryCondition(size_t bcId) const
{
	SURGSIM_ASSERT(bcId >= 0 && bcId < m_boundaryConditions.size()) << "Invalid boundary condition " << bcId;
	return m_boundaryConditions[bcId];
}

size_t MassSpringRepresentation::getNumBoundaryConditions(void) const
{
	return m_boundaryConditions.size();
}

void MassSpringRepresentation::setIntegrationScheme(IntegrationScheme integrationScheme)
{
	m_integrationScheme = integrationScheme;
}

MassSpringRepresentation::IntegrationScheme MassSpringRepresentation::getIntegrationScheme(void) const
{
	return m_integrationScheme;
}

void MassSpringRepresentation::init1D(const Vector3d extremities[2], int numNodesPerDim[1],
	double totalMass, double springStiffness, double springDamping)
{
	SURGSIM_ASSERT(numNodesPerDim[0] > 0) << "Number of nodes incorrect: " << numNodesPerDim[0];

	// Allocate all Eigen data structures and all states
	allocate(numNodesPerDim[0] * 3);

	// Initialize the nodes position, transforming them by m_initialPose
	Vector3d delta = (extremities[1] - extremities[0]) / static_cast<double>(numNodesPerDim[0] - 1);
	for (int massId = 0; massId < numNodesPerDim[0]; massId++)
	{
		MassParameter mass;
		mass.setMass(totalMass / static_cast<double>(numNodesPerDim[0]));
		mass.setVelocity(Vector3d::Zero());
		Vertex<MassParameter> vertex( m_initialPose * (extremities[0] + massId * delta), mass);
		m_initialState.addVertex(vertex);
	}

	for (int massId = 0; massId < numNodesPerDim[0] - 1; massId++)
	{
		LinearSpringParameter spring;
		std::array<unsigned int, 2> element;

		element[0] = massId;
		element[1] = massId + 1;
		const Vector3d& A = m_initialState.getVertexPosition(element[0]);
		const Vector3d& B = m_initialState.getVertexPosition(element[1]);
		spring.setStiffness(springStiffness);
		spring.setDamping(springDamping);
		spring.setInitialLength((B-A).norm());

		MeshElement<2, LinearSpringParameter> edge(element, spring);
		m_initialState.addEdge(edge);
	}

	// Initialize the final state as a copy of the initial state
	m_finalState = m_initialState;

	// Initialize the internal current and previous state (Eigen vector)
	for (int nodeId = 0; nodeId < numNodesPerDim[0]; nodeId++)
	{
		m_x.segment(3 * nodeId, 3) = m_initialState.getVertexPosition(nodeId);
		m_xPrevious.segment(3 * nodeId, 3) = m_initialState.getVertexPosition(nodeId);
		m_v.segment(3 * nodeId, 3) = m_initialState.getVertex(nodeId).data.getVelocity();
	}

	// Set the number of dof for the Representation
	setNumDof(numNodesPerDim[0]*3);
}

void MassSpringRepresentation::init2D(const Vector3d extremities[2][2], int numNodesPerDim[2],
	double totalMass, double springStiffness, double springDamping)
{

}
void MassSpringRepresentation::init3D(const Vector3d extremities[2][2][2], int numNodesPerDim[3],
	double totalMass, double springStiffness, double springDamping)
{

}

RepresentationType MassSpringRepresentation::getType() const
{
	return REPRESENTATION_TYPE_MASSSPRING;
}

void MassSpringRepresentation::beforeUpdate(double dt)
{
	if (! isActive())
	{
		return;
	}

	// Backup current state into previous state
	m_xPrevious = m_x;
}

void MassSpringRepresentation::update(double dt)
{
	if (! isActive())
	{
		return;
	}

	if (getIntegrationScheme() == INTEGRATIONSCHEME_EXPLICIT_EULER)
	{
		updateEulerExplicit(dt);
	}
	else if (getIntegrationScheme() == INTEGRATIONSCHEME_MODIFIED_EXPLICIT_EULER)
	{
		updateEulerExplicit(dt, true);
	}
}

void MassSpringRepresentation::afterUpdate(double dt)
{
	if (! isActive())
	{
		return;
	}

	// Back fill the new mass spring position into the mesh
	for (unsigned int vertexId = 0 ;vertexId < m_finalState.getNumVertices(); vertexId++)
	{
		m_finalState.getVertex(vertexId).position = m_x.segment(3 * vertexId, 3);
		m_finalState.getVertex(vertexId).data.setVelocity(m_v.segment(3 * vertexId, 3));
	}
}

void MassSpringRepresentation::applyDofCorrection(double dt,
	const Eigen::VectorBlock<SurgSim::Math::MlcpSolution::Vector>& block)
{
	if (! isActive())
	{
		return;
	}
}

void MassSpringRepresentation::updateEulerExplicit(double dt, bool useModifiedEuler)
{
	m_f.setZero();

	// For all node, we have m.a = F
	// Note that at this stage, m_x and m_v contains information at time t (not t+dt)
	// 1) Add gravity to all node if gravity enabled
	// 2) Add Rayleigh damping forces
	// 3) Add spring forces
	// 4) Compute acceleration a(t) = F(t)/m
	// 5) Apply integration scheme (with boundary conditions)
	//     Euler explicit               OR   Modified Euler Explicit
	// {x(t+dt) = x(t) + dt.v(t)             {v(t+dt) = v(t) + dt.a(t)
	// {v(t+dt) = v(t) + dt.a(t)             {x(t+dt) = x(t) + dt.v(t+dt)

	// 1) Add gravity
	if (isGravityEnabled())
	{
		for (unsigned int nodeId = 0; nodeId < getNumMasses(); nodeId++)
		{
			m_f.block(3 * nodeId, 0, 3, 1) = getGravity() * getMassParameter(nodeId).getMass();
		}
	}
	
	// 2) Add Rayleigh damping
	addRayleighDampingForce(&m_f, m_v, 1.0);

	// 3) Add spring forces
	for (unsigned int springId = 0; springId < getNumSprings(); springId++)
	{
		int nodeId0 = m_finalState.getEdge(springId).vertices[0];
		int nodeId1 = m_finalState.getEdge(springId).vertices[1];
		const Vector3d& f = m_finalState.getEdge(springId).data.getF(
			m_x.segment(3 * nodeId0, 3), m_x.segment(3 * nodeId1, 3),
			m_v.segment(3 * nodeId0, 3), m_v.segment(3 * nodeId1, 3));
		m_f.block(3 * nodeId0, 0, 3, 1) += f;
		m_f.block(3 * nodeId1, 0, 3, 1) -= f;
	}

	// 4) Compute acceleration (dividing by the mass)
	for (unsigned int nodeId = 0; nodeId < getNumMasses(); nodeId++)
	{
		m_f.block(3 * nodeId, 0, 3, 1) /= getMassParameter(nodeId).getMass();
	}

	// 5) Apply numerical integration scheme
	if (useModifiedEuler)
	{
		m_v += m_f * dt;
		// apply the boundary conditions
		for (std::vector<int>::const_iterator bcIt = m_boundaryConditions.begin(); bcIt != m_boundaryConditions.end(); bcIt++)
		{
			m_v[3 * (*bcIt) + 0] = 0.0;
			m_v[3 * (*bcIt) + 1] = 0.0;
			m_v[3 * (*bcIt) + 2] = 0.0;
		}
		m_x += m_v * dt;
	}
	else
	{
		// apply the boundary conditions
		for (std::vector<int>::const_iterator bcIt = m_boundaryConditions.begin(); bcIt != m_boundaryConditions.end(); bcIt++)
		{
			m_f[3 * (*bcIt) + 0] = 0.0;
			m_f[3 * (*bcIt) + 1] = 0.0;
			m_f[3 * (*bcIt) + 2] = 0.0;

			m_v[3 * (*bcIt) + 0] = 0.0;
			m_v[3 * (*bcIt) + 1] = 0.0;
			m_v[3 * (*bcIt) + 2] = 0.0;
		}
		m_x += m_v * dt;
		m_v += m_f * dt;
	}
}

void MassSpringRepresentation::allocate(int numDof)
{	
	// Allocate internal Eigen data structure
	m_x.resize(numDof);
	m_xPrevious.resize(numDof);
	m_v.resize(numDof);
	m_f.resize(numDof);

	// Zero-out the 4 states
	m_x.setZero();
	m_xPrevious.setZero();
	m_v.setZero();
	m_f.setZero();
}

void MassSpringRepresentation::addRayleighDampingForce(Vector *f, const Vector &v, double scale)
{
	//! Rayleigh damping mass
	if (m_rayleighDamping.massCoefficient)
	{
		for (size_t nodeID = 0; nodeID < getNumMasses(); nodeID++)
		{
			(*f)[3*nodeID+0] -=
				scale * m_rayleighDamping.massCoefficient * getMassParameter(nodeID).getMass() * v[3 * nodeID + 0];
			(*f)[3*nodeID+1] -=
				scale * m_rayleighDamping.massCoefficient * getMassParameter(nodeID).getMass() * v[3 * nodeID + 1];
			(*f)[3*nodeID+2] -=
				scale * m_rayleighDamping.massCoefficient * getMassParameter(nodeID).getMass() * v[3 * nodeID + 2];
		}
	}

	//! Rayleigh damping stiffness
	if (m_rayleighDamping.stiffnessCoefficient)
	{
		//for (std::vector<LinearSpring<T> >::const_iterator it = m_springsStretching.begin(); it != m_springsStretching.end(); it++)
		//{
		//	//(*it).addDForceTo(f, v, v, scale*m_RayleighDampingStiffness);
		//	int nodeID0 = (*it).getNodeID(0);
		//	int nodeID1 = (*it).getNodeID(1);
		//	const Matrix33& K = (*it).getDForce_dx();
		//	Vector3 Kv = K*(v.block(3*nodeID0,0 , 3,1) - v.block(3*nodeID1,0 , 3,1));
		//	f[3*nodeID0+0] -= scale*m_RayleighDampingStiffness * Kv[0];
		//	f[3*nodeID0+1] -= scale*m_RayleighDampingStiffness * Kv[1];
		//	f[3*nodeID0+2] -= scale*m_RayleighDampingStiffness * Kv[2];

		//	f[3*nodeID1+0] += scale*m_RayleighDampingStiffness * Kv[0];
		//	f[3*nodeID1+1] += scale*m_RayleighDampingStiffness * Kv[1];
		//	f[3*nodeID1+2] += scale*m_RayleighDampingStiffness * Kv[2];
		//}
	}
}

} // namespace Physics

} // namespace SurgSim