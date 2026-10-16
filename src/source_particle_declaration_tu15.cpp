#include "halo2/source_particle_declaration_tu15.hpp"
#include "halo2/source_effect_particle_draw_tu15.hpp"
#include <algorithm>

namespace halo2 {
namespace {
constexpr std::uint32_t workspace=0xf7610000,context=workspace+0x100,
    output=workspace+0x500,sp=workspace+0x1800,stop=workspace+0x1ffc;
class DeclarationMemory final : public SourcePhysicsContactMemory {
public:
    explicit DeclarationMemory(SourcePhysicsContactMemory &source):source_(source) {
        local.mount(workspace,std::vector<std::byte>(8192),SourceRuntimeMemory::Access::ReadWrite,
            "native original particle declaration caller/cache workspace");
        mount_source_particle_declaration_data_tu15(local);
    }
    void read(std::uint32_t p,std::span<std::byte> out) const override {
        for(const auto [base,n]:ranges) {
            if(p<std::uint64_t(base)+n && std::uint64_t(p)+out.size()>base) {
                if(p<base || out.size()>n || p-base>n-out.size())
                    throw FormatError("Particle declaration read crosses its native workspace/data owner");
                local.read(p,out);return;
            }
        }
        source_.read(p,out);
    }
    void write(std::uint32_t p,std::span<const std::byte> in) override {
        if(p<workspace || in.size()>8192 || p-workspace>8192-in.size())
            throw FormatError("Original particle declaration escaped its native output workspace");
        local.write(p,in);
    }
    SourceRuntimeMemory local;
private:
    SourcePhysicsContactMemory &source_;
    static constexpr std::array<std::pair<std::uint32_t,std::size_t>,4> ranges{{
        {workspace,8192},{0x357f4,72},{0x35878,72},{0x44983a,21}}};
};
std::uint32_t word(const SourcePhysicsContactMemory &m,std::uint32_t p) {
    std::array<std::byte,4> b;m.read(p,b);return Bytes(b).u32(0);
}
}
SourceParticleDeclarationTu15 source_particle_declaration_tu15(
    SourcePhysicsContactMemory &source,const ParticleVertexSelection &vertex) {
    if(vertex.vertex_datum==0xffffffffu || vertex.classification<7 ||
       vertex.classification>9 || vertex.program.empty())
        throw FormatError("Particle declaration requires an actual original vertex-tag selection");
    DeclarationMemory memory(source);
    const auto table=word(source,0x4f0414);
    const auto tag=word(source,table+(vertex.vertex_datum&65535u)*16+8);
    if(word(source,tag+4)<=vertex.classification)
        throw FormatError("Particle declaration classification escapes its actual vertex tag");
    const auto row=std::uint64_t(word(source,tag+8))+vertex.classification*28;
    if(row>0xffffffe3ull)throw FormatError("Particle declaration tag row wraps source storage");
    const auto semantics=word(source,unsigned(row)+4),list=word(source,unsigned(row)+8);
    if(semantics>21 || std::uint64_t(list)+semantics*2>0x100000000ull)
        throw FormatError("Particle declaration semantic list exceeds the original21-entry workspace");
    for(unsigned n=0;n<semantics;++n) {
        std::array<std::byte,2> bytes;source.read(list+n*2,bytes);
        if(Bytes(bytes).u16(0)>=21)throw FormatError("Particle declaration semantic escapes the original workspace");
    }
    const auto put=[&](unsigned p,unsigned v){memory.local.write(p,std::as_bytes(std::span(&v,1)));};
    const auto call=[&](unsigned entry,SourcePhysicsWorldRuntimeAbi &abi) {
        abi.gp[4]=sp;abi.eflags=0x202;abi.floating_point=SourcePhysicsFactoryFloatingPoint{0x27f,0x1f80};put(sp,stop);
        SourcePhysicsWorldRuntimeServices services;services.required_owner=[](auto &,const auto &,auto &,const auto &){throw FormatError("Original particle declaration requires an unowned callee");};
        source_run_effect_particle_draw_tu15(memory,entry,abi,*abi.floating_point,services);
    };
    SourcePhysicsWorldRuntimeAbi lookup{};lookup.gp[0]=vertex.classification;
    lookup.gp[1]=vertex.vertex_datum;lookup.gp[2]=workspace+0x40;call(0x1c2c0,lookup);
    std::vector<std::byte> actual(vertex.program.size());memory.read(lookup.gp[0],actual);
    if(actual!=vertex.program)throw FormatError("Particle declaration belongs to a different original program");
    SourcePhysicsWorldRuntimeAbi reset{};reset.gp[2]=context;call(0x1c690,reset);
    // Declared native selection from the actual material. Whole1C570 uses
    // slot0 in this context;247F7F supplies this exact packed declaration.
    put(context+4,vertex.vertex_datum);put(context+8,vertex.classification);
    put(context+0x80,0);put(context+0x8c,0x44983a);
    put(sp+4,context);put(sp+8,output);SourcePhysicsWorldRuntimeAbi build{};call(0x1c310,build);
    SourceParticleDeclarationTu15 result;result.vertex_datum=vertex.vertex_datum;
    result.classification=vertex.classification;
    memory.local.read(output,std::as_writable_bytes(std::span(result.elements)));
    for(unsigned n=0;n<16;++n)if(result.elements[n][2]!=2)result.input_mask|=1u<<n;
    return result;
}
}
