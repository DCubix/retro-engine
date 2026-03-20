#include <SDL3/SDL.h>
#include <iostream>

#include "core/core_engine.h"
#include "core/asset_manager.h"
#include "core/window.h"
#include "rendering/rendering_engine.h"
#include "rendering/shaders/generated/dissolve_glsl.h"
#include "utils/string_utils.h"

#include "external/stb_image/stb_image.h"

#include "logic/scene.h"
#include "logic/components/model.h"
#include "logic/components/transform.h"
#include "logic/components/camera.h"
#include "logic/components/light_source.h"

Vector3 hsv2rgb(Vector3 hsv)
{
	hsv.y = fminf(fmaxf(hsv.y, 0.0f), 1.0f);
	hsv.z = fminf(fmaxf(hsv.z, 0.0f), 1.0f);
	float c = hsv.y * hsv.z;
	float x = c * (1 - fabsf(fmodf(hsv.x / 60.0f, 2) - 1));
	float m = hsv.z - c;
	Vector3 rgb;
	if (hsv.x < 60) {
		rgb = { c, x, 0 };
	}
	else if (hsv.x < 120) {
		rgb = { x, c, 0 };
	}
	else if (hsv.x < 180) {
		rgb = { 0, c, x };
	}
	else if (hsv.x < 240) {
		rgb = { 0, x, c };
	}
	else if (hsv.x < 300) {
		rgb = { x, 0, c };
	}
	else {
		rgb = { c, 0, x };
	}
	return rgb + Vector3{ m, m, m };
}

float Random() {
	return static_cast<float>(rand()) / static_cast<float>(RAND_MAX);
}

float RandomFloat(float vmin, float vmax) {
	return vmin + Random() * (vmax - vmin);
}

class DissolveMaterial : public MaterialShader {
public:
	DissolveMaterial()
		: MaterialShader(dissolve_glsl_src) {}

	void OnBeforeRender() override {
		SetUniform1f("uAmount", amount);
		SetUniformTextureHandle("uNoise", noiseTexture.lock()->GetBindlessHandle());
	}

	float amount{ 0.0f };
	WPtr<Texture> noiseTexture;
};

class CameraOrbitControl : public Component {
public:
	CameraOrbitControl(u32 windowWidth, u32 windowHeight)
		: windowWidth(windowWidth), windowHeight(windowHeight) {}

	void OnUpdate(InputSystem& input, f32 deltaTime) override {
		Transform* camTransform = GetOwner()->GetComponent<Transform>();
		Transform* pivot = camTransform->GetParent();
		Camera* camera = GetOwner()->GetComponent<Camera>();

		auto mouse = input.GetDevice<Mouse>();

		auto lmb = mouse->GetButtonState(Mouse::Button::left);
		if (lmb.isPressed) {
			prevMouse = mouse->GetPosition();
			isDragging = true;
		}
		else if (!lmb.isHeldDown) {
			isDragging = false;
		}

		if (isDragging) {
			auto mousePos = mouse->GetPosition();
			f32 deltaX = (f32(mousePos.x - prevMouse.x) / 150.0f);
			f32 deltaY = (f32(mousePos.y - prevMouse.y) / 150.0f);
			
			// pivot rotates around the Y and X axis
			pivot->RotateGlobal({ 0.0f, 1.0f, 0.0f }, -deltaX);
			pivot->RotateLocal({ 1.0f, 0.0f, 0.0f }, -deltaY);

			//mouse->SetPosition({ halfW, halfH });
			prevMouse.x = mousePos.x;
			prevMouse.y = mousePos.y;
		}

		auto localPos = camTransform->GetPosition();
		localPos.z -= mouse->GetScroll().y * 0.25f;
		camTransform->SetPosition(localPos);
		//camera->SetFoV(camera->GetFoV() - mouse->GetScroll().y * 0.9f);
		//camera->SetFoV(std::clamp(camera->GetFoV(), 20.0f, 90.0f));
	}

	bool isDragging{ false };
	u32 windowWidth, windowHeight;
	Vector2 prevMouse{ 0.0f, 0.0f };
};

class TestApplication : public IApplication {
public:
	void OnStart(CoreEngine& engine, RenderingEngine& renderer) override {
		FileSystem::Mount("assets", "/");

		//mesh = AssetManager::Get().GetMesh("/cat.obj");

		//diffuseTex = AssetManager::Get().GetTexture("/textures/orange_cat.1001.png");
		//normalTex = AssetManager::Get().GetTexture("/textures/concrete_cat_statue_nor_gl_4k.png");
		//roughnessTex = AssetManager::Get().GetTexture("/textures/concrete_cat_statue_rough_4k.png");
		//metallicTex = AssetManager::Get().GetTexture("/textures/concrete_cat_statue_metal_4k.png");
		//emissiveTex = AssetManager::Get().GetTexture("/textures/concrete_cat_statue_emis_4k.png");
		//noiseTex = AssetManager::Get().GetTexture("/textures/noise.jpg");

		//material.SetTexture(TextureSlot::diffuse, diffuseTex);
		//material.SetTexture(TextureSlot::normal, normalTex);
		//material.SetTexture(TextureSlot::roughness, roughnessTex);
		//material.SetTexture(TextureSlot::metallic, metallicTex);
		//material.SetTexture(TextureSlot::emissive, emissiveTex);

		//materialShader = std::make_shared<DissolveMaterial>();
		//materialShader->noiseTexture = noiseTex;
		//material.customShader = materialShader;

		scene = AssetManager::Get().LoadSceneJSON("/scene/test.json");

		// Create many cats in a grid
		//scene = std::make_unique<Scene>();

		//const u32 maxCount = 8;
		//for (int i = 0; i < maxCount; ++i) {
		//	for (int j = 0; j < maxCount; ++j) {
		//		f32 x = ((f32(i) / maxCount) * 2.0f - 1.0f) * 2.5f * (f32(maxCount) / 2.0f);
		//		f32 y = ((f32(j) / maxCount) * 2.0f - 1.0f) * 2.5f * (f32(maxCount) / 2.0f);
		//		auto entity = scene->Create();
		//		auto transform = entity->AddComponent<Transform>();
		//		transform->SetPosition({ x, -0.5f, y });
		//		transform->SetScale({ 0.5f, 0.5f, 0.5f });
		//		entity->AddComponent<Model>(mesh, material);
		//	}
		//}

		eng = &engine;
		auto [w, h] = engine.GetWindow()->GetSize();

		// Create camera entity
		cameraCenter = scene->Create();
		camera = scene->Create();
		camera->AddComponent<Camera>();
		Transform* pivot = cameraCenter->AddComponent<Transform>();
		Transform* camTransform = camera->AddComponent<Transform>();
		camTransform->SetParent(pivot);
		camTransform->SetPosition({ 0.0f, 0.0f, 5.0f });
		camera->AddComponent<CameraOrbitControl>(w, h);

		// get the Spot light and parent to the camera
		/*auto light = scene->FindByTag("Light.002");
		auto lightTransform = light->GetComponent<Transform>();
		
		camTransform->SetParent(lightTransform);
		camTransform->LookAt({ 1.0f, 0.0f, 0.0f }, { 0.0f, 1.0f, 0.0f });*/

		auto camera = scene->FindByTag("Camera");
		camera->GetComponent<Camera>()->SetActive(false);
		//auto camTransform = camera->GetComponent<Transform>();
		//camTransform->SetPosition({ 0.0f, 4.0f, 5.0f });

		// Post processing filters
		blurFilter = std::make_unique<BlurFilter>();
		blurFilter->SetPassCount(8);
		blurFilter->SetLod(2.0f);
		blurFilter->SetInput(FilterInput::emissiveMetallic);

		combineFilter = std::make_unique<CombineFilter>();
		combineFilter->SetCombineMode(CombineMode::add);

		renderer.AddFilter(blurFilter.get());
		renderer.AddFilter(combineFilter.get());

		windowWidth = w;
		windowHeight = h;
	}

	void OnStop() override {
	}

	void OnUpdate(InputSystem& input, f32 deltaTime) override {
		time += deltaTime;

		//materialShader->amount = ::sinf(time) * 0.5f + 0.5f;

		scene->Update(input, deltaTime);
	}

	void OnRender(RenderingEngine& renderer) override {
		scene->Render(renderer);

		// auto dd = renderer.GetDebugDraw();

		// str stats = "";
		
		// stats += "[/deepskyblue]CORE ENGINE\n";
		// stats += "[/darkgray]-------------------------------------[/reset]\n";

		// auto totalTimeCE = eng->GetProfiler().GetTotalTime();
		// for (const auto& [name, data] : eng->GetProfiler().GetData()) {
		// 	float percent = data.duration.count() / totalTimeCE.count() * 100.0f;
		// 	stats += fmt::format("[/gray]{}:[/reset] {:.2f}ms ({:.0f}%)\n", name, data.duration.count(), percent);
		// }
		// stats += "[/darkgray]-------------------------------------\n";
		// stats += fmt::format("[/yellow:60]Total:[/yellow] {:.2f}ms\n\n", totalTimeCE.count());

		// stats += "[/deepskyblue]RENDERING ENGINE\n";
		// stats += "[/darkgray]-------------------------------------[/reset]\n";

		// auto totalTime = renderer.GetProfiler().GetTotalTime();
		// for (const auto& [name, data] : renderer.GetProfiler().GetData()) {
		// 	float percent = data.duration.count() / totalTime.count() * 100.0f;
		// 	stats += fmt::format("[/gray]{}:[/reset] {:.2f}ms ({:.0f}%)\n", name, data.duration.count(), percent);
		// }
		// stats += fmt::format("[/yellow:50]Total:[/yellow] {:.2f}ms\n", totalTime.count());

		// dd->String(10, 10, stats, 1.0f);
	}

	SPtr<Mesh> mesh;
	SPtr<Texture> diffuseTex, normalTex, roughnessTex, metallicTex, emissiveTex, noiseTex;

	f32 time{ 0.0f };
	f32 spawnTime{ 0.0f };

	UPtr<BlurFilter> blurFilter;
	UPtr<CombineFilter> combineFilter;

	Material material{
		.roughness = 1.0f,
		.metallic = 1.0f,
		.emissive = 10.0f,
	};
	SPtr<DissolveMaterial> materialShader;

	UPtr<Scene> scene;

	Entity* cameraCenter{ nullptr };
	Entity* camera{ nullptr };

	u32 windowWidth{ 800 };
	u32 windowHeight{ 600 };

	CoreEngine* eng;
};

int main(int argc, const char** argv) {
    WindowConfiguration windowConfig{
        .title = "My Game",
        .width = 1280,
        .height = 720,
    };
    UPtr<CoreEngine> coreEngine = std::make_unique<CoreEngine>(
		new Window(windowConfig),
		new TestApplication()
	);

    coreEngine->Start(); // Start the core engine
    return 0;
}
