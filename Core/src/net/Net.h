#pragma once
#include <memory>

namespace chil::net
{
	class IServer
	{
	public:
		virtual ~IServer() = default;
		static std::unique_ptr<IServer> Make();
	};

	class IClient
	{
	public:
		virtual ~IClient() = default;
		static std::unique_ptr<IClient> Make();
	};
}