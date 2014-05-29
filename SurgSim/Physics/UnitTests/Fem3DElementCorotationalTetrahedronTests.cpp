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

#include <gtest/gtest.h>

#include <memory>
#include <array>

#include "SurgSim/Math/Matrix.h"
#include "SurgSim/Math/OdeState.h"
#include "SurgSim/Math/Vector.h"
#include "SurgSim/Physics/Fem3DElementCorotationalTetrahedron.h"

using SurgSim::Physics::Fem3DElementCorotationalTetrahedron;
using SurgSim::Math::Vector3d;
using SurgSim::Math::Vector;
using SurgSim::Math::Matrix;

namespace
{
const double epsilonAddForce = 1e-8;
const double epsilonAddMatVec = 1e-10;
};

class MockFem3DElementCorotationalTet : public Fem3DElementCorotationalTetrahedron
{
public:
	MockFem3DElementCorotationalTet(std::array<unsigned int, 4> nodeIds)
		: Fem3DElementCorotationalTetrahedron(nodeIds)
	{
	}

	const Eigen::Matrix<double,12 ,1>& getInitialPosition() const
	{
		return m_x0;
	}

	const SurgSim::Math::Matrix33d& getRotation() const
	{
		return m_rotation;
	}

	const Eigen::Matrix<double, 12, 12>& getMassMatrix() const
	{
		return m_M;
	}

	const Eigen::Matrix<double, 12, 12>& getRotatedStiffness() const
	{
		return m_corotationalStiffnessMatrix;
	}

	const Eigen::Matrix<double, 12, 12>& getNonRotatedStiffness() const
	{
		return m_K;
	}

	const SurgSim::Math::Matrix44d& getVinverse() const
	{
		return m_Vinverse;
	}

	void setupInitialParams(const SurgSim::Math::OdeState &state,
							double massDensity,
							double poissonRatio,
							double youngModulus)
	{
		setMassDensity(massDensity);
		setPoissonRatio(poissonRatio);
		setYoungModulus(youngModulus);
		initialize(state);
	}
};

class Fem3DElementCorotationalTetrahedronTests : public ::testing::Test
{
public:
	std::array<unsigned int, 4> m_nodeIds;
	std::vector<unsigned int> m_nodeIdsAsVector;
	SurgSim::Math::OdeState m_restState, m_state;
	double m_rho, m_E, m_nu;

	SurgSim::Math::Matrix33d m_rotation;
	Eigen::Matrix<double, 12, 12> m_R12x12;
	Vector3d m_translation;

	virtual void SetUp() override
	{
		m_nodeIds[0] = 3;
		m_nodeIds[1] = 1;
		m_nodeIds[2] = 14;
		m_nodeIds[3] = 9;
		m_nodeIdsAsVector.assign(m_nodeIds.cbegin(), m_nodeIds.cend());

		m_restState.setNumDof(3, 15);
		Vector& x0 = m_restState.getPositions();
		std::array<Vector3d, 4> points = {{Vector3d(0.0, 0.0, 0.0), Vector3d(1.0, 0.0, 0.0),
			Vector3d(0.0, 1.0, 0.0), Vector3d(0.0, 0.0, 1.0) }};

		// Tet is aligned with the axis (X,Y,Z), centered on (0.0, 0.0, 0.0), embedded in a cube of size 1
		for (size_t nodeId = 0; nodeId < 4; ++nodeId)
		{
			SurgSim::Math::getSubVector(x0, m_nodeIds[nodeId], 3) = points[nodeId];
		}

		m_rho = 1000.0;
		m_E = 1e6;
		m_nu = 0.45;

		Vector3d axis(1.1, 2.2, 3.3);
		axis.normalize();
		m_rotation = SurgSim::Math::makeRotationMatrix(1.45, axis);
		m_R12x12 = Eigen::Matrix<double, 12, 12>::Zero();
		for (size_t nodeId = 0; nodeId < 4; ++nodeId)
		{
			m_R12x12.block<3, 3>(3* nodeId, 3* nodeId) = m_rotation;
		}
		m_translation = Vector3d(1.2, 2.3, 3.4);
	}
};

TEST_F(Fem3DElementCorotationalTetrahedronTests, ConstructorTest)
{
	ASSERT_NO_THROW({MockFem3DElementCorotationalTet tet(m_nodeIds);});
	ASSERT_NO_THROW({MockFem3DElementCorotationalTet* tet = new MockFem3DElementCorotationalTet(m_nodeIds);
		delete tet;});
	ASSERT_NO_THROW({std::shared_ptr<MockFem3DElementCorotationalTet> tet =
		std::make_shared<MockFem3DElementCorotationalTet>(m_nodeIds);});
}

TEST_F(Fem3DElementCorotationalTetrahedronTests, InitializeTest)
{
	MockFem3DElementCorotationalTet tet(m_nodeIds);
	tet.setupInitialParams(m_restState, m_rho, m_nu, m_E);

	EXPECT_TRUE(tet.getRotation().isIdentity());

	EXPECT_TRUE(tet.getRotatedStiffness().isApprox(tet.getNonRotatedStiffness()));

	// V^1 = (a b c d)^-1
	//       (1 1 1 1)
	SurgSim::Math::Matrix44d expectedV = SurgSim::Math::Matrix44d::Ones();
	for (size_t axis = 0; axis < 3; ++axis)
	{
		expectedV(axis, 0) = m_restState.getPosition(m_nodeIds[0])(axis);
		expectedV(axis, 1) = m_restState.getPosition(m_nodeIds[1])(axis);
		expectedV(axis, 2) = m_restState.getPosition(m_nodeIds[2])(axis);
		expectedV(axis, 3) = m_restState.getPosition(m_nodeIds[3])(axis);
	}
	EXPECT_TRUE(tet.getVinverse().isApprox(expectedV.inverse()));
}

TEST_F(Fem3DElementCorotationalTetrahedronTests, UpdateTest)
{
	SurgSim::Math::RigidTransform3d transformation;

	{
		SCOPED_TRACE("No rotation, no translation");
		transformation.linear().setIdentity();
		transformation.translation().setZero();

		MockFem3DElementCorotationalTet tet(m_nodeIds);
		tet.setupInitialParams(m_restState, m_rho, m_nu, m_E);

		m_state = m_restState;
		for (size_t nodeId = 0; nodeId < m_restState.getNumNodes(); ++nodeId)
		{
			SurgSim::Math::setSubVector(transformation * m_restState.getPosition(nodeId),
				nodeId, 3, &m_state.getPositions());
		}

		ASSERT_NO_THROW(tet.update(m_state));
		EXPECT_TRUE(tet.getRotation().isIdentity());
		EXPECT_TRUE(tet.getRotatedStiffness().isApprox(tet.getNonRotatedStiffness()));
	}

	{
		SCOPED_TRACE("Pure translation");
		transformation.linear().setIdentity();
		transformation.translation() = m_translation;

		MockFem3DElementCorotationalTet tet(m_nodeIds);
		tet.setupInitialParams(m_restState, m_rho, m_nu, m_E);

		m_state = m_restState;
		for (size_t nodeId = 0; nodeId < m_restState.getNumNodes(); ++nodeId)
		{
			SurgSim::Math::setSubVector(transformation * m_restState.getPosition(nodeId),
				nodeId, 3, &m_state.getPositions());
		}

		ASSERT_NO_THROW(tet.update(m_state));
		EXPECT_TRUE(tet.getRotation().isIdentity());
		EXPECT_TRUE(tet.getRotatedStiffness().isApprox(tet.getNonRotatedStiffness()));
	}

	{
		SCOPED_TRACE("Pure rotation");
		transformation.linear() = m_rotation;
		transformation.translation().setZero();

		MockFem3DElementCorotationalTet tet(m_nodeIds);
		tet.setupInitialParams(m_restState, m_rho, m_nu, m_E);

		m_state = m_restState;
		for (size_t nodeId = 0; nodeId < m_restState.getNumNodes(); ++nodeId)
		{
			SurgSim::Math::setSubVector(transformation * m_restState.getPosition(nodeId),
				nodeId, 3, &m_state.getPositions());
		}

		ASSERT_NO_THROW(tet.update(m_state));
		EXPECT_TRUE(tet.getRotation().isApprox(m_rotation));
		EXPECT_TRUE(tet.getRotatedStiffness().isApprox(
			m_R12x12 * tet.getNonRotatedStiffness() * m_R12x12.transpose()));
	}

	{
		SCOPED_TRACE("Translation + Rotation");
		transformation.linear() = m_rotation;
		transformation.translation() = m_translation;

		MockFem3DElementCorotationalTet tet(m_nodeIds);
		tet.setupInitialParams(m_restState, m_rho, m_nu, m_E);

		m_state = m_restState;
		for (size_t nodeId = 0; nodeId < m_restState.getNumNodes(); ++nodeId)
		{
			SurgSim::Math::setSubVector(transformation * m_restState.getPosition(nodeId),
				nodeId, 3, &m_state.getPositions());
		}

		ASSERT_NO_THROW(tet.update(m_state));
		EXPECT_TRUE(tet.getRotation().isApprox(m_rotation));
		EXPECT_TRUE(tet.getRotatedStiffness().isApprox(
			m_R12x12 * tet.getNonRotatedStiffness() * m_R12x12.transpose()));
	}
}

namespace
{
void defineCurrentState(const SurgSim::Math::OdeState& x0, SurgSim::Math::OdeState* x,
						const SurgSim::Math::RigidTransform3d& t, bool addSmallDeformation)
{
	std::array<Vector3d, 3> delta = {{Vector3d(0.01, -0.02, 0.005),
		Vector3d(-0.01, -0.01, -0.03), Vector3d(0.0, -0.015, 0.03)}};

	*x = x0;
	for (size_t nodeId = 0; nodeId < x0.getNumNodes(); ++nodeId)
	{
		(*x).getPositions().segment<3>(3 * nodeId) = t * x0.getPositions().segment<3>(3 * nodeId);
		if (addSmallDeformation)
		{
			(*x).getPositions().segment<3>(3 * nodeId) += delta[nodeId % 3];
		}
	}
}
}; // anonymous namespace

TEST_F(Fem3DElementCorotationalTetrahedronTests, AddStiffnessTest)
{
	using SurgSim::Math::makeRigidTransform;

	MockFem3DElementCorotationalTet tet(m_nodeIds);
	tet.setupInitialParams(m_restState, m_rho, m_nu, m_E);

	unsigned int numDof = m_restState.getNumDof();

	{
		SCOPED_TRACE("Without rotation, scale 1.0");

		Matrix expectedK = Matrix::Zero(numDof, numDof);
		SurgSim::Math::addSubMatrix(tet.getNonRotatedStiffness(), m_nodeIdsAsVector, 3, &expectedK);

		Matrix K = Matrix::Zero(numDof, numDof);
		tet.addStiffness(m_restState, &K);

		EXPECT_TRUE(K.isApprox(expectedK));
	}

	{
		SCOPED_TRACE("Without rotation, scale 0.4");

		Matrix expectedK = Matrix::Zero(numDof, numDof);
		SurgSim::Math::addSubMatrix(tet.getNonRotatedStiffness() * 0.4, m_nodeIdsAsVector, 3, &expectedK);

		Matrix K = Matrix::Zero(numDof, numDof);
		tet.addStiffness(m_restState, &K, 0.4);

		EXPECT_TRUE(K.isApprox(expectedK));
	}

	{
		SCOPED_TRACE("With rotation, scale 1.0");

		defineCurrentState(m_restState, &m_state, makeRigidTransform(m_rotation, Vector3d::Zero()), false);
		ASSERT_NO_THROW(tet.update(m_state));

		Matrix expectedK = Matrix::Zero(numDof, numDof);
		SurgSim::Math::addSubMatrix(m_R12x12 * tet.getNonRotatedStiffness() * m_R12x12.transpose(),
			m_nodeIdsAsVector, 3, &expectedK);

		Matrix K = Matrix::Zero(numDof, numDof);
		tet.addStiffness(m_restState, &K);

		EXPECT_TRUE(K.isApprox(expectedK));
	}

	{
		SCOPED_TRACE("With rotation, scale 0.4");

		defineCurrentState(m_restState, &m_state, makeRigidTransform(m_rotation, Vector3d::Zero()), false);
		ASSERT_NO_THROW(tet.update(m_state));

		Matrix expectedK = Matrix::Zero(numDof, numDof);
		SurgSim::Math::addSubMatrix(0.4 * m_R12x12 * tet.getNonRotatedStiffness() * m_R12x12.transpose(),
			m_nodeIdsAsVector, 3, &expectedK);

		Matrix K = Matrix::Zero(numDof, numDof);
		tet.addStiffness(m_restState, &K, 0.4);

		EXPECT_TRUE(K.isApprox(expectedK));
	}
}

namespace
{
void testAddForce(MockFem3DElementCorotationalTet* tet,
				  const SurgSim::Math::OdeState& state0,
				  const SurgSim::Math::RigidTransform3d& t,
				  bool addLocalDeformation)
{
	SurgSim::Math::OdeState statet;
	SurgSim::Math::Matrix K = tet->getNonRotatedStiffness();

	// F = -RK(R^t.x - x0)
	Eigen::Matrix<double, 12, 1> x, x0;
	SurgSim::Math::getSubVector(state0.getPositions(), tet->getNodeIds(), 3, &x0);
	defineCurrentState(state0, &statet, t, addLocalDeformation);
	ASSERT_NO_THROW(tet->update(statet));
	SurgSim::Math::getSubVector(statet.getPositions(), tet->getNodeIds(), 3, &x);

	// Note that the element rotation is not necessarily the RigidTransform rotation
	// If addDeformation is true, it will add pseudo-random variation that will affect the rigid motion slightly
	SurgSim::Math::Matrix33d R = tet->getRotation();
	Eigen::Matrix<double, 12, 12> R12x12 = Eigen::Matrix<double, 12, 12>::Zero();
	for (size_t nodeId = 0; nodeId < 4; ++nodeId)
	{
		R12x12.block<3, 3>(3 * nodeId, 3 * nodeId) = R;
	}

	SurgSim::Math::Vector expectedF;
	expectedF.resize(statet.getNumDof());
	expectedF.setZero();
	Eigen::Matrix<double, 12 ,1> f = - R12x12 * K * R12x12.transpose() * (x - (R12x12 * x0));
	SurgSim::Math::addSubVector(f, tet->getNodeIds(), 3, &expectedF);

	EXPECT_TRUE(tet->getRotation().isApprox(R));
	EXPECT_TRUE(tet->getNonRotatedStiffness().isApprox(K));
	EXPECT_TRUE(tet->getRotatedStiffness().isApprox(R12x12 * K * R12x12.transpose()));
	EXPECT_TRUE(tet->getInitialPosition().isApprox(x0));
	{
		SCOPED_TRACE("Scale 1.0");
		SurgSim::Math::Vector F;
		F.resize(statet.getNumDof());
		F.setZero();
		tet->addForce(statet, &F);
		EXPECT_LT((F - expectedF).norm(), epsilonAddForce);
		if (!addLocalDeformation)
		{
			EXPECT_TRUE(F.isZero(epsilonAddForce));
		}
	}

	{
		SCOPED_TRACE("Scale 0.4");
		SurgSim::Math::Vector F;
		F.resize(statet.getNumDof());
		F.setZero();
		tet->addForce(statet, &F, 0.4);
		EXPECT_LT((F - 0.4 * expectedF).norm(), epsilonAddForce);
		if (!addLocalDeformation)
		{
			EXPECT_TRUE(F.isZero(epsilonAddForce));
		}
	}
}

};

TEST_F(Fem3DElementCorotationalTetrahedronTests, AddForceTest)
{
	MockFem3DElementCorotationalTet tet(m_nodeIds);
	tet.setupInitialParams(m_restState, m_rho, m_nu, m_E);

	SurgSim::Math::RigidTransform3d transformation;
	transformation.linear() = m_rotation;
	transformation.translation() = m_translation;

	{
		SCOPED_TRACE("No deformation, no rigid transformation");
		testAddForce(&tet, m_restState, SurgSim::Math::RigidTransform3d::Identity(), false);
	}

	{
		SCOPED_TRACE("No deformation, rigid transformation");
		testAddForce(&tet, m_restState, transformation, false);
	}

	{
		SCOPED_TRACE("Deformation, no rigid transformation");
		testAddForce(&tet, m_restState, SurgSim::Math::RigidTransform3d::Identity(), true);
	}

	{
		SCOPED_TRACE("Deformation, rigid transformation");
		testAddForce(&tet, m_restState, transformation, true);
	}
}

TEST_F(Fem3DElementCorotationalTetrahedronTests, AddMatVecTest)
{
	MockFem3DElementCorotationalTet tet(m_nodeIds);
	tet.setupInitialParams(m_restState, m_rho, m_nu, m_E);

	SurgSim::Math::RigidTransform3d transformation;
	transformation.linear() = m_rotation;
	transformation.translation() = m_translation;

	SurgSim::Math::OdeState state;
	defineCurrentState(m_restState, &state, transformation, true);
	ASSERT_NO_THROW(tet.update(state));

	Eigen::Matrix<double, 12, 12> M = tet.getMassMatrix();
	Eigen::Matrix<double, 12, 12> K = tet.getRotatedStiffness();

	SurgSim::Math::Vector ones = SurgSim::Math::Vector::Ones(state.getNumDof());

	{
		SCOPED_TRACE("Mass only");

		SurgSim::Math::Vector result = SurgSim::Math::Vector::Zero(state.getNumDof());
		tet.addMatVec(state, 1.4, 0.0, 0.0, ones, &result);

		SurgSim::Math::Vector expectedResult = SurgSim::Math::Vector::Zero(state.getNumDof());
		Eigen::Matrix<double, 12, 1> f = 1.4 * M * SurgSim::Math::Vector::Ones(12);
		SurgSim::Math::addSubVector(f, m_nodeIdsAsVector, 3, &expectedResult);

		EXPECT_TRUE(result.isApprox(expectedResult));
	}

	{
		SCOPED_TRACE("Damping only");

		SurgSim::Math::Vector result = SurgSim::Math::Vector::Zero(state.getNumDof());
		tet.addMatVec(state, 0.0, 1.5, 0.0, ones, &result);

		EXPECT_TRUE(result.isZero());
	}

	{
		SCOPED_TRACE("Stiffness only");

		SurgSim::Math::Vector result = SurgSim::Math::Vector::Zero(state.getNumDof());
		tet.addMatVec(state, 0.0, 0.0, 1.6, ones, &result);

		SurgSim::Math::Vector expectedResult = SurgSim::Math::Vector::Zero(state.getNumDof());
		Eigen::Matrix<double, 12, 1> f = 1.6 * K * SurgSim::Math::Vector::Ones(12);
		SurgSim::Math::addSubVector(f, m_nodeIdsAsVector, 3, &expectedResult);

		EXPECT_TRUE(result.isApprox(expectedResult));
	}

	{
		SCOPED_TRACE("Mass/Damping/Stiffness");

		SurgSim::Math::Vector result = SurgSim::Math::Vector::Zero(state.getNumDof());
		tet.addMatVec(state, 1.4, 1.5, 1.6, ones, &result);

		SurgSim::Math::Vector expectedResult = SurgSim::Math::Vector::Zero(state.getNumDof());
		Eigen::Matrix<double, 12, 1> f = (1.4 * M + 1.6 * K) * SurgSim::Math::Vector::Ones(12);
		SurgSim::Math::addSubVector(f, m_nodeIdsAsVector, 3, &expectedResult);

		EXPECT_TRUE(result.isApprox(expectedResult, epsilonAddMatVec));
	}
}