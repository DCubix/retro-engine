#include "RmlRenderer.h"

#include "../../external/glad/glad.h"
#include <fmt/format.h>

// Saved GL state for restore around RmlUi rendering
struct SavedGLState {
    GLboolean blend;
    GLboolean cull_face;
    GLboolean depth_test;
    GLboolean scissor_test;
    GLint blend_src_rgb;
    GLint blend_dst_rgb;
    GLint blend_src_alpha;
    GLint blend_dst_alpha;
    GLint blend_eq_rgb;
    GLint blend_eq_alpha;
    GLint scissor_box[4];
    GLint viewport[4];
    GLint current_program;
    GLint vao;
    GLint vbo;
    GLint ibo;
    GLint active_texture;
    GLint texture_2d;
};

static SavedGLState s_saved_state{};

static const char* k_vertex_shader = R"glsl(
#version 330 core
uniform vec2 uTranslate;
uniform mat4 uTransform;

layout(location = 0) in vec2 aPos;
layout(location = 1) in vec4 aColor;
layout(location = 2) in vec2 aUV;

out vec4 vColor;
out vec2 vUV;

void main() {
    vColor = aColor / 255.0;
    vUV = aUV;
    gl_Position = uTransform * vec4(aPos + uTranslate, 0.0, 1.0);
}
)glsl";

static const char* k_fragment_shader = R"glsl(
#version 330 core
uniform sampler2D uTex;
uniform bool uUseTex;

in vec4 vColor;
in vec2 vUV;

out vec4 fragColor;

void main() {
    if (uUseTex) {
        fragColor = vColor * texture(uTex, vUV);
    } else {
        fragColor = vColor;
    }
}
)glsl";

static unsigned int CompileGLShader(GLenum type, const char* source) {
    unsigned int shader = glCreateShader(type);
    glShaderSource(shader, 1, &source, nullptr);
    glCompileShader(shader);

    GLint success;
    glGetShaderiv(shader, GL_COMPILE_STATUS, &success);
    if (!success) {
        char info[512];
        glGetShaderInfoLog(shader, 512, nullptr, info);
        fmt::print(stderr, "[RmlRenderer] Shader compile error: {}\n", info);
        glDeleteShader(shader);
        return 0;
    }
    return shader;
}

RmlRenderer::RmlRenderer() {}

RmlRenderer::~RmlRenderer() {
    Shutdown();
}

bool RmlRenderer::Init() {
    CompileShaderProgram();
    m_transform = Rml::Matrix4f::Identity();
    return m_shader != 0;
}

void RmlRenderer::Shutdown() {
    // Release all compiled geometries
    for (auto& [handle, geom] : m_geometries) {
        if (geom.vao) glDeleteVertexArrays(1, &geom.vao);
        if (geom.vbo) glDeleteBuffers(1, &geom.vbo);
        if (geom.ibo) glDeleteBuffers(1, &geom.ibo);
    }
    m_geometries.clear();

    // Release all textures
    for (auto& [handle, tex_id] : m_textures) {
        glDeleteTextures(1, &tex_id);
    }
    m_textures.clear();

    if (m_shader) { glDeleteProgram(m_shader); m_shader = 0; }
}

void RmlRenderer::CompileShaderProgram() {
    unsigned int vs = CompileGLShader(GL_VERTEX_SHADER, k_vertex_shader);
    unsigned int fs = CompileGLShader(GL_FRAGMENT_SHADER, k_fragment_shader);

    if (!vs || !fs) {
        if (vs) glDeleteShader(vs);
        if (fs) glDeleteShader(fs);
        return;
    }

    m_shader = glCreateProgram();
    glAttachShader(m_shader, vs);
    glAttachShader(m_shader, fs);
    glLinkProgram(m_shader);

    GLint success;
    glGetProgramiv(m_shader, GL_LINK_STATUS, &success);
    if (!success) {
        char info[512];
        glGetProgramInfoLog(m_shader, 512, nullptr, info);
        fmt::print(stderr, "[RmlRenderer] Program link error: {}\n", info);
        glDeleteProgram(m_shader);
        m_shader = 0;
    }

    glDeleteShader(vs);
    glDeleteShader(fs);

    if (m_shader) {
        m_uniform_transform = glGetUniformLocation(m_shader, "uTransform");
        m_uniform_translate = glGetUniformLocation(m_shader, "uTranslate");
        m_uniform_tex       = glGetUniformLocation(m_shader, "uTex");
        m_uniform_use_tex   = glGetUniformLocation(m_shader, "uUseTex");
    }
}

void RmlRenderer::SaveGLState() {
    s_saved_state.blend        = glIsEnabled(GL_BLEND);
    s_saved_state.cull_face    = glIsEnabled(GL_CULL_FACE);
    s_saved_state.depth_test   = glIsEnabled(GL_DEPTH_TEST);
    s_saved_state.scissor_test = glIsEnabled(GL_SCISSOR_TEST);
    glGetIntegerv(GL_BLEND_SRC_RGB,        &s_saved_state.blend_src_rgb);
    glGetIntegerv(GL_BLEND_DST_RGB,        &s_saved_state.blend_dst_rgb);
    glGetIntegerv(GL_BLEND_SRC_ALPHA,      &s_saved_state.blend_src_alpha);
    glGetIntegerv(GL_BLEND_DST_ALPHA,      &s_saved_state.blend_dst_alpha);
    glGetIntegerv(GL_BLEND_EQUATION_RGB,   &s_saved_state.blend_eq_rgb);
    glGetIntegerv(GL_BLEND_EQUATION_ALPHA, &s_saved_state.blend_eq_alpha);
    glGetIntegerv(GL_SCISSOR_BOX,          s_saved_state.scissor_box);
    glGetIntegerv(GL_VIEWPORT,             s_saved_state.viewport);
    glGetIntegerv(GL_CURRENT_PROGRAM,      &s_saved_state.current_program);
    glGetIntegerv(GL_VERTEX_ARRAY_BINDING, &s_saved_state.vao);
    glGetIntegerv(GL_ARRAY_BUFFER_BINDING, &s_saved_state.vbo);
    glGetIntegerv(GL_ELEMENT_ARRAY_BUFFER_BINDING, &s_saved_state.ibo);
    glGetIntegerv(GL_ACTIVE_TEXTURE,       &s_saved_state.active_texture);
    glActiveTexture(GL_TEXTURE0);
    glGetIntegerv(GL_TEXTURE_BINDING_2D,   &s_saved_state.texture_2d);
}

void RmlRenderer::RestoreGLState() {
    if (s_saved_state.blend)        glEnable(GL_BLEND); else glDisable(GL_BLEND);
    if (s_saved_state.cull_face)    glEnable(GL_CULL_FACE); else glDisable(GL_CULL_FACE);
    if (s_saved_state.depth_test)   glEnable(GL_DEPTH_TEST); else glDisable(GL_DEPTH_TEST);
    if (s_saved_state.scissor_test) glEnable(GL_SCISSOR_TEST); else glDisable(GL_SCISSOR_TEST);
    glBlendFuncSeparate(s_saved_state.blend_src_rgb,  s_saved_state.blend_dst_rgb,
                        s_saved_state.blend_src_alpha, s_saved_state.blend_dst_alpha);
    glBlendEquationSeparate(s_saved_state.blend_eq_rgb, s_saved_state.blend_eq_alpha);
    glScissor(s_saved_state.scissor_box[0], s_saved_state.scissor_box[1],
              s_saved_state.scissor_box[2], s_saved_state.scissor_box[3]);
    glViewport(s_saved_state.viewport[0], s_saved_state.viewport[1],
               s_saved_state.viewport[2], s_saved_state.viewport[3]);
    glUseProgram(s_saved_state.current_program);
    glBindVertexArray(s_saved_state.vao);
    glBindBuffer(GL_ARRAY_BUFFER, s_saved_state.vbo);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, s_saved_state.ibo);
    glActiveTexture(s_saved_state.active_texture);
    glBindTexture(GL_TEXTURE_2D, s_saved_state.texture_2d);
}

// --- Compiled geometry -------------------------------------------------------

Rml::CompiledGeometryHandle RmlRenderer::CompileGeometry(Rml::Span<const Rml::Vertex> vertices,
                                                          Rml::Span<const int> indices) {
    CompiledGeometry geom{};
    geom.num_indices = static_cast<int>(indices.size());

    glGenVertexArrays(1, &geom.vao);
    glGenBuffers(1, &geom.vbo);
    glGenBuffers(1, &geom.ibo);

    glBindVertexArray(geom.vao);

    glBindBuffer(GL_ARRAY_BUFFER, geom.vbo);
    glBufferData(GL_ARRAY_BUFFER,
                 static_cast<GLsizeiptr>(vertices.size() * sizeof(Rml::Vertex)),
                 vertices.data(), GL_STATIC_DRAW);

    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, geom.ibo);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER,
                 static_cast<GLsizeiptr>(indices.size() * sizeof(int)),
                 indices.data(), GL_STATIC_DRAW);

    // Position (vec2)
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, sizeof(Rml::Vertex),
                          (void*)offsetof(Rml::Vertex, position));
    // Color (4 bytes RGBA unsigned char)
    glEnableVertexAttribArray(1);
    glVertexAttribPointer(1, 4, GL_UNSIGNED_BYTE, GL_FALSE, sizeof(Rml::Vertex),
                          (void*)offsetof(Rml::Vertex, colour));
    // UV (vec2)
    glEnableVertexAttribArray(2);
    glVertexAttribPointer(2, 2, GL_FLOAT, GL_FALSE, sizeof(Rml::Vertex),
                          (void*)offsetof(Rml::Vertex, tex_coord));

    glBindVertexArray(0);

    Rml::CompiledGeometryHandle handle = m_next_geometry_handle++;
    m_geometries[handle] = geom;
    return handle;
}

void RmlRenderer::RenderGeometry(Rml::CompiledGeometryHandle geometry,
                                  Rml::Vector2f translation,
                                  Rml::TextureHandle texture) {
    auto it = m_geometries.find(geometry);
    if (it == m_geometries.end()) return;

    const CompiledGeometry& geom = it->second;

    SaveGLState();

    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    glDisable(GL_CULL_FACE);
    glDisable(GL_DEPTH_TEST);

    glUseProgram(m_shader);
    glUniformMatrix4fv(m_uniform_transform, 1, GL_FALSE, m_transform.data());
    glUniform2f(m_uniform_translate, translation.x, translation.y);

    bool use_tex = (texture != 0);
    glUniform1i(m_uniform_use_tex, use_tex ? 1 : 0);
    if (use_tex) {
        auto tex_it = m_textures.find(texture);
        if (tex_it != m_textures.end()) {
            glActiveTexture(GL_TEXTURE0);
            glBindTexture(GL_TEXTURE_2D, tex_it->second);
            glUniform1i(m_uniform_tex, 0);
        }
    }

    glBindVertexArray(geom.vao);
    glDrawElements(GL_TRIANGLES, geom.num_indices, GL_UNSIGNED_INT, nullptr);
    glBindVertexArray(0);

    RestoreGLState();
}

void RmlRenderer::ReleaseGeometry(Rml::CompiledGeometryHandle geometry) {
    auto it = m_geometries.find(geometry);
    if (it == m_geometries.end()) return;

    CompiledGeometry& geom = it->second;
    if (geom.vao) glDeleteVertexArrays(1, &geom.vao);
    if (geom.vbo) glDeleteBuffers(1, &geom.vbo);
    if (geom.ibo) glDeleteBuffers(1, &geom.ibo);

    m_geometries.erase(it);
}

// --- Textures ----------------------------------------------------------------

Rml::TextureHandle RmlRenderer::LoadTexture(Rml::Vector2i& texture_dimensions,
                                             const Rml::String& source) {
    // Texture loading from file paths is not implemented.
    // RmlUi will use GenerateTexture for font atlases, which is the main use case.
    (void)texture_dimensions;
    (void)source;
    return 0;
}

Rml::TextureHandle RmlRenderer::GenerateTexture(Rml::Span<const Rml::byte> source,
                                                  Rml::Vector2i source_dimensions) {
    GLuint tex_id;
    glGenTextures(1, &tex_id);
    glBindTexture(GL_TEXTURE_2D, tex_id);

    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA8,
                 source_dimensions.x, source_dimensions.y,
                 0, GL_RGBA, GL_UNSIGNED_BYTE, source.data());

    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);

    glBindTexture(GL_TEXTURE_2D, 0);

    Rml::TextureHandle handle = m_next_texture_handle++;
    m_textures[handle] = tex_id;
    return handle;
}

void RmlRenderer::ReleaseTexture(Rml::TextureHandle texture) {
    auto it = m_textures.find(texture);
    if (it != m_textures.end()) {
        glDeleteTextures(1, &it->second);
        m_textures.erase(it);
    }
}

// --- Scissor -----------------------------------------------------------------

void RmlRenderer::EnableScissorRegion(bool enable) {
    if (enable) {
        glEnable(GL_SCISSOR_TEST);
    } else {
        glDisable(GL_SCISSOR_TEST);
    }
}

void RmlRenderer::SetScissorRegion(Rml::Rectanglei region) {
    // RmlUi uses top-left origin; OpenGL uses bottom-left — flip Y.
    GLint viewport[4];
    glGetIntegerv(GL_VIEWPORT, viewport);
    int x      = region.Left();
    int y      = viewport[3] - region.Bottom();
    int width  = region.Width();
    int height = region.Height();
    glScissor(x, y, width, height);
}

// --- Transform ---------------------------------------------------------------

void RmlRenderer::SetTransform(const Rml::Matrix4f* transform) {
    if (transform) {
        m_transform = *transform;
    } else {
        m_transform = Rml::Matrix4f::Identity();
    }
}
