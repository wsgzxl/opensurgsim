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

#include "SurgSim/Framework/Accessible.h"
#include "SurgSim/Math/Matrix.h"

namespace SurgSim
{
namespace Framework
{

boost::any Framework::Accessible::getValue(const std::string& name)
{
	auto functors = m_functors.find(name);
	if (functors != std::end(m_functors) && functors->second.getter != nullptr)
	{
		return functors->second.getter();
	}
	else
	{
		SURGSIM_FAILURE() << "Can't get property: " << name << "." <<
			((functors == std::end(m_functors)) ? "Property not found." : "No getter defined for property.");
		return boost::any();
	}
}

void Framework::Accessible::setValue(const std::string& name, const boost::any& value)
{
	auto functors = m_functors.find(name);
	if (functors != std::end(m_functors) && functors->second.setter != nullptr)
	{
		functors->second.setter(value);
	}
	else
	{
		SURGSIM_FAILURE() << "Can't set property: " << name << "." <<
			((functors == std::end(m_functors)) ? "Property not found." : "No setter defined for property.");
	}
}


void Accessible::setGetter(const std::string& name, GetterType func)
{
	SURGSIM_ASSERT(func != nullptr) << "Getter functor can't be nullptr";

	m_functors[name].getter = func;
}

void Accessible::setSetter(const std::string& name, SetterType func)
{
	SURGSIM_ASSERT(func != nullptr) << "Getter functor can't be nullptr";

	m_functors[name].setter = func;
}

void Accessible::setAccessors(const std::string& name, GetterType getter, SetterType setter)
{
	setGetter(name, getter);
	setSetter(name, setter);
}

bool Accessible::isReadable(const std::string& name) const
{
	auto functors = m_functors.find(name);
	return (functors != m_functors.end() && functors->second.getter != nullptr);
}

bool Accessible::isWriteable(const std::string& name) const
{
	auto functors = m_functors.find(name);
	return (functors != m_functors.end() && functors->second.getter != nullptr);
}

void Accessible::setSerializable(const std::string& name, EncoderType encoder, DecoderType decoder)
{
	SURGSIM_ASSERT(encoder != nullptr) << "Encoder functor can't be nullptr.";
	SURGSIM_ASSERT(decoder != nullptr) << "Decoder functor can't be nullptr.";

	m_functors[name].encoder = encoder;
	m_functors[name].decoder = decoder;
}

YAML::Node Accessible::encode() const
{
	YAML::Node result;
	for (auto functors = m_functors.cbegin(); functors != m_functors.cend(); ++functors)
	{
		auto encoder = functors->second.encoder;
		if (encoder != nullptr)
		{
			result[functors->first] = encoder();
		}
	}
	return result;
}

void  Accessible::decode(const YAML::Node& node)
{
	bool result = false;
	SURGSIM_ASSERT(node.IsMap()) << "Node to decode accessible has to be map.";
	for (auto functors = m_functors.cbegin(); functors != m_functors.cend(); ++functors)
	{
		auto decoder = functors->second.decoder;
		if (decoder != nullptr)
		{
			YAML::Node temporary = node[functors->first];
			if (!temporary.IsNull() && temporary.IsDefined())
			{
				try
				{
					decoder(&temporary);
				}
				catch (std::exception e)
				{
					SURGSIM_FAILURE() << e.what();
				}
			}
		}
	}
}

template<>
SurgSim::Math::Matrix44f convert(boost::any val)
{

	SurgSim::Math::Matrix44f floatResult;
	// Use try in case this conversion was created using a Matrix44f, in which case the any_cast will
	// still fail and throw an exception
	try
	{
		SurgSim::Math::Matrix44d result = boost::any_cast<SurgSim::Math::Matrix44d>(val);
		floatResult = result.cast<float>();
	}
	catch (boost::bad_any_cast &)
	{
		floatResult = boost::any_cast<SurgSim::Math::Matrix44f>(val);
	}
	return floatResult;
}


}; // Framework
}; // SurgSim
