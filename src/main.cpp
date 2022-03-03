
#include <GL/glew.h>
#include <GLFW/glfw3.h>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <imgui.h>
#include <imgui_impl_glfw.h>
#include <imgui_impl_opengl3.h>
#include <chrono>
#include <thread>
#include <iostream>
#include <vector>

#include "shader.hpp"
#include "camera.hpp"
#include "sandpile.hpp"
#include "stb_image.h"

//screen dimensions
const int SCREEN_WIDTH = 1200;
const int SCREEN_HEIGHT = 675;

//initial camera settings
Camera camera(glm::vec3(-6.5f, 10.0f, -6.5f), 45.0f, 0.0f);

//directional lighting
const glm::vec3 lightDir(1.2f, 2.5f, 2.0f);

float lastX = SCREEN_WIDTH / 2.0;
float lastY = SCREEN_HEIGHT / 2.0;
bool mouseMoved = false;

//GUI info
bool pause = true;
bool mouseFocused = true;
bool highlight = false;
unsigned int playTexture;
unsigned int pauseTexture;

unsigned int cubeVAO = 0;
unsigned int cubeVBO = 0;
unsigned int plateVAO;

//animation info for render function, 1 is no animation
int animationFrames = 10;
std::vector<std::vector<int>> plateImage;
int currentFrame = 0;
const int maxFPS = 60;
const int msPerFrame = (int) (((double) 1 / (double) maxFPS) * 1000);

//temp variables for GUI to store reference to
int plateWidth, plateHeight;

//store capacity history data
std::vector<float> capacityData;

void framebuffer_size_callback(GLFWwindow *window, int width, int height);
void mouse_callback(GLFWwindow *window, double xpos, double ypos);
void key_callback(GLFWwindow* window, int key, int scancode, int action, int mods);
void processInput(GLFWwindow *window, double deltaTime);
void renderCube();
void renderScene(const Shader &shader, const Sandpile &pile);
void renderGUI(Sandpile &pile);

int main()
{
	//initialize GLFW
	glfwInit();
	glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
	glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
	glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);

	//make window
	GLFWwindow *window = glfwCreateWindow(SCREEN_WIDTH, SCREEN_HEIGHT, "Sandpile", nullptr, nullptr);
	glfwMakeContextCurrent(window);
	glfwSetFramebufferSizeCallback(window, framebuffer_size_callback);
	glfwSetCursorPosCallback(window, mouse_callback);
	glfwSetKeyCallback(window, key_callback);
	glfwSwapInterval(0);

	glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_DISABLED);

	//initialize GLEW
	glewExperimental = GL_TRUE;
	glewInit();

	glEnable(GL_DEPTH_TEST);

	//compile shaders
	Shader lightingShader("cubeShader.vert", "cubeShader.frag");
	Shader simpleDepthShader("depthShader.vert", "depthShader.frag");

	//define plate vertices
	float plateVertices[] = {
		19.5f, -0.5f,  19.5f,  0.0f, 1.0f, 0.0f,
		-0.5f, -0.5f,  19.5f,  0.0f, 1.0f, 0.0f,
		-0.5f, -0.5f, -0.5f,  0.0f, 1.0f, 0.0f,

		19.5f, -0.5f,  19.5f,  0.0f, 1.0f, 0.0f,
		-0.5f, -0.5f, -0.5f,  0.0f, 1.0f, 0.0f,
		19.5f, -0.5f, -0.5f,  0.0f, 1.0f, 0.0f,
	};

	//set up plate VAO
	unsigned int plateVBO;
	glGenVertexArrays(1, &plateVAO);
	glGenBuffers(1, &plateVBO);
	glBindVertexArray(plateVAO);
	glBindBuffer(GL_ARRAY_BUFFER, plateVBO);
	glBufferData(GL_ARRAY_BUFFER, sizeof(plateVertices), plateVertices, GL_STATIC_DRAW);
	glEnableVertexAttribArray(0);
	glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 6 * sizeof(float), (void*) 0);
	glEnableVertexAttribArray(1);
	glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 6 * sizeof(float), (void*) (3 * sizeof(float)));
	glBindVertexArray(0);

	//set up depth texture
	const unsigned int SHADOW_WIDTH = 1024, SHADOW_HEIGHT = 1024;
	unsigned int depthMapFBO;
	glGenFramebuffers(1, &depthMapFBO);

	unsigned int depthMap;
	glGenTextures(1, &depthMap);
	glBindTexture(GL_TEXTURE_2D, depthMap);
	glTexImage2D(GL_TEXTURE_2D, 0, GL_DEPTH_COMPONENT, SHADOW_WIDTH, SHADOW_HEIGHT, 0, GL_DEPTH_COMPONENT, GL_FLOAT, nullptr);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_BORDER);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_BORDER);
	float borderColor[] = {1.0f, 1.0f, 1.0f, 1.0f};
	glTexParameterfv(GL_TEXTURE_2D, GL_TEXTURE_BORDER_COLOR, borderColor);

	//attach depth texture as FBO's depth buffer
	glBindFramebuffer(GL_FRAMEBUFFER, depthMapFBO);
	glFramebufferTexture2D(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_TEXTURE_2D, depthMap, 0);
	glDrawBuffer(GL_NONE);
	glReadBuffer(GL_NONE);
	glBindFramebuffer(GL_FRAMEBUFFER, 0);

	//light space calculations
	glm::mat4 lightProjection, lightView;
	glm::mat4 lightSpaceMatrix;
	float near_plate = -10.5f, far_plate = 20.5f;
	lightProjection = glm::ortho(-20.0f, 20.0f, -10.0f, 10.0f, near_plate, far_plate);
	lightView = glm::lookAt(lightDir * 1.0f + glm::vec3(10, 0, 10), glm::vec3(0.0f) + glm::vec3(10, 0, 10), glm::vec3(0.0, 1.0, 0.0));
	lightSpaceMatrix = lightProjection * lightView;

	//set depth attributes
	simpleDepthShader.use();
	simpleDepthShader.setMat4(lightSpaceMatrix, "lightSpaceMatrix");

	//set light attributes
	lightingShader.use();
	lightingShader.setVec3(glm::vec3(0.2f, 0.2f, 0.2f), "light.ambient");
	lightingShader.setVec3(glm::vec3(0.5f, 0.5f, 0.5f), "light.diffuse");
	lightingShader.setVec3(glm::vec3(1.0f, 1.0f, 1.0f), "light.specular");
	lightingShader.setVec3(lightDir, "light.direction");
	lightingShader.setMat4(lightSpaceMatrix, "lightSpaceMatrix");
	lightingShader.setInt(1, "shadowMap");

	//configure camera values
	camera.moveSpeed = 10.0;

	//initialize image to nothing and sandpile object to random pile
	Sandpile pile(20, 20);
	pile.fillValue(0);
	plateImage = pile.plate;

	//create button textures
	glGenTextures(1, &playTexture);
	glBindTexture(GL_TEXTURE_2D, playTexture);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
	int width, height, nrChannels;
	unsigned char *playData = stbi_load("res/play.png", &width, &height, &nrChannels, 0);
	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, width, height, 0, GL_RGBA, GL_UNSIGNED_BYTE, playData);
	glGenerateMipmap(GL_TEXTURE_2D);
	stbi_image_free(playData);

	glGenTextures(1, &pauseTexture);
	glBindTexture(GL_TEXTURE_2D, pauseTexture);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
	unsigned char *pauseData = stbi_load("res/pause.png", &width, &height, &nrChannels, 0);
	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, width, height, 0, GL_RGBA, GL_UNSIGNED_BYTE, pauseData);
	glGenerateMipmap(GL_TEXTURE_2D);
	stbi_image_free(pauseData);

	//init imgui
	IMGUI_CHECKVERSION();
	ImGui::CreateContext();
	ImGui_ImplGlfw_InitForOpenGL(window, true);
	ImGui_ImplOpenGL3_Init("#version 330 core");
	ImGui::StyleColorsDark();

	//set up per-frame logic
	double deltaTime = 0;
	double lastTime = glfwGetTime();
	currentFrame = 0;

	//main render loop
	while (!glfwWindowShouldClose(window)) {
		double startTime = glfwGetTime();
		deltaTime = startTime - lastTime;
		lastTime = startTime;

		//input
		processInput(window, deltaTime);

		//update animation
		if (!pause) {
			if (currentFrame == animationFrames - 1) {
				plateImage = pile.plate;
				pile.update();
				currentFrame = 0;
				capacityData.push_back((float) pile.capacity / (float) pile.width * pile.height * 4);
			} else {
				currentFrame++;
			}
		}

		//render scene from light's point of view
		simpleDepthShader.use();

		glViewport(0, 0, SHADOW_WIDTH, SHADOW_HEIGHT);
		glBindFramebuffer(GL_FRAMEBUFFER, depthMapFBO);
		glClear(GL_DEPTH_BUFFER_BIT);

		renderScene(simpleDepthShader, pile);

		//reset viewport
		glBindFramebuffer(GL_FRAMEBUFFER, 0);
		glViewport(0, 0, SCREEN_WIDTH, SCREEN_HEIGHT);
		glClearColor(0.1f, 0.1f, 0.1f, 1.0f);
		glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

		//render normally
		lightingShader.use();
		glm::mat4 projection = glm::perspective(glm::radians(45.0f), (float) SCREEN_WIDTH / (float) SCREEN_HEIGHT, 0.1f, 200.0f);
		glm::mat4 view = camera.getViewMatrix();
		lightingShader.setMat4(projection, "projection");
		lightingShader.setMat4(view, "view");
		lightingShader.setVec3(camera.pos, "viewPos");

		glActiveTexture(GL_TEXTURE1);
		glBindTexture(GL_TEXTURE_2D, depthMap);

		renderScene(lightingShader, pile);

		//render GUI
		renderGUI(pile);

		glfwPollEvents();

		double endTime = glfwGetTime();
		double renderTime = endTime - startTime;
		int delayTime = (msPerFrame - 1) - ((int) (renderTime * 1000));
		std::this_thread::sleep_for(std::chrono::milliseconds(delayTime));

		glfwSwapBuffers(window);
	}

	//clean up
	ImGui_ImplOpenGL3_Shutdown();
	ImGui_ImplGlfw_Shutdown();
	ImGui::DestroyContext();
	glDeleteVertexArrays(1, &cubeVAO);
	glDeleteVertexArrays(1, &plateVAO);
	glDeleteBuffers(1, &cubeVBO);
	glDeleteBuffers(1, &plateVBO);

	glfwTerminate();
	return 0;
}

void framebuffer_size_callback(GLFWwindow *window, int width, int height)
{
	glViewport(0, 0, width, height);
}

void mouse_callback(GLFWwindow *window, double xpos, double ypos)
{
	if (!mouseFocused) {
		return;
	}
	if (!mouseMoved) {
		lastX = xpos;
		lastY = ypos;
		mouseMoved = true;
	}
	float xoffset = xpos - lastX;
	float yoffset = lastY - ypos;
	lastX = xpos;
	lastY = ypos;
	camera.processMouseMovement(xoffset, yoffset);
}

void key_callback(GLFWwindow* window, int key, int scancode, int action, int mods)
{
	if (key == GLFW_KEY_LEFT_SHIFT && action == GLFW_PRESS) {
		mouseFocused = !mouseFocused;
		if (mouseFocused) {
			glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_DISABLED);
		} else {
			mouseMoved = false;
			glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_NORMAL);
		}
	}
	if (key == GLFW_KEY_P && action == GLFW_PRESS) {
		pause = !pause;
	}
	/*
	if (key == GLFW_KEY_G && action == GLFW_PRESS) {
		gui = !gui;
	}
	if (key == GLFW_KEY_K && action == GLFW_PRESS) {
		pause = true;
		next = true;
	}
	*/
}

void processInput(GLFWwindow *window, double deltaTime)
{
	if (glfwGetKey(window, GLFW_KEY_ESCAPE) == GLFW_PRESS)
		glfwSetWindowShouldClose(window, true);
	if (glfwGetKey(window, GLFW_KEY_W) == GLFW_PRESS)
		camera.processKeyboard(FORWARD, deltaTime);
	if (glfwGetKey(window, GLFW_KEY_S) == GLFW_PRESS)
		camera.processKeyboard(BACKWARD, deltaTime);
	if (glfwGetKey(window, GLFW_KEY_A) == GLFW_PRESS)
		camera.processKeyboard(LEFT, deltaTime);
	if (glfwGetKey(window, GLFW_KEY_D) == GLFW_PRESS)
		camera.processKeyboard(RIGHT, deltaTime);
	if (glfwGetKey(window, GLFW_KEY_SPACE) == GLFW_PRESS)
		camera.processKeyboard(UP, deltaTime);
	if (glfwGetKey(window, GLFW_KEY_LEFT_CONTROL) == GLFW_PRESS)
		camera.processKeyboard(DOWN, deltaTime);
	//prevent camera from moving below plate or too high
	if (camera.pos.y < 0)
		camera.pos.y = 0;
	if (camera.pos.y > 100)
		camera.pos.y = 100;
}

void renderCube()
{
	if (cubeVAO == 0) {
		float vertices[] = {
			-0.5f, -0.5f, -0.5f,  0.0f,  0.0f, -1.0f,
			0.5f, -0.5f, -0.5f,  0.0f,  0.0f, -1.0f,
			0.5f,  0.5f, -0.5f,  0.0f,  0.0f, -1.0f,
			0.5f,  0.5f, -0.5f,  0.0f,  0.0f, -1.0f,
			-0.5f,  0.5f, -0.5f,  0.0f,  0.0f, -1.0f,
			-0.5f, -0.5f, -0.5f,  0.0f,  0.0f, -1.0f,

			-0.5f, -0.5f,  0.5f,  0.0f,  0.0f,  1.0f,
			0.5f, -0.5f,  0.5f,  0.0f,  0.0f,  1.0f,
			0.5f,  0.5f,  0.5f,  0.0f,  0.0f,  1.0f,
			0.5f,  0.5f,  0.5f,  0.0f,  0.0f,  1.0f,
			-0.5f,  0.5f,  0.5f,  0.0f,  0.0f,  1.0f,
			-0.5f, -0.5f,  0.5f,  0.0f,  0.0f,  1.0f,

			-0.5f,  0.5f,  0.5f, -1.0f,  0.0f,  0.0f,
			-0.5f,  0.5f, -0.5f, -1.0f,  0.0f,  0.0f,
			-0.5f, -0.5f, -0.5f, -1.0f,  0.0f,  0.0f,
			-0.5f, -0.5f, -0.5f, -1.0f,  0.0f,  0.0f,
			-0.5f, -0.5f,  0.5f, -1.0f,  0.0f,  0.0f,
			-0.5f,  0.5f,  0.5f, -1.0f,  0.0f,  0.0f,

			0.5f,  0.5f,  0.5f,  1.0f,  0.0f,  0.0f,
			0.5f,  0.5f, -0.5f,  1.0f,  0.0f,  0.0f,
			0.5f, -0.5f, -0.5f,  1.0f,  0.0f,  0.0f,
			0.5f, -0.5f, -0.5f,  1.0f,  0.0f,  0.0f,
			0.5f, -0.5f,  0.5f,  1.0f,  0.0f,  0.0f,
			0.5f,  0.5f,  0.5f,  1.0f,  0.0f,  0.0f,

			-0.5f, -0.5f, -0.5f,  0.0f, -1.0f,  0.0f,
			0.5f, -0.5f, -0.5f,  0.0f, -1.0f,  0.0f,
			0.5f, -0.5f,  0.5f,  0.0f, -1.0f,  0.0f,
			0.5f, -0.5f,  0.5f,  0.0f, -1.0f,  0.0f,
			-0.5f, -0.5f,  0.5f,  0.0f, -1.0f,  0.0f,
			-0.5f, -0.5f, -0.5f,  0.0f, -1.0f,  0.0f,

			-0.5f,  0.5f, -0.5f,  0.0f,  1.0f,  0.0f,
			0.5f,  0.5f, -0.5f,  0.0f,  1.0f,  0.0f,
			0.5f,  0.5f,  0.5f,  0.0f,  1.0f,  0.0f,
			0.5f,  0.5f,  0.5f,  0.0f,  1.0f,  0.0f,
			-0.5f,  0.5f,  0.5f,  0.0f,  1.0f,  0.0f,
			-0.5f,  0.5f, -0.5f,  0.0f,  1.0f,  0.0f
		};
		glGenVertexArrays(1, &cubeVAO);
		glGenBuffers(1, &cubeVBO);

		glBindBuffer(GL_ARRAY_BUFFER, cubeVBO);
		glBufferData(GL_ARRAY_BUFFER, sizeof(vertices), vertices, GL_DYNAMIC_DRAW);

		glBindVertexArray(cubeVAO);
		glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 6 * sizeof(float), (void*) 0);
		glEnableVertexAttribArray(0);
		glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 6 * sizeof(float), (void*) (3 * sizeof(float)));
		glEnableVertexAttribArray(1);
		glBindBuffer(GL_ARRAY_BUFFER, 0);
	}
	glBindVertexArray(cubeVAO);
	glDrawArrays(GL_TRIANGLES, 0, 36);
	glBindVertexArray(0);
}

void renderScene(const Shader &shader, const Sandpile &pile)
{
	//set plate material attributes
	glm::mat4 model = glm::mat4(1.0f);
	shader.setMat4(model, "model");
	shader.setVec3(glm::vec3(0.5f, 0.5f, 0.5f), "material.ambient");
	shader.setVec3(glm::vec3(0.4f, 0.4f, 0.4f), "material.diffuse");
	shader.setVec3(glm::vec3(0.2f, 0.2f, 0.2f), "material.specular");
	shader.setFloat(10.0f, "material.shininess");

	//draw plate
	glBindVertexArray(plateVAO);
	glDrawArrays(GL_TRIANGLES, 0, 6);

	/*
	//set cube material attributes
	shader.setVec3(glm::vec3(1.0f, 1.0f, 1.0f), "material.ambient");
	shader.setVec3(glm::vec3(1.0f, 1.0f, 1.0f), "material.diffuse");
	shader.setVec3(glm::vec3(0.2f, 0.2f, 0.2f), "material.specular");
	*/
	shader.setFloat(10.0f, "material.shininess");


	//render cubes according to sandpile matrix
	for (int i = 0; i < pile.width; i++) {
		for (int j = 0; j < pile.height; j++) {
			int target = pile.plate[i][j];
			int prev = plateImage[i][j];
			//frame 0 is no progress, frame animationFrames - 1 is one from finishing the animation
			double progress = (double) currentFrame / (double) animationFrames;
			//provide a buffer underneath plate if sand is being added
			for (int k = std::min(0, prev - target); k < prev; k++) {
				model = glm::mat4(1.0f);
				model = glm::translate(model, glm::vec3(i, k + (target - prev) * progress, j));
				shader.setMat4(model, "model");
				if (highlight && prev > target) {
					//cyan
					shader.setVec3(glm::vec3(0.0f, 1.0f, 1.0f), "material.ambient");
					shader.setVec3(glm::vec3(0.0f, 1.0f, 1.0f), "material.diffuse");
					shader.setVec3(glm::vec3(0.0f, 0.2f, 0.2f), "material.specular");
				} else {
					//white
					shader.setVec3(glm::vec3(1.0f, 1.0f, 1.0f), "material.ambient");
					shader.setVec3(glm::vec3(1.0f, 1.0f, 1.0f), "material.diffuse");
					shader.setVec3(glm::vec3(0.2f, 0.2f, 0.2f), "material.specular");
				}
				renderCube();
			}
		}
	}
}

void renderGUI(Sandpile &pile)
{
	ImGui_ImplOpenGL3_NewFrame();
	ImGui_ImplGlfw_NewFrame();
	ImGui::NewFrame();

	ImGui::Begin("Controls", nullptr, ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoMove);

	if ((pause) ? ImGui::ImageButton((void*) (intptr_t) playTexture, ImVec2(32, 32)) : ImGui::ImageButton((void*) (intptr_t) pauseTexture, ImVec2(32, 32))) {
		pause = !pause;
	}
	
	ImGui::Checkbox("center", &pile.center);
	ImGui::Checkbox("highlight", &highlight);

	if (ImGui::Button("Clear"))
		pile.fillValue(0);
	ImGui::SameLine();
	if (ImGui::Button("Randomize"))
		pile.fillRand();

	ImGui::End();

	ImGui::Render();
	ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());
}