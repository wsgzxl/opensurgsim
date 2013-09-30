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

#include <SurgSim/Collision/UnitTests/ContactCalculationTestsCommon.h>
#include <SurgSim/Collision/SpherePlaneDcdContact.h>

using SurgSim::Physics::SphereShape;
using SurgSim::Physics::PlaneShape;

namespace SurgSim
{
namespace Collision
{

void doSpherePlaneTest(std::shared_ptr<SphereShape> sphere,
					   const Quaterniond& sphereQuat,
					   const Vector3d& sphereTrans,
					   std::shared_ptr<PlaneShape> plane,
					   const Quaterniond& planeQuat,
					   const Vector3d& planeTrans,
					   bool expectedIntersect,
					   const double& expectedDepth = 0,
					   const Vector3d& expectedNorm = Vector3d::Zero())
{
	std::shared_ptr<CollisionRepresentation> planeRep =
		std::make_shared<MockCollisionRepresentation>("Plane Shape",plane,planeQuat,planeTrans);
	std::shared_ptr<CollisionRepresentation> sphereRep =
		std::make_shared<MockCollisionRepresentation>("Sphere Shape",sphere,sphereQuat,sphereTrans);

	SpherePlaneDcdContact calcNormal;
	std::shared_ptr<CollisionPair> pair = std::make_shared<CollisionPair>(sphereRep, planeRep);

	// Again this replicates the way this is calculated in the contact calculation just with different
	// starting values
	Vector3d spherePenetration = sphereTrans - expectedNorm * sphere->getRadius();
	Vector3d planePenetration = sphereTrans - expectedNorm * (sphere->getRadius() - expectedDepth);

	calcNormal.calculateContact(pair);
	if (expectedIntersect)
	{
		ASSERT_TRUE(pair->hasContacts());
		std::shared_ptr<Contact> contact = pair->getContacts().front();
		EXPECT_NEAR(expectedDepth, contact->depth, SurgSim::Math::Geometry::DistanceEpsilon);
		EXPECT_TRUE(eigenEqual(expectedNorm, contact->normal));
		EXPECT_TRUE(contact->penetrationPoints.first.globalPosition.hasValue());
		EXPECT_TRUE(contact->penetrationPoints.second.globalPosition.hasValue());
		EXPECT_TRUE(eigenEqual(spherePenetration,
							   contact->penetrationPoints.first.globalPosition.getValue()));
		EXPECT_TRUE(eigenEqual(planePenetration,
							   contact->penetrationPoints.second.globalPosition.getValue()));
	}
	else
	{
		EXPECT_FALSE(pair->hasContacts());
	}
}

TEST(SpherePlaneContactCalculationTests, UnitTests)
{
	std::shared_ptr<PlaneShape> plane = std::make_shared<PlaneShape>();
	std::shared_ptr<SphereShape> sphere = std::make_shared<SphereShape>(1.0);

	{
		SCOPED_TRACE("No Intersection, no transformation");
		doSpherePlaneTest(sphere, Quaterniond::Identity(), Vector3d(0.0,2.0,0.0),
						  plane, Quaterniond::Identity(), Vector3d(0.0,0.5,0.0), false);
	}

	{
		SCOPED_TRACE("Intersection front, no transformation");
		doSpherePlaneTest(sphere, Quaterniond::Identity(), Vector3d(0.0,1.0,0.0),
						  plane,Quaterniond::Identity(), Vector3d(0.0,0.5,0.0),
						  true, 0.5, Vector3d(0.0,1.0,0.0));
	}

	{
		SCOPED_TRACE("Intersection back, no transformation");
		doSpherePlaneTest(sphere, Quaterniond::Identity(), Vector3d(0.0,0.0,0.0),
						  plane, Quaterniond::Identity(), Vector3d(0.0,0.5,0.0),
						  true, 1.5, Vector3d(0.0,1.0,0.0));
	}

	{
		SCOPED_TRACE("Intersection front, sphere center on the plane, rotated plane");
		doSpherePlaneTest(sphere, Quaterniond::Identity(), Vector3d(0.0,0,0.0),
						  plane, SurgSim::Math::makeRotationQuaternion(M_PI_2, Vector3d(1.0,0.0,0.0)),
						  Vector3d(0.0,0.0,0.0), true, 1.0, Vector3d(0.0,0.0,1.0));
	}

	{
		SCOPED_TRACE("Intersection front, rotated Plane");
		doSpherePlaneTest(sphere, Quaterniond::Identity(), Vector3d(0.0,0.0,0.5),
						  plane, SurgSim::Math::makeRotationQuaternion(M_PI_2, Vector3d(1.0,0.0,0.0)),
						  Vector3d(0.0,0.0,0.0), true, 0.5, Vector3d(0.0,0.0,1.0));
	}
}

}; // namespace Collision
}; // namespace SurgSim