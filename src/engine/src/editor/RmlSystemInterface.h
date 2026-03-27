#pragma once

#include <RmlUi/Core.h>
#include <SDL3/SDL.h>

/// SDL3 system interface for RmlUi.
/// Implements Rml::SystemInterface for timing and logging.
/// Also provides a static helper for injecting SDL3 events into an Rml::Context.
class RmlSystemInterface : public Rml::SystemInterface {
public:
    RmlSystemInterface() = default;
    ~RmlSystemInterface() override = default;

    // --- Rml::SystemInterface ---
    double GetElapsedTime() override;
    bool LogMessage(Rml::Log::Type type, const Rml::String& message) override;

    /// Map an SDL3 event to the corresponding RmlUi input calls on the given context.
    static void InjectSDLEvent(Rml::Context* context, const SDL_Event& event);

private:
    static Rml::Input::KeyIdentifier MapSDLKey(SDL_Keycode keycode);
    static int MapSDLModifiers(SDL_Keymod mod);
};
