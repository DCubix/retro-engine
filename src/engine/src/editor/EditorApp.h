#pragma once

#include <imgui.h>
#include <SDL3/SDL.h>

class RenderingEngine;
class Texture;

/// ImGui-based editor UI.
/// Owns the ImGui context, backends, and per-frame UI drawing.
/// Not coupled to CoreEngine — initialised and driven by EditorApplication.
class EditorApp {
public:
    EditorApp() = default;
    ~EditorApp() = default;

    /// Initialise ImGui with the SDL3 + OpenGL3 backends.
    /// Must be called with an active OpenGL context.
    bool Init(SDL_Window* window, void* glContext);

    /// Forward an SDL event to the ImGui SDL3 backend.
    void ProcessEvent(const SDL_Event& event);

    /// Begin a new ImGui frame (call before DrawUI).
    void NewFrame();

    /// Draw the full editor layout: dockspace + all panels.
    /// @param renderer   Used to retrieve the scene viewport texture.
    void DrawUI(RenderingEngine& renderer);

    /// Render the ImGui draw data to the current OpenGL context.
    void Render();

    /// Destroy ImGui and backends.
    void Shutdown();

private:
    bool m_initialised{ false };

    void ApplyDarkTheme();
    void DrawMenuBar();
    void DrawViewport(RenderingEngine& renderer);
    void DrawSceneHierarchy();
    void DrawInspector();
    void DrawAssetBrowser();
    void DrawConsole();
};
