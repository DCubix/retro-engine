#pragma once

#include "../utils/custom_types.h"
#include <any>
#include <vector>

class Listener;
class Message {
public:
	Message(Listener* source, const str& text, std::any data)
		: mSource(source), mText(text), mData(data)
	{}

	const str& GetText() const { return mText; }

	template <typename T>
	T GetData() const { return std::any_cast<T>(mData); }
	bool HasData() const { return mData.has_value(); }

	const Listener* GetSource() const { return mSource; }

protected:
	friend class MessageBus;
	std::any mData;
	Listener* mSource{ nullptr };
	str mText;
};

class MessageBus;
class Listener {
public:
	virtual ~Listener() = default;

	/// <summary>
	/// Called when a message is received.
	/// </summary>
	/// <param name="message"></param>
	virtual void OnMessage(const Message& message) {};

	/// <summary>
	/// Called when a response is received.
	/// </summary>
	/// <param name="message"></param>
	virtual void OnResponse(const Message& message) {}

	void Respond(const str& text = "", std::any data = {});
	void SendMessage(const str& text = "", std::any data = {});
};

class MessageBus {
public:
	MessageBus() = default;
	~MessageBus() = default;

	void SendMessage(Listener* source, std::any data = {}, const str& text = "");

	void Respond(Listener* source, std::any data = {}, const str& text = "");

	void RegisterListener(Listener* listener);
	void UnregisterListener(Listener* listener);

	static MessageBus& Get();
private:
	std::vector<Listener*> mListeners;

	static UPtr<MessageBus> mInstance;
};