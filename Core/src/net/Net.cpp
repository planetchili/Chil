#include "Net.h"
#define _ALLOW_COROUTINE_ABI_MISMATCH
#include "WrapAsio.h"
#include <cereal/cereal.hpp>
#include <cereal/types/string.hpp>
#include <cereal/types/variant.hpp>
#include <cereal/types/vector.hpp>
#include <cereal/archives/binary.hpp>
#include <sstream>
#include "../../third/reflect.hpp"

namespace as = boost::asio;
using as::ip::tcp;
using namespace std::literals;

template <typename T, typename = void> struct IsMemberOfStd : std::false_type {};
template <typename T> struct IsMemberOfStd<T, decltype(AdlIsMemberOfStd_impl_(std::declval<T>()))> : std::true_type {};

// technically bad mojo
namespace std
{
	template <typename T>
	auto AdlIsMemberOfStd_impl_(T&&) -> void;
}

namespace cereal
{
	template <typename T, typename = void> struct IsMemberOfCereal : std::false_type {};
	template <typename T> struct IsMemberOfCereal<T, decltype(AdlIsMemberOfCereal_impl_(std::declval<T>()))> : std::true_type {};
	template <typename T>
	auto AdlIsMemberOfCereal_impl_(T&&) -> void;

	template<class S>
	concept IsNotNativelySerializable = std::is_class_v<S> && !IsMemberOfStd<S>::value && !IsMemberOfCereal<S>::value;

	template<class Archive, IsNotNativelySerializable S>
	void serialize(Archive& archive, S& s)
	{
		reflect::for_each([&](const auto I) {
			archive(reflect::get<I>(s));
		}, s);
	}
}

namespace chil::net
{
	class Server : public IServer
	{
	private:
		class VectorPacketStreambuf_ : public std::streambuf
		{
		public:
			VectorPacketStreambuf_()
			{
				reset();
			}
			void reset()
			{
				buffer_.resize(sizeof(uint32_t));
			}
			as::const_buffer yield()
			{
				*reinterpret_cast<uint32_t*>(buffer_.data()) = uint32_t(buffer_.size() - sizeof(uint32_t));
				return as::buffer(buffer_);
			}
		protected:
			std::streamsize xsputn(const char* data, std::streamsize size) override
			{
				buffer_.insert(buffer_.end(), data, data + size);
				return size;
			}
			int overflow(int ch) override
			{
				if (ch != EOF) {
					buffer_.push_back(static_cast<uint8_t>(ch));
					return ch;
				}
				return EOF;
			}
		private:
			std::vector<uint8_t> buffer_;
		};
	public:
		Server()
		{
			tcp::acceptor acceptor{ ioctx_, tcp::endpoint{ tcp::v4(), 22441 } };
			acceptor.accept(socket_);
		}
		void SendCommand(const Command& cmd) override
		{
			writeArchive_(cmd);
			socket_.send(writeBuffer_.yield());
			writeBuffer_.reset();
		}
	private:
		as::io_context ioctx_;
		tcp::socket socket_{ ioctx_ };
		VectorPacketStreambuf_ writeBuffer_;
		std::ostream writeStream_{ &writeBuffer_ };
		cereal::BinaryOutputArchive writeArchive_{ writeStream_ };
	};

	std::unique_ptr<IServer> IServer::Make()
	{
		return std::make_unique<Server>();
	}


	class Client : public IClient
	{
	public:
		Client()
		{
			tcp::resolver resolver(ioctx_);
			auto endpoint = resolver.resolve("127.0.0.1", "22441");
			as::connect(socket_, endpoint);
			auto ReceiverStrand = [this]() -> as::awaitable<void> {
				while (true) {
					uint32_t payloadSize;
					co_await as::async_read(socket_, readBuf_, as::transfer_exactly(sizeof(payloadSize)), as::use_awaitable);
					readStream_.read(reinterpret_cast<char*>(&payloadSize), sizeof(payloadSize));
					co_await as::async_read(socket_, readBuf_, as::transfer_exactly(payloadSize), as::use_awaitable);
					receivedCommands_.emplace_back();
					readArchive_(receivedCommands_.back());
				}
			};
			as::co_spawn(ioctx_, ReceiverStrand, as::detached);
		}
		std::vector<Command> ReceiveCommands() override
		{
			ioctx_.run_for(1ms);
			return std::move(receivedCommands_);
		}
	private:
		std::vector<Command> receivedCommands_;
		as::io_context ioctx_;
		tcp::socket socket_{ ioctx_ };
		as::streambuf readBuf_;
		std::istream readStream_{ &readBuf_ };
		cereal::BinaryInputArchive readArchive_{ readStream_ };
	};

	std::unique_ptr<IClient> IClient::Make()
	{
		return std::make_unique<Client>();
	}
}