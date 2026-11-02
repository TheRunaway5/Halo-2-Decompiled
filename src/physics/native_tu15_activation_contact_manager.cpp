#include "halo2/native_tu15_activation_contact_manager.hpp"
#include "halo2/native_tu15_island_activation.hpp"
#include "native_tu15_contact_unwind_internal.hpp"
#include <bit>
#include <stdexcept>
namespace halo2 {
namespace {
class Manager {
    SourcePhysicsContactMemory&memory_;const SourcePhysicsContactRecordObserver&observer_;
    const NativeTu15ActivationContactManagerServices&services_;NativeTu15PhysicsFrameFp&fp_;
    NativeTu15ActivationContactManagerStats*stats_;
public:
    Manager(SourcePhysicsContactMemory&m,const SourcePhysicsContactRecordObserver&o,
      const NativeTu15ActivationContactManagerServices&s,NativeTu15PhysicsFrameFp&fp,
      NativeTu15ActivationContactManagerStats*stats):memory_(m),observer_(o),services_(s),fp_(fp),stats_(stats){}
    std::uint32_t word(std::uint32_t p,unsigned width=4)const{
        std::array<std::byte,4>bytes{};memory_.read(p,std::span(bytes).first(width));std::uint32_t value{};
        for(unsigned n=0;n<width;++n)value|=std::to_integer<unsigned>(bytes[n])<<(n*8u);return value;
    }
    void word(std::uint32_t site,std::uint32_t p,std::uint32_t value,unsigned width=4){
        std::array<std::byte,4>bytes{};for(unsigned n=0;n<width;++n)bytes[n]=std::byte(value>>(n*8u));
        SourcePhysicsContactRecordStore event{site,p,{},{}};if(observer_){event.before.resize(width);memory_.read(p,event.before);event.written.assign(bytes.begin(),bytes.begin()+width);}
        memory_.write(p,std::span(bytes).first(width));if(observer_)observer_(event);
    }
    void grow(std::uint32_t header){
        if(stats_)++stats_->array_growths;NativeTu15ArrayGrowthMemory array(memory_,{header},observer_);
        native_tu15_grow_array(array,4u,fp_,services_.allocation);
    }
    void append_listener(std::uint32_t body,std::uint32_t listener){ //315E10
        if(stats_)++stats_->registrations;
        const auto capacity=word(body+0x90u),header=body+0x88u,count=word(header+4u);
        if(count==(capacity&0x7fffffffu))grow(header);
        const auto current_count=word(header+4u),entries=word(header);
        word(0x315e3du,entries+current_count*4u,listener);
        word(0x315e40u,header+4u,word(header+4u)+1u);
    }
    void defer_listener_removal(std::uint32_t world,std::uint32_t body){ //316DC0
        if(stats_)++stats_->deferred_references;
        word(0x316dc6u,body+6u,word(body+6u,2)+1u,2);
        const auto capacity=word(world+0x11cu),header=world+0x114u,count=word(header+4u);
        if(count==(capacity&0x7fffffffu))grow(header);
        const auto current_count=word(header+4u),entries=word(header);
        word(0x316df2u,entries+current_count*4u,body);
        word(0x316dfau,header+4u,word(header+4u)+1u);
    }
    void remove_listener(std::uint32_t body,std::uint32_t listener){ //315D10
        if(stats_)++stats_->unregistrations;
        const auto count=word(body+0x8cu);std::uint32_t index{};bool found{};
        if(std::bit_cast<std::int32_t>(count)>0){
            const auto entries=word(body+0x88u);
            do{if(word(entries+index*4u)==listener){found=true;break;}++index;}
            while(std::bit_cast<std::int32_t>(index)<std::bit_cast<std::int32_t>(count));
        }
        if(!found)index=0xffffffffu;
        const auto entries=word(body+0x88u);word(0x315d3eu,entries+index*4u,0);
        const auto world=word(body+8u);if(world)defer_listener_removal(world,body);
    }
    bool decrement_body(std::uint32_t body,std::uint32_t decrement_site,std::uint32_t protected_site){
        word(decrement_site,body+6u,word(body+6u,2)-1u,2);
        if(word(body+6u,2))return false;
        if(native_tu15_reserved_body_reference(memory_,{body})){
            if(stats_)++stats_->protected_references;word(protected_site,body+6u,1u,2);return true;
        }
        const auto table=word(body),owner=word(table);
        if(!services_.destroy_body)throw std::runtime_error("Native TU1.5 activation manager requires its actual body destructor");
        if(stats_)++stats_->body_destructions;services_.destroy_body({body},{owner},1u,fp_);return false;
    }
    void set_body(std::uint32_t collector,std::uint32_t body,bool second){ //325BD0/325C40
        const auto field=collector+(second?0x10u:0xcu);
        if(body){word(second?0x325c4cu:0x325bdcu,body+6u,word(body+6u,2)+1u,2);append_listener(body,collector);}
        const auto old=word(field);bool protected_reference{};
        if(old){
            remove_listener(old,collector);const auto current_old=word(field);
            protected_reference=decrement_body(current_old,second?0x325c69u:0x325bf9u,second?0x325c81u:0x325c11u);
        }
        word(second?(protected_reference?0x325c88u:0x325c99u):(protected_reference?0x325c18u:0x325c29u),field,body);
    }
    void initialize_collector(std::uint32_t p){ //326100
        if(stats_)++stats_->collectors;
        word(0x326102u,p+6u,1u,2);word(0x326108u,p+8u,0x41f3f0u);
        word(0x32610fu,p,0x41f478u);word(0x326115u,p+8u,0x41f468u);
        word(0x32611eu,p+0x14u,0);word(0x326121u,p+0x10u,0);word(0x326124u,p+0xcu,0);
        word(0x326127u,p+0x18u,0);word(0x32612au,p+0x24u,0);word(0x32612du,p+0x28u,0x80000008u);
        word(0x326137u,p+0x20u,p+0x2cu);word(0x32613au,p+0x34u,0);word(0x32613du,p+0x38u,0);
        word(0x326140u,p+0x3cu,0x80000000u);
        constexpr std::array<std::uint32_t,8>zero_sites{0x326147u,0x32614au,0x32614du,0x326150u,0x326153u,0x326156u,0x326159u,0x32615cu};
        for(unsigned n=0;n<zero_sites.size();++n)word(zero_sites[n],p+0x50u+n*4u,0);
        word(0x32615fu,p+0x70u,0x3e88d677u);word(0x326166u,p+0x74u,0x3f08d677u);
        word(0x326172u,p+0x78u,0x3f4d41b3u);word(0x326175u,p+0x7cu,0x3f4d41b3u);
        word(0x326178u,p+0x94u,0);word(0x32617eu,p+0x98u,0x80000004u);
        auto current=p+0x9cu;word(0x32618eu,p+0x90u,current);
        constexpr std::array<std::uint32_t,8>entry_sites{0x326194u,0x326196u,0x32619cu,0x32619eu,0x3261a4u,0x3261a6u,0x3261acu,0x3261aeu};
        for(unsigned n=0;n<4;++n){word(entry_sites[n*2u],current,0);word(entry_sites[n*2u+1u],current+4u,0);current+=0x1cu;}
    }
    void release(std::uint32_t data,std::uint32_t bytes,std::uint32_t memory_class){
        const auto allocator=word(0x48c938u),table=word(allocator),owner=word(table+0x14u);
        if(!services_.allocation.release)throw std::runtime_error("Native TU1.5 activation manager requires its actual release owner");
        services_.allocation.release({allocator,owner,data,bytes,memory_class},fp_);
    }
    void destroy_collector(std::uint32_t p){ //31F660 ->325AF0 ->B4150
        if(stats_)++stats_->collector_destructions;
        auto capacity=word(p+0x98u);if(!(capacity&0x80000000u)){const auto data=word(p+0x90u);release(data,(capacity&0x7fffffffu)*0x1cu,0x12u);}
        capacity=word(p+0x3cu);if(!(capacity&0x80000000u)){const auto data=word(p+0x34u);release(data,(capacity&0x7fffffffu)<<5u,0x12u);}
        capacity=word(p+0x28u);if(!(capacity&0x80000000u)){const auto data=word(p+0x20u);release(data,capacity&0x7fffffffu,0x12u);}
        word(0x325b10u,p,0x41f410u);word(0x325b16u,p+8u,0x41f400u);
        auto body=word(p+0xcu);if(body){remove_listener(body,p);body=word(p+0xcu);decrement_body(body,0x325b35u,0x325b4du);}
        body=word(p+0x10u);if(body){remove_listener(body,p);body=word(p+0x10u);decrement_body(body,0x325b6du,0x325b85u);}
        word(0x325b97u,p+8u,0x41f3f0u);
        const auto flags=word(p+4u,2);word(0xb415bu,p,0x45eddcu);
        if(flags&0x8000u){
            word(0xb416au,p+4u,flags&0x7fffu,2);
            if(!services_.release_base_lock)throw std::runtime_error("Native TU1.5 activation manager requires actual base-lock release ownership");
            if(stats_)++stats_->base_lock_releases;services_.release_base_lock(p,0u,fp_);
        }
        word(0xb4173u,p,0x45edf0u);
    }
    void construct(std::uint32_t p,std::uint32_t user_data,std::uint32_t first,std::uint32_t second){ //323230
        if(stats_)++stats_->constructors;
        native_contact_detail::UnwindGuard base([&]{word(0x2f7a10u,p,0x45edf0u);});
        native_contact_detail::UnwindGuard collector([&]{destroy_collector(p+0x10u);});
        base.arm();word(0x32325bu,p,0x41f124u);initialize_collector(p+0x10u);
        collector.arm();word(0x323276u,p+8u,user_data);set_body(p+0x10u,first,false);set_body(p+0x10u,second,true);
        word(0x32328eu,p+0xcu,1u);collector.disarm();base.disarm();
    }
    void release_storage(std::uint32_t p){ //316130: global/table before size read.
        if(stats_)++stats_->storage_releases;
        const auto allocator=word(0x48c938u),table=word(allocator),bytes=word(p+4u,2),owner=word(table+0x14u);
        if(!services_.allocation.release)throw std::runtime_error("Native TU1.5 activation manager requires actual storage release ownership");
        services_.allocation.release({allocator,owner,p,bytes,0x1du},fp_);
    }
    std::uint32_t create(std::uint32_t factory,NativeTu15ContactPayload first,NativeTu15ContactPayload second){
        if(stats_)++stats_->factories;
        const auto second_body=word(second.identity+0x20u),allocator=word(0x48c938u),table=word(allocator),first_body=word(first.identity+0x20u),owner=word(table+0x10u);
        if(!services_.allocation.allocate)throw std::runtime_error("Native TU1.5 activation manager requires actual allocation ownership");
        const auto p=services_.allocation.allocate({allocator,owner,0x120u,0x1du},fp_);
        native_contact_detail::UnwindGuard storage([&]{release_storage(p);});
        word(0x32331au,p+4u,0x120u,2);const auto user_data=word(factory+8u);
        storage.arm();construct(p,user_data,first_body,second_body);storage.disarm();return p;
    }
};
}
NativeTu15ActivationContactManager native_tu15_create_activation_contact_manager(SourcePhysicsContactMemory&memory,
 std::uint32_t factory,NativeTu15ContactPayload first,NativeTu15ContactPayload second,NativeTu15PhysicsFrameFp&fp,
 const NativeTu15ActivationContactManagerServices&services,const SourcePhysicsContactRecordObserver&observer,
 NativeTu15ActivationContactManagerStats*stats){return{Manager(memory,observer,services,fp,stats).create(factory,first,second)};}
} // namespace halo2

namespace halo2 {
void native_tu15_destroy_activation_collector(SourcePhysicsContactMemory& memory,
 std::uint32_t collector,NativeTu15PhysicsFrameFp& fp,
 const NativeTu15ActivationContactManagerServices& services,
 const SourcePhysicsContactRecordObserver& observer,
 NativeTu15ActivationContactManagerStats* stats) {
    Manager(memory,observer,services,fp,stats).destroy_collector(collector);
}
} // namespace halo2
