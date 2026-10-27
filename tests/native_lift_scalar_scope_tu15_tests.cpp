#include "halo2/native_lift_scalar_consumer_tu15.hpp"
#include <iostream>
#include <stdexcept>
#include <thread>
namespace {
using namespace halo2;
void check(bool value,const char*message){if(!value)throw std::runtime_error(message);}
struct Memory final:SourcePhysicsContactMemory {
    void read(std::uint32_t,std::span<std::byte>)const override{throw std::runtime_error("Admission must not read a bank");}
    void write(std::uint32_t,std::span<const std::byte>)override{throw std::runtime_error("Admission must not write a bank");}
};
}
int main(){try{
    Memory first,second;bool trace=false;NativeLiftScalarActivityTu15 activity;
    check(!native_lift_scalar_consumer_policy_tu15(first),"Lift admission leaked from another call");
    {
        NativeLiftScalarConsumerScopeTu15 outer(first,{0x60000000,0x60100000,{},&trace,nullptr,true,&activity});
        const auto*admitted=native_lift_scalar_consumer_policy_tu15(first);
        check(admitted&&admitted->world_activity==&activity&&admitted->observation_only_stores,"Owned lift policy was lost");
        check(!native_lift_scalar_consumer_policy_tu15(second),"Lift permission crossed world memory");
        bool cross_thread=false;std::thread other([&]{cross_thread=native_lift_scalar_consumer_policy_tu15(first)!=nullptr;});other.join();
        check(!cross_thread,"Lift permission crossed its executor thread");
        {
            NativeLiftScalarConsumerScopeTu15 nested(second,{0x1000,0x2000,{},nullptr,nullptr,false,nullptr});
            check(native_lift_scalar_consumer_policy_tu15(first)==admitted,"Another world's nested call erased its owner");
            check(!native_lift_scalar_consumer_policy_tu15(second)->observation_only_stores,"Mutable observer was silently admitted as read-only");
            {
                NativeLiftScalarConsumerScopeTu15 override(first,{0x3000,0x4000,{},nullptr,nullptr,false,nullptr});
                check(native_lift_scalar_consumer_policy_tu15(first)->caller_stack_begin==0x3000,"Nested same-world caller was ignored");
            }
            check(native_lift_scalar_consumer_policy_tu15(first)==admitted,"Nested same-world return lost actual owner");
        }
        check(!native_lift_scalar_consumer_policy_tu15(second),"Returned nested permission remained live");
        bool refused=false;try{NativeLiftScalarConsumerScopeTu15 invalid(first,{0x2000,0x1000,{},{},{},false,nullptr});}catch(const std::invalid_argument&){refused=true;}
        check(refused&&native_lift_scalar_consumer_policy_tu15(first)==admitted,"Invalid stack admission changed the active owner");
        trace=true;check(*admitted->first_trace,"Trace eligibility was snapshotted before the actual call");
    }
    check(!native_lift_scalar_consumer_policy_tu15(first),"Returned lift permission remained live");
    std::cout<<"Native lift permission: world/thread isolation, nested lifetime, mutable observations and invalid stack refusal passed\n";return 0;
}catch(const std::exception&e){std::cerr<<e.what()<<'\n';return 1;}}
