#pragma once

#include "messaging.h"
#include "input_system.h"
#include "window.h"
#include "../utils/custom_types.h"
#include "../utils/profiler.h"

#include <SDL3/SDL.h>

class CoreEngine;
class RenderingEngine;

class IApplication {
public:
	virtual ~IApplication() = default;
	/// This function is called when the application is started.
	/// It should be used to initialize the application.
	virtual void OnStart(CoreEngine& engine, RenderingEngine& renderer) = 0;
	/// This function is called when the application is stopped.
	/// It should be used to clean up the application.
	virtual void OnStop() = 0;
	/// This function is called every frame.
	/// It should be used to update the application.
	virtual void OnUpdate(InputSystem& input, f32 deltaTime) = 0;
	/// This function is called every frame for rendering
	/// It should be used to render the application.
	virtual void OnRender(RenderingEngine& renderer) = 0;
	/// Called after the engine's GPU render passes complete, before SwapBuffers.
	/// Override to draw overlay UI (e.g. ImGui) on top of the rendered scene.
	virtual void OnPostRender(RenderingEngine& renderer) {}
	/// Called for each SDL event before it is forwarded to the input system.
	/// Override to inject events into custom UI layers (e.g. ImGui).
	virtual void OnEvent(const SDL_Event& event) {}
};

/// This is the core (logic) engine, it runs on a separate thread from the rendering engine.
/// It is responsible for the game logic and the game loop.
/// All logic related to updating things should be done here.
class CoreEngine : public Listener {
public:
	CoreEngine() = default;
    ~CoreEngine() = default;

    CoreEngine(Window* window, IApplication* application);

    /// Starts the core engine and the main loop thread.
    /// This function will not block
    void Start();

    /// Stops the core engine and the main loop thread.
    void Stop();

	void Tick();

	Window* GetWindow() const { return mWindow.get(); }
	RenderingEngine* GetRenderingEngine() const { return mRenderingEngine.get(); }
	InputSystem* GetInputSystem() const { return mInputSystem.get(); }
	const Profiler& GetProfiler() const { return mProfiler; }

	void OnMessage(const Message& message) override;

private:
    bool mIsRunning{ false };
	f64 mLastTime{ 0.0 }, mAccumulatedTime{ 0.0 };
	f32 mCleanupTime{ 0.0f };

	f32 mFrameRate{ 60.0f }; // Default frame rate

	UPtr<IApplication> mApplication{ nullptr };
	UPtr<Window> mWindow{ nullptr };
	UPtr<RenderingEngine> mRenderingEngine{ nullptr };
	UPtr<InputSystem> mInputSystem{ nullptr };

	Profiler mProfiler{};

    void MainLoopThread();

	void RenderFrame();
	bool UpdateLogic();
	void ProcessInput();

};