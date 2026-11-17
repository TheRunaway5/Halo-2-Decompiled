#include "halo2/physics_live_support_tu15.hpp"
#include "halo2/source_combat_action_tu15.hpp"
#include "halo2/source_model_render_tu15.hpp"
#include "physics/runtime/native_tu15_codec_format_four_source_owner.hpp"
#include "native_tu15_mopp_ray_frame_test_memory.hpp"
#include <fstream>
#include <iostream>

using namespace halo2;
using namespace halo2::mopp_ray_frame_test;

namespace {
namespace decoder=native_codec_format_four_owner_detail;
enum class Mode {quiet,empty,trace_after_load,observer_fault,foreign_exception,replace_after_child};

J abi_packet(const SourcePhysicsContactRuntimeAbi& abi) {
    J stack=J::array();
    for(unsigned index=0;index<abi.x87_depth;++index)
        stack.push_back(hex(std::span(reinterpret_cast<const std::byte*>(&abi.x87[index]),10u)));
    const auto& fp=*abi.floating_point;
    return J{{"gp",abi.gp},{"flags",abi.eflags},{"xmm",abi.xmm},{"fs",abi.fs_base},
             {"fp",J::array({fp.x87_control_word,abi.x87_status,fp.mxcsr})},{"x87",stack}};
}

struct Seed {
    SourcePhysicsContactRuntimeAbi abi{};
    std::map<std::uint32_t,std::vector<std::byte>> banks;
    explicit Seed(const J& row) {
        abi.gp=row.at("gp");abi.eflags=row.at("eflags");abi.xmm=row.at("xmm");
        const auto& fp=row.at("before_fp");abi.floating_point=SourcePhysicsFactoryFloatingPoint{fp[0],fp[2]};
        abi.x87_status=fp[1];abi.x87_depth=row.at("live_x87").size();
        for(unsigned index=0;index<abi.x87_depth;++index){
            const auto raw=bytes(row.at("live_x87")[index]);std::memcpy(&abi.x87[index],raw.data(),10u);
        }
        for(const auto& bank:row.at("banks"))banks.emplace(bank.at("address"),bytes(bank.at("before")));
        for(auto address:{0x510f34u,0x510f5cu,0x510f80u})
            std::fill_n(banks.at(0x510000u).begin()+address-0x510000u,address==0x510f80u?1u:4u,std::byte{});
    }
};

struct Provider final:Memory {
    mutable std::function<void()> after_read;
    void read(std::uint32_t address,std::span<std::byte> out) const override {
        Memory::read(address,out);
        if(after_read)after_read();
    }
    void read_vector(std::uint32_t address,std::span<std::byte,16> out) const override {
        SourcePhysicsContactMemory::read_vector(address,out);
    }
};

struct Counts {unsigned cases{},exceptions{},returned_children{},translation_groups{},retired_calls{};};

std::array<unsigned,3> host_fp() {
    std::uint16_t control{},status{};
    asm volatile("fnstcw %0;fnstsw %1":"=m"(control),"=m"(status));
    return {control,status,_mm_getcsr()};
}

void run(const Seed& seed,bool primary,Mode mode,std::ostream& stream,Counts& count) {
    auto abi=seed.abi;Provider memory;memory.banks=seed.banks;
    SourcePhysicsContactRuntimeServices services;
    bool trace{},changed{};
    J observed=J::array(),callbacks=J::array(),children=J::array();
    const auto callback=[&](std::uint32_t site){memory.pc=site;callbacks.push_back(site);};
    static constexpr std::array sites{0x38846eu,0x21f590u,0x33b595u};
    if(mode!=Mode::empty){
        services.instruction=callback;
        services.instruction_cursor={&memory.pc,&trace,nullptr,sites,std::this_thread::get_id(),
            &services.instruction.target_type(),true,true};
    }
    const SourcePhysicsContactRecordObserver observer=mode==Mode::empty?SourcePhysicsContactRecordObserver{}:
        SourcePhysicsContactRecordObserver{[&](const SourcePhysicsContactRecordStore& event){
            observed.push_back(J::array({event.instruction,event.address,hex(event.before),hex(event.written)}));
            if(mode==Mode::observer_fault&&event.instruction==0x293290u)throw Boundary{};
        }};
    memory.after_read=[&]{
        if(memory.pc!=0x293299u)return;
        if(mode==Mode::trace_after_load&&!changed){trace=true;changed=true;}
        if(mode==Mode::foreign_exception)throw decoder::Continuation{0x29329fu,nullptr};
    };
    services.required_owner=[&](auto& child_memory,const auto& call,auto& child_abi,const auto& child_observer){
        check(&child_memory==&memory&&&child_observer==&observer,"Linked codec changed required owner references");
        check(!call.tail,"Linked codec unexpectedly tail-called a row child");
        switch(call.target){
        case 0x2963d0u:check(!primary&&call.instruction==0x293329u,"Wrong linked quaternion call");break;
        case 0x296410u:check(!primary&&call.instruction==0x293467u,"Wrong linked translation call");break;
        case 0x2cec60u:check(call.instruction==0x29354au,"Wrong linked scale call");break;
        default:throw std::runtime_error("Linked codec escaped real row-eight children");
        }
        const auto before=abi_packet(child_abi);
        const auto child=call.target==0x2cec60u?source_run_model_render_tu15:source_run_physics_live_support_tu15;
        child(child_memory,call.target,child_abi,*child_abi.floating_point,services,child_observer);
        children.push_back(J::array({call.instruction,call.target,before,abi_packet(child_abi)}));
        if(mode==Mode::replace_after_child&&!changed){
            changed=true;services.instruction=[&,tag=7u](std::uint32_t site){(void)tag;callback(site);};
        }
    };
    const auto before=decoder::activity();const auto caller_fp=host_fp();
    bool stopped{};
    const auto caller=primary?source_run_physics_live_support_tu15:source_run_combat_action_tu15;
    try{caller(memory,0x284290u,abi,*abi.floating_point,services,observer);}
    catch(const Boundary&){check(mode==Mode::observer_fault,"Unexpected linked codec boundary");stopped=true;}
    catch(const decoder::Continuation& foreign){
        check(mode==Mode::foreign_exception&&!foreign.owner&&foreign.pc==0x29329fu,"Linked codec changed foreign exception");
        stopped=true;
    }
    check(stopped==(mode==Mode::observer_fault||mode==Mode::foreign_exception),"Linked codec completion differs");
    check(host_fp()==caller_fp,"Linked codec owner changed caller FP");
    const auto after=decoder::activity();
    count.retired_calls+=after.calls-before.calls;
    count.returned_children+=children.size();count.exceptions+=stopped;++count.cases;
    for(std::size_t index=0;index+2<observed.size();++index){
        if(observed[index][0]!=0x293496u||observed[index+1][0]!=0x2934a3u||observed[index+2][0]!=0x2934b1u)continue;
        check(observed[index][1]==observed[index+1][1]&&observed[index][1]==observed[index+2][1],
              "Linked decoder changed a repeated translation address");
        ++count.translation_groups;
    }
    stream<<J{{"primary",primary},{"mode",unsigned(mode)},{"stopped",stopped},{"abi",abi_packet(abi)},
        {"cursor",memory.pc},{"io",memory.operations},{"observed",observed},{"callbacks",callbacks},
        {"children",children}}.dump()<<'\n';
    for(const auto&[address,bank]:memory.banks){
        stream<<address<<' '<<bank.size()<<'\n';stream.write(reinterpret_cast<const char*>(bank.data()),bank.size());
    }
}
}

int main(int argc,char** argv)try {
    check(argc==4,"Supply linked format-four inputs, owned stream, and format");
    std::ifstream input(argv[1]);const auto rows=J::parse(input);
    std::ofstream stream(argv[2],std::ios::binary);check(bool(stream),"Open linked codec comparison stream");
    const bool prepared=std::string_view(argv[3])=="prepared";
    check(prepared||std::string_view(argv[3])=="raw","Unknown linked codec source format");
    check(source_physics_live_support_tu15_owns_entry(0x284290u)&&source_combat_action_tu15_owns_entry(0x284290u)&&
        source_model_render_tu15_owns_entry(0x2cec60u),"Missing linked codec owner route");
    Counts count;
    for(const auto& row:rows){
        const Seed seed{row};
        for(bool primary:{false,true}){
            run(seed,primary,Mode::empty,stream,count);
            if(prepared)for(auto mode:{Mode::quiet,Mode::trace_after_load,Mode::observer_fault,
                    Mode::foreign_exception,Mode::replace_after_child})run(seed,primary,mode,stream,count);
        }
    }
    stream.close();check(bool(stream),"Write complete linked codec comparison stream");
    std::cout<<J{{"cases",count.cases},{"exceptions",count.exceptions},{"external_child_returns",count.returned_children},
        {"translation_groups",count.translation_groups},{"native_entries",count.retired_calls}}.dump()<<'\n';
    return 0;
}catch(const std::exception& error){std::cerr<<error.what()<<'\n';return 1;}
