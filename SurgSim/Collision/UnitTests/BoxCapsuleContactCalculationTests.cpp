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

#include "SurgSim/Collision/BoxCapsuleDcdContact.h"
#include "SurgSim/Collision/Representation.h"
#include "SurgSim/Collision/ShapeCollisionRepresentation.h"
#include "SurgSim/Math/BoxShape.h"
#include "SurgSim/Math/CapsuleShape.h"
#include "SurgSim/Math/Quaternion.h"
#include "SurgSim/Math/RigidTransform.h"

using SurgSim::Math::BoxShape;
using SurgSim::Math::CapsuleShape;
using SurgSim::Math::makeRigidTransform;
using SurgSim::Math::makeRotationQuaternion;
using SurgSim::Math::Quaterniond;
using SurgSim::Math::Vector3d;

namespace SurgSim
{
namespace Collision
{

void doBoxCapsuleTest(std::shared_ptr<BoxShape> box,
					  const Quaterniond& boxQuat,
					  const Vector3d& boxTrans,
					  std::shared_ptr<CapsuleShape> capsule,
					  const Quaterniond& capsuleQuat,
					  const Vector3d& capsuleTrans,
					  const bool expectedInContact)
{
	std::shared_ptr<ShapeCollisionRepresentation> boxRep =
		std::make_shared<ShapeCollisionRepresentation>("Collision Box 0");
	boxRep->setShape(box);
	boxRep->setLocalPose(makeRigidTransform(boxQuat, boxTrans));

	std::shared_ptr<ShapeCollisionRepresentation> capsuleRep =
		std::make_shared<ShapeCollisionRepresentation>("Collision Capsule 0");
	capsuleRep->setShape(capsule);
	capsuleRep->setLocalPose(makeRigidTransform(capsuleQuat, capsuleTrans));

	// Perform collision detection.
	BoxCapsuleDcdContact calcContact;
	std::shared_ptr<CollisionPair> pair = std::make_shared<CollisionPair>(boxRep, capsuleRep);
	calcContact.calculateContact(pair);

	EXPECT_EQ(expectedInContact, pair->hasContacts());

	if (expectedInContact)
	{
		Vector3d capsuleToBox = boxTrans - capsuleTrans;

		double depthMax = box->getSize().norm();
		depthMax += capsule->getLength() / 2.0 + capsule->getRadius();

		auto contacts = pair->getContacts();
		for (auto contact=contacts.cbegin(); contact!=contacts.cend(); ++contact)
		{
			if (! capsuleToBox.isZero())
			{
				// Check that each normal is pointing into the box
				EXPECT_LT(0.0, (*contact)->normal.dot(capsuleToBox));
			}

			// Check that the depth is sane
			EXPECT_LT(0.0, (*contact)->depth);
			EXPECT_GT(depthMax, (*contact)->depth);

			// Check that the locations are sane
			Vector3d boxPenetrationPoint = (*contact)->penetrationPoints.first.globalPosition.getValue();
			Vector3d capsulePenetrationPoint = (*contact)->penetrationPoints.second.globalPosition.getValue();
			EXPECT_GT(0.0, (*contact)->normal.dot(boxPenetrationPoint - boxTrans));
			EXPECT_LT(0.0, (*contact)->normal.dot(capsulePenetrationPoint - capsuleTrans));
		}
	}
}

TEST(BoxCapsuleContactCalculationTests, UnitTests)
{
	std::shared_ptr<BoxShape> box = std::make_shared<BoxShape>(1.0, 1.0, 1.0);
	std::shared_ptr<CapsuleShape> capsule = std::make_shared<CapsuleShape>(4.0, 1.0);
	Quaterniond boxQuat;
	Vector3d boxTrans;
	Quaterniond capsuleQuat;
	Vector3d capsuleTrans;

	{
		SCOPED_TRACE("No intersection, box in front of capsule");
		boxQuat = Quaterniond::Identity();
		boxTrans = Vector3d(10.6, 0.0, 0.0);
		capsuleQuat = Quaterniond::Identity();
		capsuleTrans = Vector3d::Zero();
		bool expectedInContact = false;
		doBoxCapsuleTest(box, boxQuat, boxTrans, capsule, capsuleQuat, capsuleTrans, expectedInContact);
	}

	{
		SCOPED_TRACE("No intersection, capsule beyond corner of box");
		boxQuat = Quaterniond::Identity();
		boxTrans = Vector3d::Zero();
		capsuleQuat = Quaterniond::Identity();
		bool expectedInContact = false;
		capsuleTrans = Vector3d(1.5, 0.0, 1.5);
		doBoxCapsuleTest(box, boxQuat, boxTrans, capsule, capsuleQuat, capsuleTrans, expectedInContact);
		capsuleTrans = Vector3d(1.5, 0.0, -1.5);
		doBoxCapsuleTest(box, boxQuat, boxTrans, capsule, capsuleQuat, capsuleTrans, expectedInContact);
		capsuleTrans = Vector3d(-1.5, 0.0, 1.5);
		doBoxCapsuleTest(box, boxQuat, boxTrans, capsule, capsuleQuat, capsuleTrans, expectedInContact);
		capsuleTrans = Vector3d(-1.5, 0.0, -1.5);
		doBoxCapsuleTest(box, boxQuat, boxTrans, capsule, capsuleQuat, capsuleTrans, expectedInContact);
	}

	{
		SCOPED_TRACE("No intersection, box below capsule");
		boxQuat = Quaterniond::Identity();
		boxTrans = Vector3d(0.0, -3.6, 0.0);
		capsuleQuat = Quaterniond::Identity();
		capsuleTrans = Vector3d::Zero();
		bool expectedInContact = false;
		doBoxCapsuleTest(box, boxQuat, boxTrans, capsule, capsuleQuat, capsuleTrans, expectedInContact);
	}

	{
		SCOPED_TRACE("Intersection, box intersection with capsule side");
		boxQuat = Quaterniond::Identity();
		boxTrans = Vector3d(1.0 , 0.0, 0.0);
		capsuleQuat = Quaterniond::Identity();
		capsuleTrans = Vector3d::Zero();
		bool expectedInContact = true;
		doBoxCapsuleTest(box, boxQuat, boxTrans, capsule, capsuleQuat, capsuleTrans, expectedInContact);
	}

	{
		SCOPED_TRACE("Intersection, box intersection with upside down capsule");
		boxQuat = Quaterniond::Identity();
		boxTrans = Vector3d(1.0 , 0.0, 0.0);
		capsuleQuat = makeRotationQuaternion(M_PI, Vector3d(0.0, 0.0, 1.0));
		capsuleTrans = Vector3d::Zero();
		bool expectedInContact = true;
		doBoxCapsuleTest(box, boxQuat, boxTrans, capsule, capsuleQuat, capsuleTrans, expectedInContact);
	}

	{
		SCOPED_TRACE("Intersection, box intersection with z-axis capsule");
		boxQuat = Quaterniond::Identity();
		boxTrans = Vector3d(1.0 , 0.0, 0.0);
		capsuleQuat = makeRotationQuaternion(M_PI_2, Vector3d(1.0, 0.0, 0.0));
		capsuleTrans = Vector3d::Zero();
		bool expectedInContact = true;
		doBoxCapsuleTest(box, boxQuat, boxTrans, capsule, capsuleQuat, capsuleTrans, expectedInContact);
	}

	{
		SCOPED_TRACE("Intersection, box intersection with x-axis capsule");
		boxQuat = makeRotationQuaternion(M_PI, Vector3d(0.0, 0.0, 1.0));
		boxTrans = Vector3d(1.0 , 0.0, 0.0);
		capsuleQuat = makeRotationQuaternion(M_PI_2, Vector3d(1.0, 0.0, 0.0));
		capsuleTrans = Vector3d::Zero();
		bool expectedInContact = true;
		doBoxCapsuleTest(box, boxQuat, boxTrans, capsule, capsuleQuat, capsuleTrans, expectedInContact);
	}

	{
		SCOPED_TRACE("Intersection, box intersection with capsule cap");
		boxQuat = makeRotationQuaternion(M_PI_2, Vector3d(0.0, 0.0, 1.0));
		boxTrans = Vector3d(0.1 , 0.0, 0.1);
		capsuleQuat = Quaterniond::Identity();
		capsuleTrans = Vector3d(0.0 , 2.6, 0.0);
		bool expectedInContact = true;
		doBoxCapsuleTest(box, boxQuat, boxTrans, capsule, capsuleQuat, capsuleTrans, expectedInContact);
	}

	{
		SCOPED_TRACE("No intersection, capsule near box corner, but not intersecting");
		boxQuat = Quaterniond::Identity();
		boxTrans = Vector3d(0.0 , 0.0, 0.0);
		capsuleQuat = Quaterniond::Identity();
		capsuleTrans = Vector3d(1.3 , 0.0, 1.3);
		bool expectedInContact = false;
		doBoxCapsuleTest(box, boxQuat, boxTrans, capsule, capsuleQuat, capsuleTrans, expectedInContact);
	}

	{
		SCOPED_TRACE("Intersection, capsule intersecting with box corner");
		boxQuat = Quaterniond::Identity();
		boxTrans = Vector3d(0.0 , 0.0, 0.0);
		capsuleQuat = Quaterniond::Identity();
		capsuleTrans = Vector3d(1.2 , 0.0, 1.2);
		bool expectedInContact = true;
		doBoxCapsuleTest(box, boxQuat, boxTrans, capsule, capsuleQuat, capsuleTrans, expectedInContact);
	}


	{
		SCOPED_TRACE("Intersection, box inside capsule");
		boxQuat = Quaterniond::Identity();
		boxTrans = Vector3d::Zero();
		capsuleQuat = Quaterniond::Identity();
		capsuleTrans = Vector3d::Zero();
		bool expectedInContact = true;
		doBoxCapsuleTest(box, boxQuat, boxTrans, capsule, capsuleQuat, capsuleTrans, expectedInContact);
	}

	{
		SCOPED_TRACE("Intersection, capsule inside box");
		std::shared_ptr<BoxShape> bigBox = std::make_shared<BoxShape>(10.0, 10.0, 10.0);
		boxQuat = makeRotationQuaternion(-M_PI_4, Vector3d(0.0, 1.0, 0.0));
		boxTrans = Vector3d::Zero();
		capsuleQuat = makeRotationQuaternion(M_PI, Vector3d(1.0, 0.0, 0.0));
		capsuleTrans = Vector3d(0.0, 0.0, 0.0);
		bool expectedInContact = true;
		doBoxCapsuleTest(bigBox, boxQuat, boxTrans, capsule, capsuleQuat, capsuleTrans, expectedInContact);
	}

	{
		SCOPED_TRACE("Intersection, capsule bottom at box center");
		boxQuat = Quaterniond::Identity();
		boxTrans = Vector3d::Zero();
		capsuleQuat = Quaterniond::Identity();
		capsuleTrans = Vector3d(0.0, -2.0, 0.0);
		bool expectedInContact = true;
		doBoxCapsuleTest(box, boxQuat, boxTrans, capsule, capsuleQuat, capsuleTrans, expectedInContact);
	}

	{
		SCOPED_TRACE("Intersection, capsule top at box center");
		boxQuat = Quaterniond::Identity();
		boxTrans = Vector3d::Zero();
		capsuleQuat = Quaterniond::Identity();
		capsuleTrans = Vector3d(0.0, 2.0, 0.0);
		bool expectedInContact = true;
		doBoxCapsuleTest(box, boxQuat, boxTrans, capsule, capsuleQuat, capsuleTrans, expectedInContact);
	}
}


}; // namespace Collision
}; // namespace SurgSim
