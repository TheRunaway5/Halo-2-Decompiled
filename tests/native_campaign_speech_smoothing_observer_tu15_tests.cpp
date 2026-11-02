#define main smoothing_fixture_main
#include "native_campaign_speech_smoothing_tu15_tests.cpp"
#undef main
namespace {
struct DiagnosticReadFault {};
struct StoreFault {};
struct ObserverFault {};
struct ObservedMemory:Memory {
    std::uint32_t field;mutable unsigned field_reads{};mutable J attempts=J::array();
    bool reject_diagnostic{},reject_store{};
    ObservedMemory(const J &row,std::uint32_t at):Memory(row),field(at){}
    void read(std::uint32_t at,std::span<std::byte> out)const override {
        attempts.push_back(J{{"address",at},{"bytes",out.size()}});
        if(at==field&&++field_reads==3&&reject_diagnostic)throw DiagnosticReadFault{};
        Memory::read(at,out);
    }
    void write(std::uint32_t at,std::span<const std::byte> data)override {
        if(reject_store)throw StoreFault{};
        Memory::write(at,data);
    }
};
}
int main(int argc,char **argv)try {
    check(argc==2,"Supply original observer admission fixture");std::ifstream input(argv[1]);const auto f=J::parse(input);
    auto find=[&](const std::string &name)->const J&{for(const auto &r:f.at("cases"))if(r.at("name")==name)return r;throw std::runtime_error("Missing original observer boundary");};
    unsigned checked{},diagnostic_faults{},store_faults{},callback_faults{},ordinary_calls{};
    for(const std::string seed:{"fault_seed_-2.0","fault_seed_0.5","fault_seed_2.0"}) {
        const auto &row=find(seed);const auto at=row.at("writes")[0].at("address").get<std::uint32_t>();
        for(unsigned mode=0;mode<4;++mode) {
            const auto &boundary=mode==0?row:find(seed+(mode==3?"_after_write_0":"_before_write_0"));
            ObservedMemory memory(row,at);memory.reject_diagnostic=mode<=1;memory.reject_store=mode==2;
            auto abi=caller(row);const auto initial=abi;J events=J::array();SourcePhysicsContactRecordObserver observer;
            if(mode)observer=[&](const auto &e){events.push_back(J{{"instruction",e.instruction},{"address",e.address},{"before",hex(e.before)},{"written",hex(e.written)}});if(mode==3)throw ObserverFault{};};
            const auto mxcsr=_mm_getcsr();std::uint16_t cw{},sw{};asm volatile("fnstcw %0\n\tfnstsw %1":"=m"(cw),"=m"(sw));
            unsigned stopped{};SourcePhysicsContactRuntimeServices services;services.required_owner=[](auto&,const auto&,auto&,const auto&){throw std::runtime_error("Observer requested a supplied dependency");};
            try{native_tu15_sound_manager_run(memory,0x18f7a0u,abi,abi.floating_point.value(),observer,services);}
            catch(const DiagnosticReadFault &){stopped=1;++diagnostic_faults;}
            catch(const StoreFault &){stopped=2;++store_faults;}
            catch(const ObserverFault &){stopped=3;++callback_faults;}
            std::uint16_t after_cw{},after_sw{};asm volatile("fnstcw %0\n\tfnstsw %1":"=m"(after_cw),"=m"(after_sw));
            check(stopped==mode,seed+": observer fault identity or stage differs");
            check(cw==after_cw&&sw==after_sw&&mxcsr==_mm_getcsr(),seed+": observer host FP leaked");
            check(abi.gp[4]==initial.gp[4]+(mode?0u:4u),seed+": observer return stack differs");
            check(abi.xmm[0]==boundary.at("boundary_xmm0").get<std::array<std::uint32_t,4>>()&&abi.xmm[1]==boundary.at("boundary_xmm1").get<std::array<std::uint32_t,4>>(),seed+": observer SIMD lanes differ");
            check(J::array({abi.floating_point->x87_control_word,abi.x87_status,abi.floating_point->mxcsr})==boundary.at("after_fp"),seed+": observer FP differs");
            for(unsigned n=0;n<8;++n)if(n!=4)check(abi.gp[n]==initial.gp[n],seed+": observer GP policy differs");
            for(unsigned n=2;n<8;++n)check(abi.xmm[n]==initial.xmm[n],seed+": observer preserved SIMD differs");
            check(abi.x87_depth==initial.x87_depth,seed+": observer x87 depth differs");for(unsigned n=0;n<abi.x87_depth;++n)check(!std::memcmp(&abi.x87[n],&initial.x87[n],10),seed+": observer x87 quantity differs");
            if(abi.active_mmx)check(initial.active_mmx&&abi.active_mmx->physical==initial.active_mmx->physical&&abi.active_mmx->known_mask==initial.active_mmx->known_mask,seed+": observer MMX differs");
            for(const auto &bank:boundary.at("banks"))check(hex(memory.banks.at(bank.at("address")))==bank.at("after").get<std::string>(),seed+": observer retained bytes differ");
            check(memory.writes==boundary.at("writes"),seed+": observer actual stores differ");
            J expected=J::array();for(const auto &read:boundary.at("reads"))expected.push_back(J::array({read.at("address"),read.at("bytes")}));
            if(mode>=2)expected.push_back(J::array({at,4}));
            check(memory.reads==expected,seed+": explicit diagnostic read sequence differs");
            check(memory.field_reads==(mode?3u:2u),seed+": observer extra read was hidden");
            if(mode==1)check(memory.attempts.back()==J{{"address",at},{"bytes",4}},seed+": diagnostic read fault operand differs");
            check(events==(mode==3?boundary.at("writes"):J::array()),seed+": observer event timing differs");
            if(mode==0)++ordinary_calls;++checked;
        }
    }
    check(checked==12&&diagnostic_faults==3&&store_faults==3&&callback_faults==3&&ordinary_calls==3,"Observer admission matrix incomplete");
    std::cout<<checked<<" explicit observer admission cases: "<<diagnostic_faults<<" third-read faults, "<<store_faults<<" stores rejected after diagnostic read, "<<callback_faults<<" post-store observer faults, "<<ordinary_calls<<" ordinary calls with no third read\n";return 0;
}catch(const std::exception &e){std::cerr<<e.what()<<'\n';return 1;}
