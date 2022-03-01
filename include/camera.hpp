
#ifndef CAMERA_HPP
#define CAMERA_HPP

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>

enum Camera_Movement
{
	FORWARD,
	BACKWARD,
	LEFT,
	RIGHT,
	UP,
	DOWN
};

class Camera
{
public:
	glm::vec3 pos;
	glm::vec3 viewDir;
	glm::vec3 moveDir;
	glm::vec3 up;
	glm::vec3 right;
	float yaw;
	float pitch;
	float moveSpeed;
	float mouseSens;
	Camera(glm::vec3 position, float yaw, float pitch, glm::vec3 up = glm::vec3(0.0, 1.0, 0.0));
	void processKeyboard(Camera_Movement direction, float deltaTime);
	void processMouseMovement(float xoffset, float yoffset);
	glm::mat4 getViewMatrix();
private:
	void updateCameraVectors();
};

#endif