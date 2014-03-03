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

#include "SurgSim/Collision/BoxSphereDcdContact.h"

#include "SurgSim/Collision/CollisionPair.h"
#include "SurgSim/Collision/Representation.h"
#include "SurgSim/Math/BoxShape.h"
#include "SurgSim/Math/Geometry.h"
#include "SurgSim/Math/RigidTransform.h"
#include "SurgSim/Math/SphereShape.h"
#include "SurgSim/Math/Vector.h"

using SurgSim::Math::BoxShape;
using SurgSim::Math::SphereShape;
using SurgSim::Math::Vector3d;

namespace SurgSim
{
namespace Collision
{

BoxSphereDcdContact::BoxSphereDcdContact()
{
}

std::pair<int,int> BoxSphereDcdContact::getShapeTypes()
{
	return std::pair<int,int>(SurgSim::Math::SHAPE_TYPE_BOX, SurgSim::Math::SHAPE_TYPE_SPHERE);
}

void BoxSphereDcdContact::doCalculateContact(std::shared_ptr<CollisionPair> pair)
{
	using SurgSim::Math::Geometry::DistanceEpsilon;
	using SurgSim::Math::Geometry::SquaredDistanceEpsilon;

	std::shared_ptr<Representation> representationBox;
	std::shared_ptr<Representation> representationSphere;

	representationBox = pair->getFirst();
	representationSphere = pair->getSecond();

	std::shared_ptr<BoxShape> box = std::static_pointer_cast<BoxShape>(representationBox->getShape());
	std::shared_ptr<SphereShape> sphere = std::static_pointer_cast<SphereShape>(representationSphere->getShape());

	// Sphere center...
	Vector3d sphereCenter = representationSphere->getPose().translation();
	// ... in Box coordinate system.
	Vector3d boxLocalSphereCenter =  representationBox->getPose().inverse() * sphereCenter;

	// Box half size.
	Vector3d boxSize(box->getSizeX() * 0.5, box->getSizeY() * 0.5, box->getSizeZ() * 0.5);

	// Determine the closest point to the sphere center in the box
	Vector3d closestPoint = boxLocalSphereCenter;
	closestPoint.x() = std::min(boxSize.x(), closestPoint.x());
	closestPoint.x() = std::max(-boxSize.x(), closestPoint.x());
	closestPoint.y() = std::min(boxSize.y(), closestPoint.y());
	closestPoint.y() = std::max(-boxSize.y(), closestPoint.y());
	closestPoint.z() = std::min(boxSize.z(), closestPoint.z());
	closestPoint.z() = std::max(-boxSize.z(), closestPoint.z());

	// Distance between the closestPoint and boxLocalSphereCenter.  Normal points into first representation.
	Vector3d normal = closestPoint - boxLocalSphereCenter;
	double distanceSquared = normal.squaredNorm();
	if (distanceSquared - (sphere->getRadius() * sphere->getRadius()) > SquaredDistanceEpsilon)
	{
		// There is no collision.
		return;
	}

	double distance = 0.0;

	// If sphere center is inside box, it is handled differently.
	if (distanceSquared <= SquaredDistanceEpsilon)
	{
		// Sphere center is inside the box.
		// In this case closestPoint is equal to boxLocalSphereCenter.
		// Find which face of the box which is closest to the closestPoint.
		// abs(boxSize.x - closestPoint.x) and abs(-boxSize.x - closestPoint.x) are the distances between the
		// closestPoint and the two faces (along x-axis) of the box.
		// But since the abs(closestPoint.x) will always <= boxSize.x (because the point is inside box),
		// (boxSize.x() - abs(closestPoint.x())) gives the distance of the face from the closestPoint in x-axis.
		// This value is calculated for all the axes. The axis with the minimum value is assumed to contain the
		// colliding face.
		double distancesFromFaces[3] = {boxSize.x() - std::abs(closestPoint.x()),
										boxSize.y() - std::abs(closestPoint.y()),
										boxSize.z() - std::abs(closestPoint.z())};
		int minimumDistanceId = SurgSim::Math::indexOfMinimum(distancesFromFaces[0], distancesFromFaces[1],
															  distancesFromFaces[2]);
		// The mininumDistanceId gives the normal of the closet face.
		double direction = closestPoint[minimumDistanceId] > -DistanceEpsilon ? 1.0 : -1.0;
		normal.setZero();
		normal[minimumDistanceId] = direction;
		closestPoint[minimumDistanceId] = boxSize[minimumDistanceId] * direction;
		distance = -std::abs(distancesFromFaces[minimumDistanceId]);
	}
	else
	{
		// Sphere center is outside box.
		distance = normal.norm();
		normal /= distance;
	}

	// Transform normal into global pose.
	normal = representationBox->getPose().linear() * normal;

	// Create the contact.
	std::pair<Location,Location> penetrationPoints;
	penetrationPoints.first.globalPosition.setValue(representationBox->getPose() * closestPoint);
	penetrationPoints.second.globalPosition.setValue(sphereCenter + (normal * sphere->getRadius()));

	pair->addContact(std::abs(distance - sphere->getRadius()), normal, penetrationPoints);
}

}; // namespace Collision
}; // namespace SurgSim
