#pragma once
#include "halo2/physics_contact_runtime.hpp"

namespace halo2 {
// Complete13FE00 window preference,13FE50 allocation and140310 retirement.
// CPU pool allocation/deletion and the private32A660 stack probe run here.
// Live busy/delete callbacks retain their actual call ABI through services;
// no callback, page handle or resource-I/O result is supplied by this owner.
void native_tu15_page_window_prefer_abi(SourcePhysicsContactMemory &,
    SourcePhysicsContactRuntimeAbi &);
void native_tu15_page_allocate_abi(SourcePhysicsContactMemory &,
    SourcePhysicsContactRuntimeAbi &,const SourcePhysicsContactRuntimeServices &,
    const SourcePhysicsContactRecordObserver & = {});
void native_tu15_page_delete_abi(SourcePhysicsContactMemory &,
    SourcePhysicsContactRuntimeAbi &,const SourcePhysicsContactRuntimeServices &,
    const SourcePhysicsContactRecordObserver & = {});
}
