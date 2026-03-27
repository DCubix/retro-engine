#pragma once

#include <SDL3/SDL.h>
#include "../utils/custom_types.h"
#include "messaging.h"

struct WindowConfiguration {
    str title;
    u32 width;
    u32 height;
    bool fullScreen;
    bool resizable;
};

const WindowConfiguration defaultWindowConfig = {
    .title = "Retro Engine",
    .width = 800,
    .height = 600,
    .fullScreen = false,
    .resizable = false
};

class Window : public Listener {
public:
    Window(const WindowConfiguration& config = defaultWindowConfig);
    ~Window();

    Tup<u32, u32> GetSize() const;
    void SetSize(u32 width, u32 height);

    str GetTitle() const;
    void SetTitle(const str& title);

	void SwapBuffers() const;

	SDL_Window* GetSDLWindow() const { return mWindow; }

    void OnMessage(const Message& message) override;
private:
    SDL_Window* mWindow{ nullptr };
	SDL_GLContext mContext{ nullptr };
};