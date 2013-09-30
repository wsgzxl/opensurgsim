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

#include <SurgSim/Collision/SphereSphereDcdContact.h>

#include <SurgSim/Math/RigidTransform.h>
#include <SurgSim/Collision/CollisionPair.h>
#include <SurgSim/Physics/SphereShape.h>

using SurgSim::Physics::SphereShape;

namespace SurgSim
{
namespace Collision
{

void SphereSphereDcdContact::doCalculateContact(std::shared_ptr<CollisionPair> pair)
{
	std::shared_ptr<SphereShape> firstSphere = std::static_pointer_cast<SphereShape>(pair->getFirst()->getShape());
	std::shared_ptr<SphereShape> secondSphere = std::static_pointer_cast<SphereShape>(pair->getSecond()->getShape());

	Vector3d firstCenter = pair->getFirst()->getPose().translation();
	Vector3d secondCenter = pair->getSecond()->getPose().translation();

	Vector3d normal = firstCenter - secondCenter;
	double dist = normal.norm();
	double maxDist = firstSphere->getRadius() + secondSphere->getRadius();
	if (dist < maxDist)
	{
		std::pair<Location,Location> penetrationPoints;
		normal.normalize();
		penetrationPoints.first.globalPosition.setValue(firstCenter - normal * firstSphere->getRadius());
		penetrationPoints.second.globalPosition.setValue(secondCenter + normal * secondSphere->getRadius());

		pair->addContact(maxDist - dist, normal, penetrationPoints);
	}
}

}; // namespace Collision
}; // namespace SurgSim