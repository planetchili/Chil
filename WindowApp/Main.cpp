#include <Core/src/win/ChilWin.h>
#include <Core/src/log/Log.h>
#include <Core/src/utl/String.h>
#include <format>
#include "CliOptions.h"
#include "StateMachineMode.h"

using namespace chil;

int WINAPI WinMain(
	HINSTANCE hInstance,
	HINSTANCE hPrevInstance,
	PSTR pCmdLine,
	int nCmdShow)
{
	try {
		// parse the command line 
		if (auto code = opt::Init()) {
			if (*code == 0) {
				MessageBoxA(nullptr, opt::GetDiagnostics().c_str(), "Command Line Help",
					MB_ICONINFORMATION | MB_APPLMODAL | MB_SETFOREGROUND);
			}
			else {
				MessageBoxA(nullptr, opt::GetDiagnostics().c_str(), "Command Line Parse Error",
					MB_ICONERROR | MB_APPLMODAL | MB_SETFOREGROUND);
			}
			return *code;
		}
		auto& opts = opt::Get();
		// run the execution mode entry function
		if (*opts.runMode == RunMode::StateMachine) {
			RunStateMachineMode();
		}
		else if (*opts.runMode == RunMode::GeneratorCoro) {
			throw std::runtime_error{ "Unimplemented" };
		}
		else {
			throw std::runtime_error{ "Unknown RunMode" };
		}
	}
	catch (const std::exception& e) {
		chilog.error(L"Error caught at top level: " + utl::ToWide(e.what())).no_trace();
		return -1;
	}

	return 0;
}