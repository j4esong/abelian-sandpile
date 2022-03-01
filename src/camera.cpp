
#include "camera.hpp"

Camera::Camera(glm::vec3 pos, float yaw, float pitch, glm::vec3 up)
	: pos(pos), up(up), yaw(yaw), pitch(pitch), moveSpeed(1.0), mouseSens(0.05)
{
	updateCameraVectors();
}

void Camera::processKeyboard(Camera_Movement direction, float deltaTime)
{
	float dist = moveSpeed * deltaTime;
	if (direction == FORWARD)
		pos += moveDir * dist;
	if (direction == BACKWARD)
		pos -= moveDir * dist;
	if (direction == LEFT)
		pos -= right * dist;
	if (direction == RIGHT)
		pos += right * dist;
	if (direction == UP)
		pos += up * dist;
	if (direction == DOWN)
		pos -= up * dist;
}

void Camera::processMouseMovement(float xoffset, float yoffset)
{
	xoffset *= mouseSens;
	yoffset *= mouseSens;
	yaw += xoffset;
	pitch += yoffset;
	if (pitch > 89.0f)
		pitch = 89.0f;
	if (pitch < -89.0f)
		pitch = -89.0f;
	updateCameraVectors();
}

glm::mat4 Camera::getViewMatrix()
{
	return glm::lookAt(pos, pos + viewDir, up);
}

void Camera::updateCameraVectors()
{
	glm::vec3 newDir;
	newDir.x = cos(glm::radians(yaw)) * cos(glm::radians(pitch));
	newDir.y = sin(glm::radians(pitch));
	newDir.z = sin(glm::radians(yaw)) * cos(glm::radians(pitch));
	viewDir = glm::normalize(newDir);
	newDir.y = 0;
	moveDir = glm::normalize(newDir);
	right = glm::normalize(glm::cross(moveDir, up));
}