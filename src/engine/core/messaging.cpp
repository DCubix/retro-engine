#include "messaging.h"
#include <algorithm>

UPtr<MessageBus> MessageBus::mInstance;

void MessageBus::SendMessage(Listener* source, std::any data, const str& text)
{
	Message message{ source, text, data };
	for (auto& listener : mListeners) {
		if (listener && listener != source) {
			listener->OnMessage(message);
		}
	}
}

void MessageBus::Respond(Listener* source, std::any data, const str& text)
{
	Message response{ source, text, data };
	for (auto& listener : mListeners) {
		if (listener && listener != source) {
			listener->OnResponse(response);
		}
	}
}

void MessageBus::RegisterListener(Listener* listener)
{
	auto it = std::find(mListeners.begin(), mListeners.end(), listener);
	if (it == mListeners.end()) {
		mListeners.push_back(listener);
	}
}

void MessageBus::UnregisterListener(Listener* listener)
{
	auto it = std::find(mListeners.begin(), mListeners.end(), listener);
	if (it != mListeners.end()) {
		mListeners.erase(it);
	}
}

MessageBus& MessageBus::Get()
{
	if (!mInstance) {
		mInstance = std::make_unique<MessageBus>();
	}
	return *mInstance;
}

void Listener::Respond(const str& text, std::any data)
{
	MessageBus::Get().Respond(this, data, text);
}

void Listener::SendMessage(const str& text, std::any data)
{
	MessageBus::Get().SendMessage(this, data, text);
}
