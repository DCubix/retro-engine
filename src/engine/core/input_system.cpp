#include "input_system.h"

Mouse::Mouse()
{
	MessageBus::Get().RegisterListener(this);
}

void Mouse::OnProcessEvent(SDL_Event& event)
{
	switch (event.type) {
		case SDL_MOUSEBUTTONDOWN:
		{
			u8 button = event.button.button;
			mButtons[button].isPressed = true;
			mButtons[button].isHeldDown = true;
			mPosition.x = event.button.x;
			mPosition.y = event.button.y;
		} break;
		case SDL_MOUSEBUTTONUP:
		{
			u8 button = event.button.button;
			mButtons[button].isReleased = true;
			mButtons[button].isHeldDown = false;
			mPosition.x = event.button.x;
			mPosition.y = event.button.y;
		} break;
		case SDL_MOUSEMOTION:
		{
			mPosition.x = event.motion.x;
			mPosition.y = event.motion.y;
		} break;
		case SDL_MOUSEWHEEL:
		{
			mScroll.x = event.wheel.preciseX;
			mScroll.y = event.wheel.preciseY;
			mPosition.x = event.wheel.mouseX;
			mPosition.y = event.wheel.mouseY;
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
		case SDL_KEYDOWN:
		{
			u32 keyCode = event.key.keysym.sym;
			mKeys[keyCode].isPressed = true;
			mKeys[keyCode].isHeldDown = true;
		} break;
		case SDL_KEYUP:
		{
			u32 keyCode = event.key.keysym.sym;
			mKeys[keyCode].isReleased = true;
			mKeys[keyCode].isHeldDown = false;
		} break;
		case SDL_TEXTINPUT:
		{
			if (SDL_IsTextInputActive()) {
				mTextEditState.text += event.text.text;
				mTextEditState.cursorPosition += ::strlen(event.text.text);
				mTextEditState.selectionLength = 0;
			}
		} break;
		case SDL_TEXTEDITING:
		{
			if (SDL_IsTextInputActive()) {
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

void Keyboard::BeginTextInput()
{
	SDL_StartTextInput();
	mTextEditState.text.clear();
	mTextEditState.cursorPosition = 0;
	mTextEditState.selectionLength = 0;
}

void Keyboard::EndTextInput() const
{
	SDL_StopTextInput();
}
