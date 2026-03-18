#include "model.h"

#include "../entity.h"
#include "transform.h"

void Model::OnRender(RenderingEngine& renderer)
{
	Transform* transform = GetOwner()->GetComponent<Transform>();
	Matrix modelMatrix = transform != nullptr
		? transform->GetWorldTransformation()
		: MatrixIdentity();
	renderer.AddToRender(mMesh.lock().get(), mMaterial, modelMatrix);

#ifdef IS_DEBUG
	auto dd = renderer.GetDebugDraw();
	dd->SetTwoDMode(false);
	dd->SetDepthEnabled(true);
	dd->SetStippleFactor(1.0f);
	dd->SetStipplePattern(0xFFFF);
	// Draw the model's bounding box
	auto mesh = mMesh.lock();
	if (mesh) {
		auto bboxMin = mesh->GetAABBMin();
		auto bboxMax = mesh->GetAABBMax();
		auto bboxCenter = (bboxMin + bboxMax) * 0.5f;
		auto bboxSize = bboxMax - bboxMin;
		dd->Cube(bboxCenter, bboxSize, colors::Gray, modelMatrix);
	}
#endif
}
