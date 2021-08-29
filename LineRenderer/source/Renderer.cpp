#include "Renderer.h"

#include <stdio.h>
#include <string>
#include <random>
#include <time.h>

void Renderer::initialize(int width, int height)
{
	srand(time(0));
	mR = static_cast<float>(rand()) / RAND_MAX;
	mG = static_cast<float>(rand()) / RAND_MAX;
	mB = static_cast<float>(rand()) / RAND_MAX;

	// Create texture for framebuffer
	glGenTextures(1, &mFramebufferTexture);
	glBindTexture(GL_TEXTURE_2D, mFramebufferTexture);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, width, height, 0, GL_RGBA, GL_UNSIGNED_BYTE, nullptr);

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

	glClearColor(0.0f, 0.0f, 0.0f, 1.0f);
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
		fprintf(stdout, "Error compiling fragment shader:\n%s\n", error_data.c_str());
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
	glDetachShader(mProgramToDisplay, fragmentShader);
	glDetachShader(mProgramToDisplay, vertexShader);

	// SDF program
	GLuint fragmentSDFShader = glCreateShader(GL_FRAGMENT_SHADER);
	const GLchar* fsdf_code =
		"#version 330\n"

		// Uniforms
		"uniform ivec2 start;\n"
		"uniform ivec2 end;\n"
		"uniform vec3 color;\n"
		"uniform float radius;\n"

		"out vec4 fragColor;\n"

		"float udSegment( in vec2 p, in vec2 a, in vec2 b )\n"
		"{\n"
		"	vec2 ba = b - a;\n"
		"	vec2 pa = p - a;\n"
		"	float h = clamp(dot(pa, ba) / dot(ba, ba), 0.0, 1.0);\n"
		"	return length(pa - h * ba);\n"
		"}\n"

		"void main()\n"
		"{\n"
		"	vec2 p = (gl_FragCoord.xy - 512.0f) / 512.0f;\n"

		"	vec2 v1 = vec2(start / 512.0f);\n"
		"	vec2 v2 = vec2(end / 512.0f);\n"
		"	float d = udSegment(p, v1, v2) - radius;\n"

		"	float alpha = 1.0f - sign(d);\n"
		// Smooth edges
		"	alpha = mix(alpha, 1.0, 1.0 - smoothstep(0.0, 0.005, abs(d)));\n"

		"	fragColor = vec4(color, alpha);\n"
		"}";
	glShaderSource(fragmentSDFShader, 1, &fsdf_code, 0);
	glCompileShader(fragmentSDFShader);

	// Error checking
	compiled = 0;
	glGetShaderiv(fragmentSDFShader, GL_COMPILE_STATUS, &compiled);
	if (compiled == GL_FALSE)
	{
		// Get error string and delete shader
		GLint error_length = 0;
		glGetShaderiv(fragmentSDFShader, GL_INFO_LOG_LENGTH, &error_length);
		std::string error_data;
		error_data.resize(error_length);
		glGetShaderInfoLog(fragmentSDFShader, error_length, &error_length, &error_data[0]);
		glDeleteShader(fragmentSDFShader); fragmentSDFShader = 0;

		// Output error info
		fprintf(stdout, "Error compiling sdf shader:\n%s\n", error_data.c_str());
	}

	mProgramSDF = glCreateProgram();
	glAttachShader(mProgramSDF, vertexShader);
	glAttachShader(mProgramSDF, fragmentSDFShader);
	glLinkProgram(mProgramSDF);

	// Error checking
	linked = 0;
	glGetProgramiv(mProgramSDF, GL_LINK_STATUS, &linked);
	if (linked == GL_FALSE)
	{
		// Get error string and delete the program
		GLint error_length = 0;
		glGetProgramiv(mProgramSDF, GL_INFO_LOG_LENGTH, &error_length);
		std::string error;
		error.resize(error_length);
		glGetProgramInfoLog(mProgramSDF, error_length, &error_length, &error[0]);
		glDeleteProgram(mProgramSDF); mProgramSDF = 0;

		// Output error
		fprintf(stdout, "Error linking program with shaders with error: %s\n", error.c_str());
	}
	glDetachShader(mProgramSDF, fragmentSDFShader);
	glDetachShader(mProgramSDF, vertexShader);

	// Release resources we no longer need
	glDeleteShader(fragmentSDFShader);
	glDeleteShader(fragmentShader);
	glDeleteShader(vertexShader);
}

void Renderer::render()
{
	glBindFramebuffer(GL_FRAMEBUFFER, 0u);
	glEnable(GL_BLEND);
	glBlendEquationSeparate(GL_FUNC_ADD, GL_FUNC_ADD);
	glBlendFuncSeparate(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA, GL_ONE, GL_ONE_MINUS_SRC_ALPHA);
	glUseProgram(mProgramToDisplay);
	glActiveTexture(GL_TEXTURE0);
	glBindTexture(GL_TEXTURE_2D, mFramebufferTexture);
	glUniform1i(glGetUniformLocation(mProgramToDisplay, "image"), 0);
	glBindBuffer(GL_ARRAY_BUFFER, mPlane);
	glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);

	// If we are still drawing a line
	if (mIsDrawingLine)
	{
		glEnable(GL_BLEND);
		glUseProgram(mProgramSDF);
		glUniform2i(glGetUniformLocation(mProgramSDF, "start"), mStartX, mStartY);
		glUniform2i(glGetUniformLocation(mProgramSDF, "end"), mEndX, mEndY);
		glUniform3f(glGetUniformLocation(mProgramSDF, "color"), mR, mG, mB);
		glUniform1f(glGetUniformLocation(mProgramSDF, "radius"), mRadius);
		glBindBuffer(GL_ARRAY_BUFFER, mPlane);
		glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);
	}
}

void Renderer::shutdown()
{
	glDeleteProgram(mProgramToDisplay);
	glDeleteBuffers(1, &mPlane);
	glDeleteFramebuffers(1, &mFramebuffer);
	glDeleteTextures(1, &mFramebufferTexture);
}

void Renderer::end_line(int x, int y)
{
	mIsDrawingLine = false;

	// Render finished line to static image so we don't have to compute it every time
	glBindFramebuffer(GL_FRAMEBUFFER, mFramebuffer);
	glUseProgram(mProgramSDF);
	glUniform2i(glGetUniformLocation(mProgramSDF, "start"), mStartX, mStartY);
	glUniform2i(glGetUniformLocation(mProgramSDF, "end"), mEndX, mEndY);
	glUniform3f(glGetUniformLocation(mProgramSDF, "color"), mR, mG, mB);
	glUniform1f(glGetUniformLocation(mProgramSDF, "radius"), mRadius);
	glBindBuffer(GL_ARRAY_BUFFER, mPlane);
	glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);
	glBindFramebuffer(GL_FRAMEBUFFER, 0);

	mR = static_cast<float>(rand()) / RAND_MAX;
	mG = static_cast<float>(rand()) / RAND_MAX;
	mB = static_cast<float>(rand()) / RAND_MAX;
}
