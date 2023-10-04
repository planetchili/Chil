#pragma once
#include <concurrentqueue/concurrentqueue.h>
#include <optional>
#include <array>
#include <atomic>

namespace chil::win
{
	// IDEA: capture list of events once per frame
	// generate keystate at time of capture

	struct KeyEvent
	{
		enum class Type : uint8_t
		{
			Press,
			Release,
		};
		Type type;
		uint8_t code;
	};

	class IKeyboardSource
	{
	public:
		virtual ~IKeyboardSource() = default;
		virtual std::optional<KeyEvent> GetEvent() = 0;
		virtual bool KeyIsPressed(uint8_t code) const = 0;
	};

	class IKeyboardSink
	{
	public:
		virtual ~IKeyboardSink() = default;
		virtual void PutEvent(KeyEvent e) = 0;
	};

	class Keyboard : public IKeyboardSource, public IKeyboardSink
	{
	public:
		std::optional<KeyEvent> GetEvent() override
		{
			KeyEvent e;
			if (queue_.try_dequeue(e)) {
				return e;
			}
			return {};
		}
		bool KeyIsPressed(uint8_t code) const override
		{
			return keys_[code];
		}
		void PutEvent(KeyEvent e) override
		{
			queue_.enqueue(e);
			keys_[e.code] = e.type == KeyEvent::Type::Press;
		}
	private:
		moodycamel::ConcurrentQueue<KeyEvent> queue_;
		std::array<std::atomic<bool>, 256> keys_{};
	};
}