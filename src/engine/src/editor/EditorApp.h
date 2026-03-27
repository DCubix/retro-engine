#pragma once

#include "RmlRenderer.h"
#include "RmlSystemInterface.h"

#include <RmlUi/Core.h>
#include <SDL3/SDL.h>

/// Top-level editor class that owns the RmlUi context and integrates with
/// the engine's SDL3/OpenGL setup.
class EditorApp {
public:
    EditorApp() = default;
    ~EditorApp() = default;

    /// Initialize RmlUi, load the editor document, and show it.
    /// Must be called after OpenGL context is current.
    /// @param window   The SDL3 window.
    /// @param width    Viewport width in pixels.
    /// @param height   Viewport height in pixels.
    bool Init(SDL_Window* window, int width, int height);

    /// Call each frame before Render() to update the RmlUi context.
    void Update();

    /// Call each frame (after the engine scene render) to render the UI.
    void Render();

    /// Call on shutdown to clean up RmlUi resources.
    void Shutdown();

    /// Forward an SDL3 event to the RmlUi context.
    void InjectEvent(const SDL_Event& event);

    /// Call when the window is resized.
    void Resize(int width, int height);

private:
    RmlRenderer         m_renderer;
    RmlSystemInterface  m_system_interface;
    Rml::Context*       m_context{ nullptr };

    int m_width{ 0 };
    int m_height{ 0 };

    void SetupProjection();
};
