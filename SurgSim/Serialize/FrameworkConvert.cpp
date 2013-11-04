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

#include <SurgSim/Serialize/Convert.h>

namespace YAML
{
	/// Specialize of YAML::convert<> template Component class.
	Node convert<SurgSim::Framework::Component>::encode(const SurgSim::Framework::Component& rhs)
	{
		Node node;
		node["name"] = rhs.getName();
		return node;
	}

	bool convert<SurgSim::Framework::Component>::decode(const Node& node,
		 std::shared_ptr<SurgSim::Framework::Component> rhs)
	{
		if (! node.IsMap())
		{
			return false;
		}
		rhs->setName(node["name"].as<std::string>());
		return true;

	}
}