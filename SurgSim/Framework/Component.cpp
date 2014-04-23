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

#include "SurgSim/Framework/Component.h"

#include <boost/uuid/random_generator.hpp>

#include "SurgSim/Framework/Assert.h"
#include "SurgSim/Framework/LogMacros.h"

namespace SurgSim
{
namespace Framework
{

// Forward References
class Runtime;
class Scene;
class SceneElement;

Component::Component(const std::string& name) :
	m_name(name),
	m_uuid(boost::uuids::random_generator()()),
	m_didInit(false),
	m_didWakeUp(false),
	m_isInitialized(false),
	m_isAwake(false)
{
}

Component::~Component()
{
}

std::string Component::getName() const
{
	return m_name;
}

void Component::setName(const std::string& name)
{
	m_name = name;
}

bool Component::isInitialized() const
{
	return m_isInitialized;
}

bool Component::initialize(const std::weak_ptr<Runtime>& runtime)
{
	SURGSIM_ASSERT(!m_didInit) << "Double initialization called in component " << getName();
	SURGSIM_ASSERT(!runtime.expired()) << "Runtime cannot be expired at initialization in component " << getName();
	m_runtime = runtime;

	m_didInit = true;
	m_isInitialized = doInitialize();

	return m_isInitialized;
}

bool Component::isAwake() const
{
	return m_isAwake;
}

bool Component::wakeUp()
{
	SURGSIM_ASSERT(! m_didWakeUp) << "Double wakeup called on component." << getName();
	SURGSIM_ASSERT(m_didInit) << "Component " << getName() << " was awoken without being initialized.";
	SURGSIM_ASSERT(m_isInitialized) << "Wakeup called even though initialization failed on component." << getName();

	m_didWakeUp = true;
	m_isAwake = doWakeUp();

	return m_isAwake;
}

void Component::setScene(std::weak_ptr<Scene> scene)
{
	m_scene = scene;
}

std::shared_ptr<Scene> Component::getScene()
{
	return m_scene.lock();
}

void Component::setSceneElement(std::weak_ptr<SceneElement> sceneElement)
{
	m_sceneElement = sceneElement;
}

std::shared_ptr<SceneElement> Component::getSceneElement()
{
	return m_sceneElement.lock();
}

std::shared_ptr<Runtime> Component::getRuntime() const
{
	return m_runtime.lock();
}

boost::uuids::uuid Component::getUuid() const
{
	return m_uuid;
}

Component::FactoryType& Component::getFactory()
{
	static FactoryType factory;
	return factory;
}

std::string Component::getClassName() const
{
	SURGSIM_LOG_WARNING(Logger::getDefaultLogger()) << "getClassName() called on Component base class, this is wrong" <<
			" in almost all cases, this means there is a class that does not have getClassName() defined.";
	return "SurgSim::Framework::Component";
}

std::shared_ptr<Component> Component::getSharedPtr()
{
	std::shared_ptr<Component> result;
	try
	{
		result = shared_from_this();
	}
	catch (const std::exception&)
	{
		SURGSIM_FAILURE() << "Component was not created as a shared_ptr.";
	}
	return result;
}

}; // namespace Framework
}; // namespace SurgSim
