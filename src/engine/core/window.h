#pragma once

#include "SDL.h"
#include "../utils/custom_types.h"
#include "messaging.h"

#include <windows.h>
#undef near
#undef far

struct RENGINE_API WindowConfiguration {
    str title;
    u32 width;
    u32 height;
    bool fullScreen;
    bool resizable;
    HWND windowHandle{ nullptr };
};

const WindowConfiguration defaultWindowConfig = {
    .title = "Retro Engine",
    .width = 800,
    .height = 600,
    .fullScreen = false,
    .resizable = false
};

class RENGINE_API Window : public Listener {
public:
    Window(const WindowConfiguration& config = defaultWindowConfig);
    ~Window();

    Tup<u32, u32> GetSize() const;
    void SetSize(u32 width, u32 height);

    const str& GetTitle() const;
    void SetTitle(const str& title);

	void SwapBuffers() const;

    void OnMessage(const Message& message) override;
private:
    SDL_Window* mWindow{ nullptr };
	SDL_GLContext mContext{ nullptr };
};