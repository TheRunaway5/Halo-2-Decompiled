#include "halo2/source_effect_compiled_symbols_tu15.hpp"
#include "halo2/source_effect_particle_draw_tu15.hpp"
#include "native_tu15_object_property_source_test_harness.hpp"

int main(int argc,char** argv)try {
    using namespace halo2;
    using namespace halo2::object_property_source_test;
    constexpr auto entry=0x17de60u;
    check(argc==2&&source_effect_particle_draw_tu15_owns_entry(entry),"Supply the linked 17DE60 fixture");
    std::ifstream stream(argv[1]);const auto rows=J::parse(stream);
    J results=J::array();
    for(unsigned n=0;n<rows.size();++n){
        const auto& row=rows[n];const Seed seed{row};
        for(unsigned mode=0;mode<(n<9?3u:2u);++mode){
            auto abi=seed.abi;Memory memory;memory.banks=seed.banks;
            SourcePhysicsContactRuntimeServices services;
            SourcePhysicsContactRecordObserver observe;
            J callbacks=J::array(),stores=J::array(),live=J::array(),changes=J::array();
            if(mode){
                services.instruction=[&](std::uint32_t pc){
                    callbacks.push_back(pc);memory.pc=pc;
                    if(mode==2&&pc==0x17de86u)throw Boundary{};
                };
                observe=[&](const SourcePhysicsContactRecordStore& event){
                    stores.push_back(J::array({event.instruction,event.address,hex(event.before),hex(event.written)}));
                };
            }
            bool stopped=false;
            try {
                source_run_effect_particle_draw_tu15(memory,entry,abi,*abi.floating_point,services,observe);
            }catch(const Boundary&){stopped=true;}
            check(abi.floating_point.has_value()&&!abi.active_mmx,"Unexpected linked 17DE60 floating point state");
            for(unsigned index=0;index<abi.x87_depth;++index)
                live.push_back(hex(std::span(reinterpret_cast<const std::byte*>(&abi.x87[index]),10)));
            for(const auto& [address,before]:seed.banks){
                const auto& after=memory.banks.at(address);
                for(unsigned i=0;i<before.size();++i)
                    if(before[i]!=after[i])changes.push_back(J::array({address+i,std::to_integer<unsigned>(after[i])}));
            }
            results.push_back(J{{"owner","particle-draw"},{"case",row.at("name")},{"mode",mode},
                {"stopped",stopped},{"gp",abi.gp},{"flags",abi.eflags},{"xmm",abi.xmm},{"fs",abi.fs_base},
                {"fp",J::array({abi.floating_point->x87_control_word,abi.x87_status,abi.floating_point->mxcsr})},
                {"live_x87",live},{"changes",changes},{"callbacks",callbacks},{"observer",stores},
                {"memory",std::move(memory.operations)}});
        }
    }
    std::cout<<results.dump()<<'\n';return 0;
}catch(const std::exception& e){std::cerr<<e.what()<<'\n';return 1;}
