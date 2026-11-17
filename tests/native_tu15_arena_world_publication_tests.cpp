// Interface/lifetime proof only. These two standalone friend issuers model
// access control; the validator separately compiles the actual World/Arena
// production translation units with the authenticated constructor prototype.
#include "physics/runtime/native_tu15_arena_world_publication.hpp"
#include <array>
#include <iostream>
#include <stdexcept>
#include <type_traits>

using namespace halo2;
using Services=SourcePhysicsContactRuntimeServices;
namespace publication=halo2::native_arena_world_publication_detail;
struct Memory final:SourcePhysicsContactMemory {
    void read(std::uint32_t,std::span<std::byte>)const override{}
    void write(std::uint32_t,std::span<const std::byte>)override{}
};
struct World {
    std::uint32_t position{};bool first{},second{};
    const std::thread::id executor=std::this_thread::get_id();
    const std::array<std::uint32_t,3>sites{0x38846eu,0x21f590u,0x33b595u};
    struct Instruction{World*const world;void operator()(std::uint32_t pc)const{world->position=pc;}};
    struct Required{
        World*const world;
        void operator()(SourcePhysicsContactMemory&,const SourcePhysicsContactRuntimeCall&,
            SourcePhysicsContactRuntimeAbi&,const SourcePhysicsContactRecordObserver&)const{}
        static bool matches(const Services::RequiredOwner&callback,const void*owner)noexcept{
            const auto*body=callback.target<Required>();
            return body&&body->world==owner&&body->world->executor==std::this_thread::get_id()&&
                !body->world->first&&!body->world->second;
        }
    };
    Services services(){
        Services result;result.instruction=Instruction{this};
        result.instruction_cursor={&position,&first,&second,sites,executor,&result.instruction.target_type()};
        result.instruction_cursor.literal_service_sites=true;
        result.required_owner=Required{this};
        result.required_owner_policy={&result.required_owner.target_type(),executor,this,&Required::matches};
        return result;
    }
};
namespace halo2 {
class PhysicsLiveWorldTu15 {
public:
    template<class Call>static void publish(const SourcePhysicsGameArenaNative&arena,const SourcePhysicsContactMemory&bank,
        const Services&producer,const void*world,Call call){publication::Construction scope{arena,bank,producer,world};call();}
};
class SourcePhysicsGameArenaNative {
    const SourcePhysicsContactMemory&bank_;
    const Services lower_;
    static bool matches(const Services&joined,const void*owner,const SourcePhysicsGameArenaNative&arena)noexcept{
        return owner==&arena&&publication::Construction::qualified(arena,arena.bank_,arena.lower_)&&
            publication::same_instruction(joined,arena.lower_)&&
            native_required_owner_policy_detail::preserves_instruction_policy(joined);
    }
public:
    SourcePhysicsGameArenaNative(const SourcePhysicsContactMemory&bank,const Services&lower):bank_(bank),lower_(lower){}
    template<class Call>void publish(const SourcePhysicsContactMemory&memory,const Services&joined,Call call)const{
        publication::Borrow scope{*this,memory,joined,this,&matches};call();
    }
};
}
static_assert(!std::is_constructible_v<publication::Construction,const SourcePhysicsGameArenaNative&,
    const SourcePhysicsContactMemory&,const Services&,const void*>);
static_assert(!std::is_copy_constructible_v<publication::Construction>);
static_assert(!std::is_copy_constructible_v<publication::Borrow>);
int main()try{
    unsigned checks{};const auto check=[&](bool value){if(!value)throw std::runtime_error("World Arena publication ownership differs");++checks;};
    Memory bank,other_bank,shared,other_shared;World world,foreign;
    const auto producer=world.services();auto joined=producer;const auto original=joined;
    SourcePhysicsGameArenaNative arena{bank,producer},other_arena{bank,producer},wrong_bank{other_bank,producer},wrong_world{bank,foreign.services()};
    arena.publish(shared,joined,[&]{check(!publication::Borrow::qualified(shared,joined));});
    PhysicsLiveWorldTu15::publish(arena,bank,producer,&world,[&]{
        arena.publish(shared,joined,[&]{
            check(publication::Borrow::qualified(shared,joined));
            check(!publication::Borrow::qualified(other_shared,joined));
            const auto copy=joined;check(!publication::Borrow::qualified(shared,copy));
            world.first=true;check(!publication::Borrow::qualified(shared,joined));world.first=false;
            world.second=true;check(!publication::Borrow::qualified(shared,joined));world.second=false;
            joined.instruction={};check(!publication::Borrow::qualified(shared,joined));joined=original;
            joined.instruction=[](std::uint32_t){};check(!publication::Borrow::qualified(shared,joined));joined=original;
            joined.instruction_cursor.position=&foreign.position;check(!publication::Borrow::qualified(shared,joined));joined=original;
            joined.instruction_cursor.executor=std::thread::id{};check(!publication::Borrow::qualified(shared,joined));joined=original;
            joined.required_owner_policy.owner=&foreign;check(!publication::Borrow::qualified(shared,joined));joined=original;
            joined.required_owner_policy.matches=nullptr;check(!publication::Borrow::qualified(shared,joined));joined=original;
            auto nested=joined;
            arena.publish(shared,nested,[&]{check(publication::Borrow::qualified(shared,nested));check(!publication::Borrow::qualified(shared,joined));});
            check(publication::Borrow::qualified(shared,joined));
            PhysicsLiveWorldTu15::publish(other_arena,bank,producer,&world,[&]{check(!publication::Borrow::qualified(shared,joined));});
            check(publication::Borrow::qualified(shared,joined));
        });
        other_arena.publish(shared,joined,[&]{check(!publication::Borrow::qualified(shared,joined));});
        wrong_bank.publish(shared,joined,[&]{check(!publication::Borrow::qualified(shared,joined));});
        wrong_world.publish(shared,joined,[&]{check(!publication::Borrow::qualified(shared,joined));});
        bool foreign_thread{};std::thread thread{[&]{arena.publish(shared,joined,[&]{foreign_thread=publication::Borrow::qualified(shared,joined);});}};thread.join();check(!foreign_thread);
        struct Original{};bool caught{};
        try{arena.publish(shared,joined,[&]{throw Original{};});}catch(const Original&){caught=true;}
        check(caught&&!publication::Borrow::qualified(shared,joined));
    });
    check(!publication::Construction::current()&&!publication::Borrow::qualified(shared,joined));
    std::cout<<"{\"passed\":true,\"checks\":"<<checks<<",\"scope\":\"standalone private publication interface only\"}\n";return 0;
}catch(const std::exception&error){std::cerr<<error.what()<<'\n';return 1;}
