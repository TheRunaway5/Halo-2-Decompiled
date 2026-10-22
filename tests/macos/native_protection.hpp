#pragma once
#include <cstddef>
#include <cstdint>
#include <stdexcept>
#ifdef __APPLE__
#include <mach/mach.h>
#include <mach/mach_vm.h>
#else
#include <fstream>
#include <sstream>
#include <string>
#endif

namespace halo2::macos_test {
inline bool native_writable(void *mapping,std::size_t offset) {
    const auto address=reinterpret_cast<std::uintptr_t>(mapping)+offset;
#ifdef __APPLE__
    mach_vm_address_t region=address;mach_vm_size_t bytes{};
    vm_region_basic_info_data_64_t info{};
    mach_msg_type_number_t count=VM_REGION_BASIC_INFO_COUNT_64;
    mach_port_t object=MACH_PORT_NULL;
    const auto status=mach_vm_region(mach_task_self(),&region,&bytes,VM_REGION_BASIC_INFO_64,
        reinterpret_cast<vm_region_info_t>(&info),&count,&object);
    if(object!=MACH_PORT_NULL)mach_port_deallocate(mach_task_self(),object);
    if(status!=KERN_SUCCESS || region>address || address>=region+bytes)
        throw std::runtime_error("Darwin native protection query failed");
    return info.protection&VM_PROT_WRITE;
#else
    std::ifstream maps("/proc/self/maps");std::string line;
    while(std::getline(maps,line)) {
        std::istringstream row(line);std::string range,permissions;row>>range>>permissions;
        const auto split=range.find('-');
        if(split!=std::string::npos && address>=std::stoull(range.substr(0,split),nullptr,16) &&
           address<std::stoull(range.substr(split+1),nullptr,16))return permissions.size()>1 && permissions[1]=='w';
    }
    throw std::runtime_error("Native protection query failed");
#endif
}
}
