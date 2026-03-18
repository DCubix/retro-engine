#pragma once

#include "../../rendering/mesh.h"
#include "../../rendering/material.h"
#include "../../rendering/texture.h"
#include "../component.h"

class RENGINE_API Model : public Component {
public:
	Model() = default;
	Model(const SPtr<Mesh>& mesh, const Material& material)
		: mMesh(mesh), mMaterial(material) {}

	void OnRender(RenderingEngine& renderer) override;

	WPtr<Mesh> GetMesh() const { return mMesh; }
	void SetMesh(const SPtr<Mesh>& mesh) { mMesh = mesh; }
	void SetMesh(const WPtr<Mesh>& mesh) { mMesh = mesh; }

	Material& GetMaterial() { return mMaterial; }
	void SetMaterial(const Material& material) { mMaterial = material; }

private:
	WPtr<Mesh> mMesh;
	Material mMaterial; // TODO: this should be an asset
};
