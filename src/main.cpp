
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
#include <string>
#include <iostream>
#include <vector>
#include <map>

#include "shader.hpp"
#include "camera.hpp"
#include "sandpile.hpp"
#include "stb_image.h"

//screen dimensions
int screenWidth = 1200;
int screenHeight = 675;

//initial settings
Camera camera(glm::vec3(-6.5f, 10.0f, -6.5f), 45.0f, 0.0f);
const glm::vec3 lightDir(1.2f, 2.5f, 2.0f);

//mouse variables
float lastX = screenWidth / 2.0;
float lastY = screenHeight / 2.0;
bool mouseMoved = false;

//GUI variables
bool pause = true;
bool mouseFocused = true;
bool highlight = false;
bool infinite = true;
bool pauseOnNextUpdate = false;
bool resizeOnNextUpdate = false;
int maxDrops = 10;
int tempAnimationFrames = 5;
int tempPlateWidth = 20;
int tempPlateHeight = 20;
unsigned int playTexture;
unsigned int pauseTexture;

unsigned int cubeVAO = 0;
unsigned int cubeVBO = 0;
unsigned int plateVAO;

//animation info for render function, 1 is no animation
int animationFrames = 5;
std::vector<std::vector<int>> plateImage;
int currentFrame = 0;
const int maxFPS = 60;
const int msPerFrame = (int) (((double) 1 / (double) maxFPS) * 1000);

//temp variables for GUI to store reference to
int plateWidth, plateHeight;

//data tracking
std::vector<int> sizeData;
std::string lastReset = "clear";
int centerCount = 0;
int randomCount = 0;

void framebuffer_size_callback(GLFWwindow *window, int width, int height);
void mouse_callback(GLFWwindow *window, double xpos, double ypos);
void key_callback(GLFWwindow *window, int key, int scancode, int action, int mods);
void processInput(GLFWwindow *window, double deltaTime);
void renderCube();
void renderScene(const Shader &shader, const Sandpile &pile);
void renderGUI(Sandpile &pile);
void updateLightSpace(Shader &a, Shader &b);
void reset(Sandpile &pile, bool rand, bool resize);
void exportFrequencyDistribution(const std::vector<int> &data, const Sandpile &pile);

int main()
{
	//initialize GLFW
	glfwInit();
	glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
	glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
	glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);

	//make window
	GLFWwindow *window = glfwCreateWindow(screenWidth, screenHeight, "Sandpile", nullptr, nullptr);
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
		1.0f, -0.5f,  1.0f,  0.0f, 1.0f, 0.0f,
		0.0f, -0.5f, 1.0f,  0.0f, 1.0f, 0.0f,
		0.0f, -0.5f, 0.0f,  0.0f, 1.0f, 0.0f,

		1.0f, -0.5f,  1.0f,  0.0f, 1.0f, 0.0f,
		0.0f, -0.5f, 0.0f,  0.0f, 1.0f, 0.0f,
		1.0f, -0.5f, 0.0f,  0.0f, 1.0f, 0.0f,
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
	const unsigned int SHADOW_WIDTH = 2048, SHADOW_HEIGHT = 2048;
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

	updateLightSpace(simpleDepthShader, lightingShader);

	//set light attributes
	lightingShader.use();
	lightingShader.setVec3(glm::vec3(0.2f, 0.2f, 0.2f), "light.ambient");
	lightingShader.setVec3(glm::vec3(0.5f, 0.5f, 0.5f), "light.diffuse");
	lightingShader.setVec3(glm::vec3(1.0f, 1.0f, 1.0f), "light.specular");
	lightingShader.setVec3(lightDir, "light.direction");
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

	//main render loop
	while (!glfwWindowShouldClose(window)) {
		double startTime = glfwGetTime();
		deltaTime = startTime - lastTime;
		lastTime = startTime;

		//process input
		processInput(window, deltaTime);

		//check max drop
		if (pile.drops >= maxDrops) {
			pauseOnNextUpdate = true;
		}

		//update animation
		if (!pause) {
			if (currentFrame == animationFrames - 1) {
				//animation is ending and pause is queued -> stay at end of animation under old animationFrames and don't update
				if (pauseOnNextUpdate || resizeOnNextUpdate) {
					pause = true;
					pauseOnNextUpdate = false;
					animationFrames = tempAnimationFrames;
					currentFrame = animationFrames - 1;
					if (resizeOnNextUpdate) {
						resizeOnNextUpdate = false;
						pile.width = tempPlateWidth;
						pile.height = tempPlateHeight;
						pile.resize();
						updateLightSpace(simpleDepthShader, lightingShader);
					}
					plateImage = pile.plate;
				} else {
					//end of update: change to next update
					plateImage = pile.plate;
					//if about to drop get size data afterwards (after first update affectedCells is not empty)
					if (pile.affectedCells.size() == 0) {
						pile.update();
						sizeData.push_back(pile.size);
						//count type of drop
						if (pile.center)
							centerCount++;
						else
							randomCount++;
					} else {
						pile.update();
					}
					currentFrame = 0;
				}
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
		glViewport(0, 0, screenWidth, screenHeight);
		glClearColor(0.1f, 0.1f, 0.1f, 1.0f);
		glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

		//render normally
		lightingShader.use();
		glm::mat4 projection = glm::perspective(glm::radians(45.0f), (float) screenWidth / (float) screenHeight, 0.1f, 200.0f);
		glm::mat4 view = camera.getViewMatrix();
		lightingShader.setMat4(projection, "projection");
		lightingShader.setMat4(view, "view");
		lightingShader.setVec3(camera.pos, "viewPos");

		glActiveTexture(GL_TEXTURE1);
		glBindTexture(GL_TEXTURE_2D, depthMap);

		renderScene(lightingShader, pile);

		//render GUI & event polling
		renderGUI(pile);
		glfwPollEvents();

		//delay, to cap fps
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
	screenWidth = width;
	screenHeight = height;
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

void key_callback(GLFWwindow *window, int key, int scancode, int action, int mods)
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
	//translate plate
	glm::mat4 model = glm::mat4(1.0f);
	model = glm::translate(model, glm::vec3(-0.5f, 0.0f, -0.5f));
	model = glm::scale(model, glm::vec3(pile.width, 1.0f, pile.height));

	//set plate material attributes
	shader.setMat4(model, "model");
	shader.setVec3(glm::vec3(0.6f, 0.6f, 0.6f), "material.ambient");
	shader.setVec3(glm::vec3(0.4f, 0.4f, 0.4f), "material.diffuse");
	shader.setVec3(glm::vec3(0.2f, 0.2f, 0.2f), "material.specular");
	shader.setFloat(10.0f, "material.shininess");
	shader.setBool(false, "cube");

	//draw plate
	glBindVertexArray(plateVAO);
	glDrawArrays(GL_TRIANGLES, 0, 6);

	//set shininess (same for all cubes)
	shader.setFloat(10.0f, "material.shininess");
	shader.setBool(true, "cube");

	//render cubes according to sandpile matrix
	for (int i = 0; i < pile.width; i++) {
		for (int j = 0; j < pile.height; j++) {
			int target = pile.plate[i][j];
			int prev = plateImage[i][j];
			//frame 0 is just started, frame animationFrames - 1 is finished animation
			double progress = (double) (currentFrame + 1) / (double) animationFrames;
			//provide a buffer underneath plate if sand is being added
			for (int k = std::min(0, prev - target); k < prev; k++) {
				model = glm::mat4(1.0f);
				//offset so that when the animation is paused on the last frame, the buffer does not show above the surface
				model = glm::translate(model, glm::vec3(i, k + (target - prev) * progress - 0.005, j));
				shader.setMat4(model, "model");
				if (highlight && target >= 4) {
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

	if ((pause) ? ImGui::ImageButton((void*) (intptr_t) playTexture, ImVec2(32, 32)) : ImGui::ImageButton((void*) (intptr_t) pauseTexture, ImVec2(32, 32)))
		pause = !pause;

	ImGui::SameLine();
	if (ImGui::Button("export data")) {
		exportFrequencyDistribution(sizeData, pile);
	}

	ImGui::Checkbox("center", &pile.center);

	ImGui::SameLine();
	ImGui::Checkbox("highlight", &highlight);

	ImGui::SameLine();
	ImGui::Checkbox("infinite", &infinite);

	if (!infinite) {
		ImGui::InputInt("drops", &maxDrops);
	} else {
		maxDrops = INT_MAX;
	}

	if (ImGui::Button("clear"))
		reset(pile, false, false);

	ImGui::SameLine();
	if (ImGui::Button("randomize"))
		reset(pile, true, false);

	ImGui::SliderInt("frames", &tempAnimationFrames, 1, 20);
	if (ImGui::IsItemDeactivatedAfterEdit()) {
		pause = true;
		currentFrame = 0;
		animationFrames = tempAnimationFrames;
	}

	//clear before resizing
	ImGui::InputInt("width", &tempPlateWidth);
	if (ImGui::IsItemDeactivatedAfterEdit()) {
		if (tempPlateWidth > 100)
			tempPlateWidth = 100;
		reset(pile, false, true);
	}

	ImGui::InputInt("height", &tempPlateHeight);
	if (ImGui::IsItemDeactivatedAfterEdit()) {
		if (tempPlateHeight > 100)
			tempPlateHeight = 100;
		reset(pile, false, true);
	}

	ImGui::End();

	ImGui::Render();
	ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());
}

void updateLightSpace(Shader &a, Shader &b)
{
	//light space calculations
	glm::mat4 lightProjection, lightView;
	glm::mat4 lightSpaceMatrix;
	float camPos = std::max((float) tempPlateWidth / 2, (float) tempPlateHeight / 2);
	float near_plate = -0.75 * camPos, far_plate = 1.5 * camPos;
	lightProjection = glm::ortho((float) - 2.0 * camPos, (float) 2.0 * camPos, (float) - camPos, (float) camPos, near_plate, far_plate);
	lightView = glm::lookAt(lightDir * 1.0f + glm::vec3(camPos, 0, (float) camPos),
	                        glm::vec3(0.0f) + glm::vec3(camPos, 0, (float) camPos),
	                        glm::vec3(0.0, 1.0, 0.0));
	lightSpaceMatrix = lightProjection * lightView;

	//set light space matrices
	a.use();
	a.setMat4(lightSpaceMatrix, "lightSpaceMatrix");
	b.use();
	b.setMat4(lightSpaceMatrix, "lightSpaceMatrix");
}

void reset(Sandpile &pile, bool rand, bool resize)
{
	//unpause to play collpasing animation, and then pause again once the animation is complete
	if (pause)
		pause = false;
	currentFrame = 0;
	animationFrames = 10;
	if (resize)
		resizeOnNextUpdate = true;
	else
		pauseOnNextUpdate = true;
	if (rand) {
		pile.fillRand();
		lastReset = "randomize";
	} else {
		pile.fillValue(0);
		lastReset = "clear";
	}
	centerCount = 0;
	randomCount = 0;
	pile.drops = 0;
	sizeData.clear();
}

void exportFrequencyDistribution(const std::vector<int> &data, const Sandpile &pile)
{
	std::map<int, int> dist;
	for (int i : data) {
		if (dist.find(i) == dist.end()) {
			dist[i] = 1;
		} else {
			dist[i]++;
		}
	}
	std::ofstream fs("sizeFreq.txt");
	if (!fs) {
		std::cout << "Could not open the output file." << std::endl;
		return;
	}
	fs << lastReset << "\n"
	   << pile.width << "\n"
	   << pile.height << "\n"
	   << pile.drops << "\n"
	   << centerCount << "\n"
	   << randomCount << "\n";
	fs << "\tSize\tFreq.\n";
	int row = 0;
	for (auto &pair : dist) {
		row++;
		fs << row << "\t" << pair.first << "\t" << pair.second << "\n";
	}
}