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

#include "SurgSim/Physics/RigidCollisionRepresentation.h"

#include "SurgSim/Framework/FrameworkConvert.h"
#include "SurgSim/Framework/ObjectFactory.h"
#include "SurgSim/Math/Shape.h"
#include "SurgSim/Physics/RigidRepresentationBase.h"

namespace
{
SURGSIM_REGISTER(SurgSim::Framework::Component, SurgSim::Physics::RigidCollisionRepresentation);
}

namespace SurgSim
{
namespace Physics
{

RigidCollisionRepresentation::RigidCollisionRepresentation(const std::string& name):
	Representation(name)
{
	SURGSIM_ADD_SERIALIZABLE_PROPERTY(RigidCollisionRepresentation,
		std::shared_ptr<SurgSim::Physics::RigidRepresentationBase>,
		PhysicsRepresentation, getRigidRepresentation, setRigidRepresentation);
}

RigidCollisionRepresentation::~RigidCollisionRepresentation()
{
}

void RigidCollisionRepresentation::setRigidRepresentation(
	std::shared_ptr<SurgSim::Physics::RigidRepresentationBase> representation)
{
	m_physicsRepresentation = representation;
}

std::shared_ptr<SurgSim::Physics::RigidRepresentationBase> RigidCollisionRepresentation::getRigidRepresentation()
{
	return m_physicsRepresentation.lock();
}

int RigidCollisionRepresentation::getShapeType() const
{
	auto physicsRepresentation = m_physicsRepresentation.lock();
	SURGSIM_ASSERT(physicsRepresentation != nullptr) <<
			"PhysicsRepresentation went out of scope for Collision Representation " << getName();
	return physicsRepresentation->getCurrentParameters().getShapeUsedForMassInertia()->getType();
}

const std::shared_ptr<SurgSim::Math::Shape> RigidCollisionRepresentation::getShape() const
{
	auto physicsRepresentation = m_physicsRepresentation.lock();
	SURGSIM_ASSERT(physicsRepresentation != nullptr) <<
			"PhysicsRepresentation went out of scope for Collision Representation " << getName();
	return physicsRepresentation->getCurrentParameters().getShapeUsedForMassInertia();
}

SurgSim::Math::RigidTransform3d RigidCollisionRepresentation::getPose() const
{
	auto physicsRepresentation = m_physicsRepresentation.lock();
	SURGSIM_ASSERT(physicsRepresentation != nullptr) <<
			"PhysicsRepresentation went out of scope for Collision Representation " << getName();
	const SurgSim::Math::RigidTransform3d& physicsPose = physicsRepresentation->getCurrentState().getPose();
	return physicsPose * physicsRepresentation->getLocalPose().inverse() * getLocalPose();
}

}; // namespace Collision
}; // namespace SurgSim
