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

#ifndef SURGSIM_GRAPHICS_SHADER_H
#define SURGSIM_GRAPHICS_SHADER_H

#include <string>

namespace SurgSim
{

namespace Graphics
{

/// Base class that defines the interface for graphics shaders.
///
/// Shaders are the programs executed on the GPU to render the scene geometry.
class Shader
{
public:
	/// Destructor
	virtual ~Shader() = 0;

	/// Returns true if the vertex shader has been set, otherwise false.
	virtual bool hasVertexShader() = 0;

	/// Removes the vertex shader, returning that portion of the shader program to fixed-function.
	virtual void clearVertexShader() = 0;

	/// Loads the vertex shader source code from a file
	/// \param	filePath	Path to file containing shader source code
	/// \return	True if the source is successfully loaded, otherwise false.
	virtual bool loadVertexShaderSource(const std::string& filePath) = 0;

	/// Set the vertex shader source code
	/// \param	source	Shader source code
	virtual void setVertexShaderSource(const std::string& source) = 0;

	/// Gets the vertex shader source code
	/// \return	Shader source code
	virtual bool getVertexShaderSource(std::string* source) const = 0;

	/// Returns true if the geometry shader has been set, otherwise false.
	virtual bool hasGeometryShader() const = 0;

	/// Removes the geometry shader, returning that portion of the shader program to fixed-function.
	virtual void clearGeometryShader() = 0;

	/// Loads the geometry shader source code from a file
	/// \param	filePath	Path to file containing shader source code
	/// \return	True if the source is successfully loaded, otherwise false.
	virtual bool loadGeometryShaderSource(const std::string& filePath) = 0;

	/// Set the geometry shader source code
	/// \param	source	Shader source code
	virtual void setGeometryShaderSource(const std::string& source) = 0;

	/// Gets the geometry shader source code
	/// \return	Shader source code
	virtual bool getGeometryShaderSource(std::string* source) const = 0;


	/// Returns true if the fragment shader has been set, otherwise false.
	virtual bool hasFragmentShader() const = 0;

	/// Removes the fragment shader, returning that portion of the shader program to fixed-function.
	virtual void clearFragmentShader() = 0;

	/// Loads the fragment shader source code from a file
	/// \param	filePath	Path to file containing shader source code
	/// \return	True if the source is successfully loaded, otherwise false.
	virtual bool loadFragmentShaderSource(const std::string& filePath) = 0;

	/// Set the fragment shader source code
	/// \param	source	Shader source code
	virtual void setFragmentShaderSource(const std::string& source) = 0;

	/// Gets the fragment shader source code
	/// \return	Shader source code
	virtual bool getFragmentShaderSource(std::string* source) const = 0;

	/// Clears the entire shader, returning to fixed-function pipeline.
	virtual void clear()
	{
		clearVertexShader();
		clearGeometryShader();
		clearFragmentShader();
	}
};

inline Shader::~Shader()
{
}

};  // namespace Graphics

};  // namespace SurgSim

#endif  // SURGSIM_GRAPHICS_SHADER_H