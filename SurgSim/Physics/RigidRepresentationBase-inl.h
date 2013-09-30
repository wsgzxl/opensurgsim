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

#ifndef SURGSIM_PHYSICS_RIGIDREPRESENTATIONBASE_INL_H
#define SURGSIM_PHYSICS_RIGIDREPRESENTATIONBASE_INL_H

namespace SurgSim
{

namespace Physics
{

template <class T>
std::shared_ptr<T> SurgSim::Physics::RigidRepresentationBase::createTypedLocalization(const Location& location)
{
	// Change when we deal with the meshes as shapes
	std::shared_ptr<T> result = std::make_shared<T>();

	SURGSIM_ASSERT(location.globalPosition.hasValue() || location.rigidLocalPosition.hasValue()) <<
		"Tried to create a rigid localization without valid position information";

	SurgSim::Math::Vector3d localPosition;
	if (!location.rigidLocalPosition.hasValue())
	{
		localPosition = this->getCurrentPose().inverse() * location.globalPosition.getValue();
	}
	else
	{
		localPosition = location.rigidLocalPosition.getValue();
	}

	result->setLocalPosition(localPosition);

	return std::move(result);
}

}; // Physics

}; // SurgSim

#endif // SURGSIM_PHYSICS_RIGIDREPRESENTATIONBASE_INL_H