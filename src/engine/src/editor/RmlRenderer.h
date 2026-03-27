#pragma once

#include <RmlUi/Core.h>
#include <unordered_map>

/// Holds GPU resources for a single compiled geometry object.
struct CompiledGeometry {
    unsigned int vao{ 0 };
    unsigned int vbo{ 0 };
    unsigned int ibo{ 0 };
    int num_indices{ 0 };
};

/// OpenGL 3.3 render backend for RmlUi.
/// Implements Rml::RenderInterface to draw RmlUi geometry using a simple
/// passthrough GLSL shader with VAO/VBO, and handles texture management.
class RmlRenderer : public Rml::RenderInterface {
public:
    RmlRenderer();
    ~RmlRenderer() override;

    /// Initialize OpenGL resources (shaders, VAO/VBO). Must be called after
    /// an OpenGL context is current.
    bool Init();
    void Shutdown();

    // --- Required: Rml::RenderInterface (RmlUi 6.x compiled-geometry API) ---
    Rml::CompiledGeometryHandle CompileGeometry(Rml::Span<const Rml::Vertex> vertices,
                                                Rml::Span<const int> indices) override;
    void RenderGeometry(Rml::CompiledGeometryHandle geometry,
                        Rml::Vector2f translation,
                        Rml::TextureHandle texture) override;
    void ReleaseGeometry(Rml::CompiledGeometryHandle geometry) override;

    Rml::TextureHandle LoadTexture(Rml::Vector2i& texture_dimensions,
                                   const Rml::String& source) override;
    Rml::TextureHandle GenerateTexture(Rml::Span<const Rml::byte> source,
                                       Rml::Vector2i source_dimensions) override;
    void ReleaseTexture(Rml::TextureHandle texture) override;

    void EnableScissorRegion(bool enable) override;
    void SetScissorRegion(Rml::Rectanglei region) override;

    // --- Optional ---
    void SetTransform(const Rml::Matrix4f* transform) override;

private:
    unsigned int m_shader{ 0 };

    int m_uniform_transform{ -1 };
    int m_uniform_translate{ -1 };
    int m_uniform_tex{ -1 };
    int m_uniform_use_tex{ -1 };

    Rml::Matrix4f m_transform;

    // Maps CompiledGeometryHandle -> CompiledGeometry GPU objects
    std::unordered_map<Rml::CompiledGeometryHandle, CompiledGeometry> m_geometries;
    Rml::CompiledGeometryHandle m_next_geometry_handle{ 1 };

    // Maps TextureHandle -> GL texture ID
    std::unordered_map<Rml::TextureHandle, unsigned int> m_textures;
    Rml::TextureHandle m_next_texture_handle{ 1 };

    void SaveGLState();
    void RestoreGLState();
    void CompileShaderProgram();
};

