#include "RmlSystemInterface.h"

#include <RmlUi/Core/Input.h>
#include <fmt/format.h>

double RmlSystemInterface::GetElapsedTime() {
    return static_cast<double>(SDL_GetTicks()) / 1000.0;
}

bool RmlSystemInterface::LogMessage(Rml::Log::Type type, const Rml::String& message) {
    const char* prefix = "[RmlUi]";
    switch (type) {
        case Rml::Log::LT_ERROR:
        case Rml::Log::LT_ASSERT:
            fmt::print(stderr, "{} ERROR: {}\n", prefix, message);
            break;
        case Rml::Log::LT_WARNING:
            fmt::print(stderr, "{} WARNING: {}\n", prefix, message);
            break;
        default:
            fmt::print("{} {}\n", prefix, message);
            break;
    }
    return true;
}

void RmlSystemInterface::InjectSDLEvent(Rml::Context* context, const SDL_Event& event) {
    if (!context) return;

    switch (event.type) {
        case SDL_EVENT_MOUSE_MOTION: {
            context->ProcessMouseMove(
                static_cast<int>(event.motion.x),
                static_cast<int>(event.motion.y),
                MapSDLModifiers(SDL_GetModState())
            );
            break;
        }
        case SDL_EVENT_MOUSE_BUTTON_DOWN: {
            context->ProcessMouseButtonDown(event.button.button - 1,
                                            MapSDLModifiers(SDL_GetModState()));
            break;
        }
        case SDL_EVENT_MOUSE_BUTTON_UP: {
            context->ProcessMouseButtonUp(event.button.button - 1,
                                          MapSDLModifiers(SDL_GetModState()));
            break;
        }
        case SDL_EVENT_MOUSE_WHEEL: {
            context->ProcessMouseWheel(
                Rml::Vector2f(-event.wheel.x, -event.wheel.y),
                MapSDLModifiers(SDL_GetModState())
            );
            break;
        }
        case SDL_EVENT_KEY_DOWN: {
            context->ProcessKeyDown(
                MapSDLKey(event.key.key),
                MapSDLModifiers(event.key.mod)
            );
            // Also forward printable characters as text input if not handled by TEXT_INPUT
            break;
        }
        case SDL_EVENT_KEY_UP: {
            context->ProcessKeyUp(
                MapSDLKey(event.key.key),
                MapSDLModifiers(event.key.mod)
            );
            break;
        }
        case SDL_EVENT_TEXT_INPUT: {
            context->ProcessTextInput(Rml::String(event.text.text));
            break;
        }
        default:
            break;
    }
}

int RmlSystemInterface::MapSDLModifiers(SDL_Keymod mod) {
    int modifiers = 0;
    if (mod & SDL_KMOD_CTRL)  modifiers |= Rml::Input::KM_CTRL;
    if (mod & SDL_KMOD_SHIFT) modifiers |= Rml::Input::KM_SHIFT;
    if (mod & SDL_KMOD_ALT)   modifiers |= Rml::Input::KM_ALT;
    if (mod & SDL_KMOD_GUI)   modifiers |= Rml::Input::KM_META;
    return modifiers;
}

Rml::Input::KeyIdentifier RmlSystemInterface::MapSDLKey(SDL_Keycode keycode) {
    using K = Rml::Input::KeyIdentifier;
    switch (keycode) {
        case SDLK_UNKNOWN:      return K::KI_UNKNOWN;
        case SDLK_SPACE:        return K::KI_SPACE;
        case SDLK_0:            return K::KI_0;
        case SDLK_1:            return K::KI_1;
        case SDLK_2:            return K::KI_2;
        case SDLK_3:            return K::KI_3;
        case SDLK_4:            return K::KI_4;
        case SDLK_5:            return K::KI_5;
        case SDLK_6:            return K::KI_6;
        case SDLK_7:            return K::KI_7;
        case SDLK_8:            return K::KI_8;
        case SDLK_9:            return K::KI_9;
        case SDLK_A:            return K::KI_A;
        case SDLK_B:            return K::KI_B;
        case SDLK_C:            return K::KI_C;
        case SDLK_D:            return K::KI_D;
        case SDLK_E:            return K::KI_E;
        case SDLK_F:            return K::KI_F;
        case SDLK_G:            return K::KI_G;
        case SDLK_H:            return K::KI_H;
        case SDLK_I:            return K::KI_I;
        case SDLK_J:            return K::KI_J;
        case SDLK_K:            return K::KI_K;
        case SDLK_L:            return K::KI_L;
        case SDLK_M:            return K::KI_M;
        case SDLK_N:            return K::KI_N;
        case SDLK_O:            return K::KI_O;
        case SDLK_P:            return K::KI_P;
        case SDLK_Q:            return K::KI_Q;
        case SDLK_R:            return K::KI_R;
        case SDLK_S:            return K::KI_S;
        case SDLK_T:            return K::KI_T;
        case SDLK_U:            return K::KI_U;
        case SDLK_V:            return K::KI_V;
        case SDLK_W:            return K::KI_W;
        case SDLK_X:            return K::KI_X;
        case SDLK_Y:            return K::KI_Y;
        case SDLK_Z:            return K::KI_Z;
        case SDLK_RETURN:       return K::KI_RETURN;
        case SDLK_ESCAPE:       return K::KI_ESCAPE;
        case SDLK_BACKSPACE:    return K::KI_BACK;
        case SDLK_TAB:          return K::KI_TAB;
        case SDLK_DELETE:       return K::KI_DELETE;
        case SDLK_INSERT:       return K::KI_INSERT;
        case SDLK_HOME:         return K::KI_HOME;
        case SDLK_END:          return K::KI_END;
        case SDLK_PAGEUP:       return K::KI_PRIOR;
        case SDLK_PAGEDOWN:     return K::KI_NEXT;
        case SDLK_LEFT:         return K::KI_LEFT;
        case SDLK_RIGHT:        return K::KI_RIGHT;
        case SDLK_UP:           return K::KI_UP;
        case SDLK_DOWN:         return K::KI_DOWN;
        case SDLK_F1:           return K::KI_F1;
        case SDLK_F2:           return K::KI_F2;
        case SDLK_F3:           return K::KI_F3;
        case SDLK_F4:           return K::KI_F4;
        case SDLK_F5:           return K::KI_F5;
        case SDLK_F6:           return K::KI_F6;
        case SDLK_F7:           return K::KI_F7;
        case SDLK_F8:           return K::KI_F8;
        case SDLK_F9:           return K::KI_F9;
        case SDLK_F10:          return K::KI_F10;
        case SDLK_F11:          return K::KI_F11;
        case SDLK_F12:          return K::KI_F12;
        case SDLK_LSHIFT:
        case SDLK_RSHIFT:       return K::KI_LSHIFT;
        case SDLK_LCTRL:
        case SDLK_RCTRL:        return K::KI_LCONTROL;
        case SDLK_LALT:
        case SDLK_RALT:         return K::KI_LMENU;
        default:                return K::KI_UNKNOWN;
    }
}
