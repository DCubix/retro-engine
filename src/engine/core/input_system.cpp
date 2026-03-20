#include "input_system.h"

Mouse::Mouse()
{
	MessageBus::Get().RegisterListener(this);
}

void Mouse::OnProcessEvent(SDL_Event& event)
{
	switch (event.type) {
		case SDL_EVENT_MOUSE_BUTTON_DOWN:
		{
			u8 button = event.button.button;
			mButtons[button].isPressed = true;
			mButtons[button].isHeldDown = true;
			mPosition.x = event.button.x;
			mPosition.y = event.button.y;
		} break;
		case SDL_EVENT_MOUSE_BUTTON_UP:
		{
			u8 button = event.button.button;
			mButtons[button].isReleased = true;
			mButtons[button].isHeldDown = false;
			mPosition.x = event.button.x;
			mPosition.y = event.button.y;
		} break;
		case SDL_EVENT_MOUSE_MOTION:
		{
			mPosition.x = event.motion.x;
			mPosition.y = event.motion.y;
		} break;
		case SDL_EVENT_MOUSE_WHEEL:
		{
			mScroll.x += event.wheel.x;
			mScroll.y += event.wheel.y;
			mPosition.x = event.wheel.mouse_x;
			mPosition.y = event.wheel.mouse_y;
		} break;
	}
}

void Mouse::OnReset()
{
	mScroll = { 0.0f, 0.0f };
	for (auto& button : mButtons) {
		button.isPressed = false;
		button.isReleased = false;
	}
}

void Mouse::SetPosition(const Vector2& position) {
	mPosition = position;
	SendMessage("WIN_set_cursor_position", Tup<i32, i32>{ i32(position.x), i32(position.y) });
}

InputSystem::InputSystem()
{
	InstallDevice<Mouse>();
	InstallDevice<Keyboard>();
}

void InputSystem::ProcessEvent(SDL_Event& event) {
	for (auto& [_, device] : mInputDevices) {
		device->OnProcessEvent(event);
	}
}

void InputSystem::ResetDevices() {
	for (auto& [_, device] : mInputDevices) {
		device->OnReset();
	}
}

void Keyboard::OnProcessEvent(SDL_Event& event)
{
	switch (event.type) {
		case SDL_EVENT_KEY_DOWN:
		{
			u32 keyCode = event.key.key;
			if (!event.key.repeat) {
				mKeys[keyCode].isPressed = true;
			}
			mKeys[keyCode].isHeldDown = true;
		} break;
		case SDL_EVENT_KEY_UP:
		{
			u32 keyCode = event.key.key;
			mKeys[keyCode].isReleased = true;
			mKeys[keyCode].isHeldDown = false;
		} break;
		case SDL_EVENT_TEXT_INPUT:
		{
			if (SDL_TextInputActive(mWindow)) {
				mTextEditState.text += event.text.text;
				mTextEditState.cursorPosition += ::strlen(event.text.text);
				mTextEditState.selectionLength = 0;
			}
		} break;
		case SDL_EVENT_TEXT_EDITING:
		{
			if (SDL_TextInputActive(mWindow)) {
				mTextEditState.cursorPosition = event.edit.start;
				mTextEditState.selectionLength = event.edit.length;
				mTextEditState.text = event.edit.text;
			}
		} break;
	}
}

void Keyboard::OnReset()
{
	for (auto& [_, state] : mKeys) {
		state.isPressed = false;
		state.isReleased = false;
	}
}

void Keyboard::BeginTextInput(SDL_Window* window)
{
	mWindow = window;
	SDL_StartTextInput(window);
	mTextEditState.text.clear();
	mTextEditState.cursorPosition = 0;
	mTextEditState.selectionLength = 0;
}

void Keyboard::EndTextInput() const
{
	SDL_StopTextInput(mWindow);
}
