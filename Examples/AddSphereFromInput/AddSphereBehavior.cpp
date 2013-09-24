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

#include <Examples/AddSphereFromInput/AddSphereBehavior.h>

#include <SurgSim/Blocks/SphereElement.h>
#include <SurgSim/DataStructures/DataGroup.h>
#include <SurgSim/Framework/Behavior.h>
#include <SurgSim/Framework/SceneElement.h>
#include <SurgSim/Input/InputComponent.h>
#include <SurgSim/Framework/Scene.h>

using SurgSim::Math::RigidTransform3d;

namespace SurgSim
{

namespace Input
{
	AddSphereFromInputBehavior::AddSphereFromInputBehavior(
		const std::string& name, std::shared_ptr<SurgSim::Input::InputComponent> from):
		SurgSim::Framework::Behavior(name), m_from(from)
	{
	}

	void AddSphereFromInputBehavior::update(double dt)
	{
		SurgSim::DataStructures::DataGroup dataGroup;
		m_from->getData(&dataGroup);
		RigidTransform3d pose;
		dataGroup.poses().get("pose", &pose);

		// Dynamically add sphere to the scene from input
		static int m_numElements = 0;
		static bool previouslyPressed = false;
		bool button1;
		dataGroup.booleans().get("button1", &button1);
		if (button1 && ! previouslyPressed )
		{
			std::string name = "sphereId_" + m_numElements++;
			std::shared_ptr<SurgSim::Framework::SceneElement> m_element =
				std::make_shared<SurgSim::Blocks::SphereElement>(name, pose);

			getScene()->addSceneElement(m_element);
		}
		previouslyPressed = !button1;
	}

	bool AddSphereFromInputBehavior::doInitialize()
	{
		return true;
	}

	bool AddSphereFromInputBehavior::doWakeUp()
	{
		return true;
	}

};  // namespace Blocks

};  // namespace SurgSim
