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

#ifndef SURGSIM_PHYSICS_RIGIDREPRESENTATION_H
#define SURGSIM_PHYSICS_RIGIDREPRESENTATION_H

#include <SurgSim/Physics/RigidRepresentationBase.h>
#include <SurgSim/Physics/RigidRepresentationState.h>

#include <SurgSim/Math/Vector.h>
#include <SurgSim/Math/Matrix.h>
#include <SurgSim/Math/RigidTransform.h>

namespace SurgSim
{

namespace Physics
{

/// The RigidRepresentation class defines the dynamic rigid body representation
/// Note that the rigid representation is velocity-based, therefore its degrees of
/// freedom are the linear and angular velocities: 6 Dof
class RigidRepresentation : public RigidRepresentationBase
{
public:
	/// Constructor
	/// \param name The rigid representation's name
	explicit RigidRepresentation(const std::string& name);

	/// Destructor
	virtual ~RigidRepresentation();

	/// Query the representation type
	/// \return the RepresentationType for this representation
	virtual RepresentationType getType() const override;

	/// Set the initial parameters of the rigid representation
	/// \param parameters The initial parameters
	/// This will also set the current parameters to the initial parameters
	void setInitialParameters(const RigidRepresentationParameters& parameters);
	/// Set the current parameters of the rigid representation
	/// \param parameters The current parameters
	void setCurrentParameters(const RigidRepresentationParameters& parameters);

	/// Set the current pose of the rigid representation
	/// \param pose The current pose (translation + rotation)
	/// \note Does Not Apply to this representation (the pose is fully controlled by the
	/// physics simulation).
	void setPose(const SurgSim::Math::RigidTransform3d& pose);

	/// Set the external force being applied to the rigid representation,
	/// not including gravity
	/// \param force The external force
	void setExternalForce(const SurgSim::Math::Vector3d& force);

	/// Set the external torque being applied to the rigid representation
	/// \param torque The external torque 
	void setExternalTorque(const SurgSim::Math::Vector3d& torque);

	/// Preprocessing done before the update call
	/// \param dt The time step (in seconds)
	virtual void beforeUpdate(double dt) override;
	/// Update the representation state to the current time step (compute free motion)
	/// \param dt The time step (in seconds)
	virtual	void update(double dt) override;
	/// Postprocessing done after the update call
	/// \param dt The time step (in seconds)
	virtual	void afterUpdate(double dt) override;

	/// Apply a correction to the internal degrees of freedom
	/// \param dt The time step
	/// \param block The block of a vector containing the correction to be applied to the dof
	void applyDofCorrection(double dt, const Eigen::VectorBlock<SurgSim::Math::MlcpSolution::Vector>& block) override;

	/// Reset the rigid representation parameters to the initial parameters
	void resetParameters();

	/// Retrieve the rigid body 6x6 compliance matrix
	/// \return the 6x6 compliance matrix
	const Eigen::Matrix<double, 6,6, Eigen::DontAlign | Eigen::RowMajor>& getComplianceMatrix() const;

protected:
	/// Inertia matrices in global coordinates
	SurgSim::Math::Matrix33d m_globalInertia;
	/// Inverse of inertia matrix in global coordinates
	SurgSim::Math::Matrix33d m_invGlobalInertia;

	/// Current force applied on the rigid representation (in N)
	SurgSim::Math::Vector3d m_force;
	/// Current torque applied on the rigid representation (in N.m)
	SurgSim::Math::Vector3d m_torque;

	SurgSim::Math::Vector3d m_externalForce;
	SurgSim::Math::Vector3d m_externalTorque;

	/// Compliance matrix (size of the number of Dof = 6)
	Eigen::Matrix<double, 6,6, Eigen::DontAlign | Eigen::RowMajor> m_C;

private:
	/// Compute compliance matrix (internal data structure)
	/// \param dt The time step in use
	void computeComplianceMatrix(double dt);

	/// Update global inertia matrices (internal data structure)
	/// \param state The state of the rigid representation to use for the update
	virtual void updateGlobalInertiaMatrices(const RigidRepresentationState& state) override;
};

}; // Physics

}; // SurgSim

#endif // SURGSIM_PHYSICS_RIGIDREPRESENTATION_H
