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

#ifndef SURGSIM_INPUT_INPUTCOMPONENT_H
#define SURGSIM_INPUT_INPUTCOMPONENT_H

#include <SurgSim/Framework/Component.h>
#include <SurgSim/Input/InputConsumerInterface.h>
#include <SurgSim/Input/DeviceInterface.h>
#include <SurgSim/DataStructures/DataGroup.h>
#include <SurgSim/Framework/LockedContainer.h>

namespace SurgSim
{
namespace Input
{
class InputConsumer;

/// InputComponent combines the Component interface and the InputConsumerInterface so that input devices can
/// provide input through the normal component interface. Multiple InputComponents can be added to
/// the same device.
class InputComponent : public SurgSim::Framework::Component
{
public:
	InputComponent(std::string name, std::string deviceName);
	virtual ~InputComponent();

	bool isDeviceConnected();
	void connectDevice(std::shared_ptr<SurgSim::Input::DeviceInterface> device);
	void disconnectDevice(std::shared_ptr<SurgSim::Input::DeviceInterface> device);

	/// Gets the input data.
	/// \param [out] dataGroup The location to write the data.  The pointer must be non-null.
	void getData(SurgSim::DataStructures::DataGroup* dataGroup);

	/// Overriden from Component, do nothing
	virtual bool doInitialize();

	/// Overriden from Component, do nothing
	virtual bool doWakeUp();

	/// Gets device name.
	/// \return	The device name.
	std::string getDeviceName() const;

private:
	std::string m_deviceName;
	bool m_deviceConnected;
	std::shared_ptr<InputConsumer> m_input;
};

}; // namespace Input
}; // namespace SurgSim


#endif
