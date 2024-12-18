//////////////////////////////////////////////////////////////////////////////
//
//  --- LoadShaders.cxx ---
//
//////////////////////////////////////////////////////////////////////////////

#include <cstdlib>
#include <iostream>

#include "glew.h"
#include "LoadShaders.h"

#ifdef __cplusplus
extern "C" {
#endif // __cplusplus

	//----------------------------------------------------------------------------

	static const GLchar*
		ReadShader(const char* filename)
	{
		// Attempt to open the shader file
		FILE* infile;
		fopen_s(&infile, filename, "rb");

		if (!infile) {
#ifdef _DEBUG
			std::cerr << "Unable to open shader file '" << filename << "'" << std::endl;
#endif /* DEBUG */
			return NULL;
		}

		// Get the file length
		fseek(infile, 0, SEEK_END);
		long len = ftell(infile); // Use long to avoid overflow issues
		fseek(infile, 0, SEEK_SET);

		// Read the shader source code
		GLchar* source = new GLchar[len + 1]; // Allocate space for null terminator

		// Check if memory allocation is successful
		if (source == nullptr) {
			std::cerr << "Memory allocation for shader source failed!" << std::endl;
			fclose(infile);
			return NULL;
		}

		size_t bytesRead = fread(source, 1, len, infile);
		if (bytesRead != len) {
			std::cerr << "Error reading shader file '" << filename << "'. Expected " << len << " bytes, but read " << bytesRead << " bytes." << std::endl;
			delete[] source;
			fclose(infile);
			return NULL;
		}

		fclose(infile);

		source[len] = 0; // Null-terminate the shader code

#ifdef _DEBUG
		std::cerr << "Shader file '" << filename << "' read successfully, length: " << len << " bytes." << std::endl;
#endif /* DEBUG */

		return const_cast<const GLchar*>(source);
	}

	//----------------------------------------------------------------------------

	GLuint
		LoadShaders(ShaderInfo* shaders)
	{
		if (shaders == NULL) {
#ifdef _DEBUG
			std::cerr << "No shaders provided to LoadShaders." << std::endl;
#endif /* DEBUG */
			return 0;
		}

		GLuint program = glCreateProgram();
		if (program == 0) {
			std::cerr << "Error creating shader program." << std::endl;
			return 0;
		}

#ifdef _DEBUG
		std::cerr << "Shader program created successfully." << std::endl;
#endif /* DEBUG */

		ShaderInfo* entry = shaders;
		while (entry->type != GL_NONE) {
			GLuint shader = glCreateShader(entry->type);

			entry->shader = shader;

			// Read shader source code
			const GLchar* source = ReadShader(entry->filename);
			if (source == NULL) {
#ifdef _DEBUG
				std::cerr << "Failed to read shader file: " << entry->filename << std::endl;
#endif /* DEBUG */
				for (entry = shaders; entry->type != GL_NONE; ++entry) {
					glDeleteShader(entry->shader);
					entry->shader = 0;
				}

				return 0;
			}

			// Compile shader
			glShaderSource(shader, 1, &source, NULL);
			delete[] source;

			glCompileShader(shader);

			// Check for compilation errors
			GLint compiled;
			glGetShaderiv(shader, GL_COMPILE_STATUS, &compiled);
			if (!compiled) {
#ifdef _DEBUG
				GLsizei len;
				glGetShaderiv(shader, GL_INFO_LOG_LENGTH, &len);

				GLchar* log = new GLchar[len + 1];
				glGetShaderInfoLog(shader, len, &len, log);
				std::cerr << "Shader compilation failed for " << entry->filename << ": " << log << std::endl;
				delete[] log;
#endif /* DEBUG */

				return 0;
			}

#ifdef _DEBUG
			std::cerr << "Shader compiled successfully: " << entry->filename << std::endl;
#endif /* DEBUG */

			glAttachShader(program, shader);

			++entry;
		}

		// Link program
		glLinkProgram(program);

		// Check for linking errors
		GLint linked;
		glGetProgramiv(program, GL_LINK_STATUS, &linked);
		if (!linked) {
#ifdef _DEBUG
			GLsizei len;
			glGetProgramiv(program, GL_INFO_LOG_LENGTH, &len);

			GLchar* log = new GLchar[len + 1];
			glGetProgramInfoLog(program, len, &len, log);
			std::cerr << "Shader program linking failed: " << log << std::endl;
			delete[] log;
#endif /* DEBUG */

			for (entry = shaders; entry->type != GL_NONE; ++entry) {
				glDeleteShader(entry->shader);
				entry->shader = 0;
			}

			return 0;
		}

#ifdef _DEBUG
		std::cerr << "Shader program linked successfully." << std::endl;
#endif /* DEBUG */

		return program;
	}

	//----------------------------------------------------------------------------

#ifdef __cplusplus
}
#endif // __cplusplus
