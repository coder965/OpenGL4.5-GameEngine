#include "CameraDebugInput.h"

#include <include/gl.h>
#include <Core/WindowObject.h>
#include <Core/Camera/Camera.h>

CameraDebugInput::CameraDebugInput(Camera *camera)
{
	this->camera = camera;
}

void CameraDebugInput::OnInputUpdate(float deltaTime, int mods)
{
	if (mods != GLFW_MOD_ALT) return;

	if (window->KeyHold(GLFW_KEY_W))
		camera->MoveForward(deltaTime);
	if (window->KeyHold(GLFW_KEY_S))
		camera->MoveBackward(deltaTime);
	if (window->KeyHold(GLFW_KEY_A))
		camera->MoveLeft(deltaTime);
	if (window->KeyHold(GLFW_KEY_D))
		camera->MoveRight(deltaTime);
	if (window->KeyHold(GLFW_KEY_Q))
		camera->MoveDown(deltaTime);
	if (window->KeyHold(GLFW_KEY_E))
		camera->MoveUp(deltaTime);

	if (window->KeyHold(GLFW_KEY_KP_MULTIPLY))
		camera->UpdateSpeed();
	if (window->KeyHold(GLFW_KEY_KP_DIVIDE))
		camera->UpdateSpeed(-0.2f);

	if (window->KeyHold(GLFW_KEY_KP_4))
		camera->RotateOY( 500 * deltaTime);
	if (window->KeyHold(GLFW_KEY_KP_6))
		camera->RotateOY(-500 * deltaTime);
	if (window->KeyHold(GLFW_KEY_KP_8))
		camera->RotateOX( 700 * deltaTime);
	if (window->KeyHold(GLFW_KEY_KP_5))
		camera->RotateOX(-700 * deltaTime);

	camera->Update();
}