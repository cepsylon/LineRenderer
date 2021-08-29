#include "Renderer.h"

#include <stdio.h>
#include <string>

void Renderer::initialize(int width, int height)
{
	// Create texture for framebuffer
	glGenTextures(1, &mFramebufferTexture);
	glBindTexture(GL_TEXTURE_2D, mFramebufferTexture);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, width, height, 0, GL_RGB, GL_UNSIGNED_BYTE, nullptr);

	// Create framebuffer
	glGenFramebuffers(1, &mFramebuffer);
	glBindFramebuffer(GL_FRAMEBUFFER, mFramebuffer);
	glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, mFramebufferTexture, 0);
	GLenum color_attachments[] = {
		GL_COLOR_ATTACHMENT0
	};
	glDrawBuffers(sizeof(color_attachments) / sizeof(GLenum), color_attachments);

	// Error checking
	if (glCheckFramebufferStatus(GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE)
		fprintf(stdout, "Framebuffer failed to complete.\n");

	glClearColor(1.0f, 1.0f, 1.0f, 1.0f);
	glClear(GL_COLOR_BUFFER_BIT);

	glBindFramebuffer(GL_FRAMEBUFFER, 0u);

	// Create plane
	float vertices[] = {
		-1.0f, -1.0f, 0.0f, 0.0f,	// V0
		1.0f, -1.0f, 1.0f, 0.0f,	// V1
		-1.0f, 1.0f, 0.0f, 1.0f,	// V2
		1.0f, 1.0f, 1.0f, 1.0f		// V3
	};

	glGenBuffers(1, &mPlane);
	glBindBuffer(GL_ARRAY_BUFFER, mPlane);
	glBufferData(GL_ARRAY_BUFFER, sizeof(vertices), vertices, GL_STATIC_DRAW);
	glEnableVertexAttribArray(0);
	glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 4 * sizeof(float), (void*)0);
	glEnableVertexAttribArray(1);
	glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 4 * sizeof(float), (void*)(2 * sizeof(float)));

	// Create shader
	GLuint vertexShader = glCreateShader(GL_VERTEX_SHADER);
	const GLchar* v_code = 
	"#version 330\n"
	// From input assembler
	"layout(location = 0) in vec2 v_position;\n"
	"layout(location = 1) in vec2 v_uv;\n"
	// Outputs to next stage
	"out vec2 f_uv;\n"

	"void main()\n"
	"{\n"
		"gl_Position = vec4(v_position, 0.0f, 1.0f);\n"
		"f_uv = v_uv;\n"
	"}";
	glShaderSource(vertexShader, 1, &v_code, 0);
	glCompileShader(vertexShader);

	// Error checking
	GLint compiled = 0;
	glGetShaderiv(vertexShader, GL_COMPILE_STATUS, &compiled);
	if (compiled == GL_FALSE)
	{
		// Get error string and delete shader
		GLint error_length = 0;
		glGetShaderiv(vertexShader, GL_INFO_LOG_LENGTH, &error_length);
		std::string error_data;
		error_data.resize(error_length);
		glGetShaderInfoLog(vertexShader, error_length, &error_length, &error_data[0]);
		glDeleteShader(vertexShader); vertexShader = 0;

		// Output error info
		fprintf(stdout, "Error compiling vertex shader:\n%s\n", error_data.c_str());
	}

	GLuint fragmentShader = glCreateShader(GL_FRAGMENT_SHADER);
	const GLchar* f_code =
		"#version 330\n"
		// Inputs from previous stage
		"in vec2 f_uv;\n"
		// Uniforms
		"uniform sampler2D image;\n"
		// Outputs to buffer
		"out vec4 out_color;\n"
		"void main()\n"
		"{\n"
			"out_color = texture(image, f_uv);\n"
		"}";
	glShaderSource(fragmentShader, 1, &f_code, 0);
	glCompileShader(fragmentShader);

	// Error checking
	compiled = 0;
	glGetShaderiv(fragmentShader, GL_COMPILE_STATUS, &compiled);
	if (compiled == GL_FALSE)
	{
		// Get error string and delete shader
		GLint error_length = 0;
		glGetShaderiv(fragmentShader, GL_INFO_LOG_LENGTH, &error_length);
		std::string error_data;
		error_data.resize(error_length);
		glGetShaderInfoLog(fragmentShader, error_length, &error_length, &error_data[0]);
		glDeleteShader(fragmentShader); fragmentShader = 0;

		// Output error info
		fprintf(stdout, "Error compiling vertex shader:\n%s\n", error_data.c_str());
	}

	// Create program
	mProgramToDisplay = glCreateProgram();
	glAttachShader(mProgramToDisplay, vertexShader);
	glAttachShader(mProgramToDisplay, fragmentShader);
	glLinkProgram(mProgramToDisplay);

	// Error checking
	GLint linked = 0;
	glGetProgramiv(mProgramToDisplay, GL_LINK_STATUS, &linked);
	if (linked == GL_FALSE)
	{
		// Get error string and delete the program
		GLint error_length = 0;
		glGetProgramiv(mProgramToDisplay, GL_INFO_LOG_LENGTH, &error_length);
		std::string error;
		error.resize(error_length);
		glGetProgramInfoLog(mProgramToDisplay, error_length, &error_length, &error[0]);
		glDeleteProgram(mProgramToDisplay); mProgramToDisplay = 0;

		// Output error
		fprintf(stdout, "Error linking program with shaders with error: %s\n", error.c_str());
	}

	// Release resources we no longer need
	glDetachShader(mProgramToDisplay, fragmentShader);
	glDetachShader(mProgramToDisplay, vertexShader);
	glDeleteShader(fragmentShader);
	glDeleteShader(vertexShader);
}

void Renderer::render()
{
	glBindFramebuffer(GL_FRAMEBUFFER, 0u);
	glUseProgram(mProgramToDisplay);
	glActiveTexture(GL_TEXTURE0);
	glBindTexture(GL_TEXTURE_2D, mFramebufferTexture);
	glUniform1i(glGetUniformLocation(mFramebufferTexture, "image"), 0);
	glBindBuffer(GL_ARRAY_BUFFER, mPlane);
	glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);
}

void Renderer::shutdown()
{
	glDeleteFramebuffers(1, &mFramebuffer);
	glDeleteTextures(1, &mFramebufferTexture);
}
