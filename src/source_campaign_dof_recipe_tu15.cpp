#include "halo2/source_campaign_dof_recipe_tu15.hpp"
#include "halo2/cache.hpp"
#include <bit>
namespace halo2 {
namespace {
std::uint32_t word(const SourceRuntimeMemory &m,std::uint32_t p){return Bytes(m.view(p,4)).u32(0);}
void put(SourceRuntimeMemory &m,std::uint32_t p,std::uint32_t v){m.write(p,std::as_bytes(std::span(&v,1)));}
void writable(SourceRuntimeMemory &m,std::uint32_t p,std::size_t n){
    m.mount(p,std::vector<std::byte>(n),SourceRuntimeMemory::Access::ReadWrite,"native CPU DoF recipe scratch, never a GPU resource");
}
}
std::vector<SourceCampaignDofCommandTu15> source_campaign_dof_recipe_tu15(
    float far_blur,float near_blur,float focal_distance,std::uint32_t current_target) {
    if(far_blur>16777216.f || near_blur>16777216.f)
        throw FormatError("DoF positive blur amount reaches its nonadvancing original loop");
    SourceRuntimeMemory m;
    writable(m,0x40eeb0u,0x200);writable(m,0x411568u,12);writable(m,0x491788u,240);
    writable(m,0x4920d8u,4);writable(m,0x90000000u,0x1000);writable(m,0x60000000u,65536);
    // Every literal below is read by the authenticated complete source
    // bodies. 48D4F4 is the declared CRT default policy input. Error handlers
    // themselves remain required owners instead of producing guessed values.
    const auto literal=[&](unsigned p,std::initializer_list<unsigned> words){
        m.mount(p,std::as_bytes(std::span(words.begin(),words.size())),SourceRuntimeMemory::Access::ReadOnly,"exact OG Xbox TU1.5 DoF/CRT operand");
    };
    literal(0x4202c8u,{0xffffffffu,0x7fefffffu});
    literal(0x4202d0u,{0x00000000u,0x00100000u});
    literal(0x4202d8u,{0x00000000u,0xc0980000u});
    literal(0x4202e0u,{0x00000000u,0x40980000u});
    literal(0x4202e8u,{0x00000000u,0x7ff00000u});
    literal(0x4202f0u,{0x00000000u,0x00000000u});
    literal(0x46a0ccu,{0x3f000000u,0x3f800000u});literal(0x46a0e8u,{0});
    literal(0x46a11cu,{0x40000000u});literal(0x46a148u,{0x437f0000u});
    literal(0x46aa58u,{0x60000000u,0x3fd51eb8u});literal(0x473360u,{0,0x3fe00000u});
    literal(0x473368u,{0x00776f70u});literal(0x48d4f4u,{0});
    const std::array<std::byte,10> indefinite{std::byte{0},std::byte{0},std::byte{0},std::byte{0},std::byte{0},std::byte{0},std::byte{0},std::byte{0xc0},std::byte{0xff},std::byte{0xff}};
    const std::array<std::byte,10> infinity{std::byte{0},std::byte{0},std::byte{0},std::byte{0},std::byte{0},std::byte{0},std::byte{0},std::byte{0x80},std::byte{0xff},std::byte{0x7f}};
    m.mount(0x46ec98u,indefinite,SourceRuntimeMemory::Access::ReadOnly,"exact source CRT extended indefinite operand");
    m.mount(0x4733ecu,infinity,SourceRuntimeMemory::Access::ReadOnly,"exact source CRT extended infinity operand");
    put(m,0x411568u,0x90000000u);put(m,0x4920d8u,current_target);
    constexpr unsigned sp=0x60008000u,stop=0x70000000u;put(m,sp,stop);
    const std::array values{std::bit_cast<unsigned>(far_blur),std::bit_cast<unsigned>(near_blur),std::bit_cast<unsigned>(focal_distance)};
    m.write(sp+4,std::as_bytes(std::span(values)));
    SourcePhysicsWorldRuntimeAbi a{};a.gp[4]=sp;a.eflags=0x202;a.floating_point=SourcePhysicsFactoryFloatingPoint{0x27f,0x1f80};
    std::vector<SourceCampaignDofCommandTu15> output;
    SourcePhysicsWorldRuntimeServices services;
    services.required_owner=[&](SourcePhysicsContactMemory &,const SourcePhysicsContactRuntimeCall &call,
        SourcePhysicsWorldRuntimeAbi &abi,const SourcePhysicsContactRecordObserver &) {
        const auto entry=call.target;const auto stack=abi.gp[4];unsigned cleanup{};
        SourceCampaignDofCommandTu15 c;c.source_entry=entry;c.source_callsite=call.instruction;
        switch(entry) {
        case 0x1ccd0u:case 0x4044b0u:
            c.kind=SourceCampaignDofCommandKindTu15::program;
            for(unsigned i=0;i<60;++i)c.words.push_back(word(m,0x491788u+i*4));
            break;
        case 0x14f40u:c.kind=SourceCampaignDofCommandKindTu15::texture;c.words={abi.gp[7],abi.gp[3]};break;
        case 0x15760u:c.kind=SourceCampaignDofCommandKindTu15::texture_state;c.words={abi.gp[1],word(m,stack+4)};cleanup=4;break;
        case 0x222b0u:c.kind=SourceCampaignDofCommandKindTu15::render_state;c.words={abi.gp[6],abi.gp[7]};break;
        case 0x34a80u:
            c.kind=SourceCampaignDofCommandKindTu15::draw;
            for(unsigned i=0;i<11;++i)c.words.push_back(word(m,stack+4+i*4));
            cleanup=44;break;
        default:throw FormatError("DoF recipe reached unowned original boundary "+std::to_string(entry)+" at "+std::to_string(call.instruction));
        }
        if(call.tail)throw FormatError("DoF void recipe boundary unexpectedly became a tail call");
        if(word(m,stack)!=call.instruction+5u)throw FormatError("DoF void recipe return address differs from its actual CALL");
        output.push_back(std::move(c));abi.gp[4]=stack+4+cleanup;
        // This void submission owner performs its original stack cleanup.
        // No EAX/XMM result or successful resource is supplied.
    };
    source_run_campaign_dof_recipe_tu15(m,0x21a60u,a,*a.floating_point,services);
    if(a.gp[4]!=sp+16 || a.x87_depth)throw FormatError("DoF source recipe did not complete its original RET12/FP ABI");
    return output;
}
}
