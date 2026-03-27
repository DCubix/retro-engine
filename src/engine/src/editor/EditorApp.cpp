#include "EditorApp.h"

#include "../../external/glad/glad.h"
#include <fmt/format.h>

// Font candidates: prefer a bundled path, fall back to a common system font.
static const char* k_font_paths[] = {
    "assets/fonts/LatoLatin-Regular.ttf",
    "assets/fonts/Roboto-Regular.ttf",
    "/usr/share/fonts/truetype/dejavu/DejaVuSans.ttf",
    "/usr/share/fonts/truetype/liberation/LiberationSans-Regular.ttf",
    nullptr
};

bool EditorApp::Init(SDL_Window* window, int width, int height) {
    m_width  = width;
    m_height = height;

    // Install our interfaces before calling Rml::Initialise()
    Rml::SetRenderInterface(&m_renderer);
    Rml::SetSystemInterface(&m_system_interface);

    if (!Rml::Initialise()) {
        fmt::print(stderr, "[EditorApp] Failed to initialise RmlUi.\n");
        return false;
    }

    if (!m_renderer.Init()) {
        fmt::print(stderr, "[EditorApp] Failed to initialise RmlRenderer.\n");
        Rml::Shutdown();
        return false;
    }

    // Load a font — try several fallback paths
    bool font_loaded = false;
    for (int i = 0; k_font_paths[i] != nullptr; ++i) {
        if (Rml::LoadFontFace(k_font_paths[i])) {
            fmt::print("[EditorApp] Loaded font: {}\n", k_font_paths[i]);
            font_loaded = true;
            break;
        }
    }
    if (!font_loaded) {
        fmt::print(stderr, "[EditorApp] Warning: no font loaded. Text may not render.\n");
    }

    m_context = Rml::CreateContext("editor", Rml::Vector2i(width, height));
    if (!m_context) {
        fmt::print(stderr, "[EditorApp] Failed to create RmlUi context.\n");
        Rml::Shutdown();
        return false;
    }

    SetupProjection();

    // Load the editor layout document
    Rml::ElementDocument* document = m_context->LoadDocument("assets/editor/EditorLayout.rml");
    if (!document) {
        fmt::print(stderr, "[EditorApp] Failed to load EditorLayout.rml.\n");
        return false;
    }
    document->Show();

    fmt::print("[EditorApp] Editor initialised ({}x{}).\n", width, height);
    return true;
}

void EditorApp::SetupProjection() {
    if (!m_context) return;

    // Build an orthographic projection matching the RmlUi coordinate system
    // (origin top-left, Y down) for the vertex shader.
    float L = 0.0f,  R = static_cast<float>(m_width);
    float T = 0.0f,  B = static_cast<float>(m_height);
    float N = -1.0f, F = 1.0f;

    // Column-major orthographic matrix
    Rml::Matrix4f ortho = Rml::Matrix4f::ProjectOrtho(L, R, B, T, N, F);

    // The renderer stores this as the current transform applied to each draw call
    m_renderer.SetTransform(&ortho);
}

void EditorApp::Update() {
    if (m_context) {
        m_context->Update();
    }
}

void EditorApp::Render() {
    if (m_context) {
        m_context->Render();
    }
}

void EditorApp::Shutdown() {
    m_renderer.Shutdown();
    if (m_context) {
        Rml::RemoveContext("editor");
        m_context = nullptr;
    }
    Rml::Shutdown();
}

void EditorApp::InjectEvent(const SDL_Event& event) {
    RmlSystemInterface::InjectSDLEvent(m_context, event);
}

void EditorApp::Resize(int width, int height) {
    m_width  = width;
    m_height = height;
    if (m_context) {
        m_context->SetDimensions(Rml::Vector2i(width, height));
        SetupProjection();
    }
}
