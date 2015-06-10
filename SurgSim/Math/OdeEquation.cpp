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

#include "SurgSim/Math/OdeEquation.h"
#include "SurgSim/Math/OdeState.h"
#include "SurgSim/Framework/Log.h"

namespace SurgSim
{

namespace Math
{

const std::shared_ptr<OdeState> OdeEquation::getInitialState() const
{
	return m_initialState;
}

const Vector& OdeEquation::getF()
{
	return m_f;
}

const SparseMatrix& OdeEquation::getM()
{
	return m_M;
}

const SparseMatrix& OdeEquation::getD()
{
	return m_D;
}

const SparseMatrix& OdeEquation::getK()
{
	return m_K;
}

void OdeEquation::update(const OdeState& state, int options)
{
	if (options == OdeEquationUpdate::FMDK)
	{
		computeFMDK(state);
	}
	else
	{
		if (options & OdeEquationUpdate::F)
		{
			computeF(state);
		}

		if (options & OdeEquationUpdate::M)
		{
			computeM(state);
		}

		if (options & OdeEquationUpdate::D)
		{
			computeD(state);
		}

		if (options & OdeEquationUpdate::K)
		{
			computeK(state);
		}
	}
}

}; // namespace Math

}; // namespace SurgSim
