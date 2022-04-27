#pragma once
#define STB_IMAGE_IMPLEMENTATION
#define GLFW_EXPOSE_NATIVE_WIN32

#include <Windows.h>
#include <iostream>
#include <gl/glew.h>
#include <GLFW/glfw3.h>
#include <GLFW/glfw3native.h>
#include <stb_image.h>
#include <boost/scope_exit.hpp>
#include "CaptureWindow.h"
#include "shards.png.hpp"
GLuint compile_shaders() {
	const char* vertexShaderSource =
		"#version 330 core\n"
		"in vec4 input_pos;\n"
		"out vec2 point_pos;\n"
		"void main() {\n"
		"   gl_Position = input_pos;\n"
		"   point_pos = vec2(1,-1) * input_pos.xy / 2 + vec2(0.5,0.5);\n"
		"}\n";
	const char* fragmentShaderSource =
		"#version 330 core\n"
		"in vec2 point_pos;\n"
		"uniform sampler2D uTexture;\n"
		"uniform float uProgress;\n"
		"uniform int uSizeX;\n"
		"uniform int uSizeY;\n"
		"uniform sampler2D uShardTexture;\n"
		"const vec2  SEED = vec2(0.0, 0.0);\n"
		"const float SHARD_SCALE  = 1;\n"
		"const float SHARD_LAYERS = 5;\n"
		"const float BLOW_FORCE   = 0.5;\n"
		"const float ACTOR_SCALE  = 2.0;\n"
		"const float PADDING      = ACTOR_SCALE / 2.0 - 0.5;\n"
		"const vec2  EPICENTER    = vec2(0.5,0.5);\n"
		"const float GRAVITY      = - 1.0;\n"
		"layout(location=0) out vec4 color;\n"
		"void main() {\n"
		"   color = vec4(0, 0, 0, 0);\n"
		"   float progress = uProgress;\n"
		"   for (float i=0; i<SHARD_LAYERS; ++i) {\n"
		"       vec2 coords = point_pos * ACTOR_SCALE - vec2(PADDING,PADDING);\n"
		"       coords -= EPICENTER;\n"
		"       coords /= mix(1.0, 1.0 + BLOW_FORCE*(i+2)/SHARD_LAYERS, progress);\n"
		"       float rotation = (mod(i, 2.0)-0.5)*0.2*progress;\n"
		"       coords = vec2(coords.x * cos(rotation) - coords.y * sin(rotation), coords.x * sin(rotation) + coords.y * cos(rotation));\n"
		"       float gravity = GRAVITY*0.1*(i+1)*progress*progress;\n"
		"       coords += vec2(0, gravity);\n"
		"       coords += EPICENTER;\n"
		"       vec2 shardCoords = (coords + SEED) * vec2(uSizeX, uSizeY) / SHARD_SCALE / 500.0;\n"
		"       vec2 shardMap = texture(uShardTexture, shardCoords).rg;\n"
		"       float shardGroup = floor(shardMap.g * SHARD_LAYERS * 0.999);\n"
		"       if (shardGroup == i) {\n"
		"           vec4 windowColor = texture(uTexture, coords);\n"
		"           float dissolve = (shardMap.x - pow(progress+0.1, 2)) > 0 ? 1: 0;\n"
		"           color = mix(color, windowColor, dissolve);\n"
		"       }\n"
		"   }\n"
		"}\n";


	GLint success;
	GLuint vertexShader = glCreateShader(GL_VERTEX_SHADER);
	glShaderSource(vertexShader, 1, &vertexShaderSource, NULL);
	glCompileShader(vertexShader);
	glGetShaderiv(vertexShader, GL_COMPILE_STATUS, &success);
	if (!success)
	{
		GLchar log[512];
		glGetShaderInfoLog(vertexShader, 512, NULL, log);
		std::cout << "Failed to compile vertex shader:\n" << log << std::endl;
		return 0;
	}
	GLuint fragmentShader = glCreateShader(GL_FRAGMENT_SHADER);
	glShaderSource(fragmentShader, 1, &fragmentShaderSource, NULL);
	glCompileShader(fragmentShader);
	glGetShaderiv(fragmentShader, GL_COMPILE_STATUS, &success);
	if (!success)
	{
		GLchar log[512];
		glGetShaderInfoLog(fragmentShader, 512, NULL, log);
		std::cout << "Failed to compile fragment shader:\n" << log << std::endl;
		return 0;
	}
	GLuint shaderProgram = glCreateProgram();
	glAttachShader(shaderProgram, vertexShader);
	glAttachShader(shaderProgram, fragmentShader);
	glLinkProgram(shaderProgram);
	// Check for linking errors
	glGetProgramiv(shaderProgram, GL_LINK_STATUS, &success);
	if (!success)
	{
		GLchar log[512];
		glGetProgramInfoLog(shaderProgram, 512, NULL, log);
		std::cout << "Failed to link program\n" << log << std::endl;
		return 0;
	}
	glDeleteShader(vertexShader);
	glDeleteShader(fragmentShader);
	return shaderProgram;
}

class HookMsgWindow
{
private:
	inline static int left_bound;
	inline static int top_bound;
	inline static int right_bound;
	inline static int bottom_bound;
	inline static LRESULT WINAPI Wndproc(
		_In_ HWND hWnd,
		_In_ UINT msg,
		_In_ WPARAM wParam,
		_In_ LPARAM lParam
	)
	{
		try {
			if (msg == msgid) {
				RECT rect;
				GetWindowRect((HWND)wParam, &rect);
				RECT scaled_rect;//resize to 2x
				scaled_rect.left = (3 * rect.left - rect.right) / 2;
				scaled_rect.right = (3 * rect.right - rect.left) / 2;
				scaled_rect.top = (3 * rect.top - rect.bottom) / 2;
				scaled_rect.bottom = (3 * rect.bottom - rect.top) / 2;
				std::cout << "rect:" << rect.left << "," << rect.top << "," << rect.right - rect.left << "," << rect.bottom - rect.top << std::endl;
				std::cout << "scaled_rect:" << scaled_rect.left << "," << scaled_rect.top << "," << scaled_rect.right - scaled_rect.left << "," << scaled_rect.bottom - scaled_rect.top << std::endl;
				RECT clipped_rect;
				memcpy(&clipped_rect, &scaled_rect, sizeof(RECT));
				if (clipped_rect.left < left_bound)
					clipped_rect.left = left_bound;
				if (clipped_rect.top < top_bound)
					clipped_rect.top = top_bound;
				if (clipped_rect.right >= right_bound)
					clipped_rect.right = right_bound - 1;
				if (clipped_rect.bottom >= bottom_bound)
					clipped_rect.bottom = bottom_bound - 1;
				std::cout << "clipped_rect:" << clipped_rect.left << "," << clipped_rect.top << "," << clipped_rect.right - clipped_rect.left << "," << clipped_rect.bottom - clipped_rect.top << std::endl;
				GLFWwindow* window = glfwCreateWindow(clipped_rect.right - clipped_rect.left, clipped_rect.bottom - clipped_rect.top, "ClosingWindow", nullptr, nullptr);
				if (window == nullptr) throw std::string("failed to create glfw3 window");
				BOOST_SCOPE_EXIT(window) {
					glfwDestroyWindow(window);
				}BOOST_SCOPE_EXIT_END
				HWND hwn = glfwGetWin32Window(window);
				DWORD style = GetWindowLong(hwn, GWL_EXSTYLE);
				SetWindowLong(hwn, GWL_EXSTYLE, (style & ~WS_EX_APPWINDOW) | WS_EX_TOOLWINDOW);
				glfwSetWindowPos(window, clipped_rect.left, clipped_rect.top);
				glfwMakeContextCurrent(window);
				GetWindowRect(hwn, &clipped_rect);

				glViewport(
					scaled_rect.left - clipped_rect.left,
					clipped_rect.bottom - scaled_rect.bottom,
					scaled_rect.right - scaled_rect.left,
					scaled_rect.bottom - scaled_rect.top
				);
				shaders_program = compile_shaders();
				if (shaders_program == 0) throw std::string("cannot link shaders");
				BOOST_SCOPE_EXIT(shaders_program) 
				{
					glDeleteProgram(shaders_program);
				}BOOST_SCOPE_EXIT_END
				glUseProgram(shaders_program);

				glGenVertexArrays(1, &VAO);
				glBindVertexArray(VAO);
				glGenBuffers(1, &VBO);
				glBindBuffer(GL_ARRAY_BUFFER, VBO);
				glBufferData(GL_ARRAY_BUFFER, sizeof(vertices), vertices, GL_STATIC_DRAW);
				GLint input_pos = glGetAttribLocation(shaders_program, "input_pos");
				glVertexAttribPointer(input_pos, 2, GL_FLOAT, GL_FALSE, 0, 0);
				glEnableVertexAttribArray(input_pos);

				glBindBuffer(GL_ARRAY_BUFFER, 0);
				glBindVertexArray(0);


				glGenTextures(1, &shard_texture);
				BOOST_SCOPE_EXIT(&shard_texture)
				{
					glDeleteTextures(1, &shard_texture);
				}BOOST_SCOPE_EXIT_END
				int width, height, nrChannels;
				//unsigned char* data2 = stbi_load("F:\\shards.bmp", &width, &height, &nrChannels, 0);
				unsigned char* data2 = stbi_load_from_memory(shards_png, shards_png_len, &width, &height, &nrChannels, 0);
				glBindTexture(GL_TEXTURE_2D, shard_texture);
				glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, width, height, 0, GL_RGB, GL_UNSIGNED_BYTE, data2);
				glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
				//glGenerateMipmap(GL_TEXTURE_2D);
				glTextureParameteri(shard_texture, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_BORDER);
				glTextureParameteri(shard_texture, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_BORDER);
				glTextureParameteri(shard_texture, GL_TEXTURE_WRAP_R, GL_CLAMP_TO_BORDER);

				GLuint captured_texture = CaptureWindow::Capture((HWND)wParam);
				BOOST_SCOPE_EXIT(&captured_texture)
				{
					glDeleteTextures(1, &captured_texture);
				}BOOST_SCOPE_EXIT_END
				ReplyMessage(0);
				
				glBindTexture(GL_TEXTURE_2D, captured_texture);
				glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
				//glGenerateMipmap(GL_TEXTURE_2D);
				glTextureParameteri(captured_texture, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_BORDER);
				glTextureParameteri(captured_texture, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_BORDER);
				glTextureParameteri(captured_texture, GL_TEXTURE_WRAP_R, GL_CLAMP_TO_BORDER);


				GLint texture_location = glGetUniformLocation(shaders_program, "uTexture");
				glUniform1i(texture_location, 0);

				GLint texture2_location = glGetUniformLocation(shaders_program, "uShardTexture");
				glUniform1i(texture2_location, 1);

				GLint m_width = glGetUniformLocation(shaders_program, "uSizeX");
				GLint m_height = glGetUniformLocation(shaders_program, "uSizeY");
				glUniform1i(m_width, 300);
				glUniform1i(m_height, 300);

				GLint m_progress = glGetUniformLocation(shaders_program, "uProgress");
				glUniform1f(m_progress, 0.0f);

				glEnable(GL_BLEND);
				glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
				glUseProgram(0);

				double timestamp = glfwGetTime();

				while (!glfwWindowShouldClose(window))
				{
					glfwPollEvents();

					glClearColor(0.0f, 0.0f, 0.0f, 0.0f);
					glClear(GL_COLOR_BUFFER_BIT);
					glUseProgram(shaders_program);

					auto current_ts = glfwGetTime();
					float progress = (float)(current_ts - timestamp) / 1.0;
					glUniform1f(m_progress, progress);
					if (current_ts - timestamp > 1.0) {
						glfwSetWindowShouldClose(window, 1);
						continue;
					}
					// Draw our first triangle

					glBindVertexArray(VAO);
					glDrawArrays(GL_TRIANGLE_FAN, 0, 4);
					glBindVertexArray(0);

					glActiveTexture(GL_TEXTURE0); // activate the texture unit first before binding texture
					glBindTexture(GL_TEXTURE_2D, captured_texture);
					glActiveTexture(GL_TEXTURE1); // activate the texture unit first before binding texture
					glBindTexture(GL_TEXTURE_2D, shard_texture);
					glfwSwapBuffers(window);
					glUseProgram(0);
				}
				
			}
		}
		catch (const std::string& err) {
			std::cerr << err << std::endl;
		}
		catch (...) {
			std::cerr << "Some error occurred" << std::endl;
		}
		return DefWindowProc(hWnd, msg, wParam, lParam);
	}
public:
	inline static UINT msgid;
	inline static HWND hwnd_msg;
	inline static GLuint shaders_program;
	inline static GLFWwindow* context_window;
	inline static GLuint shard_texture;
	inline static GLfloat vertices[] = {
		-1.0f, -1.0f,
		-1.0f,  1.0f,
		 1.0f,  1.0f,
		 1.0f, -1.0f
	};
	inline static GLuint VBO, VAO;

	inline HookMsgWindow() {
		WNDCLASSEX window_class;
		window_class.cbSize = sizeof(window_class);
		window_class.style = 0;
		window_class.lpfnWndProc = Wndproc;
		window_class.cbClsExtra = 0;
		window_class.cbWndExtra = 0;
		window_class.hInstance = GetModuleHandle(NULL);
		window_class.hIcon = NULL;
		window_class.hCursor = NULL;
		window_class.hbrBackground = NULL;
		window_class.lpszMenuName = NULL;
		window_class.lpszClassName = TEXT("MsgWindow_For_Closing_Effect");
		window_class.hIconSm = NULL;
		ATOM atom = RegisterClassEx(&window_class);
		if (atom == 0) {
			throw(std::string("Failed to register the window class for a message-only window"));
		}

		hwnd_msg = CreateWindowEx(NULL, MAKEINTATOM(atom), TEXT("MsgWindow"), 0, 0, 0, 0, 0, HWND_MESSAGE, 0, GetModuleHandle(NULL), NULL);
		if (hwnd_msg == NULL)
			throw(std::string("Failed to create the message-only window"));

		msgid = RegisterWindowMessageA("WindowClosingHookMsg");
		if (msgid == 0)
			throw(std::string("Failed to register a message"));

		CaptureWindow::InitCaptureWindow();

		glfwInit();
		glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
		glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
		glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
		glfwWindowHint(GLFW_RESIZABLE, GL_FALSE);
		glfwWindowHint(GLFW_DECORATED, GL_FALSE);
		glfwWindowHint(GLFW_FLOATING, GL_TRUE);//create an always-on-top window
		glfwWindowHint(GLFW_TRANSPARENT_FRAMEBUFFER, GL_TRUE);

		context_window = glfwCreateWindow(800, 600, "ClosingWindowInitializer", nullptr, nullptr);
		if (context_window == nullptr) throw std::string("failed to create glfw3 window");
		glfwMakeContextCurrent(context_window);
		glewExperimental = GL_FALSE;
		if (glewInit() != GLEW_OK)throw std::string("failed to initialize glew");
		glfwHideWindow(context_window);

		left_bound = GetSystemMetrics(SM_XVIRTUALSCREEN);
		top_bound = GetSystemMetrics(SM_YVIRTUALSCREEN);
		right_bound = left_bound + GetSystemMetrics(SM_CXVIRTUALSCREEN);
		bottom_bound = top_bound + GetSystemMetrics(SM_CYVIRTUALSCREEN);

	}
};

