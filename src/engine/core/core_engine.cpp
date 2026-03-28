#include "core_engine.h"

#include "asset_manager.h"
#include "../rendering/rendering_engine.h"

CoreEngine::CoreEngine(Window* window, IApplication* application)
{
    MessageBus::Get().RegisterListener(this);

	mApplication.reset(application);
	mWindow.reset(window);

    auto [width, height] = mWindow->GetSize();
	mRenderingEngine = std::make_unique<RenderingEngine>(width, height);
	mInputSystem = std::make_unique<InputSystem>();

    FileSystem::Init();

	mLastTime = SDL_GetTicks() / 1000.0;
}

void CoreEngine::Start()
{
    mIsRunning = true;
	MainLoopThread(); // Start the main loop thread
}

void CoreEngine::Stop()
{
    mIsRunning = false;
}

void CoreEngine::Tick()
{
	f64 currentTime = SDL_GetTicks() / 1000.0;
	f64 deltaTime = currentTime - mLastTime;
	mLastTime = currentTime;
	mAccumulatedTime += deltaTime;

	ProcessInput();

	if (UpdateLogic()) {
		RenderFrame();
	}
}

void CoreEngine::RenderFrame()
{
	if (!mRenderingEngine) return;

	mRenderingEngine->GetDebugDraw()->Begin(mRenderingEngine->GetViewPosition());

	mProfiler.Begin("Render Dispatch");
	mApplication->OnRender(*mRenderingEngine.get());
	mProfiler.End("Render Dispatch");

	mProfiler.Begin("API Render");
	mRenderingEngine->Render();
	mProfiler.End("API Render");

	mProfiler.Begin("Debug Drawing");
	mRenderingEngine->GetDebugDraw()->End(mRenderingEngine->GetViewProjectionMatrix());
	mProfiler.End("Debug Drawing");

	mProfiler.Begin("Post Render");
	mApplication->OnPostRender(*mRenderingEngine.get());
	mProfiler.End("Post Render");

	mWindow->SwapBuffers();
}

bool CoreEngine::UpdateLogic()
{
	mProfiler.Begin("Game Logic");

	const f32 runtimeTime = 1.0f / mFrameRate;

	bool canRender = false;
	while (mAccumulatedTime >= runtimeTime) {
		mApplication->OnUpdate(*mInputSystem.get(), runtimeTime);
		mInputSystem->ResetDevices();
		mAccumulatedTime -= runtimeTime;

		mCleanupTime += runtimeTime;
		if (mCleanupTime >= 10.0f) {
			AssetManager::Get().CleanUp();
			mCleanupTime = 0.0f;
		}

		canRender = true;
	}

	mProfiler.End("Game Logic");

	return canRender;
}

void CoreEngine::ProcessInput()
{
	if (!mInputSystem) return;

	SDL_Event event;

	mProfiler.Begin("Input Processing");

	// input/event loop
	while (SDL_PollEvent(&event)) {
		if (event.type == SDL_EVENT_QUIT) {
			Stop(); // Stop the engine if the window is closed
		}
		mApplication->OnEvent(event);
		mInputSystem->ProcessEvent(event);
	}
	mProfiler.End("Input Processing");
}

void CoreEngine::OnMessage(const Message& message)
{
	auto text = message.GetText();
	auto responseText = "re:" + text;

	if (text == "CE_quit") {
		Stop();
	}
	else if (text == "CE_get_input_system") {
		Respond(responseText, mInputSystem.get());
	}
	else if (text == "CE_get_rendering_engine") {
		Respond(responseText, mRenderingEngine.get());
	}
	else if (text == "CE_get_window") {
		Respond(responseText, mWindow.get());
	}
	else if (text == "CE_get_application") {
		Respond(responseText, mApplication.get());
	}
}

void CoreEngine::MainLoopThread()
{
	mAccumulatedTime = 0.0;

	mApplication->OnStart(*this, *mRenderingEngine.get()); // Call the OnStart function of the application
    while (mIsRunning) {
		Tick(); // Run the main loop tick
    }

	mApplication->OnStop(); // Call the OnStop function of the application
    FileSystem::Deinit();
}