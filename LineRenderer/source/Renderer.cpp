#include "Renderer.h"

#include <stdio.h>
#include <string>
#include <random>
#include <time.h>

void Renderer::initialize(int width, int height)
{
	srand(static_cast<unsigned int>(time(0)));
	assign_random_color();

	mHalfWidth = static_cast<float>(width) * 0.5f;
	mHalfHeight = static_cast<float>(height) * 0.5f;
	create_framebuffer(width, height);
	create_buffers();
	create_shaders();

	// Need to set blending for correct color merge when drawing lines
	glEnable(GL_BLEND);
	glBlendEquationSeparate(GL_FUNC_ADD, GL_FUNC_ADD);
	glBlendFuncSeparate(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA, GL_ONE, GL_ONE_MINUS_SRC_ALPHA);
}

void Renderer::render()
{
	// Copy cached lines to back buffer
	glBindFramebuffer(GL_FRAMEBUFFER, 0u);
	glUseProgram(mProgramToDisplay);
	bind_plane();
	glActiveTexture(GL_TEXTURE0);
	glBindTexture(GL_TEXTURE_2D, mFramebufferTexture);
	glUniform1i(glGetUniformLocation(mProgramToDisplay, "image"), 0);
	glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);

	// Render line if we are not yet done
	if (mIsDrawingLine)
		render_line();
}

void Renderer::shutdown()
{
	glDeleteProgram(mProgramToDisplay);
	glDeleteBuffers(1, &mLine);
	glDeleteBuffers(1, &mPlane);
	glDeleteFramebuffers(1, &mFramebuffer);
	glDeleteTextures(1, &mFramebufferTexture);
}

void Renderer::end_line(int x, int y)
{
	mIsDrawingLine = false;

	// Render finished line to static image so we don't have to compute it every time
	glBindFramebuffer(GL_FRAMEBUFFER, mFramebuffer);
	render_line();
	glBindFramebuffer(GL_FRAMEBUFFER, 0);

	assign_random_color();
}

void Renderer::create_framebuffer(int width, int height)
{
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
}

void Renderer::create_buffers()
{
	// Plane
	float vertices[] = {
		 -1.0f, -1.0f, 0.0f, 0.0f,	// V0 (2 pos, 2 uv)
		 1.0f, -1.0f, 1.0f, 0.0f,	// V1
		 -1.0f, 1.0f, 0.0f, 1.0f,	// V2
		 1.0f, 1.0f, 1.0f, 1.0f		// V3
	};

	glGenBuffers(1, &mPlane);
	bind_plane();
	glBufferData(GL_ARRAY_BUFFER, sizeof(vertices), vertices, GL_STATIC_DRAW);

	// Line
	glGenBuffers(1, &mLine);
	bind_line();
	float line[] =
	{
		static_cast<float>(mStartX), static_cast<float>(mStartY), 
		static_cast<float>(mEndX), static_cast<float>(mEndY)
	};
	glBufferData(GL_ARRAY_BUFFER, sizeof(line), line, GL_DYNAMIC_DRAW);
}

void Renderer::create_shaders()
{
	// SDF line program and transfer program
	GLuint vertexShader = glCreateShader(GL_VERTEX_SHADER);
	const GLchar* v_code =
		"#version 330\n"

		"layout(location = 0) in vec2 v_position;\n"
		"layout(location = 1) in vec2 v_uv;\n"

		"out vec2 f_uv;\n"

		"void main()\n"
		"{\n"
		"gl_Position = vec4(v_position, 0.0f, 1.0f);\n"
		"f_uv = v_uv;\n"
		"}";
	glShaderSource(vertexShader, 1, &v_code, 0);
	glCompileShader(vertexShader);
	check_shader_compiled(vertexShader, "vertex");

	GLuint fragmentShader = glCreateShader(GL_FRAGMENT_SHADER);
	const GLchar* f_code =
		"#version 330\n"

		"in vec2 f_uv;\n"

		"uniform sampler2D image;\n"

		"out vec4 out_color;\n"
		"void main()\n"
		"{\n"
			"out_color = texture(image, f_uv);\n"
		"}";
	glShaderSource(fragmentShader, 1, &f_code, 0);
	glCompileShader(fragmentShader);
	check_shader_compiled(fragmentShader, "copyTextureFragment");

	// Create program
	mProgramToDisplay = glCreateProgram();
	glAttachShader(mProgramToDisplay, vertexShader);
	glAttachShader(mProgramToDisplay, fragmentShader);
	glLinkProgram(mProgramToDisplay);
	check_program_linked(mProgramToDisplay, "ToDisplayProgram");
	glDetachShader(mProgramToDisplay, fragmentShader);
	glDetachShader(mProgramToDisplay, vertexShader);

	// SDF program
	GLuint fragmentSDFShader = glCreateShader(GL_FRAGMENT_SHADER);
	const GLchar* fsdf_code =
		"#version 330\n"

		"uniform ivec2 start;\n"
		"uniform ivec2 end;\n"
		"uniform vec3 color;\n"
		"uniform float radius;\n"

		"out vec4 fragColor;\n"

		"float udSegment( in vec2 p, in vec2 a, in vec2 b )\n"
		"{\n"
			"vec2 ba = b - a;\n"
			"vec2 pa = p - a;\n"
			"float h = clamp(dot(pa, ba) / dot(ba, ba), 0.0, 1.0);\n"
			"return length(pa - h * ba);\n"
		"}\n"

		"void main()\n"
		"{\n"
			"vec2 p = (gl_FragCoord.xy - 512.0f) / 512.0f;\n"

			"vec2 v1 = vec2(start / 512.0f);\n"
			"vec2 v2 = vec2(end / 512.0f);\n"
			"float d = udSegment(p, v1, v2) - radius;\n"

			"float alpha = 1.0f - sign(d);\n"
			// Smooth edges
			"alpha = mix(alpha, 1.0, 1.0 - smoothstep(0.0, 0.005, abs(d)));\n"

			"fragColor = vec4(color, alpha);\n"
		"}";
	glShaderSource(fragmentSDFShader, 1, &fsdf_code, 0);
	glCompileShader(fragmentSDFShader);
	check_shader_compiled(fragmentSDFShader, "SDFFragment");

	// Line rendering program
	mProgramSDF = glCreateProgram();
	glAttachShader(mProgramSDF, vertexShader);
	glAttachShader(mProgramSDF, fragmentSDFShader);
	glLinkProgram(mProgramSDF);
	check_program_linked(mProgramSDF, "SDFProgram");
	glDetachShader(mProgramSDF, fragmentSDFShader);
	glDetachShader(mProgramSDF, vertexShader);

	// Release resources we no longer need
	glDeleteShader(fragmentSDFShader);
	glDeleteShader(fragmentShader);
	glDeleteShader(vertexShader);

	//------------------------------
	// Simple line rendering program
	vertexShader = glCreateShader(GL_VERTEX_SHADER);
	v_code =
		"#version 330\n"

		"layout(location = 0) in vec2 v_position;\n"

		"out vec2 f_uv;\n"

		"void main()\n"
		"{\n"
			"gl_Position = vec4(v_position, 0.0f, 1.0f);\n"
		"}";
	glShaderSource(vertexShader, 1, &v_code, 0);
	glCompileShader(vertexShader);
	check_shader_compiled(vertexShader, "vertexLine");

	fragmentShader = glCreateShader(GL_FRAGMENT_SHADER);
	f_code =
		"#version 330\n"

		"in vec2 f_uv;\n"

		"uniform vec3 color;\n"

		"out vec4 out_color;\n"
		"void main()\n"
		"{\n"
			"out_color = vec4(color, 1.0f);\n"
		"}";
	glShaderSource(fragmentShader, 1, &f_code, 0);
	glCompileShader(fragmentShader);
	check_shader_compiled(fragmentShader, "framgentLine");

	mProgramSimple = glCreateProgram();
	glAttachShader(mProgramSimple, vertexShader);
	glAttachShader(mProgramSimple, fragmentShader);
	glLinkProgram(mProgramSimple);
	check_program_linked(mProgramSimple, "SDFProgram");
	glDetachShader(mProgramSimple, fragmentShader);
	glDetachShader(mProgramSimple, vertexShader);

	// Release resources we no longer need
	glDeleteShader(fragmentShader);
	glDeleteShader(vertexShader);
}

void Renderer::check_shader_compiled(int shader, const char* shader_name)
{
	GLint compiled = 0;
	glGetShaderiv(shader, GL_COMPILE_STATUS, &compiled);
	if (compiled == GL_FALSE)
	{
		// Get error string and delete shader
		GLint error_length = 0;
		glGetShaderiv(shader, GL_INFO_LOG_LENGTH, &error_length);
		std::string error_data;
		error_data.resize(error_length);
		glGetShaderInfoLog(shader, error_length, &error_length, &error_data[0]);
		glDeleteShader(shader);

		// Output error info
		fprintf(stdout, "Error compiling %s shader:\n%s\n", shader_name, error_data.c_str());
	}
}

void Renderer::check_program_linked(int program, const char* program_name)
{
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
		fprintf(stdout, "Error linking %s program with shaders with error: %s\n", program_name, error.c_str());
	}
}

void Renderer::assign_random_color()
{
	mR = static_cast<float>(rand()) / RAND_MAX;
	mG = static_cast<float>(rand()) / RAND_MAX;
	mB = static_cast<float>(rand()) / RAND_MAX;
}

void Renderer::render_line()
{
	if (mLineSimple)
	{
		glUseProgram(mProgramSimple);
		bind_line();
		float line[] =
		{
			static_cast<float>(mStartX) / mHalfWidth, static_cast<float>(mStartY) / mHalfHeight,
			static_cast<float>(mEndX) / mHalfWidth, static_cast<float>(mEndY) / mHalfHeight
		};
		glBufferData(GL_ARRAY_BUFFER, sizeof(line), line, GL_DYNAMIC_DRAW);
		glUniform3f(glGetUniformLocation(mProgramSimple, "color"), mR, mG, mB);
		glDrawArrays(GL_LINES, 0, 2);
	}
	else
	{
		glUseProgram(mProgramSDF);
		bind_plane();
		glUniform2i(glGetUniformLocation(mProgramSDF, "start"), mStartX, mStartY);
		glUniform2i(glGetUniformLocation(mProgramSDF, "end"), mEndX, mEndY);
		glUniform3f(glGetUniformLocation(mProgramSDF, "color"), mR, mG, mB);
		glUniform1f(glGetUniformLocation(mProgramSDF, "radius"), mRadius);
		glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);
	}
}

void Renderer::bind_plane()
{
	glBindBuffer(GL_ARRAY_BUFFER, mPlane);
	glEnableVertexAttribArray(0);
	glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 4 * sizeof(float), (void*)0);
	glEnableVertexAttribArray(1);
	glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 4 * sizeof(float), (void*)(2 * sizeof(float)));
}

void Renderer::bind_line()
{
	glBindBuffer(GL_ARRAY_BUFFER, mLine);
	glEnableVertexAttribArray(0);
	glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 2 * sizeof(float), (void*)0);
}
