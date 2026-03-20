#pragma once

#include "../utils/custom_types.h"
#include "../utils/physfs_wrapper.h"

#include "../rendering/mesh.h"
#include "../rendering/texture.h"
#include "../rendering/material.h"

#include "../logic/scene.h"

#include <assimp/Importer.hpp>
#include <assimp/postprocess.h>
#include <assimp/scene.h>
#include <assimp/types.h>
#include <assimp/material.h>
#include <assimp/texture.h>

#include "../external/json.hpp"
using json = nlohmann::json;

#include <unordered_map>

template<typename T>
using Dict = std::unordered_map<str, T>;

class Transform;
class AssetManager {
public:
	void LoadTexture(const str& path, const Opt<TextureDescription>& description);
	void LoadTexture(
		const std::vector<byte>& fileData,
		const str& path,
		bool uncompressed = false,
		u32 width = 0, u32 height = 0,
		const Opt<TextureDescription>& description = {}
	);
	
	str LoadMesh(const str& path);
	void LoadMesh(const str& name, aiMesh* mesh);

	void LoadTexture(const str& path, Texture* texture);
	void LoadMesh(const str& path, Mesh* mesh);

	UPtr<Scene> LoadSceneJSON(const str& fileName);

	SPtr<Texture> GetTexture(const str& path);
	SPtr<Mesh> GetMesh(const str& path);

	void CleanUp();

	static AssetManager& Get();

private:
	Dict<SPtr<Mesh>> mMeshes;
	Dict<SPtr<Texture>> mTextures;

	// Singleton
	static UPtr<AssetManager> sInstance;

	void ParseJSONObject(const json& data, const str& baseDir, Scene* reScene, Transform* parent);
	void ParseJSONMaterial(const json& data, const str& baseDir, Material* material);
	void ParseJSONBasicAnimation(const json& data, Entity* entity);
};