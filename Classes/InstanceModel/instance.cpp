#include "instance.hpp"
#include "../../Reverse/memory/memory.hpp"
#include "../RBX/offsets.hpp"

string rbxstring(uintptr_t address) {
	const auto size = memory->read<size_t>(address + 0x10);

	if (size >= 16) {
		address = memory->read<uintptr_t>(address);
	}

	string str;
	BYTE c = 0;

	for (int32_t i = 0; c = memory->read<uint8_t>(address + i); i++) {
		str.push_back(c);
	}

	return str;
}

void InstanceModel::set_bytecode(vector<char> bytes, int bytecode_size) {
	auto old_bytecode_ptr = memory->read<long long>(this->address + offsets::script::msbytecode);
	auto protected_str_ptr = (long long)memory->allocate_virtual_memory(bytecode_size, MEM_COMMIT | MEM_RESERVE, PAGE_READWRITE);

	memory->write_memory(protected_str_ptr, bytes.data(), bytes.size());
	memory->write<unsigned long long>(old_bytecode_ptr + 0x10, protected_str_ptr);
	memory->write<unsigned long>(old_bytecode_ptr + 0x20, bytecode_size);
}

template <typename T>
void read_mem(DWORD64 address, T* buffer, SIZE_T size = 0) {
	if (size == 0) {
		size = sizeof(T);
	}

	ReadProcessMemory(memory->get_process_handle(), (LPCVOID)address, buffer, size, NULL);
}

vector<char> read_bytes(DWORD64 address, SIZE_T size = 500) {
	vector<char> buffer(size, 0);
	read_mem(address, buffer.data(), size);

	return buffer;
}

void InstanceModel::get_bytecode(vector<char>& bytecode, size_t& bytecode_size) {
	DWORD64 bytecode_ptr;
	auto classname = this->getclassname();
	if (classname == "LocalScript") {
		bytecode_ptr = memory->read<long long>(this->address + offsets::script::lsbytecode);
	}
	else if (classname == "ModuleScript") {
		bytecode_ptr = memory->read<long long>(this->address + offsets::script::msbytecode);
	}
	else {
		return;
	}

	if (bytecode_ptr > 1000) {
		bytecode = read_bytes(bytecode_ptr + 0x10);
		read_mem(bytecode_ptr + 0x20, &bytecode_size);
	}
}

void InstanceModel::modulebypass() {
	uint64_t set = 0x100000000;
	uint64_t core = 0x1;

	memory->write(address + offsets::script::moduleflags, set);
	memory->write(address + offsets::script::iscore, core);
}

vector<InstanceModel> InstanceModel::get_children() {
	vector<InstanceModel> container;

	if (!this->address) {
		return container;
	}

	auto start = memory->read<uint64_t>(this->address + offsets::instance::children);

	if (!start) {
		return container;
	}

	auto end = memory->read<uint64_t>(start + offsets::instance::childsize);

	for (auto instances = memory->read<uint64_t>(start); instances != end; instances += 16) {
		InstanceModel child = memory->read<InstanceModel>(instances);
		container.emplace_back(child);
	}

	return container;
}

InstanceModel InstanceModel::find_first_child(string name) {
	InstanceModel target;

	for (auto& object : this->get_children()) {
		if (object.getname() == name) {
			target = static_cast<InstanceModel>(object);
			cout << "Found " + name << endl;
			break;
		}
	}

	return target;
}

InstanceModel InstanceModel::objectValue() {
	InstanceModel target = memory->read<InstanceModel>(this->address + offsets::instance::instancevalue::value);
	return target;
}

void InstanceModel::setBoolValue(bool value) {
	memory->write<bool>(this->address + offsets::instance::instancevalue::value, value);
}

InstanceModel InstanceModel::wait_for_child(string name, int timeout) {
	if (!this->address) {
		return InstanceModel{};
	}

	timeout *= 10;
	for (int times = 0; times < timeout; ++times) {
		auto child_list = memory->read<DWORD64>(this->address + offsets::instance::children);
		if (!child_list) continue;

		auto child_top = memory->read<DWORD64>(child_list);
		auto child_end = memory->read<DWORD64>(child_list + 0x8);

		for (DWORD64 child_addy = child_top; child_addy < child_end; child_addy += 0x10) {
			InstanceModel child = memory->read<InstanceModel>(child_addy);

			if (child.address > 1000 && child.getname() == name)
				return child;
		}
		Sleep(100);
	}

	return InstanceModel{};
}

string InstanceModel::getname() {
	const auto pointer = memory->read<uint64_t>(this->address + offsets::instance::name);

	if (pointer) {
		return rbxstring(pointer);
	}

	return "??? [Name]";
}

string InstanceModel::getclassname() {
	auto classDescriptor = memory->read<uint64_t>(this->address + offsets::instance::cdescriptor);
	auto className = memory->read<uint64_t>(classDescriptor + offsets::instance::cname);

	if (className) {
		return rbxstring(className);
	}

	return "??? [ClassName]";
}

bool InstanceModel::IsA(string value) {
	return this->getclassname() == value;
}