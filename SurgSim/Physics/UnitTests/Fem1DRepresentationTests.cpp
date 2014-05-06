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

/// \file Fem1DRepresentationTests.cpp
/// This file tests the functionalities of the class Fem1DRepresentation.

#include <gtest/gtest.h>

#include "SurgSim/Framework/Runtime.h"
#include "SurgSim/Math/Quaternion.h"
#include "SurgSim/Math/RigidTransform.h"
#include "SurgSim/Math/Vector.h"
#include "SurgSim/Physics/Fem1DRepresentation.h"
#include "SurgSim/Physics/UnitTests/MockObjects.h"

using SurgSim::Physics::MockFem1DRepresentation;

namespace SurgSim
{

namespace Physics
{

TEST(Fem1DRepresentationTests, ConstructorTest)
{
	ASSERT_NO_THROW({std::shared_ptr<Fem1DRepresentation> fem = std::make_shared<Fem1DRepresentation>("Fem1D");});
}

TEST(Fem1DRepresentationTests, GetTypeTest)
{
	std::shared_ptr<Fem1DRepresentation> fem = std::make_shared<Fem1DRepresentation>("Fem1D");
	EXPECT_EQ(REPRESENTATION_TYPE_FEM1D, fem->getType());
}

TEST(Fem1DRepresentationTests, GetNumDofPerNodeTest)
{
	std::shared_ptr<Fem1DRepresentation> fem = std::make_shared<Fem1DRepresentation>("Fem1D");
	EXPECT_EQ(6u, fem->getNumDofPerNode());
}

TEST(Fem1DRepresentationTests, TransformInitialStateTest)
{
	using SurgSim::Math::Vector;

	std::shared_ptr<Fem1DRepresentation> fem = std::make_shared<Fem1DRepresentation>("Fem1D");

	const size_t numNodes = 2;
	const size_t numDofPerNode = fem->getNumDofPerNode();
	const size_t numDof = numDofPerNode * numNodes;

	SurgSim::Math::RigidTransform3d initialPose;
	SurgSim::Math::Quaterniond q(1.0, 2.0, 3.0, 4.0);
	SurgSim::Math::Vector3d t(1.0, 2.0, 3.0);
	q.normalize();
	initialPose = SurgSim::Math::makeRigidTransform(q, t);
	fem->setLocalPose(initialPose);

	std::shared_ptr<DeformableRepresentationState> initialState = std::make_shared<DeformableRepresentationState>();
	initialState->setNumDof(numDofPerNode, numNodes);
	Vector x = Vector::LinSpaced(numDof, 1.0, static_cast<double>(numDof));
	Vector v = Vector::Ones(numDof);
	Vector a = Vector::Ones(numDof) * 2.0;
	initialState->getPositions() = x;
	initialState->getVelocities() = v;
	initialState->getAccelerations() = a;
	fem->setInitialState(initialState);

	Vector expectedX = x, expectedV = v, expectedA = a;
	for (size_t nodeId = 0; nodeId < numNodes; nodeId++)
	{
		expectedX.segment<3>(numDofPerNode * nodeId) = initialPose * x.segment<3>(numDofPerNode * nodeId);
		expectedV.segment<3>(numDofPerNode * nodeId) = initialPose.linear() * v.segment<3>(numDofPerNode * nodeId);
		expectedA.segment<3>(numDofPerNode * nodeId) = initialPose.linear() * a.segment<3>(numDofPerNode * nodeId);
	}

	// Initialize the component
	ASSERT_TRUE(fem->initialize(std::make_shared<SurgSim::Framework::Runtime>()));
	// Wake-up the component => apply the pose to the initial state
	ASSERT_TRUE(fem->wakeUp());

	EXPECT_TRUE(fem->getInitialState()->getPositions().isApprox(expectedX));
	EXPECT_TRUE(fem->getInitialState()->getVelocities().isApprox(expectedV));
	EXPECT_TRUE(fem->getInitialState()->getAccelerations().isApprox(expectedA));
}

TEST(Fem1DRepresentationTests, DoWakeUpTest)
{
	using SurgSim::Math::LinearSolveAndInverse;
	using SurgSim::Math::LinearSolveAndInverseTriDiagonalBlockMatrix;

	std::shared_ptr<MockFem1DRepresentation> fem = std::make_shared<MockFem1DRepresentation>("Fem1D");
	std::shared_ptr<DeformableRepresentationState> initialState = std::make_shared<DeformableRepresentationState>();
	initialState->setNumDof(fem->getNumDofPerNode(), 2);
	fem->setInitialState(initialState);

	// Initialize the component
	ASSERT_TRUE(fem->initialize(std::make_shared<SurgSim::Framework::Runtime>()));
	// Wake-up the component => create the proper ode solver with the specialized linear solver
	ASSERT_TRUE(fem->wakeUp());

	// Test that the OdeSolver has the proper linear solver type
	EXPECT_NE(nullptr, fem->getOdeSolver());
	std::shared_ptr<LinearSolveAndInverse> linearSolver = fem->getOdeSolver()->getLinearSolver();
	EXPECT_NE(nullptr, linearSolver);
	std::shared_ptr<LinearSolveAndInverseTriDiagonalBlockMatrix<6>> expectedLinearSolverType;
	expectedLinearSolverType = std::dynamic_pointer_cast<LinearSolveAndInverseTriDiagonalBlockMatrix<6>>(linearSolver);
	EXPECT_NE(nullptr, expectedLinearSolverType);
}

} // namespace Physics

} // namespace SurgSim
