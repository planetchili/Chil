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
		std::string shmitle;
	};

	using Command = std::variant<MoveCommand, TitleCommand>;

	class IServer
	{
	public:
		virtual ~IServer() = default;
		template<class T>
		void SendCommand(const T& command)
		{
			namespace rn = std::ranges;

			if constexpr (std::same_as<T, MoveCommand>) {
				SendCommand(CommandType::Move, std::span{ reinterpret_cast<const char*>(&command), sizeof(command) });
			}
			if constexpr (std::same_as<T, TitleCommand>) {
				const TitleCommand& titCommand = command;
				std::vector<char> data;
				// sizeN (2) == sizeM (2) == title (N) == shmitle (M)
				data.resize(sizeof(uint16_t) * 2 + titCommand.title.size() + titCommand.shmitle.size());
				reinterpret_cast<uint16_t*>(data.data())[0] = (uint16_t)titCommand.title.size();
				reinterpret_cast<uint16_t*>(data.data())[1] = (uint16_t)titCommand.shmitle.size();
				const auto dataStartOffset = sizeof(uint16_t) * 2;
				rn::copy(titCommand.title, data.data() + dataStartOffset);
				rn::copy(titCommand.shmitle, data.begin() + dataStartOffset + titCommand.title.size());
				SendCommand(CommandType::Title, data);
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