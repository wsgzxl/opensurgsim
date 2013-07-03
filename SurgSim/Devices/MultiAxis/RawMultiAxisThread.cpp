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

#include "SurgSim/Devices/MultiAxis/RawMultiAxisThread.h"

#include <SurgSim/Devices/MultiAxis/RawMultiAxisScaffold.h>

namespace SurgSim
{
namespace Device
{

RawMultiAxisThread::~RawMultiAxisThread()
{
}

bool RawMultiAxisThread::doUpdate(double dt)
{
	return m_scaffold->runInputFrame(m_deviceData);
}

bool RawMultiAxisThread::doInitialize()
{
	return true;
}

bool RawMultiAxisThread::doStartUp()
{
	return true;
}

};  // namespace Device
};  // namespace SurgSim