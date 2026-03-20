#pragma once

#include "messaging.h"
#include "../utils/custom_types.h"
#include "../external/raymath/raymath.h"

#include <SDL3/SDL.h>
#include <array>
#include <unordered_map>
#include <typeindex>
#include <concepts>

struct InputState {
	bool isPressed{ false };
	bool isHeldDown{ false };
	bool isReleased{ false };
};

class InputDevice {
public:
	virtual ~InputDevice() = default;
	virtual void OnProcessEvent(SDL_Event& event) = 0;
	virtual void OnReset() {};
};

class Mouse : public InputDevice, public Listener {
public:
	Mouse();

	enum class Button {
		left = SDL_BUTTON_LEFT,
		middle = SDL_BUTTON_MIDDLE,
		right = SDL_BUTTON_RIGHT,
		x1 = SDL_BUTTON_X1,
		x2 = SDL_BUTTON_X2,
	};

	void OnProcessEvent(SDL_Event& event) override;
	void OnReset() override;

	const InputState& GetButtonState(Button button) const {
		return mButtons[static_cast<u8>(button)];
	}

	const Vector2& GetPosition() const {
		return mPosition;
	}
	void SetPosition(const Vector2& position);

	const Vector2& GetScroll() const {
		return mScroll;
	}
private:
	std::array<InputState, 6> mButtons;
	Vector2 mPosition{ 0.0f, 0.0f };
	Vector2 mScroll{ 0.0f, 0.0f };
};

struct TextEditState {
	std::string text;
	u32 cursorPosition{ 0 };
	u32 selectionLength{ 0 };
};

class Keyboard : public InputDevice {
public:
	void OnProcessEvent(SDL_Event& event) override;
	void OnReset() override;

	InputState GetKeyState(u32 keyCode) const {
		auto it = mKeys.find(keyCode);
		if (it != mKeys.end()) {
			return it->second;
		}
		return InputState{};
	}

	const TextEditState& GetTextEditState() const {
		return mTextEditState;
	}

	void BeginTextInput(SDL_Window* window);
	void EndTextInput() const;
private:
	std::unordered_map<u32, InputState> mKeys;
	TextEditState mTextEditState;
	SDL_Window* mWindow{ nullptr };
};

using InputDeviceMap = std::unordered_map<std::type_index, UPtr<InputDevice>>;
class InputSystem {
public:
	InputSystem();
	~InputSystem() = default;

	// Make InputSystem non-copyable
	InputSystem(const InputSystem&) = delete;
	InputSystem& operator=(const InputSystem&) = delete;

	// Explicitly default move operations (optional but good practice)
	InputSystem(InputSystem&&) = default;
	InputSystem& operator=(InputSystem&&) = default;

	template <std::derived_from<InputDevice> T>
	T* GetDevice() {
		auto it = mInputDevices.find(typeid(T));
		if (it != mInputDevices.end()) {
			return static_cast<T*>(it->second.get());
		}
		return nullptr;
	}

	template <std::derived_from<InputDevice> T>
	void InstallDevice() {
		auto it = mInputDevices.find(typeid(T));
		if (it == mInputDevices.end()) {
			mInputDevices.emplace(typeid(T), std::make_unique<T>());
		}
	}

	void ProcessEvent(SDL_Event& event);
	void ResetDevices();

private:
	InputDeviceMap mInputDevices;
};