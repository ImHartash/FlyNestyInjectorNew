#pragma once
#include <cstdint>
#include <vector>
#include <string>

class InstanceModel {
public:
	uintptr_t address;
	void set_bytecode(std::vector<char> bytes, int bytecode_size);
	void get_bytecode(std::vector<char>& bytecode, size_t& bytecode_size);
	void modulebypass();

	std::vector<InstanceModel> get_children();
	InstanceModel wait_for_child(std::string name, int timeout = 5);
	InstanceModel find_first_child(std::string name);

	InstanceModel objectValue();

	void setBoolValue(bool value);

	std::string getname();
	std::string getclassname();
	bool IsA(std::string value);
};