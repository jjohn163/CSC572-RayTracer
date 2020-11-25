/*
CPE/CSC 471 Lab base code Wood/Dunn/Eckhardt
*/

#include <iostream>
#include <time.h>
#include <glad/glad.h>
#include <GLFW/glfw3.h>

#include "GLSL.h"
#include "Program.h"
#include "MatrixStack.h"

#include "WindowManager.h"
#include "Shape.h"
// value_ptr for glm
#include <glm/gtc/type_ptr.hpp>
#include <glm/gtc/matrix_transform.hpp>
using namespace std;
using namespace glm;

#define RESX 640.0
#define RESY 480.0
#define EPS 0.001
#define BUFFERSIZE 3
#define RAND_BUFFERSIZE 10000139

#define SPEED 0.1

shared_ptr<Shape> shape;
std::vector<float> bufInstance(RESX * RESY * 4);
std::vector<unsigned char> buffer(RESX * RESY * 4);
std::vector<unsigned char> buffer2(RESX * RESY * 4);

GLuint computeRayTracingProgram, computeFilteringProgram;
int mode = 0;


 
class ssbo_data
{
public:
	vec4 pos[BUFFERSIZE];
	vec4 col[BUFFERSIZE];
	float rand_buffer[RAND_BUFFERSIZE];
};

double get_last_elapsed_time()
{
	static double lasttime = glfwGetTime();
	double actualtime =glfwGetTime();
	double difference = actualtime- lasttime;
	lasttime = actualtime;
	return difference;
}


class Application : public EventCallbacks
{

public:

	WindowManager * windowManager = nullptr;

	// Our shader program
	std::shared_ptr<Program> postproc;

	// Contains vertex information for OpenGL
	GLuint VertexArrayID, VertexArrayIDScreen;

	// Data necessary to give our box to OpenGL
	GLuint VertexBufferID, VertexBufferTexScreen, VertexBufferIDScreen,VertexNormDBox, VertexTexBox, IndexBufferIDBox, InstanceBuffer;

	//framebufferstuff
	GLuint fb, depth_fb, FBOtex;
	//texture data
	GLuint Texture, Texture2;
	GLuint CS_tex_A, CS_tex_B, CS_tex_C;

	ssbo_data ssbo_CPUMEM;
	GLuint ssbo_GPU_id;

	//camera variables
	vec3 location{ 0, 0, 10 };
	vec3 up{ 0, 1, 0 };
	vec3 right{ 1.33333, 0, 0 };
	vec3 look_at{ 0, 0, 0 };
	vec3 horizontal, vertical, LLC;

	int tex_w, tex_h;
	bool movingForward = false;
	bool movingBackward = false;
	bool movingLeft = false;
	bool movingRight = false;

	void keyCallback(GLFWwindow *window, int key, int scancode, int action, int mods)
	{
		if (key == GLFW_KEY_ESCAPE && action == GLFW_PRESS)
		{
			glfwSetWindowShouldClose(window, GL_TRUE);
		}
		if (key == GLFW_KEY_W && action == GLFW_PRESS) {
			movingForward = true;
		}
		if (key == GLFW_KEY_W && action == GLFW_RELEASE) {
			movingForward = false;
		}
		if (key == GLFW_KEY_S && action == GLFW_PRESS) {
			movingBackward = true;
		}
		if (key == GLFW_KEY_S && action == GLFW_RELEASE) {
			movingBackward = false;
		}
		if (key == GLFW_KEY_A && action == GLFW_PRESS) {
			movingLeft = true;
		}
		if (key == GLFW_KEY_A && action == GLFW_RELEASE) {
			movingLeft = false;
		}
		if (key == GLFW_KEY_D && action == GLFW_PRESS) {
			movingRight = true;
		}
		if (key == GLFW_KEY_D && action == GLFW_RELEASE) {
			movingRight = false;
		}
		if (key == GLFW_KEY_M && action == GLFW_PRESS) {
			mode = (mode + 1) % 4;
		}
		
	}

	void mouseCallback(GLFWwindow *window, int button, int action, int mods)
	{
		return;
	}

	//if the window is resized, capture the new size and reset the viewport
	void resizeCallback(GLFWwindow *window, int in_width, int in_height)
	{
		//get the window size - may be different then pixels for retina
		int width, height;
		glfwGetFramebufferSize(window, &width, &height);
		glViewport(0, 0, width, height);
	}

	/*Note that any gl calls must always happen after a GL state is initialized */
	void initGeom()
	{

		string resourceDirectory = "../resources";
		
		glGenBuffers(1, &ssbo_GPU_id);
		glBindBuffer(GL_SHADER_STORAGE_BUFFER, ssbo_GPU_id);
		glBufferData(GL_SHADER_STORAGE_BUFFER, sizeof(ssbo_data), &ssbo_CPUMEM, GL_DYNAMIC_COPY);
		glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 3, ssbo_GPU_id);
		glBindBuffer(GL_SHADER_STORAGE_BUFFER, 0); // unbind
		
		//screen plane
		glGenVertexArrays(1, &VertexArrayIDScreen);
		glBindVertexArray(VertexArrayIDScreen);
		//generate vertex buffer to hand off to OGL
		glGenBuffers(1, &VertexBufferIDScreen);
		//set the current state to focus on our vertex buffer
		glBindBuffer(GL_ARRAY_BUFFER, VertexBufferIDScreen);
		vec3 vertices[6];
		vertices[0] = vec3(-1,-1,0);
		vertices[1] = vec3(1, -1, 0);
		vertices[2] = vec3(1, 1, 0);
		vertices[3] = vec3(-1, -1, 0);
		vertices[4] = vec3(1, 1, 0);
		vertices[5] = vec3(-1, 1, 0);
		//actually memcopy the data - only do this once
		glBufferData(GL_ARRAY_BUFFER, 6 * sizeof(vec3), vertices, GL_STATIC_DRAW);
		//we need to set up the vertex array
		glEnableVertexAttribArray(0);
		//key function to get up how many elements to pull out at a time (3)
		glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 0, (void*)0);
		//generate vertex buffer to hand off to OGL
		glGenBuffers(1, &VertexBufferTexScreen);
		//set the current state to focus on our vertex buffer
		glBindBuffer(GL_ARRAY_BUFFER, VertexBufferTexScreen);
		vec2 texscreen[6];
		texscreen[0] = vec2(0, 0);
		texscreen[1] = vec2(1, 0);
		texscreen[2] = vec2(1, 1);
		texscreen[3] = vec2(0, 0);
		texscreen[4] = vec2(1, 1);
		texscreen[5] = vec2(0, 1);
		//actually memcopy the data - only do this once
		glBufferData(GL_ARRAY_BUFFER, 6 * sizeof(vec2), texscreen, GL_STATIC_DRAW);
		//we need to set up the vertex array
		glEnableVertexAttribArray(1);
		//key function to get up how many elements to pull out at a time (3)
		glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 0, (void*)0);
		glBindVertexArray(0);
		


		

	

		int width, height, channels;
		char filepath[1000];

		//texture 1
	
		//[TWOTEXTURES]
		//set the 2 textures to the correct samplers in the fragment shader:
		GLuint Tex1Location;

		Tex1Location = glGetUniformLocation(postproc->pid, "tex");//tex, tex2... sampler in the fragment shader
		glUseProgram(postproc->pid);
		glUniform1i(Tex1Location, 0);


		glfwGetFramebufferSize(windowManager->getHandle(), &width, &height);
		//RGBA8 2D texture, 24 bit depth texture, 256x256
		//-------------------------
		//Does the GPU support current FBO configuration?
		GLenum status;
		status = glCheckFramebufferStatus(GL_FRAMEBUFFER);
		switch (status)
		{
		case GL_FRAMEBUFFER_COMPLETE:
			cout << "status framebuffer: good";
			break;
		default:
			cout << "status framebuffer: bad!!!!!!!!!!!!!!!!!!!!!!!!!";
		}
		glBindFramebuffer(GL_FRAMEBUFFER, 0);

		//make a texture (buffer) on the GPU to store the input image
		tex_w = RESX, tex_h = RESY;		//size
		glGenTextures(1, &CS_tex_A);		//Generate texture and store context number
		glActiveTexture(GL_TEXTURE0);		//since we have 2 textures in this program, we need to associate the input texture with "0" meaning first texture
		glBindTexture(GL_TEXTURE_2D, CS_tex_A);	//highlight input texture
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);	//texture sampler parameter
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);	//texture sampler parameter
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);		//texture sampler parameter
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);		//texture sampler parameter
		glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA32F, tex_w, tex_h, 0, GL_RGBA, GL_FLOAT, NULL);	//copy image data to texture
		glBindImageTexture(0, CS_tex_A, 0, GL_FALSE, 0, GL_READ_WRITE, GL_RGBA32F);	//enable texture in shader

		//make a texture (buffer) on the GPU to store the output image
		glGenTextures(1, &CS_tex_B);
		glActiveTexture(GL_TEXTURE1);
		glBindTexture(GL_TEXTURE_2D, CS_tex_B);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
		glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA32F, tex_w, tex_h, 0, GL_RGBA, GL_FLOAT, NULL);
		glBindImageTexture(1, CS_tex_B, 0, GL_FALSE, 0, GL_READ_WRITE, GL_RGBA32F);

		glGenTextures(1, &CS_tex_C);
		glActiveTexture(GL_TEXTURE2);
		glBindTexture(GL_TEXTURE_2D, CS_tex_C);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
		glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA32F, tex_w, tex_h, 0, GL_RGBA, GL_FLOAT, NULL);
		glBindImageTexture(2, CS_tex_C, 0, GL_FALSE, 0, GL_READ_WRITE, GL_RGBA32F);
	}
	void setupScene() {
		ssbo_CPUMEM.pos[0] = vec4(0.9, -0.5, -2, 1);
		ssbo_CPUMEM.col[0] = vec4(0.5, 0.5, 0.5, 0.1);
		ssbo_CPUMEM.pos[1] = vec4(-3, 0.5, 1, 2);
		ssbo_CPUMEM.col[1] = vec4(1, 0.3, 0.0, 1);
		ssbo_CPUMEM.pos[2] = vec4(7, 3, -5, 5);
		ssbo_CPUMEM.col[2] = vec4(0, 0.3, 1, 0.8);
		srand((unsigned)time(NULL));
		for (int i = 0; i < RAND_BUFFERSIZE; i++) {
			ssbo_CPUMEM.rand_buffer[i] = ((float)rand() / RAND_MAX) * 2.0f - 1.0f;
		}
	}

	//General OGL initialization - set OGL state here
	void init(const std::string& resourceDirectory)
	{
		
		GLSL::checkVersion();
		setupScene();
		// Set background color.
		glClearColor(0.1f, 0.1f, 0.1f, 1.0f);
		// Enable z-buffer test.
		glEnable(GL_DEPTH_TEST);

		// Initialize the GLSL program.
		

		//program for the postprocessing
		postproc = std::make_shared<Program>();
		postproc->setVerbose(true);
		postproc->setShaderNames(resourceDirectory + "/postproc_vertex.glsl", resourceDirectory + "/postproc_fragment.glsl");
		if (!postproc->init())
		{
			std::cerr << "One or more shaders failed to compile... exiting!" << std::endl;
			exit(1);
		}
		postproc->addAttribute("vertPos");
		postproc->addAttribute("vertTex");

		//load the compute shader
		std::string ShaderString = readFileAsString("../resources/compute_ray_tracing.glsl");
		const char* shader = ShaderString.c_str();
		GLuint computeRayTracing = glCreateShader(GL_COMPUTE_SHADER);
		glShaderSource(computeRayTracing, 1, &shader, nullptr);
		GLint rc;
		CHECKED_GL_CALL(glCompileShader(computeRayTracing));
		CHECKED_GL_CALL(glGetShaderiv(computeRayTracing, GL_COMPILE_STATUS, &rc));
		if (!rc)	//error compiling the shader file
		{
			GLSL::printShaderInfoLog(computeRayTracing);
			std::cout << "Error compiling fragment shader " << std::endl;
			exit(1);
		}
		computeRayTracingProgram = glCreateProgram();
		glAttachShader(computeRayTracingProgram, computeRayTracing);
		glLinkProgram(computeRayTracingProgram);
		glUseProgram(computeRayTracingProgram);

		//load the vorticity shader
		std::string FilterString = readFileAsString("../resources/compute_filtering.glsl");
		const char* shader_filter = FilterString.c_str();
		GLuint computeFiltering = glCreateShader(GL_COMPUTE_SHADER);
		glShaderSource(computeFiltering, 1, &shader_filter, nullptr);
		GLint rcv;
		CHECKED_GL_CALL(glCompileShader(computeFiltering));
		CHECKED_GL_CALL(glGetShaderiv(computeFiltering, GL_COMPILE_STATUS, &rcv));
		if (!rc)	//error compiling the shader file
		{
			GLSL::printShaderInfoLog(computeFiltering);
			std::cout << "Error compiling vorticity shader " << std::endl;
			exit(1);
		}
		computeFilteringProgram = glCreateProgram();
		glAttachShader(computeFilteringProgram, computeFiltering);
		glLinkProgram(computeFilteringProgram);
		glUseProgram(computeFilteringProgram);

	}


	/****DRAW
	This is the most important function in your program - this is where you
	will actually issue the commands to draw any geometry you have set up to
	draw
	********/

	int compute(int printframes) 
		{
			static bool flap = 1;
			if (printframes == 0) {
				glBindBuffer(GL_SHADER_STORAGE_BUFFER, ssbo_GPU_id);
				GLvoid* p = glMapBuffer(GL_SHADER_STORAGE_BUFFER, GL_READ_WRITE);
				int siz = sizeof(ssbo_data);
				memcpy(p, &ssbo_CPUMEM, siz);
				glUnmapBuffer(GL_SHADER_STORAGE_BUFFER);
			}
			
			

			glUseProgram(computeRayTracingProgram);
			GLint loc = glGetUniformLocation(computeRayTracingProgram, "horizontal");
			glUniform3f(loc, horizontal.x, horizontal.y, horizontal.z);
			loc = glGetUniformLocation(computeRayTracingProgram, "vertical");
			glUniform3f(loc, vertical.x, vertical.y, vertical.z);
			loc = glGetUniformLocation(computeRayTracingProgram, "LLC");
			glUniform3f(loc, LLC.x, LLC.y, LLC.z);
			loc = glGetUniformLocation(computeRayTracingProgram, "eye");
			glUniform3f(loc, location.x, location.y, location.z);
			loc = glGetUniformLocation(computeRayTracingProgram, "time");
			glUniform1f(loc, printframes);
			glDispatchCompute((GLuint)tex_w, (GLuint)tex_h, 1);
			glMemoryBarrier(GL_SHADER_IMAGE_ACCESS_BARRIER_BIT);
			glBindImageTexture(!flap, CS_tex_A, 0, GL_FALSE, 0, GL_READ_WRITE, GL_RGBA32F);
			glBindImageTexture(flap, CS_tex_B, 0, GL_FALSE, 0, GL_READ_WRITE, GL_RGBA32F);
			glBindImageTexture(2, CS_tex_C, 0, GL_FALSE, 0, GL_READ_WRITE, GL_RGBA32F);
			

			glUseProgram(computeFilteringProgram);
			loc = glGetUniformLocation(computeFilteringProgram, "mode");
			glUniform1i(loc, mode);
			glDispatchCompute((GLuint)tex_w, (GLuint)tex_h, 1);
			glMemoryBarrier(GL_SHADER_IMAGE_ACCESS_BARRIER_BIT);
			glBindImageTexture(!flap, CS_tex_B, 0, GL_FALSE, 0, GL_READ_WRITE, GL_RGBA32F);
			glBindImageTexture(flap, CS_tex_A, 0, GL_FALSE, 0, GL_READ_WRITE, GL_RGBA32F);
			glBindImageTexture(2, CS_tex_C, 0, GL_FALSE, 0, GL_READ_WRITE, GL_RGBA32F);

			return flap;
	}
	//*****************************************************************************************

	void updateCam() {

		if (movingForward) {
			location.z -= SPEED;
			look_at.z -= SPEED;
		}
		if (movingBackward) {
			location.z += SPEED;
			look_at.z += SPEED;
		}
		if (movingLeft) {
			location.x -= SPEED;
			look_at.x -= SPEED;
		}
		if (movingRight) {
			location.x += SPEED;
			look_at.x += SPEED;
		}
		vec3 w = location - look_at;
		w = normalize(w);
		vec3 u = cross(up, w);
		vec3 v = cross(w, u);
		horizontal = u * length(right);
		vertical = v * length(up);
		LLC = location - (horizontal * 0.5f) - (vertical * 0.5f) - w;
	}
	void render(int texnum)
	{
		updateCam();
		// Get current frame buffer size.
		int width, height;
		glfwGetFramebufferSize(windowManager->getHandle(), &width, &height);
		glViewport(0, 0, width, height);
		// Clear framebuffer.
		glClearColor(1.0f, 0.1f, 0.1f, 1.0f);
		glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
		postproc->bind();
		glActiveTexture(GL_TEXTURE0);
		if(texnum==0)
			glBindTexture(GL_TEXTURE_2D, CS_tex_B);
		else
			glBindTexture(GL_TEXTURE_2D, CS_tex_A);

		glBindVertexArray(VertexArrayIDScreen);
		glDrawArrays(GL_TRIANGLES, 0, 6);
		postproc->unbind();
	}
};
//******************************************************************************************
int main(int argc, char **argv)
{
	//initialize Open GL
	glfwInit();
	//make a window
	GLFWwindow* window = glfwCreateWindow(512, 512, "Dummy", nullptr, nullptr);
	glfwMakeContextCurrent(window);
	//initialize Open GL Loader function
	gladLoadGL();

	std::string resourceDir = "../resources"; // Where the resources are loaded from
	if (argc >= 2)
	{
		resourceDir = argv[1];
	}

	Application* application = new Application();
	/* your main will always include a similar set up to establish your window
	and GL context, etc. */
	WindowManager* windowManager = new WindowManager();
	windowManager->init(RESX, RESY);
	windowManager->setEventCallbacks(application);
	application->windowManager = windowManager;

	/* This is the code that will likely change program to program as you
		may need to initialize or set up different data and state */
		// Initialize scene.
	application->init(resourceDir);
	application->initGeom();

	glUseProgram(computeRayTracingProgram);
	glUseProgram(computeFilteringProgram);

	// Loop until the user closes the window.
	double timef = 0;
	int printframes = 0;
	while(! glfwWindowShouldClose(windowManager->getHandle()))
	{

		int ret = application->compute(printframes++);

		application->render(ret);

		// Swap front and back buffers.
		glfwSwapBuffers(windowManager->getHandle());
		// Poll for and process events.
		glfwPollEvents();
	}

	// Quit program.
	windowManager->shutdown();
	return 0;
}
