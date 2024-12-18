#pragma once
#include <memory>
#include <vector>
#include <Core/src/spa/Vec2.h>

namespace chil::net
{
	struct MoveCommand
	{
		spa::Vec2F start;
		spa::Vec2F end;
	};

	class IServer
	{
	public:
		virtual ~IServer() = default;
		virtual void SendCommand(const MoveCommand&) = 0;
		static std::unique_ptr<IServer> Make();
	};

	class IClient
	{
	public:
		virtual ~IClient() = default;
		virtual std::vector<MoveCommand> ReceiveCommands() = 0;
		static std::unique_ptr<IClient> Make();
	};
}