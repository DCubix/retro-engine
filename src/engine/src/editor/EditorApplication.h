#pragma once

#include "EditorApp.h"
#include "../../core/core_engine.h"

/// IApplication wrapper that layers the ImGui editor on top of any game application.
/// CoreEngine only knows about IApplication — it has no direct dependency on the editor.
///
/// Usage in main.cpp:
///   coreEngine = new CoreEngine(window, new EditorApplication(std::make_unique<MyGameApp>()));
class EditorApplication : public IApplication {
public:
    /// Takes ownership of the wrapped application.
    explicit EditorApplication(UPtr<IApplication> wrapped);
    ~EditorApplication() override = default;

    void OnStart(CoreEngine& engine, RenderingEngine& renderer) override;
    void OnStop() override;
    void OnUpdate(InputSystem& input, f32 deltaTime) override;
    void OnRender(RenderingEngine& renderer) override;
    void OnPostRender(RenderingEngine& renderer) override;
    void OnEvent(const SDL_Event& event) override;

private:
    UPtr<IApplication> m_wrapped;
    EditorApp          m_editor;
};
