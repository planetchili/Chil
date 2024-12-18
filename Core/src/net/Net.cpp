#include "Net.h"
#define _ALLOW_COROUTINE_ABI_MISMATCH
#include "WrapAsio.h"

namespace as = boost::asio;
using as::ip::tcp;
using namespace std::literals;


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
		void SendCommand(const MoveCommand& cmd) override
		{
			socket_.send(as::buffer(reinterpret_cast<const char*>(&cmd), sizeof(cmd)));
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
				MoveCommand cmd{};
				while (true) {
					co_await as::async_read(socket_, as::buffer(&cmd, sizeof(cmd)), as::use_awaitable);
					receivedCommands_.push_back(cmd);
				}
			};
			as::co_spawn(ioctx_, ReceiverStrand, as::detached);
		}
		std::vector<MoveCommand> ReceiveCommands() override
		{
			ioctx_.run_for(1ms);
			return std::move(receivedCommands_);
		}
	private:
		std::vector<MoveCommand> receivedCommands_;
		as::io_context ioctx_;
		tcp::socket socket_{ ioctx_ };
	};

	std::unique_ptr<IClient> IClient::Make()
	{
		return std::make_unique<Client>();
	}
}