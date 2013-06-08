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

#ifndef SURGSIM_GRAPHICS_GROUP_H
#define SURGSIM_GRAPHICS_GROUP_H

#include <SurgSim/Framework/Component.h>

#include <memory>
#include <vector>

namespace SurgSim
{

namespace Graphics
{

class Actor;

/// Base graphics group class, which defines the interface that all graphics groups must implement.
///
/// Graphics::Group allows the organization of Graphics::Actor objects so that different algorithms can operate on
/// specific sub-sets rather than the entire scene.
class Group : public SurgSim::Framework::Component
{
public:
	/// Constructor. The group is initially empty.
	/// \param	name	Name of the group
	explicit Group(const std::string& name);

	/// Destructor
	virtual ~Group();

	/// Sets whether the group is currently visible
	/// \param	visible	True for visible, false for invisible
	virtual void setVisible(bool visible) = 0;

	/// Gets whether the group is currently visible
	/// \return	visible	True for visible, false for invisible
	virtual bool isVisible() const = 0;

	/// Adds an actor
	/// \param	actor	Actor to add to this group
	/// \return	True if the actor is added successfully, false if failure
	virtual bool add(std::shared_ptr<Actor> actor);

	/// Adds all actors in another group to this group
	/// \param	group	Group of actors to add
	/// \return	True if all actors are added successfully, false if failure
	virtual bool append(std::shared_ptr<Group> group);

	/// Removes an actor
	/// \param	actor	Actor to remove from this group
	/// \return	True if the actor is removed successfully, false if actor is not in this group or other failure
	virtual bool remove(std::shared_ptr<Actor> actor);

	/// Returns the actors in this group
	const std::vector<std::shared_ptr<Actor>>& getMembers() const
	{
		return m_actors;
	}

	/// Removes all actors
	virtual void clear();

private:
	/// Initialize the component
	/// \return	True if succeeded, false if failed
	virtual bool doInitialize();
	/// Wake up the component
	/// \return	True if succeeded, false if failed
	virtual bool doWakeUp();

	/// Actors in this group
	std::vector<std::shared_ptr<Actor>> m_actors;
};

};  // namespace Graphics

};  // namespace SurgSim

#endif  // SURGSIM_GRAPHICS_GROUP_H
