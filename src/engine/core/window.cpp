#include "window.h"

#include "../external/glad/glad.h"
#include <stdexcept>
#include <fmt/core.h>
#include <fmt/color.h>
#include <fmt/printf.h>

void APIENTRY GLDebugMessageCallback(GLenum source, GLenum type, GLuint id,
                                     GLenum severity, GLsizei length,
                                     const GLchar* message, const void* userParam) {
    // Choose color based on severity
    if (severity == GL_DEBUG_SEVERITY_HIGH) {
        fmt::print(fmt::fg(fmt::color::red), "[OpenGL Error] {}\n", message);
    }
    else if (severity == GL_DEBUG_SEVERITY_MEDIUM) {
        fmt::print(fmt::fg(fmt::color::orange), "[OpenGL Warning] {}\n", message);
    }
    else if (severity == GL_DEBUG_SEVERITY_LOW) {
        fmt::print(fmt::fg(fmt::color::yellow), "[OpenGL Notice] {}\n", message);
    }
    else if (severity == GL_DEBUG_SEVERITY_NOTIFICATION) {
        fmt::print(fmt::fg(fmt::color::light_blue), "[OpenGL Info] {}\n", message);
    }
    else {
        // Unknown severity, print normally
        fmt::print("[OpenGL Debug] {}\n", message);
    }
}

Window::Window(const WindowConfiguration& config)
{
	MessageBus::Get().RegisterListener(this);

    SDL_SetHint(SDL_HINT_VIDEO_FOREIGN_WINDOW_OPENGL, "1");

    if (SDL_Init(SDL_INIT_VIDEO) != 0) {
        throw std::runtime_error("Failed to initialize SDL: " + str(SDL_GetError()));
    }

    Uint32 flags = SDL_WINDOW_OPENGL;
    if (config.fullScreen) {
        flags |= SDL_WINDOW_FULLSCREEN;
    }
    if (config.resizable) {
        flags |= SDL_WINDOW_RESIZABLE;
    }

    // configure OpenGL 4.6
	SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, 4);
	SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, 6);
	SDL_GL_SetAttribute(SDL_GL_CONTEXT_PROFILE_MASK, SDL_GL_CONTEXT_PROFILE_CORE);

	// color and depth buffer
	SDL_GL_SetAttribute(SDL_GL_RED_SIZE, 8);
	SDL_GL_SetAttribute(SDL_GL_GREEN_SIZE, 8);
	SDL_GL_SetAttribute(SDL_GL_BLUE_SIZE, 8);
	SDL_GL_SetAttribute(SDL_GL_ALPHA_SIZE, 8);
	SDL_GL_SetAttribute(SDL_GL_DEPTH_SIZE, 24);
	SDL_GL_SetAttribute(SDL_GL_STENCIL_SIZE, 8);
	SDL_GL_SetAttribute(SDL_GL_DOUBLEBUFFER, 1);

    // Multisample
	SDL_GL_SetAttribute(SDL_GL_MULTISAMPLEBUFFERS, 1);
	SDL_GL_SetAttribute(SDL_GL_MULTISAMPLESAMPLES, 4);

    // debug mode
#ifdef IS_DEBUG
	SDL_GL_SetAttribute(SDL_GL_CONTEXT_FLAGS, SDL_GL_CONTEXT_DEBUG_FLAG);
#endif

    if (!config.windowHandle) {
        mWindow = SDL_CreateWindow(
            config.title.c_str(),
            SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED,
            config.width, config.height,
            flags
        );
    }
    else {
		mWindow = SDL_CreateWindowFrom(config.windowHandle);
		if (!mWindow) {
			throw std::runtime_error("Failed to create window from handle: " + str(SDL_GetError()));
		}
    }

    if (!mWindow) {
        throw std::runtime_error("Failed to create window: " + str(SDL_GetError()));
    }

	// create OpenGL context
	mContext = SDL_GL_CreateContext(mWindow);
	if (!mContext) {
		SDL_DestroyWindow(mWindow);
		throw std::runtime_error("Failed to create OpenGL context: " + str(SDL_GetError()));
	}

	// load OpenGL functions
	if (gladLoadGLLoader((GLADloadproc)SDL_GL_GetProcAddress) == 0) {
		SDL_GL_DeleteContext(mContext);
		SDL_DestroyWindow(mWindow);
		throw std::runtime_error("Failed to initialize OpenGL context: " + str(SDL_GetError()));
	}

#ifdef IS_DEBUG
	// enable OpenGL debug output
	glEnable(GL_DEBUG_OUTPUT);
	glDebugMessageCallback(GLDebugMessageCallback, nullptr);
	glDebugMessageControl(GL_DONT_CARE, GL_DONT_CARE, GL_DONT_CARE, 0, nullptr, GL_TRUE);
	glDebugMessageInsert(GL_DEBUG_SOURCE_APPLICATION, GL_DEBUG_TYPE_OTHER, 0, GL_DEBUG_SEVERITY_NOTIFICATION, -1, "OpenGL Debug Output Enabled");
#endif

	glEnable(GL_MULTISAMPLE);

    if (!config.windowHandle) SDL_ShowWindow(mWindow);
}

Window::~Window()
{
    if (mWindow) {
        SDL_DestroyWindow(mWindow);
        mWindow = nullptr;
    }
	if (mContext) {
		SDL_GL_DeleteContext(mContext);
		mContext = nullptr;
	}
    SDL_Quit();
}

Tup<u32, u32> Window::GetSize() const
{
    int width, height;
    SDL_GetWindowSize(mWindow, &width, &height);
    return { static_cast<u32>(width), static_cast<u32>(height) };
}

void Window::SetSize(u32 width, u32 height)
{
    SDL_SetWindowSize(mWindow, width, height);
}

const str& Window::GetTitle() const
{
    if (!mWindow) {
        throw std::runtime_error("Window is not initialized.");
    }
    return SDL_GetWindowTitle(mWindow);
}

void Window::SetTitle(const str& title)
{
    if (!mWindow) {
        throw std::runtime_error("Window is not initialized.");
    }
    SDL_SetWindowTitle(mWindow, title.c_str());
}

void Window::SwapBuffers() const
{
	if (!mWindow) {
		throw std::runtime_error("Window is not initialized.");
	}
	SDL_GL_SwapWindow(mWindow);
}

void Window::OnMessage(const Message& message)
{
	auto text = message.GetText();
	if (text == "WIN_resize") {
		auto size = message.GetData<Tup<u32, u32>>();
		SetSize(std::get<0>(size), std::get<1>(size));
	}
	else if (text == "WIN_set_title") {
		auto title = message.GetData<str>();
		SetTitle(title);
	}
	else if (text == "WIN_set_cursor_position") {
		auto pos = message.GetData<Tup<i32, i32>>();
		SDL_WarpMouseInWindow(mWindow, std::get<0>(pos), std::get<1>(pos));
	}
	else if (text == "WIN_set_cursor_visible") {
		auto visible = message.GetData<bool>();
		SDL_ShowCursor(visible ? SDL_ENABLE : SDL_DISABLE);
	}
}
