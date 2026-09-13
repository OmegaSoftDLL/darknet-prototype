#pragma once
// ─────────────────────────────────────────────────────────────────────────────
// GfxResource — wrappers RAII para recursos gráficos da raylib.
//
// Evitam vazamentos quando uma exceção ocorre durante a carga ou quando um
// objeto é destruído em caminho de erro. Cada wrapper é movable-only e libera
// o recurso no destrutor se ainda for dono dele.
// ─────────────────────────────────────────────────────────────────────────────
#include <raylib.h>
#include <utility>

class GfxModel {
public:
    GfxModel() = default;
    explicit GfxModel(Model m) : m_(m) {}

    GfxModel(GfxModel&& other) noexcept : m_(other.release()) {}
    GfxModel& operator=(GfxModel&& other) noexcept {
        if (this != &other) {
            reset();
            m_ = other.release();
        }
        return *this;
    }

    ~GfxModel() { reset(); }

    GfxModel(const GfxModel&) = delete;
    GfxModel& operator=(const GfxModel&) = delete;

    Model& get() { return m_; }
    const Model& get() const { return m_; }

    bool valid() const { return m_.meshCount > 0; }
    explicit operator bool() const { return valid(); }

    // Transfere ownership sem liberar. Zera o outro.
    Model release() {
        Model tmp = m_;
        m_ = Model{};
        return tmp;
    }

    void reset() {
        if (valid()) {
            UnloadModel(m_);
            m_ = Model{};
        }
    }

private:
    Model m_ = {};
};

class GfxTexture {
public:
    GfxTexture() = default;
    explicit GfxTexture(Texture2D t) : t_(t) {}

    GfxTexture(GfxTexture&& other) noexcept : t_(other.release()) {}
    GfxTexture& operator=(GfxTexture&& other) noexcept {
        if (this != &other) {
            reset();
            t_ = other.release();
        }
        return *this;
    }

    ~GfxTexture() { reset(); }

    GfxTexture(const GfxTexture&) = delete;
    GfxTexture& operator=(const GfxTexture&) = delete;

    Texture2D& get() { return t_; }
    const Texture2D& get() const { return t_; }

    bool valid() const { return t_.id > 0; }
    explicit operator bool() const { return valid(); }

    Texture2D release() {
        Texture2D tmp = t_;
        t_ = Texture2D{};
        return tmp;
    }

    void reset() {
        if (valid()) {
            UnloadTexture(t_);
            t_ = Texture2D{};
        }
    }

private:
    Texture2D t_ = {};
};

class GfxRenderTexture {
public:
    GfxRenderTexture() = default;
    explicit GfxRenderTexture(RenderTexture2D rt) : rt_(rt) {}

    GfxRenderTexture(GfxRenderTexture&& other) noexcept : rt_(other.release()) {}
    GfxRenderTexture& operator=(GfxRenderTexture&& other) noexcept {
        if (this != &other) {
            reset();
            rt_ = other.release();
        }
        return *this;
    }

    ~GfxRenderTexture() { reset(); }

    GfxRenderTexture(const GfxRenderTexture&) = delete;
    GfxRenderTexture& operator=(const GfxRenderTexture&) = delete;

    RenderTexture2D& get() { return rt_; }
    const RenderTexture2D& get() const { return rt_; }

    bool valid() const { return rt_.id > 0; }
    explicit operator bool() const { return valid(); }

    RenderTexture2D release() {
        RenderTexture2D tmp = rt_;
        rt_ = RenderTexture2D{};
        return tmp;
    }

    void reset() {
        if (valid()) {
            UnloadRenderTexture(rt_);
            rt_ = RenderTexture2D{};
        }
    }

private:
    RenderTexture2D rt_ = {};
};

class GfxShader {
public:
    GfxShader() = default;
    explicit GfxShader(Shader s) : s_(s) {}

    GfxShader(GfxShader&& other) noexcept : s_(other.release()) {}
    GfxShader& operator=(GfxShader&& other) noexcept {
        if (this != &other) {
            reset();
            s_ = other.release();
        }
        return *this;
    }

    ~GfxShader() { reset(); }

    GfxShader(const GfxShader&) = delete;
    GfxShader& operator=(const GfxShader&) = delete;

    Shader& get() { return s_; }
    const Shader& get() const { return s_; }

    bool valid() const { return IsShaderValid(s_); }
    explicit operator bool() const { return valid(); }

    Shader release() {
        Shader tmp = s_;
        s_ = Shader{};
        return tmp;
    }

    void reset() {
        if (valid()) {
            UnloadShader(s_);
            s_ = Shader{};
        }
    }

private:
    Shader s_ = {};
};
