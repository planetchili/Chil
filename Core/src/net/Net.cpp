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
		void SendCommand(CommandType type, std::span<const char> commandBytes) override
		{
			const Header hdr{ .payloadSize = (uint32_t)commandBytes.size_bytes(), .type = type };
			socket_.send(as::buffer(&hdr, sizeof(hdr)));
			socket_.send(as::buffer(commandBytes));
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
					Header hdr{};
					co_await as::async_read(socket_, as::buffer(&hdr, sizeof(hdr)), as::use_awaitable);
					if (hdr.type == CommandType::Move) {
						MoveCommand cmd;
						co_await as::async_read(socket_, as::buffer(&cmd, sizeof(cmd)), as::use_awaitable);
						receivedCommands_.push_back(std::move(cmd));
					}
					else if (hdr.type == CommandType::Title) {
						TitleCommand cmd;
						std::vector<char> buffer(hdr.payloadSize);
						co_await as::async_read(socket_, as::buffer(buffer), as::use_awaitable);
						cmd.title.resize(reinterpret_cast<uint16_t*>(buffer.data())[0]);
						cmd.shmitle.resize(reinterpret_cast<uint16_t*>(buffer.data())[1]);
						const auto dataStartOffset = sizeof(uint16_t) * 2;
						std::copy_n(buffer.begin() + dataStartOffset, cmd.title.size(), cmd.title.begin());
						std::copy_n(buffer.begin() + dataStartOffset + cmd.title.size(), cmd.shmitle.size(), cmd.shmitle.begin());
						receivedCommands_.push_back(std::move(cmd));
					}
					else {
						throw std::runtime_error{ "yikes dawg" };
					}
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
	};

	std::unique_ptr<IClient> IClient::Make()
	{
		return std::make_unique<Client>();
	}
}