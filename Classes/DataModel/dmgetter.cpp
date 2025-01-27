#include "../../Reverse/memory/memory.hpp"
#include "../InstanceModel/instance.hpp"
#include "../RBX/offsets.hpp"
#include <iostream>
#include <cstdint>
#include "streamio/streamio.hpp"


uintptr_t get_datamodel(uint64_t base_address) {
    uintptr_t visual_engine = memory->read<uintptr_t>(base_address + offsets::visualengine::visual_pointer);
    std::chex(visual_engine);
    uintptr_t dm_ptr = memory->read<uintptr_t>(visual_engine + offsets::visualengine::dmptr);
    std::chex(dm_ptr);
    uintptr_t datamodel = memory->read<uintptr_t>(dm_ptr + offsets::datamodel::ptrtodatamodel);
    return datamodel;
}