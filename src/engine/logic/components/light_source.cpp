#include "light_source.h"

#include "../entity.h"
#include "transform.h"

void LightSource::OnRender(RenderingEngine& renderer)
{
	Transform* transform = mOwner->GetComponent<Transform>();
	if (transform) {
		mLight.position = transform->GetGlobalPosition();
		mLight.direction = Vector3Normalize(transform->GetGlobalUp() * -1.0f);
	}
	renderer.AddToRender(mLight);

// #ifdef IS_DEBUG
// 	auto dd = renderer.GetDebugDraw();
// 	switch (mLight.type) {
// 		case LightType::directional:
// 		{
// 			dd->Circle(mLight.position, mLight.direction, { 0.0f, 1.0f, 0.0f }, mLight.radius, colors::Yellow);
// 			dd->Arrow(mLight.position, mLight.position + mLight.direction * mLight.radius, colors::Yellow, 0.4f);
// 		} break;
// 		case LightType::spot:
// 		{
// 			Vector3 end = mLight.position + mLight.direction * mLight.radius * 1.1f;

// 			dd->SetStippleFactor(4.0f);
// 			dd->SetStipplePattern(0xAAAA);
// 			dd->Line(mLight.position, end, colors::Yellow);

// 			float coneRadius = mLight.radius * ::sinf(mLight.cutoffAngle * 0.5f);
// 			dd->SetStipplePattern(0xFFFF);

// 			auto coneColor = colors::Yellow * Vector4{ 1.0f, 1.0f, 1.0f, 0.15f };
// 			dd->Cone(mLight.position, mLight.direction, coneRadius, mLight.radius, colors::Yellow, true, true);
// 			//dd->FillCone(mLight.position, mLight.direction, coneRadius, mLight.radius, coneColor, true, false);
// 		} break;
// 		case LightType::point:
// 			Vector3 viewDir = Vector3Normalize(renderer.GetViewPosition() - mLight.position);
// 			dd->SetStippleFactor(4.0f);
// 			dd->SetStipplePattern(0xAAAA);
// 			dd->Circle(mLight.position, viewDir, { 0.0f, 1.0f, 0.0f }, mLight.radius, colors::Yellow);
// 			dd->SetStipplePattern(0xFFFF);
// 			break;
// 	}
// 	dd->Point(mLight.position, Vector4{ mLight.color.x, mLight.color.y, mLight.color.z, 1.0f }, 32.0f, true);
// #endif
}
