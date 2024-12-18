#pragma once
#include <memory>
#include <vector>
#include <cstdint>
#include <variant>
#include <span>
#include <concepts>
#include <string>
#include <Core/src/spa/Vec2.h>

namespace chil::net
{
	enum class CommandType
	{
		Move,
		Title,
	};

	struct Header
	{
		uint32_t payloadSize;
		CommandType type;
	};

	struct MoveCommand
	{
		spa::Vec2F start;
		spa::Vec2F end;
	};

	struct TitleCommand
	{
		std::string title;
	};

	using Command = std::variant<MoveCommand, TitleCommand>;

	class IServer
	{
	public:
		virtual ~IServer() = default;
		template<class T>
		void SendCommand(const T& command)
		{
			if constexpr (std::same_as<T, MoveCommand>) {
				SendCommand(CommandType::Move, std::span{ reinterpret_cast<const char*>(&command), sizeof(command) });
			}
			if constexpr (std::same_as<T, TitleCommand>) {
				SendCommand(CommandType::Title, command.title);
			}
		}
		virtual void SendCommand(CommandType type, std::span<const char> commandBytes) = 0;
		static std::unique_ptr<IServer> Make();
	};

	class IClient
	{
	public:
		virtual ~IClient() = default;
		virtual std::vector<Command> ReceiveCommands() = 0;
		static std::unique_ptr<IClient> Make();
	};
}