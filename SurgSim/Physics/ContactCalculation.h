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

#ifndef SURGSIM_PHYSICS_CONTACTCALCULATION_H
#define SURGSIM_PHYSICS_CONTACTCALCULATION_H

#include <memory>

#include <SurgSim/Framework/ReuseFactory.h>
#include <SurgSim/Physics/CollisionPair.h>

namespace SurgSim
{
namespace Physics
{

/// Base class responsible for calculating contact data between two given shapes, calculateContact needs to
/// determine whether the two shapes intersect, and if yes calculate the correct data for this contact, which
/// consists of, the normal to displace the first shape so that the two shapes just barely touch. And the
/// penetration point (the point that is furthest inside the other object) for each shape.
/// This base class also handles the swapping of the shapes if the pair is asymmetric. The sub classes
/// assume that the pair is always in correct order.
class ContactCalculation
{
public:

	/// Constructor
	explicit ContactCalculation()
	{
	}


	/// Destructor
	virtual ~ContactCalculation()
	{
	}

	/// Function that handles asymmetric pair and calls the actual contact calculation routine of the sub class.
	/// \param	pair	The pair that is under consideration.
	void calculateContact(std::shared_ptr<CollisionPair> pair)
	{
		if (needsSwap(pair->getFirst()->getShapeType(), pair->getSecond()->getShapeType()))
		{
			pair->swapRepresentations();
		}
		doCalculateContact(pair);
	}

	virtual std::pair<int,int> getShapeTypes() = 0;

private:

	/// Calculate the actual contact between two shapes of the given CollisionPair.
	/// \param	pair	The symmetric pair that is under consideration.
	virtual void doCalculateContact(std::shared_ptr<CollisionPair> pair) = 0;

	bool needsSwap(int firstShapeType, int secondShapeType)
	{
		std::pair<int,int> shapeTypes = getShapeTypes();
		return firstShapeType != secondShapeType && firstShapeType == shapeTypes.second &&
			secondShapeType == shapeTypes.first;
	}

};

/// A default calculation, it does nothing and can be used as a placeholder
class DefaultContactCalculation : public ContactCalculation
{
public:

	/// Constructor
	/// \param doAssert If set the calculation will throw an exception if it is executed, this
	/// 				can be used to detect cases where a  contact calculation is being called
	/// 				on a pair that should be implemented
	explicit DefaultContactCalculation(bool doAssert = false) : m_doAssert(doAssert)
	{
	}

	/// Destructor
	virtual ~DefaultContactCalculation() {}

	virtual std::pair<int,int> getShapeTypes() override
	{
		return std::pair<int,int>(RIGID_SHAPE_TYPE_NONE, RIGID_SHAPE_TYPE_NONE);
	}

private:
	bool m_doAssert;

	/// Calculate the actual contact between two shapes of the given CollisionPair.
	/// \param	pair	The symmetric pair that is under consideration.
	virtual void doCalculateContact(std::shared_ptr<CollisionPair> pair) override;
};

/// Class to calculate intersections between spheres
class SphereSphereDcdContact : public ContactCalculation
{
public:
	explicit SphereSphereDcdContact()
	{
	}

	virtual std::pair<int,int> getShapeTypes() override
	{
		return std::pair<int,int>(RIGID_SHAPE_TYPE_SPHERE, RIGID_SHAPE_TYPE_SPHERE);
	}

private:
	/// Calculate the actual contact between two shapes of the given CollisionPair.
	/// \param	pair	The symmetric pair that is under consideration.
	virtual void doCalculateContact(std::shared_ptr<CollisionPair> pair);
};


/// Class to calculate intersections between Spheres and DoubleSidedPlanes
class SphereDoubleSidedPlaneDcdContact : public ContactCalculation
{
public:

	/// Constructor.
	explicit SphereDoubleSidedPlaneDcdContact()
	{
	}

	virtual std::pair<int,int> getShapeTypes() override
	{
		return std::pair<int,int>(RIGID_SHAPE_TYPE_SPHERE, RIGID_SHAPE_TYPE_DOUBLESIDEDPLANE);
	}

private:
	/// Calculate the actual contact between two shapes of the given CollisionPair.
	/// \param	pair	The symmetric pair that is under consideration.
	virtual void doCalculateContact(std::shared_ptr<CollisionPair> pair);

};


/// Class to calculate intersections between Spheres and Planes
class SpherePlaneDcdContact : public ContactCalculation
{
public:

	/// Constructor.
	explicit SpherePlaneDcdContact()
	{
	}

	virtual std::pair<int,int> getShapeTypes() override
	{
		return std::pair<int,int>(RIGID_SHAPE_TYPE_SPHERE, RIGID_SHAPE_TYPE_PLANE);
	}

private:
	/// Calculate the actual contact between two shapes of the given CollisionPair.
	/// \param	pair	The symmetric pair that is under consideration.
	virtual void doCalculateContact(std::shared_ptr<CollisionPair> pair);

};


/// Class to calculate intersections between Boxes and Planes
class BoxPlaneDcdContact : public ContactCalculation
{
public:

    /// Constructor.
    explicit BoxPlaneDcdContact()
    {
    }

	virtual std::pair<int,int> getShapeTypes() override
	{
		return std::pair<int,int>(RIGID_SHAPE_TYPE_BOX, RIGID_SHAPE_TYPE_PLANE);
	}

private:
	/// Calculate the actual contact between two shapes of the given CollisionPair.
	/// \param	pair	The symmetric pair that is under consideration.
	virtual void doCalculateContact(std::shared_ptr<CollisionPair> pair);

};


/// Class to calculate intersections between a capsule and a sphere
class CapsuleSphereDcdContact : public ContactCalculation
{
public:

    /// Constructor.
    explicit CapsuleSphereDcdContact()
    {
    }

    virtual std::pair<int,int> getShapeTypes() override
    {
        return std::pair<int,int>(RIGID_SHAPE_TYPE_CAPSULE, RIGID_SHAPE_TYPE_SPHERE);
    }

private:
    /// Calculate the actual contact between two shapes of the given CollisionPair.
    /// \param    pair    The symmetric pair that is under consideration.
    virtual void doCalculateContact(std::shared_ptr<CollisionPair> pair) override;

};


}; // Physics
}; // SurgSim

#endif
