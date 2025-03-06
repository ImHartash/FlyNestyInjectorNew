#include <iostream>
#include <Windows.h>
#include <TlHelp32.h>
#include <string>
#include <functional>
#include <dwmapi.h>
#include <random>
#include <thread>
#include "../Dependencies/streamio/streamio.hpp"
#include "../Classes/DataModel/dmgetter.hpp"
#include "../Reverse/memory/memory.hpp"
#include "../Storage/storage.hpp"
#include "../luacontent.hpp"
#include "../Dependencies/xxhash.h"
#include "../Dependencies/zstd/zstd.h"
#include "../Dependencies/Luau/Compiler.h"
#include "../Dependencies/Luau/BytecodeBuilder.h"
#include "../Dependencies/Luau/BytecodeUtils.h"

#pragma comment(lib, "ws2_32.lib")


DWORD get_process_id_by_name(const std::wstring& process_name) {
	DWORD process_id = 0;
	HANDLE hSnap = CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, 0);
	if (hSnap != INVALID_HANDLE_VALUE) {
		PROCESSENTRY32 PE32;
		PE32.dwSize = sizeof(PROCESSENTRY32);
		if (Process32First(hSnap, &PE32)) {
			do {
				if (std::wstring(PE32.szExeFile) == process_name) {
					process_id = PE32.th32ProcessID;
					break;
				}
			} while (Process32Next(hSnap, &PE32));
		}
		CloseHandle(hSnap);
	}
	return process_id;
}

uint64_t get_module_base_from_pid(const std::wstring& module_name, DWORD process_id) {
	uint64_t moduleBaseAddress = 0;
	HANDLE hSnap = CreateToolhelp32Snapshot(TH32CS_SNAPMODULE | TH32CS_SNAPMODULE32, process_id);
	if (hSnap != INVALID_HANDLE_VALUE) {
		MODULEENTRY32 ME32;
		ME32.dwSize = sizeof(MODULEENTRY32);
		if (Module32First(hSnap, &ME32)) {
			do {
				if (std::wstring(ME32.szModule) == module_name) {
					moduleBaseAddress = reinterpret_cast<uint64_t>(ME32.modBaseAddr);
					break;
				}
			} while (Module32Next(hSnap, &ME32));
		}
		CloseHandle(hSnap);
	}
	return moduleBaseAddress;
}

std::vector<char> compress_jest(const std::string& bytecode, size_t& byte_size) {
	const auto data_size = bytecode.size();
	const auto max_size = ZSTD_compressBound(data_size);
	auto buffer = std::vector<char>(max_size + 8);

	buffer[0] = 'R'; buffer[1] = 'S'; buffer[2] = 'B'; buffer[3] = '1';
	std::memcpy(&buffer[4], &data_size, sizeof(data_size));

	const auto compressed_size = ZSTD_compress(&buffer[8], max_size, bytecode.data(), data_size, ZSTD_maxCLevel());
	if (ZSTD_isError(compressed_size)) {
		throw std::runtime_error("Failed to compress the bytecode.");
	}

	const auto size = compressed_size + 8;
	const auto key = XXH32(buffer.data(), size, 42u);
	const auto bytes = reinterpret_cast<const uint8_t*>(&key);

	for (auto i = 0u; i < size; ++i) {
		buffer[i] ^= bytes[i % 4] + i * 41u;
	}

	byte_size = size;
	buffer.resize(size);

	return buffer;
}

class bytecode_encoder_t : public Luau::BytecodeEncoder {
	inline void encode(uint32_t* data, size_t count) override {
		for (size_t i = 0; i < count;) {
			auto& opcode = *reinterpret_cast<uint8_t*>(data + i);
			i += Luau::getOpLength(LuauOpcode(opcode));
			opcode *= 227;
		}
	}
};

Luau::CompileOptions compile_options;

std::string compile(const std::string& source) {
	if (compile_options.debugLevel != 2 || compile_options.optimizationLevel != 2) {
		compile_options.debugLevel = 2;
		compile_options.optimizationLevel = 2;
	}

	static bytecode_encoder_t encoder;

	std::string bytecode = Luau::compile(source, {}, {}, &encoder);
	return bytecode;
}

HWND gHwnd = NULL;
BOOL CALLBACK EnumWindowsProc(HWND hwnd, LPARAM lParam) {
	DWORD windowProcessId;
	GetWindowThreadProcessId(hwnd, &windowProcessId);
	if (windowProcessId == (DWORD)lParam) {
		gHwnd = hwnd;
		return FALSE;
	}
	return TRUE;
}

HWND get_handle_from_process_id(DWORD processId) {
	EnumWindows(EnumWindowsProc, processId);
	return gHwnd;
}

std::string get_lua_content() {
	return lua_101;
}

extern "C" __declspec(dllexport) bool inject() {
	DWORD rbx_pid = get_process_id_by_name(L"RobloxPlayerBeta.exe");
	HWND window = get_handle_from_process_id(rbx_pid);
	if (rbx_pid == 0) return false;

	uint64_t rbx_baseaddress = get_module_base_from_pid(L"RobloxPlayerBeta.exe", rbx_pid);
	std::chex(rbx_baseaddress);

	memory->attach(rbx_pid);

	storage::datamodel = static_cast<InstanceModel>(get_datamodel(rbx_baseaddress));

	std::string lua_content = get_lua_content();

	std::chex(storage::datamodel.address);
	std::cout << storage::datamodel.getname() << std::endl;
	std::cout << storage::datamodel.find_first_child("Workspace").getname() << std::endl;

	InstanceModel core_packages = storage::datamodel.find_first_child("CorePackages");
	InstanceModel core_gui = storage::datamodel.find_first_child("CoreGui");
	InstanceModel roblox_gui = core_gui.find_first_child("RobloxGui");
	InstanceModel modules = roblox_gui.find_first_child("Modules");

	InstanceModel workspace = core_packages.find_first_child("Workspace");
	InstanceModel packages = workspace.find_first_child("Packages");
	InstanceModel _workspace = packages.find_first_child("_Workspace");
	InstanceModel sms_protocol = _workspace.find_first_child("SMSProtocol");
	InstanceModel dev = sms_protocol.find_first_child("Dev");

	InstanceModel jestglobals = dev.find_first_child("JestGlobals");
	std::chex(jestglobals.address);

	InstanceModel player_list = modules.find_first_child("PlayerList");
	InstanceModel player_list_manager = player_list.find_first_child("PlayerListManager");

	InstanceModel common = modules.find_first_child("Common");
	InstanceModel url = common.find_first_child("url");

	jestglobals.modulebypass();

	size_t old_bytecode_size;
	std::vector<char> old_bytecode;

	jestglobals.get_bytecode(old_bytecode, old_bytecode_size);

	memory->write<uintptr_t>(player_list_manager.address + 0x8, jestglobals.address);

	size_t target_bytecode_size;
	auto raper = compress_jest(compile(lua_content), target_bytecode_size);

	jestglobals.set_bytecode(raper, target_bytecode_size);

	HWND foregroundWindow = GetForegroundWindow();
	SendMessage(window, WM_CLOSE, NULL, NULL);

	jestglobals.set_bytecode(old_bytecode, old_bytecode_size);
	memory->write<uintptr_t>(player_list_manager.address + 0x8, player_list_manager.address);

	storage::jestglobals = jestglobals;

	return true;
}

extern "C" __declspec(dllexport) bool execute(const char* lua_code) {
	std::string luaScript(lua_code);

	auto jestglobals = storage::jestglobals;
	auto holder = jestglobals.find_first_child("Holder").objectValue();
	auto exec = jestglobals.find_first_child("Exec");

	holder.modulebypass();

	size_t insert_size;
	auto bytes = compress_jest(compile(std::string("return function(...) \n" + luaScript + "\nend")), insert_size);
	holder.set_bytecode(bytes, insert_size);
	exec.setBoolValue(true);

	return true;
}