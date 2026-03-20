#include "asset_manager.h"

#include "../external/stb_image/stb_image.h"

#include "../logic/components/transform.h"
#include "../logic/components/camera.h"
#include "../logic/components/model.h"
#include "../logic/components/light_source.h"

#include <filesystem>
#include "../logic/components/transform_animation.h"

// Singleton
UPtr<AssetManager> AssetManager::sInstance = nullptr;

void AssetManager::LoadTexture(const str& path, const Opt<TextureDescription>& description)
{
	// Check if the texture is already loaded
	if (mTextures.find(path) != mTextures.end()) {
		utils::LogW("Texture already loaded: {}", path);
		return;
	}

	File fp(path);
	if (!fp.IsOpen()) {
		utils::LogE("Failed to open texture file: {}", path);
		return;
	}

	std::vector<byte> fileData = fp.ReadAll();
	
	int w, h, comp;
	byte* data = stbi_load_from_memory(fileData.data(), fileData.size(), &w, &h, &comp, 4);
	if (!data) {
		utils::LogE("Failed to load texture: {}", stbi_failure_reason());
		return;
	}

	TextureDescription textureDesc = description.value_or(TextureDescription{
		.type = TextureType::texture2D,
		.samplerDescription = {
			.wrapS = TextureWrapMode::repeat,
			.wrapT = TextureWrapMode::repeat,
			.minFilter = TextureFilterMode::linearMipmapLinear,
			.magFilter = TextureFilterMode::linear,
		},
		.format = TextureFormat::RGBA,
	});
	textureDesc.width = u32(w);
	textureDesc.height = u32(h);
	textureDesc.layers[0] = data;

	LoadTexture(path, new Texture(textureDesc));

	stbi_image_free(data);

#ifdef IS_DEBUG
	utils::LogD("Loaded texture: {} ({}, {})", path, w, h);
#endif
}

void AssetManager::LoadTexture(
	const std::vector<byte>& fileData,
	const str& path,
	bool uncompressed,
	u32 width, u32 height,
	const Opt<TextureDescription>& description
)
{
	TextureDescription textureDesc = description.value_or(TextureDescription{
		.type = TextureType::texture2D,
		.samplerDescription = {
			.wrapS = TextureWrapMode::repeat,
			.wrapT = TextureWrapMode::repeat,
			.minFilter = TextureFilterMode::linearMipmapLinear,
			.magFilter = TextureFilterMode::linear,
		},
		.format = TextureFormat::RGBA,
	});

	if (!uncompressed) {
		int w, h, comp;
		byte* data = stbi_load_from_memory(fileData.data(), fileData.size(), &w, &h, &comp, 4);
		if (!data) {
			utils::LogE("Failed to load texture: {}", stbi_failure_reason());
			return;
		}
		textureDesc.width = u32(w);
		textureDesc.height = u32(h);
		textureDesc.layers[0] = data;
		LoadTexture(path, new Texture(textureDesc));
		stbi_image_free(data);
	}
	else {
		textureDesc.width = width;
		textureDesc.height = height;
		textureDesc.layers[0] = const_cast<byte*>(fileData.data());
		LoadTexture(path, new Texture(textureDesc));
	}
}

str AssetManager::LoadMesh(const str& path)
{
	const str name = std::filesystem::path(path).stem().string();
	if (mMeshes.find(name) != mMeshes.end()) {
		return name;
	}

	File file(path);
	if (!file.IsOpen()) {
		utils::Fail("Failed to open 3D model: {}", path);
		return "";
	}

	std::vector<byte> data = file.ReadAll();

	Assimp::Importer importer;
	const aiScene* scene = importer.ReadFileFromMemory(
		data.data(), data.size(),
		aiProcess_Triangulate |
		aiProcess_FlipUVs |
		aiProcess_JoinIdenticalVertices |
		aiProcess_CalcTangentSpace |
		aiProcess_FixInfacingNormals
	);

	if (!scene) {
		utils::Fail("Failed to load 3D model: {}", importer.GetErrorString());
		return "";
	}

	if (scene->HasMeshes()) {
		aiMesh* mesh = scene->mMeshes[0];
		LoadMesh(name, mesh); // load the first mesh for now, it's sufficient
#ifdef IS_DEBUG
		utils::LogD("Loaded mesh: {}", path);
#endif
		return name;
	}
	else {
		utils::LogE("No meshes found in the scene: {}", path);
	}

	return "";
}

void AssetManager::LoadMesh(const str& name, aiMesh* mesh)
{
	std::vector<Vertex> vertices;
	std::vector<u32> indices;

	for (u32 i = 0; i < mesh->mNumVertices; i++) {
		Vertex v{};
		
		aiVector3D pos = mesh->HasPositions() ? mesh->mVertices[i] : aiVector3D(0.0f);
		aiVector3D normal = mesh->HasNormals() ? mesh->mNormals[i] : aiVector3D(0.0f);
		aiVector3D tangent = mesh->HasTangentsAndBitangents() ? mesh->mTangents[i] : aiVector3D(0.0f);
		aiColor4D color = mesh->HasVertexColors(0) ? mesh->mColors[0][i] : aiColor4D(1.0f);
		aiVector3D texCoord = mesh->HasTextureCoords(0) ? mesh->mTextureCoords[0][i] : aiVector3D(0.0f);

		v.position = Vector3{ pos.x, pos.y, pos.z };
		v.normal = Vector3{ normal.x, normal.y, normal.z };
		v.tangent = Vector3{ tangent.x, tangent.y, tangent.z };
		v.color = Vector4{ color.r, color.g, color.b, color.a };
		v.texCoord = Vector2{ texCoord.x, texCoord.y };

		vertices.push_back(v);
	}

	for (u32 i = 0; i < mesh->mNumFaces; i++) {
		aiFace face = mesh->mFaces[i];
		for (u32 j = 0; j < face.mNumIndices; j++) {
			indices.push_back(face.mIndices[j]);
		}
	}

	MeshDescription desc{};
	desc.vertices = vertices;
	desc.indices = indices;
	desc.isStatic = true;

	LoadMesh(name, new Mesh(desc));
}

void AssetManager::LoadTexture(const str& path, Texture* texture)
{
	if (!texture) {
		utils::LogE("Texture is null");
		return;
	}
	mTextures[path] = SPtr<Texture>(texture);
	utils::LogI("Loaded texture: {}", path);
}

void AssetManager::LoadMesh(const str& path, Mesh* mesh)
{
	if (!mesh) {
		utils::LogE("Mesh is null: {}", path);
		return;
	}
	mMeshes[path] = SPtr<Mesh>(mesh);
}

int GetNodeCamera(const aiScene* scene, aiNode* node) {
	for (u32 i = 0; i < scene->mNumCameras; i++) {
		if (scene->mCameras[i] && scene->mCameras[i]->mName == node->mName) {
			return i;
		}
	}
	return -1;
}

int GetNodeLight(const aiScene* scene, aiNode* node) {
	for (u32 i = 0; i < scene->mNumLights; i++) {
		if (scene->mLights[i] && scene->mLights[i]->mName == node->mName) {
			return i;
		}
	}
	return -1;
}

aiMatrix4x4 GetNodeWorldTransform(aiNode* node) {
	aiMatrix4x4 transform = node->mTransformation;
	while (node->mParent) {
		node = node->mParent;
		transform = node->mTransformation * transform;
	}
	return transform;
}

f32 ComputeLightRadius(f32 constant, f32 linear, f32 quadratic, f32 threshold = 0.001f) {
	f32 A = quadratic;
	f32 B = linear;
	f32 C = constant - 1.0f / threshold;

	float discriminant = B * B - 4.0f * A * C;
	if (discriminant < 0.0f || A == 0.0f) {
		return 0.0f; // Light never reaches threshold
	}

	return (-B + std::sqrt(discriminant)) / (2.0f * A);
}

void AssetManager::ParseJSONObject(const json& data, const str& baseDir, Scene* reScene, Transform* parent)
{
	if (data.is_null()) {
		utils::LogE("Failed to parse JSON scene file");
		return;
	}
	for (const auto& obj : data) {
		Vector3 position, scale;
		Quaternion rotation;

		auto pos = obj["position"];
		if (pos.is_array() && pos.size() == 3) {
			position = Vector3{ pos[0].get<f32>(), pos[1].get<f32>(), pos[2].get<f32>() };
		}

		auto rot = obj["rotation"];
		if (rot.is_array() && rot.size() == 4) {
			rotation = Quaternion{ rot[0].get<f32>(), rot[1].get<f32>(), rot[2].get<f32>(), rot[3].get<f32>() };
		}

		auto scl = obj["scale"];
		if (scl.is_array() && scl.size() == 3) {
			scale = Vector3{ scl[0].get<f32>(), scl[1].get<f32>(), scl[2].get<f32>() };
		}

		const str name = obj["name"].get<str>();
		auto entity = reScene->Create();
		entity->SetTag(name);

		auto transform = entity->AddComponent<Transform>();
		transform->SetParent(parent);

		transform->SetRotation(rotation);
		transform->SetPosition(position);
		transform->SetScale(scale);

		ParseJSONBasicAnimation(obj, entity);

		if (obj["type"] == "mesh") {
			const str path = baseDir + "/" + obj["path"].get<str>();
			Material mat{};
			const str meshName = LoadMesh(path);

			// load material from JSON
			ParseJSONMaterial(obj["material"], baseDir, &mat);

			entity->AddComponent<Model>(GetMesh(meshName), mat);
		}
		else if (obj["type"] == "camera") {
			auto camera = entity->AddComponent<Camera>();
			// rotate 90 degrees around X axis
			transform->RotateLocal({ 1.0f, 0.0f, 0.0f }, -90.0f * DEG2RAD);
			camera->SetFoV(obj["fovy"].get<f32>() * RAD2DEG);
			camera->SetZNear(obj["znear"].get<f32>());
			camera->SetZFar(obj["zfar"].get<f32>());
		}
		else if (obj["type"] == "light") {
			auto light = entity->AddComponent<LightSource>();
			light->SetColor(Vector3{ obj["color"][0], obj["color"][1], obj["color"][2] });
			light->SetRadius(obj["radius"].get<f32>());
			light->SetIntensity(obj["intensity"].get<f32>());
			if (obj.contains("spot_angle")) {
				light->SetCutoffAngle(obj["spot_angle"].get<f32>());
			}
			light->SetShadowCaster(obj["shadows"].get<bool>());

			auto kind = obj["kind"].get<str>();
			if (kind == "directional") {
				light->SetType(LightType::directional);
			}
			else if (kind == "point") {
				light->SetType(LightType::point);
			}
			else if (kind == "spot") {
				light->SetType(LightType::spot);
			}
			else {
				utils::LogE("Unknown light type: {}", kind);
			}
		}

		if (obj.contains("children")) {
			ParseJSONObject(obj["children"], baseDir, reScene, transform);
		}
	}
}

void AssetManager::ParseJSONMaterial(const json& data, const str& baseDir, Material* material)
{
	// load textures (loop keys of data["textures"])
	for (auto& [key, value] : data["textures"].items()) {
		TextureSlot slot = TextureSlot::diffuse;
		if (key == "diffuse") {
			slot = TextureSlot::diffuse;
		}
		else if (key == "normal") {
			slot = TextureSlot::normal;
		}
		else if (key == "roughness") {
			slot = TextureSlot::roughness;
		}
		else if (key == "metallic") {
			slot = TextureSlot::metallic;
		}
		else if (key == "emissive") {
			slot = TextureSlot::emissive;
		}
		else if (key == "displacement") {
			slot = TextureSlot::displacement;
		}

		const str path = baseDir + "/" + value.get<str>();
		LoadTexture(path, Opt<TextureDescription>());

		material->SetTexture(slot, GetTexture(path));
	}

	// load other material properties
	if (data.contains("roughness")) {
		material->roughness = data["roughness"].get<f32>();
	}
	if (data.contains("metallic")) {
		material->metallic = data["metallic"].get<f32>();
	}	
	if (data.contains("emissive")) {
		material->emissive = data["emissive"].get<f32>();
	}
	if (data.contains("diffuseColor")) {
		auto color = data["diffuseColor"];
		material->diffuseColor = Vector4{
			color[0].get<f32>(),
			color[1].get<f32>(),
			color[2].get<f32>(),
			color[3].get<f32>()
		};
	}
	if (data.contains("castsShadows")) {
		material->castsShadows = data["castsShadows"].get<bool>();
	}
}

void AssetManager::ParseJSONBasicAnimation(const json& data, Entity* entity)
{
	const u32 animationFPS = data["animationFPS"].get<u32>();
	const auto& keyframes = data["animation"];

	if (!keyframes.is_array()) return;

	auto xfAnim = entity->AddComponent<TransformAnimation>();
	xfAnim->SetFrameRate(animationFPS);

	for (const auto& keyframe : keyframes) {
		const u32 frame = keyframe["frame"].get<u32>();
		const auto& pos = keyframe["position"];
		const auto& rot = keyframe["rotation"];
		const auto& scl = keyframe["scale"];

		Vector3 position{ pos[0].get<f32>(), pos[1].get<f32>(), pos[2].get<f32>() };
		Quaternion rotation{ rot[0].get<f32>(), rot[1].get<f32>(), rot[2].get<f32>(), rot[3].get<f32>() };
		Vector3 scale{ scl[0].get<f32>(), scl[1].get<f32>(), scl[2].get<f32>() };
		xfAnim->AddKeyframe(frame, position, rotation, scale);
		xfAnim->Play();
	}
}

UPtr<Scene> AssetManager::LoadSceneJSON(const str& fileName)
{
	File file(fileName);
	if (!file.IsOpen()) {
		utils::Fail("Failed to open scene file: {}", fileName);
		return nullptr;
	}
	UPtr<Scene> reScene = std::make_unique<Scene>();

	std::vector<byte> rawData = file.ReadAll();
	str jsonStr(rawData.begin(), rawData.end());
	json data = json::parse(jsonStr);

	if (data.is_null()) {
		utils::LogE("Failed to parse JSON scene file: {}", fileName);
		return nullptr;
	}

	const str baseDir = std::filesystem::path(fileName).parent_path().string();
	ParseJSONObject(data, baseDir, reScene.get(), nullptr);

	return reScene;
}

SPtr<Texture> AssetManager::GetTexture(const str& path)
{
	auto it = mTextures.find(path);
	if (it != mTextures.end()) {
		return it->second;
	}

	// Try loading it
	LoadTexture(path, Opt<TextureDescription>());

	auto it2 = mTextures.find(path);
	if (it2 != mTextures.end()) {
		return it2->second;
	}

	utils::LogE("Texture not found: {}", path);
	return nullptr;
}

SPtr<Mesh> AssetManager::GetMesh(const str& path)
{
	auto it = mMeshes.find(path);
	if (it != mMeshes.end()) {
		return it->second;
	}

	// Try loading it
	LoadMesh(path);

	auto it2 = mMeshes.find(path);
	if (it2 != mMeshes.end()) {
		return it2->second;
	}

	utils::LogE("Mesh not found: {}", path);
	return nullptr;
}

void AssetManager::CleanUp()
{
	// remove any textures that are not used
	for (auto it = mTextures.begin(); it != mTextures.end();) {
		if (it->second.use_count() == 0) {
#ifdef IS_DEBUG
			utils::LogD("Unloading texture: {}", it->first);
#endif
			it = mTextures.erase(it);
		}
		else {
			++it;
		}
	}

	// remove any meshes that are not used
	for (auto it = mMeshes.begin(); it != mMeshes.end();) {
		if (it->second.use_count() == 0) {
#ifdef IS_DEBUG
			utils::LogD("Unloading mesh: {}", it->first);
#endif
			it = mMeshes.erase(it);
		}
		else {
			++it;
		}
	}
}

AssetManager& AssetManager::Get()
{
	if (!sInstance) {
		sInstance = std::make_unique<AssetManager>();
	}
	return *sInstance;
}
