
#include <GL/glew.h>
#include <GLFW/glfw3.h>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <chrono>
#include <thread>
#include <iostream>
#include <vector>

#include "shader.hpp"
#include "camera.hpp"
#include "sandpile.hpp"

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

unsigned int cubeVAO = 0;
unsigned int cubeVBO = 0;
unsigned int plateVAO;

//animation info for render function
const int animationFrames = 20;
std::vector<std::vector<int>> plateImage;
int currentFrame;

void framebuffer_size_callback(GLFWwindow *window, int width, int height);
void mouse_callback(GLFWwindow *window, double xpos, double ypos);
void processInput(GLFWwindow *window, double deltaTime);
void renderCube();
void renderScene(const Shader &shader, const Sandpile &pile);

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

	glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_DISABLED);

	//initialize GLEW
	glewExperimental = GL_TRUE;
	glewInit();

	glEnable(GL_DEPTH_TEST);

	//compile shaders
	Shader lightingShader("cubeShader.vert", "cubeShader.frag");
	Shader simpleDepthShader("depthShader.vert", "depthShader.frag");

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

	//initialize image to nothing, and sandpile object to random pile
	Sandpile pile(20, 20);
	pile.fillValue(0);
	plateImage = pile.plate;
	pile.fillRand();

	//set up per-frame logic
	double lastTime = glfwGetTime();
	currentFrame = 0;

	//main render loop
	while (!glfwWindowShouldClose(window)) {
		double time = glfwGetTime();
		double deltaTime = time - lastTime;
		lastTime = time;

		//input
		processInput(window, deltaTime);

		//update animation
		if (currentFrame == animationFrames - 1) {
			plateImage = pile.plate;
			pile.update();
			currentFrame = 0;
		} else {
			currentFrame++;
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
		glm::mat4 projection = glm::perspective(glm::radians(45.0f), (float) SCREEN_WIDTH / (float) SCREEN_HEIGHT, 0.1f, 100.0f);
		glm::mat4 view = camera.getViewMatrix();
		lightingShader.setMat4(projection, "projection");
		lightingShader.setMat4(view, "view");
		lightingShader.setVec3(camera.pos, "viewPos");

		glActiveTexture(GL_TEXTURE1);
		glBindTexture(GL_TEXTURE_2D, depthMap);

		renderScene(lightingShader, pile);

		glfwSwapBuffers(window);
		glfwPollEvents();
	}

	//clean up
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
	//prevent camera from moving below plate
	if (camera.pos.y < 0)
		camera.pos.y = 0;
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
		glBufferData(GL_ARRAY_BUFFER, sizeof(vertices), vertices, GL_STATIC_DRAW);

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
	glBindVertexArray(plateVAO);
	glDrawArrays(GL_TRIANGLES, 0, 6);

	//set cube material attributes
	shader.setVec3(glm::vec3(1.0f, 1.0f, 1.0f), "material.ambient");
	shader.setVec3(glm::vec3(1.0f, 1.0f, 1.0f), "material.diffuse");
	shader.setVec3(glm::vec3(0.2f, 0.2f, 0.2f), "material.specular");
	shader.setFloat(10.0f, "material.shininess");

	//render cubes according to sandpile matrix
	for (int i = 0; i < pile.width; i++) {
		for (int j = 0; j < pile.height; j++) {
			int target = pile.plate[i][j];
			int prev = plateImage[i][j];
			//frame 0 is no progress, frame animationFrames - 1 is 1 from finishing the animation
			double progress = (double) currentFrame / (double) animationFrames;
			//provide a buffer if sand is being added
			for (int k = std::min(0, prev - target); k < prev; k++) {
				model = glm::mat4(1.0f);
				model = glm::translate(model, glm::vec3(i, k + (target - prev) * progress, j));
				shader.setMat4(model, "model");
				renderCube();
			}
		}
	}
}