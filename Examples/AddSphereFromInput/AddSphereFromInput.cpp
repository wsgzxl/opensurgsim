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

#include <memory>
#include <boost/thread.hpp>

#include "Examples/AddSphereFromInput/AddSphereBehavior.h"

#include "SurgSim/Blocks/DriveElementFromInputBehavior.h"
#include "SurgSim/Devices/MultiAxis/MultiAxisDevice.h"
#include "SurgSim/Framework/BasicSceneElement.h"
#include "SurgSim/Framework/BehaviorManager.h"
#include "SurgSim/Framework/Log.h"
#include "SurgSim/Framework/Runtime.h"
#include "SurgSim/Framework/Scene.h"
#include "SurgSim/Framework/SceneElement.h"
#include "SurgSim/Graphics/OsgBoxRepresentation.h"
#include "SurgSim/Graphics/OsgCamera.h"
#include "SurgSim/Graphics/OsgManager.h"
#include "SurgSim/Graphics/OsgMaterial.h"
#include "SurgSim/Graphics/OsgPlaneRepresentation.h"
#include "SurgSim/Graphics/OsgShader.h"
#include "SurgSim/Graphics/OsgUniform.h"
#include "SurgSim/Graphics/OsgView.h"
#include "SurgSim/Graphics/OsgViewElement.h"
#include "SurgSim/Graphics/ViewElement.h"
#include "SurgSim/Input/InputManager.h"
#include "SurgSim/Math/BoxShape.h"
#include "SurgSim/Math/DoubleSidedPlaneShape.h"
#include "SurgSim/Math/Quaternion.h"
#include "SurgSim/Math/RigidTransform.h"
#include "SurgSim/Math/Vector.h"
#include "SurgSim/Physics/PhysicsManager.h"
#include "SurgSim/Physics/FixedRepresentation.h"
#include "SurgSim/Physics/RigidCollisionRepresentation.h"
#include "SurgSim/Physics/RigidRepresentationParameters.h"

using SurgSim::Blocks::DriveElementFromInputBehavior;
using SurgSim::Framework::BasicSceneElement;
using SurgSim::Framework::Logger;
using SurgSim::Framework::SceneElement;
using SurgSim::Graphics::OsgBoxRepresentation;
using SurgSim::Graphics::OsgCamera;
using SurgSim::Graphics::OsgMaterial;
using SurgSim::Graphics::OsgPlaneRepresentation;
using SurgSim::Graphics::OsgShader;
using SurgSim::Graphics::OsgUniform;
using SurgSim::Graphics::OsgViewElement;
using SurgSim::Graphics::ViewElement;
using SurgSim::Math::BoxShape;
using SurgSim::Math::DoubleSidedPlaneShape;
using SurgSim::Math::makeRigidTransform;
using SurgSim::Math::Quaterniond;
using SurgSim::Math::Vector3d;
using SurgSim::Math::Vector4f;
using SurgSim::Physics::FixedRepresentation;
using SurgSim::Physics::Representation;
using SurgSim::Physics::PhysicsManager;
using SurgSim::Physics::RigidRepresentationParameters;


std::shared_ptr<SceneElement> createPlane(const std::string& name)
{
	std::shared_ptr<DoubleSidedPlaneShape> planeShape = std::make_shared<DoubleSidedPlaneShape>();

	std::shared_ptr<FixedRepresentation> physicsRepresentation =
		std::make_shared<FixedRepresentation>(name + " Physics");
	RigidRepresentationParameters params;
	params.setShapeUsedForMassInertia(planeShape);
	physicsRepresentation->setInitialParameters(params);

	std::shared_ptr<OsgPlaneRepresentation> graphicsRepresentation =
		std::make_shared<OsgPlaneRepresentation>(name + " Graphics");

	std::shared_ptr<OsgMaterial> material = std::make_shared<OsgMaterial>();
	std::shared_ptr<OsgShader> shader = std::make_shared<OsgShader>();

	std::shared_ptr<OsgUniform<Vector4f>> uniform = std::make_shared<OsgUniform<Vector4f>>("color");
	uniform->set(Vector4f(0.0f, 0.6f, 1.0f, 0.0f));
	material->addUniform(uniform);

	shader->setFragmentShaderSource(
		"uniform vec4 color;\n"
		"void main(void)\n"
		"{\n"
		"	gl_FragColor = color;\n"
		"}");
	material->setShader(shader);
	graphicsRepresentation->setMaterial(material);

	std::shared_ptr<SceneElement> planeElement = std::make_shared<BasicSceneElement>(name);
	planeElement->addComponent(physicsRepresentation);
	planeElement->addComponent(graphicsRepresentation);

	auto rigidCollision = std::make_shared<SurgSim::Physics::RigidCollisionRepresentation>("Plane Collision");
	physicsRepresentation->setCollisionRepresentation(rigidCollision);
	planeElement->addComponent(rigidCollision);

	return planeElement;
}


std::shared_ptr<SceneElement> createBox(const std::string& name)
{
	std::shared_ptr<BoxShape> box = std::make_shared<BoxShape>(0.2, 0.2, 0.2); // in m

	std::shared_ptr<OsgBoxRepresentation> graphicsRepresentation =
		std::make_shared<OsgBoxRepresentation>(name + "-Graphics");
	graphicsRepresentation->setSizeXYZ(box->getSizeX(), box->getSizeY(), box->getSizeZ());

	std::shared_ptr<SurgSim::Input::InputComponent> inputComponent =
		std::make_shared<SurgSim::Input::InputComponent>("input");
	inputComponent->setDeviceName("MultiAxisDevice");
	std::shared_ptr<SceneElement> boxElement = std::make_shared<BasicSceneElement>(name);
	boxElement->addComponent(graphicsRepresentation);
	boxElement->addComponent(inputComponent);

	std::shared_ptr<DriveElementFromInputBehavior> driver;
	driver = std::make_shared<DriveElementFromInputBehavior>("Driver");
	driver->setSource(inputComponent);
	boxElement->addComponent(driver);

	auto addSphere = std::make_shared<AddSphereFromInputBehavior>("SphereAdder");
	addSphere->setInputComponent(inputComponent);
	boxElement->addComponent(addSphere);

	return boxElement;
}


int main(int argc, char* argv[])
{
	std::shared_ptr<SurgSim::Graphics::OsgManager> graphicsManager = std::make_shared<SurgSim::Graphics::OsgManager>();
	std::shared_ptr<PhysicsManager> physicsManager = std::make_shared<PhysicsManager>();
	std::shared_ptr<SurgSim::Framework::BehaviorManager> behaviorManager =
		std::make_shared<SurgSim::Framework::BehaviorManager>();
	std::shared_ptr<SurgSim::Input::InputManager> inputManager = std::make_shared<SurgSim::Input::InputManager>();

	////////////////////////////////////////////////////////////////////////////////////////////////////////////////
	std::shared_ptr<SurgSim::Device::MultiAxisDevice> toolDevice =
		std::make_shared<SurgSim::Device::MultiAxisDevice>("MultiAxisDevice");
	toolDevice->setPositionScale(toolDevice->getPositionScale() * 10.0);
	toolDevice->setOrientationScale(toolDevice->getOrientationScale() * 3.0);
	SURGSIM_ASSERT(toolDevice->initialize() == true) <<
			"Could not initialize device '" << toolDevice->getName() << "' for the tool.";

	inputManager->addDevice(toolDevice);
	////////////////////////////////////////////////////////////////////////////////////////////////////////////////

	std::shared_ptr<SurgSim::Framework::Runtime> runtime(new SurgSim::Framework::Runtime());
	runtime->addManager(physicsManager);
	runtime->addManager(graphicsManager);
	runtime->addManager(behaviorManager);
	runtime->addManager(inputManager);

	std::shared_ptr<SurgSim::Framework::Scene> scene = runtime->getScene();
	scene->addSceneElement(createBox("box"));

	std::shared_ptr<SceneElement> plane = createPlane("plane");
	plane->setPose(makeRigidTransform(Quaterniond::Identity(), Vector3d(0.0, -1.0, 0.0)));
	scene->addSceneElement(plane);

	std::shared_ptr<ViewElement> viewElement = std::make_shared<OsgViewElement>("view");
	viewElement->getView()->setPosition(0, 0);
	viewElement->getView()->setDimensions(1023, 768);
	viewElement->setPose(makeRigidTransform(Quaterniond::Identity(), Vector3d(0.0, 0.5, 5.0)));
	scene->addSceneElement(viewElement);

	runtime->execute();

	return 0;
}
