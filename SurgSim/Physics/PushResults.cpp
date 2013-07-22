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

#include <SurgSim/Physics/MlcpPhysicsProblem.h>
#include <SurgSim/Physics/MlcpPhysicsSolution.h>
#include <SurgSim/Physics/PushResults.h>
#include <SurgSim/Physics/Representation.h>

namespace SurgSim
{
namespace Physics
{

PushResults::PushResults()
{}

PushResults::~PushResults()
{}

std::shared_ptr<PhysicsManagerState>
	PushResults::doUpdate(const double& dt, const std::shared_ptr<PhysicsManagerState>& state)
{
	std::shared_ptr<PhysicsManagerState> result = state;
	// 1st step
	// Compute the global dof displacement correction from the constraints forces (result of the MLCP)
	// correction = CHt . lambda
	const Eigen::VectorXd& lambda = result->getMlcpSolution().x;
	if (lambda.size() == 0)
	{
		return state;
	}
	const SurgSim::Math::MlcpProblem::Matrix& CHt = result->getMlcpProblem().CHt;
	SurgSim::Math::MlcpSolution::Vector& dofCorrection = result->getMlcpSolution().dofCorrection;
	dofCorrection = CHt * lambda;

	// 2nd step
	// Push the dof displacement correction to all representation, using their assigned index
	std::vector<std::shared_ptr<Representation>> representations = result->getRepresentations();
	auto const itEnd = representations.end();
	for (auto it = representations.begin(); it != itEnd; ++it)
	{
		int index = result->getRepresentationsMapping().getValue((*it).get());
		SURGSIM_ASSERT(index >= 0) << "Bad index found for representation " << (*it)->getName() << std::endl;
		//(*it)->applyDofCorrection(dt, dofCorrection.block(index, 0, (*it)->getNumDof(), 1));
		(*it)->applyDofCorrection(dt, dofCorrection.block(index, 0, (*it)->getNumDof(), 1));
	}

	return result;
}

}; // namespace Physics
}; // namespace SurgSim
