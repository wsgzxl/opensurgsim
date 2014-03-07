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

#include "SurgSim/Math/MlcpConstraintType.h"
#include "SurgSim/Math/Vector.h"
#include "SurgSim/Physics/ConstraintData.h"
#include "SurgSim/Physics/Representation.h"
#include "SurgSim/Physics/RigidRepresentation.h"
#include "SurgSim/Physics/RigidRepresentationBilateral3D.h"
#include "SurgSim/Physics/RigidRepresentationLocalization.h"
#include "SurgSim/Physics/UnitTests/EigenGtestAsserts.h"

using SurgSim::Math::Vector3d;

namespace
{
const double epsilon = 1e-10;
const double dt = 1e-3;
};

namespace SurgSim
{
namespace Physics
{

TEST(RigidRepresentationBilateral3DTests, Constructor)
{
	ASSERT_NO_THROW(
		{ RigidRepresentationBilateral3D constraint; });
}

TEST(RigidRepresentationBilateral3DTests, Constants)
{
	RigidRepresentationBilateral3D constraint;

	EXPECT_EQ(SurgSim::Math::MLCP_BILATERAL_3D_CONSTRAINT, constraint.getMlcpConstraintType());
	EXPECT_EQ(SurgSim::Physics::REPRESENTATION_TYPE_RIGID, constraint.getRepresentationType());
	EXPECT_EQ(3u, constraint.getNumDof());
}

TEST(RigidRepresentationBilateral3DTests, BuildMlcp)
{
	// Whitebox test which validates ConstraintImplementation::build's output parameter, MlcpPhysicsProblem.  It assumes
	// CHt and HCHt can be correctly built given H, so it does not neccessarily construct the physical parameters
	// neccessary to supply a realistic C.  It only checks H and b.
	RigidRepresentationBilateral3D constraint;

	Vector3d actual = Vector3d(8.0, 6.4, 3.5);

	// Setup parameters for RigidRepresentationBilateral3D::build
	auto localization
		= std::make_shared<RigidRepresentationLocalization>(std::make_shared<RigidRepresentation>("representation"));
	localization->setLocalPosition(actual);

	MlcpPhysicsProblem mlcpPhysicsProblem = MlcpPhysicsProblem::Zero(6, 3, 1);

	ConstraintData emptyConstraint;

	ASSERT_NO_THROW(constraint.build(
		dt, emptyConstraint, localization, &mlcpPhysicsProblem, 0, 0, SurgSim::Physics::CONSTRAINT_POSITIVE_SIDE));

	// Compare results
	Eigen::Matrix<double, 3, 1> violation = actual;
	EXPECT_NEAR_EIGEN(violation, mlcpPhysicsProblem.b, epsilon);

	Eigen::Matrix<double, 3, 6> H = Eigen::Matrix<double, 3, 6>::Zero();
	Eigen::Matrix<double, 3, 3> identity = Eigen::Matrix<double, 3, 3>::Identity();
	SurgSim::Math::setSubMatrix(dt * identity,
		0, 0, 3, 3, &H);
	SurgSim::Math::setSubMatrix(Eigen::DiagonalMatrix<double, 3, 3>(dt * actual),
		0, 1, 3, 3, &H);
	EXPECT_NEAR_EIGEN(H, mlcpPhysicsProblem.H, epsilon);

	EXPECT_EQ(0u, mlcpPhysicsProblem.constraintTypes.size());
}

TEST(RigidRepresentationBilateral3DTests, BuildMlcpTwoStep)
{
	// Whitebox test which validates ConstraintImplementation::build's output parameter, MlcpPhysicsProblem.  It assumes
	// CHt and HCHt can be correctly built given H, so it does not neccessarily construct the physical parameters
	// neccessary to supply a realistic C.  It only checks H and b.
	RigidRepresentationBilateral3D constraint;

	Vector3d actual = Vector3d(8.0, 6.4, 3.5);
	Vector3d desired = Vector3d(3.0, 7.7, 0.0);

	// Setup parameters for RigidRepresentationBilateral3D::build
	MlcpPhysicsProblem mlcpPhysicsProblem = MlcpPhysicsProblem::Zero(12, 3, 1);

	ConstraintData emptyConstraint;

	auto localization
		= std::make_shared<RigidRepresentationLocalization>(std::make_shared<RigidRepresentation>("representation"));

	localization->setLocalPosition(actual);
	ASSERT_NO_THROW(constraint.build(
		dt, emptyConstraint, localization, &mlcpPhysicsProblem, 0, 0, SurgSim::Physics::CONSTRAINT_POSITIVE_SIDE));

	localization->setLocalPosition(desired);
	ASSERT_NO_THROW(constraint.build(
		dt, emptyConstraint, localization, &mlcpPhysicsProblem, 6, 0, SurgSim::Physics::CONSTRAINT_NEGATIVE_SIDE));

	// Compare results
	Eigen::Matrix<double, 3, 1> violation = actual - desired;
	EXPECT_NEAR_EIGEN(violation, mlcpPhysicsProblem.b, epsilon);

	Eigen::Matrix<double, 3, 12> H = Eigen::Matrix<double, 3, 12>::Zero();
	Eigen::Matrix<double, 3, 3> identity = Eigen::Matrix<double, 3, 3>::Identity();
	SurgSim::Math::setSubMatrix(dt * identity,
		0, 0, 3, 3, &H);
	SurgSim::Math::setSubMatrix(Eigen::DiagonalMatrix<double, 3, 3>(dt * actual),
		0, 1, 3, 3, &H);
	SurgSim::Math::setSubMatrix(-dt * identity,
		0, 2, 3, 3, &H);
	SurgSim::Math::setSubMatrix(Eigen::DiagonalMatrix<double, 3, 3>(-dt * desired),
		0, 3, 3, 3, &H);
	EXPECT_NEAR_EIGEN(H, mlcpPhysicsProblem.H, epsilon);
}

};  //  namespace Physics
};  //  namespace SurgSim
