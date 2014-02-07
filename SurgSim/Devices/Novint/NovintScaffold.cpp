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

#include "SurgSim/Devices/Novint/NovintScaffold.h"

#include <vector>
#include <memory>
#include <algorithm>
#include <array>

#include <boost/thread/mutex.hpp>
#include <boost/thread/locks.hpp>
#include <boost/thread/thread.hpp>

#include <hdl/hdl.h>

#include "SurgSim/Devices/Novint/NovintDevice.h"
#include "SurgSim/Math/Vector.h"
#include "SurgSim/Math/Matrix.h"
#include "SurgSim/Math/RigidTransform.h"
#include "SurgSim/Framework/Assert.h"
#include "SurgSim/Framework/Log.h"
#include "SurgSim/Framework/Clock.h"
#include "SurgSim/Framework/SharedInstance.h"
#include "SurgSim/DataStructures/DataGroup.h"
#include "SurgSim/DataStructures/DataGroupBuilder.h"

using SurgSim::Math::Vector3d;
using SurgSim::Math::Matrix44d;
using SurgSim::Math::Matrix33d;
using SurgSim::Math::makeRotationMatrix;
using SurgSim::Math::RigidTransform3d;

using SurgSim::Framework::Clock;

using SurgSim::DataStructures::DataGroup;
using SurgSim::DataStructures::DataGroupBuilder;


namespace SurgSim
{
namespace Device
{


class NovintScaffold::Handle
{
public:
	Handle() :
		m_deviceHandle(HDL_INVALID_HANDLE),
		m_scaffold(NovintScaffold::getOrCreateSharedInstance())
	{
	}

	Handle(const std::string& deviceName, const std::string& initializationName) :
		m_deviceHandle(HDL_INVALID_HANDLE),
		m_scaffold(NovintScaffold::getOrCreateSharedInstance())
	{
		create(deviceName, initializationName);
	}

	~Handle()
	{
		SURGSIM_ASSERT(! isValid()) << "Expected destroy() to be called before Handle object destruction.";
	}

	bool isValid() const
	{
		return (m_deviceHandle != HDL_INVALID_HANDLE);
	}

	bool create(const std::string& deviceName, const std::string& initializationName)
	{
		SURGSIM_ASSERT(! isValid());

		HDLDeviceHandle deviceHandle = HDL_INVALID_HANDLE;
		std::string hdalName = initializationName;
		const char* hdalNameToPassSdk = hdalName.c_str();
		if (hdalName.length() == 0)
		{
			hdalName = "Default Falcon";
			hdalNameToPassSdk = nullptr; // This is how the HDAL API initializes default Falcon.
		}
		deviceHandle = hdlInitNamedDevice(hdalNameToPassSdk);

		if (m_scaffold->checkForFatalError("Failed to initialize"))
		{
			// HDAL error message already logged
			SURGSIM_LOG_INFO(m_scaffold->getLogger()) << std::endl <<
				"  Device name: '" << deviceName << "'" << std::endl <<
				"  HDAL device name: '" << hdalName << "'" << std::endl;
			return false;
		}
		else if (deviceHandle == HDL_INVALID_HANDLE)
		{
			SURGSIM_LOG_SEVERE(m_scaffold->getLogger()) << "Novint: Failed to initialize '" << deviceName << "'" <<
				std::endl <<
				"  Error details: unknown (HDAL returned an invalid handle)" << std::endl <<
				"  HDAL device name: '" << hdalName << "'" << std::endl;
			return false;
		}

		m_deviceHandle = deviceHandle;
		return true;
	}

	bool destroy()
	{
		SURGSIM_ASSERT(isValid());

		HDLDeviceHandle deviceHandle = m_deviceHandle;
		if (deviceHandle == HDL_INVALID_HANDLE)
		{
			return false;
		}
		m_deviceHandle = HDL_INVALID_HANDLE;

		hdlUninitDevice(deviceHandle);
		m_scaffold->checkForFatalError("Couldn't disable device");
		return true;
	}

	HDLDeviceHandle get() const
	{
		SURGSIM_ASSERT(isValid());
		return m_deviceHandle;
	}

private:
	// Prevent copy construction and copy assignment.  (VS2012 does not support "= delete" yet.)
	Handle(const Handle&) /*= delete*/;
	Handle& operator=(const Handle&) /*= delete*/;

	/// The HDAL device handle (or HDL_INVALID_HANDLE if not valid).
	HDLDeviceHandle m_deviceHandle;
	/// The scaffold.
	std::shared_ptr<NovintScaffold> m_scaffold;
};


class NovintScaffold::Callback
{
public:
	Callback() :
		m_callbackHandle(0),
		m_haveCallback(false),
		m_scaffold(NovintScaffold::getOrCreateSharedInstance())
	{
		create();
	}

	~Callback()
	{
		if (m_haveCallback)
		{
			destroy();
		}
	}

	bool isValid() const
	{
		return m_haveCallback;
	}

	bool create()
	{
		SURGSIM_ASSERT(! m_haveCallback);

		const bool isCallbackNonblocking = false;
		m_callbackHandle = hdlCreateServoOp(run, m_scaffold.get(), isCallbackNonblocking);
		if (m_scaffold->checkForFatalError("Couldn't run haptic callback"))
		{
			return false;
		}
		m_haveCallback = true;
		return true;
	}

	bool destroy()
	{
		SURGSIM_ASSERT(m_haveCallback);
		hdlDestroyServoOp(m_callbackHandle);
		if (m_scaffold->checkForFatalError("Couldn't stop haptic callback"))
		{
			return false;
		}
		m_haveCallback = false;
		return true;
	}

private:
	// Prevent copy construction and copy assignment.  (VS2012 does not support "= delete" yet.)
	Callback(const Callback&) /*= delete*/;
	Callback& operator=(const Callback&) /*= delete*/;

	/// The callback wrapper passed to HDAL.
	/// \param [in,out] data	The user data (in our case, the scaffold pointer).
	/// \return	HD_CALLBACK_CONTINUE to wait for the next frame, or HD_CALLBACK_DONE to terminate further calls.
	static HDLServoOpExitCode run(void* data);

	/// The haptic loop callback handle.
	HDLOpHandle m_callbackHandle;
	/// True if the callback has been created (and not destroyed).
	bool m_haveCallback;
	/// The scaffold.
	std::shared_ptr<NovintScaffold> m_scaffold;
};


struct NovintScaffold::DeviceData
{
	/// Initialize the state.
	DeviceData(const std::string& apiName, NovintCommonDevice* device) :
		initializationName(apiName),
		deviceObject(device),
		isPositionHomed(false),
		isOrientationHomed(false),
		isDeviceHomed(false),
		isDeviceHeld(false),
		isDevice7Dof(device->is7DofDevice()),
		isDeviceRollAxisReversed(false),
		eulerAngleOffsetRoll(0.0),
		eulerAngleOffsetYaw(0.0),
		eulerAngleOffsetPitch(0.0),
		forwardPointingPoseThreshold(0.9),
		torqueScale(Vector3d::Constant(1.0)),
		positionValue(positionBuffer),
		transformValue(transformBuffer),
		forceValue(forceBuffer),
		positionScale(device->getPositionScale()),
		orientationScale(device->getOrientationScale())
	{
		positionValue.setZero();   // also clears positionBuffer
		transformValue.setIdentity();   // also sets transformBuffer
		jointAngles.setZero();
		buttonStates.fill(false);

		forceValue.setZero();   // also clears forceBuffer
		torqueValue.setZero();
	}


	/// The maximum number of buttons supported by any device object.
	static const size_t MAX_NUM_BUTTONS = 4;

	/// Type used to store button states.
	typedef std::array<bool, MAX_NUM_BUTTONS> ButtonStates;


	/// The HDAL device name.
	const std::string initializationName;
	/// The corresponding device object.
	NovintCommonDevice* const deviceObject;

	/// The device handle wrapper.
	NovintScaffold::Handle deviceHandle;
	/// Time of the initialization of the handle.
	Clock::time_point initializationTime;

	/// The raw position read from the device.
	double positionBuffer[3];
	/// The raw pose transform read from the device.
	double transformBuffer[16];
	/// The joint angles for the device orientation.
	Vector3d jointAngles;
	/// The button state read from the device.
	ButtonStates buttonStates;
	/// The homing state read from the device.
	bool isPositionHomed;
	/// The homing state read from the device.
	bool isOrientationHomed;
	/// The homing state read from the device.
	bool isDeviceHomed;
	/// The proximity state read from the device.
	bool isDeviceHeld;
	/// True if this is a 7DoF device.
	bool isDevice7Dof;
	/// True if the roll axis of a 7DoF device has reverse polarity because the device is left-handed.
	bool isDeviceRollAxisReversed;

	/// The offset added to the roll Euler angle.
	double eulerAngleOffsetRoll;
	/// The offset added to the yaw Euler angle.
	double eulerAngleOffsetYaw;
	/// The offset added to the pitch Euler angle.
	double eulerAngleOffsetPitch;
	/// The threshold to determine if the device is pointing forwards before unlocking orientation.
	double forwardPointingPoseThreshold;
	/// The scaling factors for the torque axes.
	Vector3d torqueScale;

	/// The raw force to be written to the device.
	double forceBuffer[3];
	/// The torque value to be written to the device after conversion.
	Vector3d torqueValue;

	/// The position value from the device, permanently connected to positionBuffer.
	Eigen::Map<Vector3d> positionValue;
	/// The pose transform value from the device, permanently connected to transformBuffer.
	Eigen::Map<Eigen::Matrix<double, 4, 4, Eigen::ColMajor>> transformValue;

	/// The force value to be written to the device, permanently connected to forceBuffer.
	Eigen::Map<Vector3d> forceValue;

	/// Scale factor for the position axes.
	double positionScale;
	/// Scale factor for the orientation axes.
	double orientationScale;
	/// The mutex that protects the externally modifiable parameters.
	boost::mutex parametersMutex;

private:
	// Prevent copy construction and copy assignment.  (VS2012 does not support "= delete" yet.)
	DeviceData(const DeviceData&) /*= delete*/;
	DeviceData& operator=(const DeviceData&) /*= delete*/;
};


struct NovintScaffold::StateData
{
public:
	/// Initialize the state.
	StateData() : isApiInitialized(false)
	{
	}

	/// True if the API has been initialized (and not finalized).
	bool isApiInitialized;

	/// Wrapper for the haptic loop callback handle.
	std::unique_ptr<NovintScaffold::Callback> callback;

	/// The list of known devices.
	std::list<std::unique_ptr<NovintScaffold::DeviceData>> activeDeviceList;

	/// The mutex that protects the list of known devices.
	boost::mutex mutex;

private:
	// Prevent copy construction and copy assignment.  (VS2012 does not support "= delete" yet.)
	StateData(const StateData&) /*= delete*/;
	StateData& operator=(const StateData&) /*= delete*/;
};


HDLServoOpExitCode NovintScaffold::Callback::run(void* data)
{
	NovintScaffold* scaffold = static_cast<NovintScaffold*>(data);
	if (! scaffold->runHapticFrame())
	{
		//...do something?...
	}

	// Should return HDL_SERVOOP_CONTINUE to wait for the next frame, or HDL_SERVOOP_EXIT to terminate the calls.
	return HDL_SERVOOP_CONTINUE;
}



template <typename T>
static inline T clampToRange(T value, T rangeMin, T rangeMax)
{
	if (value < rangeMin)
		return rangeMin;
	if (value > rangeMax)
		return rangeMax;
	return value;
}



NovintScaffold::NovintScaffold(std::shared_ptr<SurgSim::Framework::Logger> logger) :
	m_logger(logger), m_state(new StateData)
{
	if (! m_logger)
	{
		m_logger = SurgSim::Framework::Logger::getLogger("Novint device");
		m_logger->setThreshold(m_defaultLogLevel);
	}

	{
		// Drain the HDAL error stack
		HDLError errorCode = hdlGetError();
		while (errorCode != HDL_NO_ERROR)
		{
			errorCode = hdlGetError();
		}
	}

	SURGSIM_LOG_DEBUG(m_logger) << "Novint: Shared scaffold created.";
}


NovintScaffold::~NovintScaffold()
{
	if (m_state->callback)
	{
		destroyHapticLoop();
	}
	// The following block controls the duration of the mutex being locked.
	{
		boost::lock_guard<boost::mutex> lock(m_state->mutex);

		if (! m_state->activeDeviceList.empty())
		{
			SURGSIM_LOG_SEVERE(m_logger) << "Novint: Destroying scaffold while devices are active!?!";
			// do anything special with each device?
			m_state->activeDeviceList.clear();
		}

		if (m_state->isApiInitialized)
		{
			finalizeSdk();
		}
	}
	SURGSIM_LOG_DEBUG(m_logger) << "Novint: Shared scaffold destroyed.";
}


bool NovintScaffold::registerDevice(NovintCommonDevice* device)
{
	boost::lock_guard<boost::mutex> lock(m_state->mutex);

	if (! m_state->isApiInitialized)
	{
		if (! initializeSdk())
		{
			return false;
		}
	}

	// Make sure the object is unique.
	auto sameObject = std::find_if(m_state->activeDeviceList.cbegin(), m_state->activeDeviceList.cend(),
		[device](const std::unique_ptr<DeviceData>& info) { return info->deviceObject == device; });
	SURGSIM_ASSERT(sameObject == m_state->activeDeviceList.end()) << "Novint: Tried to register a device" <<
		" which is already present!";

	// Make sure the name is unique.
	const std::string deviceName = device->getName();
	auto sameName = std::find_if(m_state->activeDeviceList.cbegin(), m_state->activeDeviceList.cend(),
		[&deviceName](const std::unique_ptr<DeviceData>& info) { return info->deviceObject->getName() == deviceName; });
	if (sameName != m_state->activeDeviceList.end())
	{
		SURGSIM_LOG_CRITICAL(m_logger) << "Novint: Tried to register a device when the same name is" <<
			" already present!";
		return false;
	}

	// Make sure the initialization name is unique.
	const std::string initializationName = device->getInitializationName();
	auto sameInitializationName = std::find_if(m_state->activeDeviceList.cbegin(), m_state->activeDeviceList.cend(),
		[&initializationName](const std::unique_ptr<DeviceData>& info)
			{ return info->deviceObject->getInitializationName() == initializationName; });
	if (sameInitializationName != m_state->activeDeviceList.end())
	{
		SURGSIM_LOG_CRITICAL(m_logger) << "Novint: Tried to register a device when the same initialization" <<
			" (HDAL) name is already present!";
		return false;
	}

	// Construct the object, start its thread, then move it to the list.
	// Note that since Visual Studio 2010 doesn't support multi-argument emplace_back() for STL containers, storing a
	// list of unique_ptr results in nicer code than storing a list of DeviceData values directly.
	std::unique_ptr<DeviceData> info(new DeviceData(initializationName, device));
	if (! initializeDeviceState(info.get()))
	{
		return false;   // message already printed
	}
	info->initializationTime = Clock::now();
	m_state->activeDeviceList.emplace_back(std::move(info));

	if (m_state->activeDeviceList.size() == 1)
	{
		// If this is the first device, create the haptic loop as well.
		createHapticLoop();
	}
	return true;
}


bool NovintScaffold::unregisterDevice(const NovintCommonDevice* const device)
{
	std::unique_ptr<DeviceData> savedInfo;
	bool haveOtherDevices = false;
	{
		boost::lock_guard<boost::mutex> lock(m_state->mutex);
		auto matching = std::find_if(m_state->activeDeviceList.begin(), m_state->activeDeviceList.end(),
			[device](const std::unique_ptr<DeviceData>& info) { return info->deviceObject == device; });
		if (matching != m_state->activeDeviceList.end())
		{
			savedInfo = std::move(*matching);
			m_state->activeDeviceList.erase(matching);
			// the iterator is now invalid but that's OK
		}
		haveOtherDevices = (m_state->activeDeviceList.size() > 0);
	}

	bool status = true;
	if (! savedInfo)
	{
		SURGSIM_LOG_WARNING(m_logger) << "Novint: Attempted to release a non-registered device.";
		status = false;
	}
	else
	{
		// The HDAL seems to do bad things (and the CRT complains) if we uninitialize the device too soon.
		const int MINIMUM_LIFETIME_MILLISECONDS = 500;
		Clock::time_point earliestEndTime =
			savedInfo->initializationTime + boost::chrono::milliseconds(MINIMUM_LIFETIME_MILLISECONDS);
		boost::this_thread::sleep_until(earliestEndTime);

		// The destroy-pop-create structure of this code mirrors the structure of the OpenHaptics code, and
		// probably isn't necessary when using the HDAL.
		destroyHapticLoop();

		finalizeDeviceState(savedInfo.get());
		savedInfo.reset(nullptr);

		if (haveOtherDevices)
		{
			createHapticLoop();
		}
	}
	return status;
}


bool NovintScaffold::initializeDeviceState(DeviceData* info)
{
	SURGSIM_ASSERT(! info->deviceHandle.isValid());

	if (! info->deviceHandle.create(info->deviceObject->getName(), info->deviceObject->getInitializationName()))
	{
		return false;  // message was already printed
	}

	// Select the handle.
	hdlMakeCurrent(info->deviceHandle.get());
	checkForFatalError("Couldn't enable the handle");

	if (info->isDevice7Dof)
	{
		int gripStatus[2] = { 0, 0 };
		// OSG2 grips report their "handedness" in the LSB of the second raw status byte
		hdlGripGetAttributes (HDL_GRIP_STATUS, 2, gripStatus);
		if (checkForFatalError("Cannot get grip status"))
		{
			// HDL reported an error.  An error message was already logged.
			return false;
		}
		bool leftHanded = ((gripStatus[1] & 0x01) != 0);
		if (leftHanded)
		{
			SURGSIM_LOG_DEBUG(m_logger) << "'" << info->initializationName << "' is LEFT-handed.";
			info->isDeviceRollAxisReversed = true;   // sigh
			// I wish we had someplace to put these instead of hardcoding.
			info->eulerAngleOffsetRoll = 0;
			info->eulerAngleOffsetYaw = -75. * M_PI / 180.;
			info->eulerAngleOffsetPitch = -50. * M_PI / 180.;
		}
		else
		{
			SURGSIM_LOG_DEBUG(m_logger) << "'" << info->initializationName << "' is right-handed.";
			info->isDeviceRollAxisReversed = false;
			// I wish we had someplace to put these instead of hardcoding.
			info->eulerAngleOffsetRoll = 0;
			info->eulerAngleOffsetYaw = +75. * M_PI / 180.;
			info->eulerAngleOffsetPitch = +50. * M_PI / 180.;
		}
	}

	return true;
}


bool NovintScaffold::finalizeDeviceState(DeviceData* info)
{
	bool status = false;
	if (info->deviceHandle.isValid())
	{
		status = info->deviceHandle.destroy();
	}
	return status;
}


bool NovintScaffold::updateDevice(DeviceData* info)
{
	const SurgSim::DataStructures::DataGroup& outputData = info->deviceObject->getOutputData();

	boost::lock_guard<boost::mutex> lock(info->parametersMutex);

	// TODO(bert): this code should cache the access indices.

	hdlMakeCurrent(info->deviceHandle.get());	// This device is now "current", and all hdlXxx calls apply to it.
	bool fatalError = checkForFatalError(false, "hdlMakeCurrent()");

	// Receive the current device position (in millimeters!), pose transform, and button state bitmap.

	hdlGripGetAttributev(HDL_GRIP_POSITION, 0, info->positionBuffer);
	fatalError = checkForFatalError(fatalError, "hdlGripGetAttributev(HDL_GRIP_POSITION)");
	hdlGripGetAttributesd(HDL_GRIP_ORIENTATION, 16, info->transformBuffer);
	fatalError = checkForFatalError(fatalError, "hdlGripGetAttributesd(HDL_GRIP_ORIENTATION)");

	info->buttonStates.fill(false);
	hdlGripGetAttributesb(HDL_GRIP_BUTTON, info->buttonStates.size(), info->buttonStates.data());
	fatalError = checkForFatalError(fatalError, "hdlGripGetAttributesb(HDL_GRIP_BUTTON)");

	// Get the additional 7DoF data if available.

	if (info->isDevice7Dof)
	{
		// We compute the device orientation from the joint angles, for two reasons.  The first that it lets us
		// compensate for recurrent bugs in the HDAL grip code.  The second is that we'll need the joint angles in
		// order to correctly generate joint torques.
		double angles[4];
		hdlGripGetAttributesd(HDL_GRIP_ANGLE, 4, angles);
		fatalError = checkForFatalError(fatalError, "hdlGripGetAttributesd(HDL_GRIP_ANGLE)");

		// The zero values are NOT the home orientation.
		info->jointAngles[0] = angles[0] + info->eulerAngleOffsetRoll;
		info->jointAngles[1] = angles[1] + info->eulerAngleOffsetYaw;
		info->jointAngles[2] = angles[2] + info->eulerAngleOffsetPitch;

		// For the Falcon 7DoF grip, the axes are perpendicular and the joint angles are Euler angles:
		Matrix33d rotationX = makeRotationMatrix(info->jointAngles[0] * info->orientationScale,
			Vector3d(Vector3d::UnitX()));
		Matrix33d rotationY = makeRotationMatrix(info->jointAngles[1] * info->orientationScale,
			Vector3d(Vector3d::UnitY()));
		Matrix33d rotationZ = makeRotationMatrix(info->jointAngles[2] * info->orientationScale,
			Vector3d(Vector3d::UnitZ()));
		Matrix33d orientation = rotationY * rotationZ * rotationX;
		// Put the result into the orientation transform matrix:
		info->transformValue.block<3, 3>(0, 0) = orientation;
	}

	checkDeviceHoming(info);

	RigidTransform3d pose;
	pose.linear() = info->transformValue.block<3,3>(0,0);
	pose.translation() = info->positionValue * info->positionScale;

	SurgSim::DataStructures::DataGroup& inputData = info->deviceObject->getInputData();
	inputData.poses().set("pose", pose);
	inputData.booleans().set("button1", info->buttonStates[0]);
	inputData.booleans().set("button2", info->buttonStates[1]);
	inputData.booleans().set("button3", info->buttonStates[2]);
	inputData.booleans().set("button4", info->buttonStates[3]);
	inputData.booleans().set("isHomed", info->isDeviceHomed);
	inputData.booleans().set("isPositionHomed", info->isPositionHomed);
	inputData.booleans().set("isOrientationHomed", info->isOrientationHomed);

	bool desiredGravityCompensation = false;
	bool shouldSetGravityCompensation = outputData.booleans().get("gravityCompensation", &desiredGravityCompensation);
	if (shouldSetGravityCompensation)
	{
		setGravityCompensation(info, desiredGravityCompensation);
	}

	info->forceValue.setZero();
	info->torqueValue.setZero();
	fatalError = fatalError || !updateForcesAndTorques(info); // short-circuit only updates forces if no fatal error
	return !fatalError;
}

void NovintScaffold::checkDeviceHoming(DeviceData* info)
{
	unsigned int deviceStateBitmask = hdlGetState();
	info->isPositionHomed = ((deviceStateBitmask & HDAL_NOT_CALIBRATED) == 0);

	if (info->isDevice7Dof)
	{
		// The homing state is communicated using the button information.
		info->isOrientationHomed = info->buttonStates[0] && info->buttonStates[1];
		// So is the state of whether the device is currently held (proximity sensor).
		info->isDeviceHeld = info->buttonStates[2];
		// There are no ACTUAL buttons on the 7DoF Falcons, so we clear the button buffer.
		info->buttonStates.fill(false);
	}
	else
	{
		// The 3-DoF device doesn't need the orientation homing shenanigans...
		info->isOrientationHomed = true;
		info->isDeviceHomed = info->isPositionHomed;
		info->isDeviceHeld = true;  // ...I guess
	}

	if (info->isPositionHomed && info->isOrientationHomed && ! info->isDeviceHomed)
	{
		// Wait until the tool is pointed forwards (i.e. perpendicular to the Falcon centerline) before proclaiming the
		// whole device homed.
		Vector3d forwardDirection = Vector3d::UnitX();
		double forwardMetric = forwardDirection.dot(info->transformValue.block<3, 3>(0, 0) * forwardDirection);

		if (forwardMetric >= info->forwardPointingPoseThreshold)
		{
			// It looks like everything is ready!
			info->isDeviceHomed = true;
		}
	}

	if (! info->isPositionHomed)
	{
		info->positionValue.setZero();
	}
	if (! info->isOrientationHomed)
	{
		info->transformValue.setIdentity();
	}
}

bool NovintScaffold::updateForcesAndTorques(DeviceData* info)
{
	const SurgSim::DataStructures::DataGroup& outputData = info->deviceObject->getOutputData();
	SurgSim::Math::Vector3d force, torque;
	if (outputData.vectors().get("force", &force))
	{
		info->forceValue = force;
	}
	if (outputData.vectors().get("torque", &torque))
	{
		info->torqueValue = torque;
	}

	// Set the force command (in newtons).
	hdlGripSetAttributev(HDL_GRIP_FORCE, 0, info->forceBuffer);  // 2nd arg is index; output force is always "vector #0"
	bool fatalError = false;
	fatalError = checkForFatalError(fatalError, "hdlGripSetAttributev(HDL_GRIP_FORCE)");
	//hdlGripSetAttributesd(HDL_GRIP_TORQUE, 3, info->torqueBuffer);  // 2nd arg is size

	// Set the torque command if applicable (and convert newton-meters to command counts).
	if (info->isDevice7Dof)
	{
		// We have the torque vector in newton-meters.  Sadly, what we need is the torque command counts FOR EACH MOTOR
		// AXIS, not for each Cartesian axis. Which means we need to go back to calculations with joint angles.
		// For the Falcon 7DoF grip, the axes are perpendicular and the joint angles are Euler angles:
		Matrix33d rotationX = makeRotationMatrix(info->jointAngles[0], Vector3d(Vector3d::UnitX()));
		Matrix33d rotationY = makeRotationMatrix(info->jointAngles[1], Vector3d(Vector3d::UnitY()));
		Matrix33d rotationZ = makeRotationMatrix(info->jointAngles[2], Vector3d(Vector3d::UnitZ()));
		// NB: the order of rotations is (rotY * rotZ * rotX), not XYZ!
		// Construct the joint axes for the CURRENT pose of the device.
		Vector3d jointAxisY = Vector3d::UnitY();
		Vector3d jointAxisZ = rotationY * Vector3d::UnitZ();
		Vector3d jointAxisX = rotationY * (rotationZ * Vector3d::UnitX());
		// To convert from Cartesian space to motor-axis space, we assemble the axes into a basis matrix and invert it.
		Matrix33d basisMatrix;
		basisMatrix.col(0) = jointAxisX;
		basisMatrix.col(1) = jointAxisY;
		basisMatrix.col(2) = jointAxisZ;
		double basisDeterminant = fabs(basisMatrix.determinant());

		// Also construct a "fake" X axis orthogonal with the other two, to be used when the pose is degenerate.
		// Note that the Y and Z axes are always perpendicular for the Falcon 7DoF, so the normalize() can't fail and
		// is basically unnecessary, but...
		Vector3d fakeAxisX  = jointAxisY.cross(jointAxisZ).normalized();
		Matrix33d fakeBasisMatrix;
		basisMatrix.col(0) = fakeAxisX;
		basisMatrix.col(1) = jointAxisY;
		basisMatrix.col(2) = jointAxisZ;

		const double mediumBasisDeterminantThreshold = 0.6;
		const double smallBasisDeterminantThreshold = 0.4;

		Matrix33d decompositionMatrix;
		if (basisDeterminant >= mediumBasisDeterminantThreshold)
		{
			// All is well!
			decompositionMatrix = basisMatrix.inverse();
		}
		else if (basisDeterminant >= smallBasisDeterminantThreshold)
		{
			// If the determinant is "medium" but not "small", the device is in a near-degenerate configuration.
			// Which axes are going to be commanded may be hugely dependent on small changes in the pose.
			// We want to gradually decrease the amount of roll torque produced near the degenerate point.
			double ratio =  ((basisDeterminant - smallBasisDeterminantThreshold) /
				(mediumBasisDeterminantThreshold - smallBasisDeterminantThreshold));
			// The computed ratio has to be 0 <= ratio < 1.  We just use linear drop-off.

			// The "fake" basis matrix replaces the X axis with a fake (so it's always invertible), but the output X
			// torque is then meaningless.
			Matrix33d fakeDecompositionMatrix = fakeBasisMatrix.inverse();
			fakeDecompositionMatrix.row(0) = Vector3d::Zero();

			decompositionMatrix = basisMatrix.inverse() * ratio + fakeDecompositionMatrix * (1.-ratio);
		}
		else
		{
			// If the determinant is small, the matrix may not be invertible.
			// The "fake" basis matrix replaces the X axis with a fake (so it's always invertible), but the output X
			// torque is then meaningless.
			decompositionMatrix = fakeBasisMatrix.inverse();
			decompositionMatrix.row(0) = Vector3d::Zero();
			// Moreover, near the degenerate position the X axis free-spins but is aligned with Y,
			// so we want to reduce Y torques as well.
			//double ratio = (basisDeterminant / smallBasisDeterminantThreshold);
			double ratio = 0;
			// The computed ratio has to be 0 <= ratio < 1.  We just use linear drop-off.
			// We just use linear drop-off.
			decompositionMatrix.row(1) *= ratio;
		}
		Vector3d axisTorqueVector = decompositionMatrix * info->torqueValue;

		// Unit conversion factors for the Falcon 7DoF.  THIS SHOULD BE PARAMETRIZED!
		const double axisTorqueMin = -2000;
		const double axisTorqueMax = +2000;
		// roll axis:  torque = 17.6 mNm  when command = 2000 (but flipped in left grip!)
		const double rollTorqueScale  = axisTorqueMax / 17.6e-3;
		// yaw axis:   torque = 47.96 mNm when command = 2000
		const double yawTorqueScale   = axisTorqueMax / 47.96e-3;
		// pitch axis: torque = 47.96 mNm when command = 2000
		const double pitchTorqueScale = axisTorqueMax / 47.96e-3;

		double deviceTorques[4];
		deviceTorques[0] = clampToRange(rollTorqueScale  * info->torqueScale.x() * axisTorqueVector.x(),
			axisTorqueMin, axisTorqueMax);
		deviceTorques[1] = clampToRange(yawTorqueScale   * info->torqueScale.y() * axisTorqueVector.y(),
			axisTorqueMin, axisTorqueMax);
		deviceTorques[2] = clampToRange(pitchTorqueScale * info->torqueScale.z() * axisTorqueVector.z(),
			axisTorqueMin, axisTorqueMax);
		deviceTorques[3] = 0;

		if (info->isDeviceRollAxisReversed)  // commence swearing.
		{
			deviceTorques[0] = -deviceTorques[0];
		}

		// Set the torque vector (from the first 3 elements of the argument array).  Also set the jaw squeeze torque
		// (as 4th element of the array)-- though this is not used anywhere at the moment.
		// The 2nd arg to this call is the count; we're setting 4 doubles.
		hdlGripSetAttributesd(HDL_GRIP_TORQUE, 4, deviceTorques);
		fatalError = checkForFatalError(fatalError, "hdlGripSetAttributesd(HDL_GRIP_TORQUE)");
	}
	return !fatalError;
}


bool NovintScaffold::initializeSdk()
{
	SURGSIM_ASSERT(! m_state->isApiInitialized);

	// nothing to do!

	m_state->isApiInitialized = true;
	return true;
}


bool NovintScaffold::finalizeSdk()
{
	SURGSIM_ASSERT(m_state->isApiInitialized);

	// nothing to do!

	m_state->isApiInitialized = false;
	return true;
}


bool NovintScaffold::runHapticFrame()
{
	boost::lock_guard<boost::mutex> lock(m_state->mutex);

	for (auto it = m_state->activeDeviceList.begin();  it != m_state->activeDeviceList.end();  ++it)
	{
		(*it)->deviceObject->pullOutput();
		if (updateDevice((*it).get()))
		{
			(*it)->deviceObject->pushInput();
		}
	}

	return true;
}


bool NovintScaffold::createHapticLoop()
{
	SURGSIM_ASSERT(! m_state->callback);

	if (! startScheduler())
	{
		return false;
	}

	std::unique_ptr<Callback> callback(new Callback);
	if (! callback->isValid())
	{
		stopScheduler();
		return false;
	}

	m_state->callback = std::move(callback);
	return true;
}


bool NovintScaffold::destroyHapticLoop()
{
	SURGSIM_ASSERT(m_state->callback);

	checkForFatalError("Error prior to stopping haptic callback");  // NOT considered an error for return code!

	bool didDestroy = m_state->callback->destroy();
	m_state->callback.reset(nullptr);

	bool didStop = stopScheduler();

	return didDestroy && didStop;
}


bool NovintScaffold::startScheduler()
{
	hdlStart();
	if (checkForFatalError("Couldn't start the scheduler"))
	{
		return false;
	}
	return true;
}


bool NovintScaffold::stopScheduler()
{
	hdlStop();
	if (checkForFatalError("Couldn't stop the scheduler"))
	{
		return false;
	}
	return true;
}


bool NovintScaffold::getGravityCompensation(const NovintScaffold::DeviceData* info, bool* gravityCompensationState)
{
	bool state1 = true;
	hdlGripGetAttributeb(HDL_GRIP_GRAVITY_COMP, 1, &state1);
	if (checkForFatalError("Cannot get gravity compensation (#1)"))
	{
		return false;  // HDAL reported an error; an error message was already logged.
	}

	bool state2 = false;
	hdlGripGetAttributeb(HDL_GRIP_GRAVITY_COMP, 1, &state2);
	if (checkForFatalError("Cannot get gravity compensation (#2)"))
	{
		return false;  // HDAL reported an error; an error message was already logged.
	}

	if (state1 == true && state2 == false)
	{
		SURGSIM_LOG_WARNING(m_logger) << "getting gravity compensation state for '" << info->deviceObject->getName() <<
			"' does nothing!";
		return false;
	}
	else if (state1 != state2)
	{
		SURGSIM_LOG_WARNING(m_logger) << "getting gravity compensation state for '" << info->deviceObject->getName() <<
			"' keeps changing?!?";
		return false;
	}

	*gravityCompensationState = state1;
	return true;
}


bool NovintScaffold::enforceGravityCompensation(const NovintScaffold::DeviceData* info, bool gravityCompensationState)
{
	bool initialState;
	bool isInitialStateValid = getGravityCompensation(info, &initialState);

	const int maxAttempts = 20;
	for (int i = 0;  i < maxAttempts;  ++i)
	{
		bool state = gravityCompensationState;
		hdlGripSetAttributeb(HDL_GRIP_GRAVITY_COMP, 1, &state);
		if (checkForFatalError("Cannot set gravity compensation state"))
		{
			return false;  // HDAL reported an error; an error message was already logged.
		}

		if (! getGravityCompensation(info, &state))
		{
			return false;  // HDAL reported an error; an error message was already logged.
		}
		else if (state == gravityCompensationState)
		{
			// If the state has been changed, log a message.
			if (isInitialStateValid && (initialState != gravityCompensationState))
			{
				if (gravityCompensationState)
				{
					SURGSIM_LOG_INFO(m_logger) << "gravity compensation for '" << info->deviceObject->getName() <<
						"' changed to ENABLED.";
				}
				else
				{
					SURGSIM_LOG_INFO(m_logger) << "gravity compensation for '" << info->deviceObject->getName() <<
						"' changed to disabled.";
				}
			}
			return true;
		}
	}

	SURGSIM_LOG_WARNING(m_logger) << "failed to set gravity compensation for '" << info->deviceObject->getName() <<
		"' to " << (gravityCompensationState ? "enabled" : "disabled") << " after " << maxAttempts << " attempts";
	return false;
}


bool NovintScaffold::setGravityCompensation(const NovintScaffold::DeviceData* info, bool gravityCompensationState)
{
	bool initialState;
	bool isInitialStateValid = getGravityCompensation(info, &initialState);

	if (isInitialStateValid && (initialState == gravityCompensationState))
	{
		return true;  // no need to do anything
	}

	return enforceGravityCompensation(info, gravityCompensationState);
}


static std::string convertErrorCodeToString(HDLError errorCode)
{
	// Convert a HDLError to text.  The text was cut+pasted from the comments in Novint's hdlErrors.h file.
	switch (errorCode)
	{
	case HDL_ERROR_STACK_OVERFLOW:
		return "Overflow of error stack";
	case HDL_ERROR_INTERNAL:
		return "HDAL internal error";
	case HDL_ERROR_INIT_FAILED:
		return "Device initialization error";
	case HDL_INIT_INI_NOT_FOUND:
		return "Could not find configuration file";
	case HDL_INIT_INI_DLL_STRING_NOT_FOUND:
		return "No DLL name in configuration file";
	case HDL_INIT_INI_MANUFACTURER_NAME_STRING_NOT_FOUND:
		return "No MANUFACTURER_NAME value in configuration file";
	case HDL_INIT_DLL_LOAD_ERROR:
		return "Could not load driver DLL";
	case HDL_INIT_DEVICE_FAILURE:
		return "Failed to initialize device";
	case HDL_INIT_DEVICE_ALREADY_INITED:
		return "Device already initialized";
	case HDL_INIT_DEVICE_NOT_CONNECTED:
		return "Requested device not connected";
	case HDL_SERVO_START_ERROR:
		return "Could not start servo thread";
	default:
		return "<unknown>";
	}
}


bool NovintScaffold::checkForFatalError(const char* message)
{
	HDLError errorCode = hdlGetError();
	if (errorCode == HDL_NO_ERROR)
	{
		return false;
	}

	// The HDAL maintains an error stack, so in theory there could be more than one error pending.
	// We do head recursion to get them all in the correct order, and hope we don't overrun the stack...
	bool anotherFatalError = checkForFatalError(message);

	bool isFatal = (errorCode != HDL_ERROR_STACK_OVERFLOW);

	SURGSIM_LOG_SEVERE(m_logger) << "Novint: " << message << std::endl <<
		"  Error text: '" << convertErrorCodeToString(errorCode) << "'" << std::endl <<
		"  Error code: 0x" << std::hex << std::setw(4) << std::setfill('0') << errorCode << std::endl;

	return (isFatal || anotherFatalError);
}

SurgSim::DataStructures::DataGroup NovintScaffold::buildDeviceInputData()
{
	DataGroupBuilder builder;
	builder.addPose("pose");
	builder.addBoolean("button1");
	builder.addBoolean("button2");
	builder.addBoolean("button3");
	builder.addBoolean("button4");
	builder.addBoolean("isHomed");
	builder.addBoolean("isPositionHomed");
	builder.addBoolean("isOrientationHomed");
	return builder.createData();
}

void NovintScaffold::setPositionScale(const NovintCommonDevice* device, double scale)
{
	boost::lock_guard<boost::mutex> lock(m_state->mutex);
	auto matching = std::find_if(m_state->activeDeviceList.begin(), m_state->activeDeviceList.end(),
		[device](const std::unique_ptr<DeviceData>& info) { return info->deviceObject == device; });
	if (matching != m_state->activeDeviceList.end())
	{
		boost::lock_guard<boost::mutex> lock((*matching)->parametersMutex);
		(*matching)->positionScale = scale;
	}
}

void NovintScaffold::setOrientationScale(const NovintCommonDevice* device, double scale)
{
	boost::lock_guard<boost::mutex> lock(m_state->mutex);
	auto matching = std::find_if(m_state->activeDeviceList.begin(), m_state->activeDeviceList.end(),
		[device](const std::unique_ptr<DeviceData>& info) { return info->deviceObject == device; });
	if (matching != m_state->activeDeviceList.end())
	{
		boost::lock_guard<boost::mutex> lock((*matching)->parametersMutex);
		(*matching)->orientationScale = scale;
	}
}


std::shared_ptr<NovintScaffold> NovintScaffold::getOrCreateSharedInstance()
{
	static SurgSim::Framework::SharedInstance<NovintScaffold> sharedInstance;
	return sharedInstance.get();
}

void NovintScaffold::setDefaultLogLevel(SurgSim::Framework::LogLevel logLevel)
{
	m_defaultLogLevel = logLevel;
}

SurgSim::Framework::LogLevel NovintScaffold::m_defaultLogLevel = SurgSim::Framework::LOG_LEVEL_INFO;


};  // namespace Device
};  // namespace SurgSim
