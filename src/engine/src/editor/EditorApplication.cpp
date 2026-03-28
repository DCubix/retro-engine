#include "EditorApplication.h"

#include "../../core/window.h"
#include "../../rendering/rendering_engine.h"
#include <fmt/format.h>

EditorApplication::EditorApplication(UPtr<IApplication> wrapped)
    : m_wrapped(std::move(wrapped)) {}

void EditorApplication::OnStart(CoreEngine& engine, RenderingEngine& renderer) {
    // Initialize the wrapped game application first
    m_wrapped->OnStart(engine, renderer);

    // Then initialize the ImGui editor using the engine's SDL window / GL context
    SDL_Window* window = engine.GetWindow()->GetSDLWindow();
    SDL_GLContext glCtx = SDL_GL_GetCurrentContext();
    if (!m_editor.Init(window, glCtx)) {
        fmt::print(stderr, "[EditorApplication] ImGui editor failed to initialize.\n");
    }
}

void EditorApplication::OnStop() {
    m_editor.Shutdown();
    m_wrapped->OnStop();
}

void EditorApplication::OnUpdate(InputSystem& input, f32 deltaTime) {
    m_wrapped->OnUpdate(input, deltaTime);
}

void EditorApplication::OnRender(RenderingEngine& renderer) {
    // Submit the wrapped app's scene draw calls
    m_wrapped->OnRender(renderer);
}

void EditorApplication::OnPostRender(RenderingEngine& renderer) {
    // Called by CoreEngine AFTER all GPU scene passes have run.
    // The scene texture is now ready to display in the viewport.
    m_editor.NewFrame();
    m_editor.DrawUI(renderer);
    m_editor.Render();
}

void EditorApplication::OnEvent(const SDL_Event& event) {
    m_editor.ProcessEvent(event);
    m_wrapped->OnEvent(event);
}
