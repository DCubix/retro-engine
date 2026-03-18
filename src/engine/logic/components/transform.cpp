#include "transform.h"

#include "../../utils/logger.h"
#include "../entity.h"

#include <algorithm>

Transform::Transform()
{
}

Matrix Transform::GetLocalTransformation()
{
	Matrix translation = MatrixTranslate(mPosition.x, mPosition.y, mPosition.z);
	Matrix scale = MatrixScale(mScale.x, mScale.y, mScale.z);
	Matrix rotation = QuaternionToMatrix(mRotation);
	Matrix result = scale * rotation * translation;

	return result;
}

Matrix Transform::GetWorldTransformation()
{
	if (mIsDirty) {
		UpdateWorldMatrix();
	}
	return mCachedWorldTransform;
}

void Transform::SetParent(Transform* parent, bool keepOffset)
{
	if (mParent == parent) return;

	if (mParent) {
		auto& siblings = mParent->mChildren;
		siblings.erase(std::remove(siblings.begin(), siblings.end(), this), siblings.end());
	}

	mParent = parent;
	
	if (parent) {
		parent->mChildren.push_back(this);
	}

	// TODO: calculate offset

	MarkDirty();
}

Quaternion Transform::GetGlobalRotation()
{
	Quaternion parentRot = QuaternionIdentity();
	if (mParent) {
		parentRot = mParent->GetGlobalRotation();
	}
	return QuaternionMultiply(parentRot, mRotation);
}

void Transform::RotateLocal(const Vector3& axis, f32 angle)
{
	RotateLocal(QuaternionFromAxisAngle(axis, angle));
}

void Transform::RotateLocal(const Quaternion& rotation)
{
	mRotation = QuaternionNormalize(QuaternionMultiply(mRotation, rotation));
	MarkDirty();
}

void Transform::RotateGlobal(const Vector3& axis, f32 angle)
{
	RotateGlobal(QuaternionFromAxisAngle(axis, angle));
}

void Transform::RotateGlobal(const Quaternion& rotation)
{
	mRotation = QuaternionNormalize(QuaternionMultiply(rotation, mRotation));
	MarkDirty();
}

void Transform::LookAt(const Vector3& target, const Vector3& up)
{
	Matrix rotation = MatrixLookAt(
		GetGlobalPosition(),
		target,
		up
	);
	mRotation = QuaternionFromMatrix(rotation);
	MarkDirty();
}

Vector3 Transform::GetGlobalPosition()
{
	return Vector3Transform({ 0.0f, 0.0f, 0.0f }, GetWorldTransformation());
}

void Transform::UpdateWorldMatrix()
{
	if (mParent) {
		mCachedWorldTransform = GetLocalTransformation() * mParent->GetWorldTransformation();
	}
	else {
		mCachedWorldTransform = GetLocalTransformation();
	}
	mIsDirty = true;

	for (Transform* child : mChildren) {
		child->MarkDirty();
	}
}

void Transform::MarkDirty()
{
	if (mIsDirty) return;
	mIsDirty = true;
	for (Transform* child : mChildren) {
		child->MarkDirty();
	}
}
