#include "EditorApp.h"

#include <cfloat>
#include <imgui_impl_sdl3.h>
#include <imgui_impl_opengl3.h>
#include <fmt/format.h>

#include "../../rendering/rendering_engine.h"
#include "../../rendering/texture.h"

bool EditorApp::Init(SDL_Window* window, void* glContext) {
    IMGUI_CHECKVERSION();
    ImGui::CreateContext();

    ImGuiIO& io = ImGui::GetIO();
    io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;
    io.ConfigFlags |= ImGuiConfigFlags_DockingEnable;
    io.IniFilename = "editor_layout.ini";

    ApplyDarkTheme();

    if (!ImGui_ImplSDL3_InitForOpenGL(window, glContext)) {
        fmt::print(stderr, "[EditorApp] Failed to init ImGui SDL3 backend.\n");
        return false;
    }
    if (!ImGui_ImplOpenGL3_Init("#version 330 core")) {
        fmt::print(stderr, "[EditorApp] Failed to init ImGui OpenGL3 backend.\n");
        ImGui_ImplSDL3_Shutdown();
        return false;
    }

    m_initialised = true;
    return true;
}

void EditorApp::ApplyDarkTheme() {
    ImGui::StyleColorsDark();
    ImGuiStyle& s = ImGui::GetStyle();

    // Window / frame
    s.WindowRounding    = 2.0f;
    s.FrameRounding     = 2.0f;
    s.ScrollbarRounding = 2.0f;
    s.GrabRounding      = 2.0f;
    s.TabRounding       = 2.0f;
    s.WindowBorderSize  = 1.0f;
    s.FrameBorderSize   = 0.0f;
    s.ItemSpacing       = ImVec2(6, 4);
    s.FramePadding      = ImVec2(6, 3);

    ImVec4* c = s.Colors;

    // Background / base surfaces
    c[ImGuiCol_WindowBg]          = ImVec4(0.118f, 0.118f, 0.118f, 1.00f); // #1e1e1e
    c[ImGuiCol_ChildBg]           = ImVec4(0.145f, 0.145f, 0.149f, 1.00f); // #252526
    c[ImGuiCol_PopupBg]           = ImVec4(0.145f, 0.145f, 0.149f, 1.00f);
    c[ImGuiCol_Border]            = ImVec4(0.235f, 0.235f, 0.235f, 1.00f); // #3c3c3c
    c[ImGuiCol_FrameBg]           = ImVec4(0.200f, 0.200f, 0.200f, 1.00f); // ~#333333
    c[ImGuiCol_FrameBgHovered]    = ImVec4(0.270f, 0.270f, 0.270f, 1.00f);
    c[ImGuiCol_FrameBgActive]     = ImVec4(0.310f, 0.310f, 0.310f, 1.00f);

    // Title bars
    c[ImGuiCol_TitleBg]           = ImVec4(0.176f, 0.176f, 0.176f, 1.00f); // #2d2d2d
    c[ImGuiCol_TitleBgActive]     = ImVec4(0.118f, 0.118f, 0.118f, 1.00f);
    c[ImGuiCol_TitleBgCollapsed]  = ImVec4(0.176f, 0.176f, 0.176f, 1.00f);

    // Tabs
    c[ImGuiCol_Tab]               = ImVec4(0.176f, 0.176f, 0.176f, 1.00f);
    c[ImGuiCol_TabHovered]        = ImVec4(0.274f, 0.451f, 0.702f, 1.00f);
    c[ImGuiCol_TabSelected]       = ImVec4(0.118f, 0.118f, 0.118f, 1.00f);
    c[ImGuiCol_TabSelectedOverline]= ImVec4(0.027f, 0.478f, 0.800f, 1.00f); // #007acc accent
    c[ImGuiCol_TabDimmed]         = ImVec4(0.176f, 0.176f, 0.176f, 1.00f);
    c[ImGuiCol_TabDimmedSelected] = ImVec4(0.176f, 0.176f, 0.176f, 1.00f);

    // Headers (collapsible sections, etc.)
    c[ImGuiCol_Header]            = ImVec4(0.200f, 0.200f, 0.200f, 1.00f);
    c[ImGuiCol_HeaderHovered]     = ImVec4(0.274f, 0.451f, 0.702f, 0.80f);
    c[ImGuiCol_HeaderActive]      = ImVec4(0.027f, 0.478f, 0.800f, 1.00f);

    // Buttons
    c[ImGuiCol_Button]            = ImVec4(0.055f, 0.388f, 0.612f, 1.00f); // #0e639c
    c[ImGuiCol_ButtonHovered]     = ImVec4(0.067f, 0.467f, 0.733f, 1.00f); // #1177bb
    c[ImGuiCol_ButtonActive]      = ImVec4(0.027f, 0.478f, 0.800f, 1.00f);

    // Scrollbar / slider / resize
    c[ImGuiCol_ScrollbarBg]       = ImVec4(0.118f, 0.118f, 0.118f, 1.00f);
    c[ImGuiCol_ScrollbarGrab]     = ImVec4(0.259f, 0.259f, 0.259f, 1.00f);
    c[ImGuiCol_ScrollbarGrabHovered] = ImVec4(0.333f, 0.333f, 0.333f, 1.00f);
    c[ImGuiCol_CheckMark]         = ImVec4(0.027f, 0.478f, 0.800f, 1.00f);
    c[ImGuiCol_SliderGrab]        = ImVec4(0.027f, 0.478f, 0.800f, 1.00f);
    c[ImGuiCol_SliderGrabActive]  = ImVec4(0.102f, 0.541f, 0.851f, 1.00f);

    // Dockspace separator
    c[ImGuiCol_Separator]         = ImVec4(0.235f, 0.235f, 0.235f, 1.00f);
    c[ImGuiCol_SeparatorHovered]  = ImVec4(0.027f, 0.478f, 0.800f, 1.00f);
    c[ImGuiCol_SeparatorActive]   = ImVec4(0.027f, 0.478f, 0.800f, 1.00f);

    // Text
    c[ImGuiCol_Text]              = ImVec4(0.831f, 0.831f, 0.831f, 1.00f); // #d4d4d4
    c[ImGuiCol_TextDisabled]      = ImVec4(0.502f, 0.502f, 0.502f, 1.00f);

    // Menu
    c[ImGuiCol_MenuBarBg]         = ImVec4(0.196f, 0.196f, 0.200f, 1.00f); // #323233
}

void EditorApp::ProcessEvent(const SDL_Event& event) {
    if (m_initialised) {
        ImGui_ImplSDL3_ProcessEvent(&event);
    }
}

void EditorApp::NewFrame() {
    if (!m_initialised) return;
    ImGui_ImplOpenGL3_NewFrame();
    ImGui_ImplSDL3_NewFrame();
    ImGui::NewFrame();
}

void EditorApp::DrawUI(RenderingEngine& renderer) {
    if (!m_initialised) return;

    // Full-screen dockspace
    ImGuiViewport* vp = ImGui::GetMainViewport();
    ImGui::SetNextWindowPos(vp->WorkPos);
    ImGui::SetNextWindowSize(vp->WorkSize);
    ImGui::SetNextWindowViewport(vp->ID);

    ImGuiWindowFlags host_flags =
        ImGuiWindowFlags_NoTitleBar  | ImGuiWindowFlags_NoCollapse |
        ImGuiWindowFlags_NoResize    | ImGuiWindowFlags_NoMove     |
        ImGuiWindowFlags_NoBringToFrontOnFocus | ImGuiWindowFlags_NoNavFocus |
        ImGuiWindowFlags_MenuBar     | ImGuiWindowFlags_NoDocking;

    ImGui::PushStyleVar(ImGuiStyleVar_WindowRounding, 0.0f);
    ImGui::PushStyleVar(ImGuiStyleVar_WindowBorderSize, 0.0f);
    ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(0, 0));
    ImGui::Begin("##EditorRoot", nullptr, host_flags);
    ImGui::PopStyleVar(3);

    DrawMenuBar();

    ImGuiID dockspace_id = ImGui::GetID("MainDockSpace");
    ImGui::DockSpace(dockspace_id, ImVec2(0, 0), ImGuiDockNodeFlags_PassthruCentralNode);

    ImGui::End();

    // Individual panels
    DrawSceneHierarchy();
    DrawViewport(renderer);
    DrawInspector();
    DrawAssetBrowser();
    DrawConsole();
}

void EditorApp::DrawMenuBar() {
    if (ImGui::BeginMenuBar()) {
        if (ImGui::BeginMenu("File")) {
            ImGui::MenuItem("New Scene");
            ImGui::MenuItem("Open Scene...");
            ImGui::Separator();
            ImGui::MenuItem("Save Scene");
            ImGui::MenuItem("Save Scene As...");
            ImGui::Separator();
            ImGui::MenuItem("Exit");
            ImGui::EndMenu();
        }
        if (ImGui::BeginMenu("Edit")) {
            ImGui::MenuItem("Undo", "Ctrl+Z");
            ImGui::MenuItem("Redo", "Ctrl+Y");
            ImGui::EndMenu();
        }
        if (ImGui::BeginMenu("View")) {
            ImGui::MenuItem("Scene Hierarchy");
            ImGui::MenuItem("Inspector");
            ImGui::MenuItem("Asset Browser");
            ImGui::MenuItem("Console");
            ImGui::EndMenu();
        }
        if (ImGui::BeginMenu("Scene")) {
            ImGui::MenuItem("Add Entity");
            ImGui::EndMenu();
        }
        if (ImGui::BeginMenu("Help")) {
            ImGui::MenuItem("About");
            ImGui::EndMenu();
        }
        ImGui::EndMenuBar();
    }
}

void EditorApp::DrawViewport(RenderingEngine& renderer) {
    ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(0, 0));
    ImGui::Begin("Viewport");
    ImGui::PopStyleVar();

    ImVec2 size = ImGui::GetContentRegionAvail();
    Texture* sceneTex = renderer.GetSceneTexture();
    if (sceneTex && size.x > FLT_EPSILON && size.y > FLT_EPSILON) {
        // Flip UV vertically: OpenGL origin is bottom-left, ImGui is top-left
        ImGui::Image(
            (ImTextureID)(uintptr_t)sceneTex->GetID(),
            size,
            ImVec2(0.0f, 1.0f),
            ImVec2(1.0f, 0.0f)
        );
    }

    ImGui::End();
}

void EditorApp::DrawSceneHierarchy() {
    ImGui::Begin("Scene Hierarchy");
    ImGui::TextDisabled("(no scene loaded)");
    ImGui::End();
}

void EditorApp::DrawInspector() {
    ImGui::Begin("Inspector");
    ImGui::TextDisabled("(select an entity)");
    ImGui::End();
}

void EditorApp::DrawAssetBrowser() {
    ImGui::Begin("Asset Browser");
    ImGui::TextDisabled("(no assets)");
    ImGui::End();
}

void EditorApp::DrawConsole() {
    ImGui::Begin("Console");
    ImGui::TextDisabled("(no messages)");
    ImGui::End();
}

void EditorApp::Render() {
    if (!m_initialised) return;
    ImGui::Render();
    ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());
}

void EditorApp::Shutdown() {
    if (!m_initialised) return;
    ImGui_ImplOpenGL3_Shutdown();
    ImGui_ImplSDL3_Shutdown();
    ImGui::DestroyContext();
    m_initialised = false;
}
