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

#ifndef SURGSIM_DATASTRUCTURES_MESHVERTEX_H
#define SURGSIM_DATASTRUCTURES_MESHVERTEX_H

#include <SurgSim/Math/Vector.h>

#include <memory>

namespace SurgSim
{

namespace DataStructures
{

/// Vertex structure for meshes. Vertices are the lowest level of structure in a Mesh, providing a position and can
/// store extra per-vertex data. MeshElements combine MeshVertices to form the structure of the mesh.
///
/// MeshVertex is to be used purely as a data structure and not provide implementation of algorithms.
/// For example, a physics FEM's nodes are not subclasses of MeshVertex if they provide code that is part of the FEM
/// algorithm, but they may used with a Mesh to store the structure of the FEM.
///
/// The extra Data is left up to the particular use of Mesh to specify. For example, for use collision detection,
/// a vertex may need a normal and adjacent triangle information, which could be stored in a struct.
///
/// If no extra Data is needed, a specialization exists for void, in which case the constructor takes no data.
///
/// \tparam	Data	Type of extra data stored in the vertex (void for no data)
/// \sa	Mesh
template <class Data>
struct MeshVertex
{
	/// Constructor
	/// \param	position	Position of the vertex
	/// \param	data	Extra data to be stored in the vertex
	MeshVertex(const SurgSim::Math::Vector3d& position, const Data& data) :
		position(position),
		data(data)
	{
	}

	/// Position of the vertex.
	SurgSim::Math::Vector3d position;
	/// Extra vertex data.
	Data data;

	/// Compare the vertices and return true if equal, false if not equal.
	friend bool operator==(const MeshVertex<Data>& vertex1, const MeshVertex<Data>& vertex2)
	{
		return vertex1.data == vertex2.data && vertex1.position == vertex2.position;
	}

	/// Compare the vertices and return false if equal, true if not equal.
	friend bool operator!=(const MeshVertex<Data>& vertex1, const MeshVertex<Data>& vertex2)
	{
		return ! (vertex1 == vertex2);
	}
};

/// Specialization of MeshVertex with no data.
/// \sa MeshVertex
template <>
struct MeshVertex<void>
{
	/// Constructor
	/// \param	position	Position of the vertex
	explicit MeshVertex(const SurgSim::Math::Vector3d& position) : position(position)
	{
	}

	/// Position of the vertex.
	SurgSim::Math::Vector3d position;

	/// Compare the vertices and return true if equal, false if not equal.
	friend bool operator==(const MeshVertex<void>& vertex1, const MeshVertex<void>& vertex2)
	{
		return vertex1.position == vertex2.position;
	}

	/// Compare the vertices and return false if equal, true if not equal.
	friend bool operator!=(const MeshVertex<void>& vertex1, const MeshVertex<void>& vertex2)
	{
		return ! (vertex1 == vertex2);
	}
};

};  // namespace DataStructures

};  // namespace SurgSim

#endif  // SURGSIM_DATASTRUCTURES_MESHVERTEX_H