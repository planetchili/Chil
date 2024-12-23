#include "Net.h"
#define _ALLOW_COROUTINE_ABI_MISMATCH
#include "WrapAsio.h"
#include <cereal/cereal.hpp>
#include <cereal/types/string.hpp>
#include <cereal/types/variant.hpp>
#include <cereal/archives/binary.hpp>
#include <sstream>

namespace as = boost::asio;
using as::ip::tcp;
using namespace std::literals;

namespace cereal
{
	template<class Archive>
	void serialize(Archive& archive, chil::spa::Vec2F& s)
	{
		archive(s.x, s.y);
	}

	template<class Archive>
	void serialize(Archive& archive, chil::net::MoveCommand& s)
	{
		archive(s.start, s.end);
	}

	template<class Archive>
	void serialize(Archive& archive, chil::net::TitleCommand& s)
	{
		archive(s.title, s.shmitle);
	}
}

namespace chil::net
{
	class Server : public IServer
	{
	public:
		Server()
		{
			tcp::acceptor acceptor{ ioctx_, tcp::endpoint{ tcp::v4(), 22441 } };
			acceptor.accept(socket_);
		}
		void SendCommand(const Command& cmd) override
		{
			std::ostringstream oss;
			cereal::BinaryOutputArchive archive{ oss };
			archive(cmd);
			const auto buf = oss.str();
			const auto size = (uint32_t)buf.size();
			socket_.send(as::buffer(&size, sizeof(size)));
			socket_.send(as::buffer(buf));
		}
	private:
		as::io_context ioctx_;
		tcp::socket socket_{ ioctx_ };
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