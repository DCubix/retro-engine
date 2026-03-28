#include "light.h"

Matrix Light::GetViewProjection(u32 face) const
{
	auto fnGetLightView = [&]() {
		switch (type) {
			case LightType::directional:
			case LightType::spot:
				return MatrixLookAt(
					position,
					position + direction,
					Vector3{ 0.0f, 1.0f, 0.0f }
				);
			case LightType::point:
			{
				switch (face) {
					case 0: // +X
						return MatrixLookAt(
							position, position + Vector3{ 1.0f, 0.0f, 0.0f },
							Vector3{ 0.0f, -1.0f, 0.0f }
						);
					case 1: // -X
						return MatrixLookAt(
							position, position + Vector3{ -1.0f, 0.0f, 0.0f },
							Vector3{ 0.0f, -1.0f, 0.0f }
						);
					case 2: // +Y
						return MatrixLookAt(
							position, position + Vector3{ 0.0f, 1.0f, 0.0f },
							Vector3{ 0.0f, 0.0f, 1.0f }
						);
					case 3: // -Y
						return MatrixLookAt(
							position, position + Vector3{ 0.0f, -1.0f, 0.0f },
							Vector3{ 0.0f, 0.0f, -1.0f }
						);
					case 4: // +Z
						return MatrixLookAt(
							position, position + Vector3{ 0.0f, 0.0f, 1.0f },
							Vector3{ 0.0f, -1.0f, 0.0f }
						);
					case 5: // -Z
						return MatrixLookAt(
							position, position + Vector3{ 0.0f, 0.0f, -1.0f },
							Vector3{ 0.0f, -1.0f, 0.0f }
						);
					default:
						return MatrixLookAt(
							position, position + Vector3{ 1.0f, 0.0f, 0.0f },
							Vector3{ 0.0f, -1.0f, 0.0f }
						);
				}
			}
			default:
				return MatrixLookAt(
					position,
					position + direction,
					Vector3{ 0.0f, 1.0f, 0.0f }
				);
		}
	};

	auto fnGetLightProj = [&]() {
		switch (type) {
			case LightType::directional:
				return MatrixOrtho(-20.0f, 20.0f, -20.0f, 20.0f, 0.01f, 1000.0f);
			case LightType::spot:
				return MatrixPerspective(cutoffAngle, 1.0f, 0.01f, radius * 10.0f);
			case LightType::point:
				return MatrixPerspective(90.0f * DEG2RAD, 1.0f, 1.0f, radius);
			default:
				return MatrixOrtho(-20.0f, 20.0f, -20.0f, 20.0f, 0.01f, 1000.0f);
		}
	};

	return fnGetLightView() * fnGetLightProj();
}
