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

#ifndef SURGSIM_GRAPHICS_OSGPLANEACTOR_H
#define SURGSIM_GRAPHICS_OSGPLANEACTOR_H

#include <SurgSim/Graphics/PlaneActor.h>
#include <SurgSim/Graphics/OsgActor.h>

#include <SurgSim/Framework/SharedInstance.h>
#include <SurgSim/Math/RigidTransform.h>

#include <osg/PositionAttitudeTransform>
#include <osg/Switch>

namespace SurgSim
{

namespace Graphics
{

class OsgPlane;

/// Base OSG implementation of a graphics plane actor.
///
/// 
class OsgPlaneActor : public PlaneActor, public OsgActor
{
public:
	/// Constructor
	/// \param	name	Name of the actor
	OsgPlaneActor(const std::string& name);

	/// Sets whether the actor is currently visible
	/// \param	visible	True for visible, false for invisible
	virtual void setVisible(bool visible);

	/// Gets whether the actor is currently visible
	/// \return	visible	True for visible, false for invisible
	virtual bool isVisible() const;

	/// Sets the pose of the actor
	/// \param	transform	Rigid transformation that describes the pose of the actor
	virtual void setPose(const SurgSim::Math::RigidTransform3d& transform);

	/// Gets the pose of the actor
	/// \return	Rigid transformation that describes the pose of the actor
	virtual const SurgSim::Math::RigidTransform3d& getPose() const;

	/// Updates the actor.
	/// \param	dt	The time in seconds of the preceding timestep.
	virtual void update(double dt);

private:
	/// Pose of the sphere
	SurgSim::Math::RigidTransform3d m_pose;
	/// OSG switch to set the visibility of the sphere
	osg::ref_ptr<osg::Switch> m_switch;
	/// OSG transform to set the pose and scale of the sphere
	/// The OSG sphere shape is a unit sphere and this transform scales it to the radius set.
	osg::ref_ptr<osg::PositionAttitudeTransform> m_transform;

	/// Shared plane, so that the geometry can be instanced rather than having multiple copies.
	std::shared_ptr<OsgPlane> m_sharedPlane;

	/// Returns the shared plane
	static std::shared_ptr<OsgPlane> getSharedPlane();
};

};  // namespace Graphics

};  // namespace SurgSim

#endif  // SURGSIM_GRAPHICS_OSGSPHEREACTOR_H
