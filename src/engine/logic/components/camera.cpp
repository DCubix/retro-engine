#include "camera.h"

#include "transform.h"
#include "../entity.h"

void Camera::OnRender(RenderingEngine& renderer)
{
	Transform* transform = GetOwner()->GetComponent<Transform>();
	Matrix cameraMatrix = MatrixIdentity();

	if (transform) {
		Matrix rotation = QuaternionToMatrix(QuaternionConjugate(transform->GetGlobalRotation()));
		Vector3 pos = transform->GetGlobalPosition() * -1.0f;
		Matrix translation = MatrixTranslate(pos.x, pos.y, pos.z);
		cameraMatrix = MatrixMultiply(translation, rotation);
	}

	renderer.CameraSetup(
		MatrixPerspective(
			mFoV * DEG2RAD,
			f32(renderer.GetFramebufferWidth()) / f32(renderer.GetFramebufferHeight()),
			mZNear,
			mZFar
		),
		cameraMatrix
	);
	renderer.SetViewPosition(transform->GetGlobalPosition());
}
