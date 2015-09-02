#include "ColorPicking.h"

#include <include/dll_export.h>
#include <include/glm.h>
#include <include/glm_utils.h>

#include <Editor/Gizmo.h>

#include <Core/GameObject.h>

#include <Component/AABB.h>
#include <Component/Mesh.h>
#include <Component/SkinnedMesh.h>
#include <Component/Text.h>
#include <Component/Transform/Transform.h>
#include <Component/ObjectInput.h>
#include <Component/Renderer.h>

#include <Manager/Manager.h>
#include <Manager/TextureManager.h>
#include <Manager/ResourceManager.h>
#include <Manager/ShaderManager.h>
#include <Manager/SceneManager.h>
#include <Manager/EventSystem.h>

#include <Core/Engine.h>
#include <Core/WindowObject.h>
#include <Core/InputSystem.h>
#include <Core/Camera/Camera.h>

#include <GPU/FrameBuffer.h>
#include <GPU/Texture.h>
#include <GPU/Shader.h>

#include <Utils/OpenGL.h>


struct _Gizmo {
	GameObject *gizmo_object[3];
	glm::vec3 axis, color;
} gizmo[3];

GIZMO_EVENT events[3] = { GIZMO_EVENT::MOVE, GIZMO_EVENT::ROTATE, GIZMO_EVENT::SCALE };

ColorPicking::ColorPicking()
	: ObjectInput(InputGroup::IG_GAMEPLAY)
{
	rotateMode = GIZMO_EVENT::ROTATE_WORLD;
}

ColorPicking::~ColorPicking() {
}

void ColorPicking::Init()
{
	selectedObject = nullptr;
	gizmo_isLocal = true;
	pickEvent = false; 
	focusEvent = false;
	
	focus_minSpeed = 1.0f;
	focus_maxSpeed = 5.0f;
	focus_accel = 0.25f;

	GEVENT = GIZMO_EVENT::MOVE;

	glm::ivec2 resolution = Engine::Window->resolution;
	
	//generate buffer for color picking
	FBO = new FrameBuffer();
	FBO->Generate(resolution.x, resolution.y, 1);

	//buffer for gizmo-only
	FBO_Gizmo = new FrameBuffer();
	FBO_Gizmo->Generate(resolution.x, resolution.y, 1);

	//get shaders
	cpShader = Manager::GetShader()->GetShader("simple");
	gizmoShader = Manager::GetShader()->GetShader("simple");

	gizmoObject = new Gizmo();

	// TODO - maybe use only One Instance for rendering ?!
	// TODO - WHY X AND Z HAVE REVERSED COLORS ?!
	// gizmo struct init
	//x
	gizmo[0].color = glm::vec3(0, 0, 1);
	gizmo[0].axis = glm::vec3(1, 0, 0);

	//y
	gizmo[1].color = glm::vec3(0, 1, 0);
	gizmo[1].axis = glm::vec3(0, 1, 0);

	//z
	gizmo[2].color = glm::vec3(1, 0, 0);
	gizmo[2].axis = glm::vec3(0, 0, -1);

	//gizmo objects
	for (int i = 0; i <= 2; i++) {
		gizmo[i].gizmo_object[GIZMO_EVENT::MOVE] = Manager::GetResource()->GetGameObject("gizmo_move");
		gizmo[i].gizmo_object[GIZMO_EVENT::ROTATE] = Manager::GetResource()->GetGameObject("gizmo_rotate");
		gizmo[i].gizmo_object[GIZMO_EVENT::SCALE] = Manager::GetResource()->GetGameObject("gizmo_scale");
	}

	ResetGizmoRotation();
}

void ColorPicking::Update(const Camera* activeCamera)
{
	this->activeCamera = activeCamera;

	if (pickEvent) {
		if (selectedObject)
			UpdateGizmo(true);
		DrawSceneForPicking();
		GetPickedObject();
		pickEvent = false;
	}

	if (selectedObject) {
		UpdateGizmoScale();
		DrawGizmo();

		if (focusEvent) {
			FocusCamera();
		}
	}
}

void ColorPicking::OnMouseBtnEvent(int mouseX, int mouseY, int button, int action, int mods)
{
	if (button == MOUSE_BUTTON::LEFT && action == GLFW_PRESS) {
		mousePosition = glm::ivec2(mouseX, mouseY);
		pickEvent = true;
	}

	if (button == MOUSE_BUTTON::LEFT && action == GLFW_RELEASE) {
		currentAxis = glm::vec3(0, 0, 0);
		gizmoEvent = false;
	}
}

void ColorPicking::OnMouseMove(int mouseX, int mouseY, int deltaX, int deltaY) {

	if (!selectedObject)
		return;

	if (gizmoEvent)
	{
		switch (GEVENT)
		{
		case GIZMO_EVENT::MOVE:
		{
			float dist = activeCamera->DistTo(selectedObject);
			glm::vec3 delta((deltaX - deltaY) * dist / 500);

			// Move Object
			selectedObject->transform->SetWorldPosition(selectedObject->transform->GetWorldPosition() + currentAxis * delta);

			// Move Camera if CTRL is pressed
			if (InputSystem::KeyHold(GLFW_KEY_LEFT_CONTROL))
			{
				activeCamera->transform->SetWorldPosition(activeCamera->transform->GetWorldPosition() + currentAxis * delta);
			}
			break;
		}

		case GIZMO_EVENT::ROTATE:
		{
			glm::vec3 rotation_vector = currentAxis * glm::vec3(deltaX - deltaY);
			selectedObject->transform->SetRotationSpeed(0.3f);
			if (rotateMode == GIZMO_EVENT::ROTATE_WORLD)
			{
				if (rotation_vector.x) selectedObject->transform->RotateWorldOX(rotation_vector.x);
				if (rotation_vector.y) selectedObject->transform->RotateWorldOY(rotation_vector.y);
				if (rotation_vector.z) selectedObject->transform->RotateWorldOZ(rotation_vector.z);
			}
			else {
				if (rotation_vector.x) selectedObject->transform->RotateLocalOX(rotation_vector.x);
				if (rotation_vector.y) selectedObject->transform->RotateLocalOY(rotation_vector.y);
				if (rotation_vector.z) selectedObject->transform->RotateLocalOZ(rotation_vector.z);
			}
			break;
		}

		case GIZMO_EVENT::SCALE:
		{
			selectedObject->transform->SetScale(selectedObject->transform->GetScale() + currentAxis *
				glm::vec3(deltaX - deltaY) / glm::vec3(200.0f));
			break;
		}
		default:
			break;
		}

		// Update Gizmo
		UpdateGizmoPosition();
		if (rotateMode == GIZMO_EVENT::ROTATE_LOCAL) {
			UpdateGizmoRotation();
		}
	}
}

void ColorPicking::OnKeyPress(int key, int mod)
{
	if (mod == GLFW_MOD_CONTROL)
	{
		GIZMO_EVENT prevEvent = GEVENT;
		switch (key)
		{
		case GLFW_KEY_W: {
			GEVENT = GIZMO_EVENT::MOVE;
			cout << "world: " << selectedObject->transform->GetWorldPosition() << endl;
			cout << "local: " << selectedObject->transform->GetLocalPosition() << endl;
			break;
		}
		case GLFW_KEY_E: {
			GEVENT = GIZMO_EVENT::ROTATE;
			cout << "roation: " << selectedObject->transform->GetRotationEuler() << endl;
			break;

		}
		case GLFW_KEY_R: {
			GEVENT = GIZMO_EVENT::SCALE;
			cout << "scale: " << selectedObject->transform->GetScale() << endl;
			break;
		}
		case GLFW_KEY_L: {
			if (rotateMode == GIZMO_EVENT::ROTATE_LOCAL) {
				rotateMode = GIZMO_EVENT::ROTATE_WORLD;
				ResetGizmoRotation();
				cout << "Rotate: [WORLD]" << endl;
			}
			else {
				rotateMode = GIZMO_EVENT::ROTATE_LOCAL;
				cout << "Rotate: [LOCAL]" << endl;
			}
			break;
		}
		case GLFW_KEY_F: {
			focus_currentSpeed = focus_minSpeed;
			focusEvent = true;
			break;
		}
		case GLFW_KEY_U: {
			if (selectedObject)
				selectedObject->transform->SetWorldRotation(glm::quat(1, 0, 0, 0));
			break;
		}
		default:
			break;
		}

		if (GEVENT != prevEvent) {
			UpdateGizmo();
		}
	}
}

// TODO:
// Switch to TPS camera for viewing objects + distance to object (SCROLL)

void ColorPicking::FocusCamera()
{
	glm::vec3 scale = selectedObject->aabb->transform->GetScale();
	float dist = 3 * max(scale.x, max (scale.y, scale.z) );
	glm::vec3 position = selectedObject->transform->GetWorldPosition() - glm::normalize(activeCamera->transform->GetLocalOZVector()) * dist;

	glm::vec3 cameraPosition = activeCamera->transform->GetWorldPosition();
	
	if (glm::distance(cameraPosition, position) < 0.01) {
		focusEvent = false;
		return;
	}

	focus_currentSpeed = min(focus_maxSpeed, focus_currentSpeed + focus_accel);
	activeCamera->transform->SetWorldPosition(cameraPosition + glm::normalize(position - cameraPosition) * min(focus_currentSpeed, glm::distance(position, cameraPosition)));
}

void ColorPicking::SetSelectedObject(GameObject * object)
{
	selectedObject = object;
	if (selectedObject) {
		cout << selectedObject->GetName() << endl;

		// Align Gizmo to the current selected objects
		UpdateGizmo();
	}
}

GameObject* ColorPicking::GetSelectedObject() const
{
	return selectedObject;
}

void ColorPicking::GetPickedObject()
{
	// read pixel from colorpicking frame buffer
	float objColor[3];
	glReadPixels(mousePosition.x, Engine::Window->resolution.y - mousePosition.y, 1, 1, GL_RGB, GL_FLOAT, objColor);
	glm::vec3 pickedColor = glm::vec3(objColor[0], objColor[1], objColor[2]);

	// check if gizmo was clicked
	for (int i = 0; i <= 2; i++) {
		if (gizmo[i].gizmo_object[GEVENT]->IdentifyByColor(pickedColor))
		{
			gizmoEvent = true;
			//the color matches the axis
			currentAxis = gizmo[i].color;
		}
	}

	if (!gizmoEvent) {
		//check for objects click
		for (auto *obj : Manager::GetScene()->activeObjects) {
			GameObject *G = obj->IdentifyByColor(pickedColor);
			if (G) {
				SetSelectedObject(G);
				Manager::Event->EmitSync(EventType::EDITOR_OBJECT_SELECTED, selectedObject);
				break;
			}
		}
	}

	FrameBuffer::Unbind();
}

void ColorPicking::DrawSceneForPicking() const
{
	FBO->Bind();
	cpShader->Use();

	activeCamera->BindPosition(cpShader->loc_eye_pos);
	activeCamera->BindViewMatrix(cpShader->loc_view_matrix);
	activeCamera->BindProjectionMatrix(cpShader->loc_projection_matrix);

	for (auto obj : Manager::GetScene()->activeObjects) {
		obj->RenderForPicking(cpShader);
	}

	if (selectedObject) {
		RenderGizmo(cpShader, true);
	}
}

void ColorPicking::UpdateGizmo(bool picking)
{
	float dist = activeCamera->DistTo(selectedObject);
	glm::vec3 scale(dist);
	if (GEVENT == GIZMO_EVENT::ROTATE) {
		scale.y = picking ? 2 * dist : dist / 4;
	}
	for (int i = 0; i < 3; i++) {
		gizmo[i].gizmo_object[GEVENT]->transform->SetScale(scale);
		gizmo[i].gizmo_object[GEVENT]->transform->SetWorldPosition(selectedObject->transform->GetWorldPosition());
	}
	gizmoObject->SetScale(scale);
	gizmoObject->transform->SetWorldPosition(selectedObject->transform->GetWorldPosition());
}

void ColorPicking::UpdateGizmoPosition()
{
	auto pos = selectedObject->transform->GetWorldPosition();
	for (int i = 0; i < 3; i++) {
		gizmo[i].gizmo_object[GEVENT]->transform->SetWorldPosition(pos);
	}
	gizmoObject->transform->SetWorldPosition(pos);
}

void ColorPicking::UpdateGizmoRotation()
{
	//for (int i = 0; i < 3; i++)
	//{
		//GameObject *obj = gizmo[i].gizmo_object[GEVENT];
		//glm::vec3 euler = glm::vec3(90.0f * gizmo[i].axis) * TO_RADIANS;
		//auto rotation = glm::quat(euler);
		//rotation = glm::inverse(rotation) * selectedObject->transform->GetWorldRotation();
		//obj->transform->SetWorldRotation(rotation);
	//}
	gizmoObject->transform->SetWorldRotation(selectedObject->transform->GetWorldRotation());
}

void ColorPicking::UpdateGizmoScale()
{
	float dist = activeCamera->DistTo(selectedObject);
	glm::vec3 scale = glm::vec3(dist, (GEVENT == GIZMO_EVENT::ROTATE) ? dist / 4 : dist, dist);
	for (int i = 0; i < 3; i++) {
		gizmo[i].gizmo_object[GEVENT]->transform->SetScale(scale);
	}
}

void ColorPicking::ResetGizmoRotation()
{
	for (int i = 0; i < 3; i++) {
		for (auto EVENT : events) {
			gizmo[i].gizmo_object[EVENT]->transform->SetWorldRotation(90.0f * gizmo[i].axis);
		}
	}
}

void ColorPicking::DrawGizmo() const
{
	FBO_Gizmo->Bind();
	gizmoShader->Use();

	activeCamera->BindPosition(gizmoShader->loc_eye_pos);
	activeCamera->BindViewMatrix(gizmoShader->loc_view_matrix);
	activeCamera->BindProjectionMatrix(gizmoShader->loc_projection_matrix);

	RenderGizmo(gizmoShader);
	// gizmoObject->Render(gizmoShader);

	FrameBuffer::Unbind();
}

void ColorPicking::RenderGizmo(const Shader* shader, bool picking) const
{
	glDisable(GL_DEPTH_TEST);
	glLineWidth(1);

	glm::vec3 wpos = selectedObject->transform->GetWorldPosition();

	glUniformMatrix4fv(shader->loc_model_matrix, 1, GL_FALSE, glm::value_ptr(glm::mat4(1.0)));
	GL_Utils::SetColorUnit(shader->loc_debug_color, 0, 1, 0);
	OpenGL::DrawLine(wpos, wpos + selectedObject->transform->GetLocalOYVector());
	GL_Utils::SetColorUnit(shader->loc_debug_color, 1, 0, 0);
	OpenGL::DrawLine(wpos, wpos + selectedObject->transform->GetLocalOXVector());
	GL_Utils::SetColorUnit(shader->loc_debug_color, 0, 0, 1);
	OpenGL::DrawLine(wpos, wpos + selectedObject->transform->GetLocalOZVector());

	glEnable(GL_CULL_FACE);
	glCullFace(GL_FRONT);
	for (int i = 0; i < 3; i++)
	{
		GameObject *obj = gizmo[i].gizmo_object[GEVENT];
		if (picking) {
			obj->RenderForPicking(shader);
		}
		else {
			GL_Utils::SetColorUnit(shader->loc_debug_color, 0.2, 0.2, 0.2);
			obj->Render(shader);
		}
	}

	glCullFace(GL_BACK);
	for (int i = 0; i < 3; i++)
	{
		GameObject *obj = gizmo[i].gizmo_object[GEVENT];

		if (picking) {
			obj->RenderForPicking(shader);
		} else {
			glm::vec3 color = (currentAxis == gizmo[i].color) ? glm::vec3(1, 1, 0) : gizmo[i].color;
			GL_Utils::SetColorUnit(shader->loc_debug_color, color);
			obj->Render(shader);
		}
	}
	glEnable(GL_DEPTH_TEST);
	glDisable(GL_CULL_FACE);
}