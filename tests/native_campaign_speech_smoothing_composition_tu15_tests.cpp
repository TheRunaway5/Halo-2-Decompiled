// Share the retained-memory and full ABI fixture views; use a distinct main.
#define main smoothing_fixture_main
#include "native_campaign_speech_smoothing_tu15_tests.cpp"
#undef main
namespace {
void transport_word(Memory &m,std::uint32_t at,std::uint32_t value) {
    const auto bank=m.locate(at,4);for(unsigned n=0;n<4;++n)bank->second[at-bank->first+n]=std::byte(value>>(8u*n));
}
void compose(Memory &memory,SourcePhysicsContactRuntimeAbi &abi,const J &row,
    const SourcePhysicsContactRuntimeServices &services) {
    const auto sp=row.at("frame_sp").get<std::uint32_t>();const auto policy=abi.floating_point.value();
    // Exact private transport of the two real18F510 CALL sites. These words
    // construct live call frames; they do not supply either callee outcome.
    transport_word(memory,sp-4u,abi.gp[0]);transport_word(memory,sp-8u,0x18f5cbu);
    abi.gp[0]=abi.gp[7];abi.gp[1]=abi.gp[5];abi.gp[4]=sp-8u;
    native_tu15_sound_manager_run(memory,0x21f860u,abi,policy,{},services);
    std::array<std::byte,4> object{};memory.read(sp+0x10u,object);abi.gp[0]=std::bit_cast<std::uint32_t>(object);
    transport_word(memory,sp-4u,0x18f5d4u);abi.gp[4]=sp-4u;
    native_tu15_sound_manager_run(memory,0x18f7a0u,abi,abi.floating_point.value(),{},services);
}
}
int main(int argc,char **argv)try {
    check(argc==2,"Supply physical original composition fixture");std::ifstream input(argv[1]);const auto f=J::parse(input);
    check(f.at("image_sha256")=="21b207e7efdacaf6124a0822837f8be570766a42505057f7fd2d2ba8f409c4fd"&&f.at("backend")=="physical-unchanged-i386-offline","Composition authority differs");
    check(f.at("instruction_patches").empty()&&!f.at("supplied_original_outcomes").get<bool>(),"Composition supplied original outcomes");
    check(native_tu15_sound_manager_owns_entry(0x21f860u)&&native_tu15_sound_manager_owns_entry(0x18f7a0u),"Actual composition registry missing");
    unsigned checked{},absent{},zero{};
    for(const auto &row:f.at("cases")) {
        const auto name=row.at("name").get<std::string>();Memory memory(row);auto abi=caller(row);const auto initial=abi;
        const auto host_mxcsr=_mm_getcsr();std::uint16_t cw{},sw{};asm volatile("fnstcw %0\n\tfnstsw %1":"=m"(cw),"=m"(sw));
        SourcePhysicsContactRuntimeServices services;services.required_owner=[](auto&,const auto&,auto&,const auto&){throw std::runtime_error("Composition requested a supplied dependency");};
        compose(memory,abi,row,services);
        std::uint16_t after_cw{},after_sw{};asm volatile("fnstcw %0\n\tfnstsw %1":"=m"(after_cw),"=m"(after_sw));
        check(cw==after_cw&&sw==after_sw&&host_mxcsr==_mm_getcsr(),name+": composition host FP leaked");
        check(abi.gp[4]==row.at("frame_sp"),name+": composition return stack differs");
        check(abi.xmm[0]==row.at("boundary_xmm0").get<std::array<std::uint32_t,4>>()&&abi.xmm[1]==row.at("boundary_xmm1").get<std::array<std::uint32_t,4>>(),name+": composition SIMD lanes differ");
        for(unsigned n=2;n<8;++n)check(abi.xmm[n]==initial.xmm[n],name+": composition preserved SIMD differs");
        for(unsigned n:{3u,5u,6u,7u})check(abi.gp[n]==row.at("boundary_gp")[n],name+": composition preserved GP differs");
        check(J::array({abi.floating_point->x87_control_word,abi.x87_status,abi.floating_point->mxcsr})==row.at("after_fp"),name+": composition FP differs");
        check(abi.x87_depth==initial.x87_depth,name+": composition x87 depth differs");for(unsigned n=0;n<abi.x87_depth;++n)check(!std::memcmp(&abi.x87[n],&initial.x87[n],10),name+": composition x87 quantity differs");
        check(bool(abi.active_mmx)==bool(initial.active_mmx),name+": composition MMX ownership differs");if(abi.active_mmx)check(abi.active_mmx->physical==initial.active_mmx->physical&&abi.active_mmx->known_mask==initial.active_mmx->known_mask,name+": composition MMX differs");
        for(const auto &bank:row.at("banks"))if(bank.at("address")!=row.at("private_bank"))check(hex(memory.banks.at(bank.at("address")))==bank.at("after").get<std::string>(),name+": composition retained bytes differ");
        J expected=J::array();for(const auto &read:row.at("reads")){
            bool retired{};for(const auto &pc:f.at("retired_private_transport_read_sites"))retired|=read.at("instruction")==pc;
            if(!retired)expected.push_back(J::array({read.at("address"),read.at("bytes")}));
        }
        if(memory.reads!=expected)std::cerr<<"actual="<<memory.reads.dump()<<" expected="<<expected.dump()<<'\n';
        check(memory.reads==expected,name+": composition ordered reads differ");
        check(memory.writes==row.at("writes"),name+": composition actual stores differ");
        check(memory.writes.size()==1,name+": zero or absent sample failed to smooth");
        if(row.at("absent").get<bool>())++absent;if(row.at("valid_zero").get<bool>())++zero;++checked;
    }
    check(checked==20&&absent==12&&zero==4,"Composition profile incomplete");
    std::cout<<checked<<" physical caller-fragment compositions, including "<<absent<<" absent and "<<zero<<" valid-zero samples, passed\n";return 0;
}catch(const std::exception &e){std::cerr<<e.what()<<'\n';return 1;}
