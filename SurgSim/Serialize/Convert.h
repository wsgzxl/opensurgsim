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

#ifndef SURGSIM_SERIALIZE_CONVERT_H
#define SURGSIM_SERIALIZE_CONVERT_H

#include <yaml-cpp/yaml.h>
#include <SurgSim/Framework/Log.h>

#define OSS_SERIALIZE(T)\
	YAML::Node encode();\
	bool decode(const YAML::Node& node);\
	std::string getClassName() {return std::string(#T);}\

namespace SurgSim
{
namespace Serialize
{
	/// Logger name for Serialization
	const std::string serializeLogger = "Serialization";

};
};

#endif // SURGSIM_SERIALIZE_CONVERT_H