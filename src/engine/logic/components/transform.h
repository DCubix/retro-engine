#pragma once

#include "../../external/raymath/raymath.h"
#include "../component.h"

class RENGINE_API Transform : public Component {
public:
	Transform();

	Matrix GetLocalTransformation();
	Matrix GetWorldTransformation();

	Transform* GetParent() const { return mParent; }
	void SetParent(Transform* parent, bool keepOffset = true);

	Vector3 GetPosition() const { return mPosition; }
	void SetPosition(const Vector3& position) { mPosition = position; MarkDirty(); }

	Quaternion GetRotation() const { return mRotation; }
	void SetRotation(const Quaternion& rotation) { mRotation = rotation; MarkDirty(); }

	Vector3 GetScale() const { return mScale; }
	void SetScale(const Vector3& scale) { mScale = scale; MarkDirty(); }

	void RotateLocal(const Vector3& axis, f32 angle);
	void RotateLocal(const Quaternion& rotation);
	void RotateGlobal(const Vector3& axis, f32 angle);
	void RotateGlobal(const Quaternion& rotation);
	void LookAt(const Vector3& target, const Vector3& up = { 0.0f, 1.0f, 0.0f });

	Vector3 GetGlobalPosition();
	Quaternion GetGlobalRotation();

	Vector3 GetLocalRight() const { return Vector3RotateByQuaternion({ 1.0f, 0.0f, 0.0f }, mRotation); }
	Vector3 GetLocalUp() const { return Vector3RotateByQuaternion({ 0.0f, 1.0f, 0.0f }, mRotation); }
	Vector3 GetLocalForward() const { return Vector3RotateByQuaternion({ 0.0f, 0.0f, 1.0f }, mRotation); }

	Vector3 GetGlobalRight() { return Vector3RotateByQuaternion({ 1.0f, 0.0f, 0.0f }, GetGlobalRotation()); }
	Vector3 GetGlobalUp() { return Vector3RotateByQuaternion({ 0.0f, 1.0f, 0.0f }, GetGlobalRotation()); }
	Vector3 GetGlobalForward() { return Vector3RotateByQuaternion({ 0.0f, 0.0f, 1.0f }, GetGlobalRotation()); }

protected:
	Transform* mParent{ nullptr };
	std::vector<Transform*> mChildren;

	Vector3 mPosition{ 0.0f, 0.0f, 0.0f };
	Quaternion mRotation{ 0.0f, 0.0f, 0.0f, 1.0f };
	Vector3 mScale{ 1.0f, 1.0f, 1.0f };

	Matrix mCachedWorldTransform{ 0.0f };

	bool mIsDirty{ true };

	void UpdateWorldMatrix();
	void MarkDirty();
};