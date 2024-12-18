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
		}
	private:
		as::io_context ioctx_;
		tcp::socket socket_{ ioctx_ };
	};

	std::unique_ptr<IClient> IClient::Make()
	{
		return std::make_unique<Client>();
	}
}