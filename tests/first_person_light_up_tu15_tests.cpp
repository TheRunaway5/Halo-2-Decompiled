#include "halo2/source_first_person_light_pose_tu15.hpp"
#include "halo2/physics_live_support_tu15.hpp"
#include "halo2/native_tu15_light_up_vector.hpp"
#include "halo2/native_tu15_normalize_vector.hpp"
#include <cstring>
#include <algorithm>
#include <fstream>
#include <iostream>
#include <nlohmann/json.hpp>
using namespace halo2;
namespace {
std::vector<std::byte> bytes(const std::string& hex) {
    std::vector<std::byte> out(hex.size()/2);
    for(unsigned n=0;n<out.size();++n)out[n]=std::byte(std::stoul(hex.substr(n*2,2),nullptr,16));
    return out;
}
struct Memory final:SourcePhysicsContactMemory {
    std::array<std::byte,0x20000> data{};
    std::vector<std::byte> constants;
    SourceDirectPageIndex pages;
    void enable_direct() {
        for(std::uint32_t n=0;n<data.size();n+=4096) {
            const auto at=0x1000000u+n;auto &block=pages.blocks[at>>22];
            if(!block)block=std::make_unique<SourceDirectPageIndex::Block>();
            (*block)[(at>>12)&1023u]={data.data()+n,true};
        }
        auto &block=pages.blocks[0x46a000u>>22];
        if(!block)block=std::make_unique<SourceDirectPageIndex::Block>();
        (*block)[(0x46a000u>>12)&1023u]={constants.data(),false};direct_pages_=&pages;
    }
    void read(std::uint32_t at,std::span<std::byte> out)const override {
        if(at>=0x1000000u && std::uint64_t(at)+out.size()<=0x1020000u)
            std::copy_n(data.begin()+at-0x1000000u,out.size(),out.begin());
        else if(at>=0x46a000u && std::uint64_t(at)+out.size()<=0x46a000u+constants.size())
            std::copy_n(constants.begin()+at-0x46a000u,out.size(),out.begin());
        else throw std::runtime_error("FP light up read an undeclared operand");
    }
    void write(std::uint32_t at,std::span<const std::byte> in)override {
        if(at<0x1000000u || std::uint64_t(at)+in.size()>0x1020000u)
            throw std::runtime_error("FP light up wrote outside its declared caller memory");
        std::ranges::copy(in,data.begin()+at-0x1000000u);
    }
};
}
int main(int argc,char**argv)try {
    if(argc!=2)throw std::runtime_error("Supply independent complete31B30 fixture");
    std::ifstream file(argv[1]);const auto fixture=nlohmann::json::parse(file);
    if(fixture.at("original_xbe_sha256")!="21b207e7efdacaf6124a0822837f8be570766a42505057f7fd2d2ba8f409c4fd")
        throw std::runtime_error("FP light pose authority changed");
    if(!source_first_person_light_pose_tu15_owns_entry(0x31b30))throw std::runtime_error("Original FP light up owner is absent");
    Memory memory;memory.constants=bytes(fixture.at("original_constant_page"));memory.enable_direct();
    NativeTu15NormalizeStats normalize_stats;unsigned external_calls{};
    SourcePhysicsWorldRuntimeServices services;
    services.normalize_vector_30bd0_stats=&normalize_stats;
    std::uint32_t position{};bool first_trace=false,second_trace=false;
    const std::array<std::uint32_t,3> service_sites{0x38846e,0x21f590,0x33b595};
    services.instruction=[&](auto pc){position=pc;};
    services.instruction_cursor={&position,&first_trace,&second_trace,service_sites,
        std::this_thread::get_id(),&services.instruction.target_type(),true};
    services.required_owner=[&](auto& m,const auto& call,auto& abi,const auto& observer) {
        if(call.target!=0x30bd0u)throw std::runtime_error("FP light up reached an unqualified child");
        ++external_calls;
        source_run_physics_live_support_tu15(m,call.target,abi,{0x27f,0x1f80},services,observer);
    };
    unsigned index=0;
    for(const auto& row:fixture.at("cases")) {
        SourcePhysicsWorldRuntimeAbi reference{};
        std::uint32_t reference_position{};
        std::array<std::byte,0x20000> reference_memory{};
        for(bool native_tail:{false,true}) {
        memory.data.fill(std::byte{});const auto input=bytes(row.at("input"));
        memory.write(0x1001000u,std::span(input).first(12));memory.write(0x1002000u,std::span(input).subspan(12));
        services.normalize_vector_30bd0=native_tail?native_tu15_normalize_vector_30bd0_resident:nullptr;
        SourcePhysicsWorldRuntimeAbi abi{};abi.gp[0]=0x1001000u;abi.gp[1]=0x1002000u;abi.gp[4]=0x101e800u;abi.eflags=2;abi.fs_base=0x7ffdf000u;
        source_run_first_person_light_pose_tu15(memory,0x31b30u,abi,{0x27f,0x1f80},services);
        std::array<std::byte,12> actual{},forward{};memory.read(0x1001000u,actual);memory.read(0x1002000u,forward);
        if(!std::ranges::equal(actual,bytes(row.at("output"))) || !std::ranges::equal(forward,bytes(row.at("forward"))))
            throw std::runtime_error("FP light up differs from whole original CPU case"+std::to_string(index));
        if(abi.gp[4]!=0x101e804u)throw std::runtime_error("FP light up lost its original caller stack");
        std::array<std::uint32_t,3> up_bits{},forward_bits{};
        std::memcpy(up_bits.data(),input.data(),12);std::memcpy(forward_bits.data(),input.data()+12,12);
        NativeTu15PhysicsFrameFp fp{0x27f,0,0x1f80};
        const auto native=native_tu15_light_orthonormal_up(memory,up_bits,forward_bits,fp);
        if(!std::ranges::equal(std::as_bytes(std::span(native.up)),actual))
            throw std::runtime_error("Native FP light up differs from translated reference"+std::to_string(index));
        bool same_live_x87=abi.x87_depth==reference.x87_depth;
        if(native_tail)for(unsigned n=0;n<abi.x87_depth;++n)
            same_live_x87= same_live_x87 && std::memcmp(&abi.x87[n],&reference.x87[n],10)==0;
        if(!native_tail){reference=abi;reference_memory=memory.data;reference_position=position;}
        else if(abi.gp!=reference.gp || abi.xmm!=reference.xmm || abi.eflags!=reference.eflags ||
            !same_live_x87 || abi.fs_base!=reference.fs_base || abi.x87_status!=reference.x87_status ||
            abi.floating_point->x87_control_word!=reference.floating_point->x87_control_word ||
            abi.floating_point->mxcsr!=reference.floating_point->mxcsr || memory.data!=reference_memory || position!=reference_position)
            throw std::runtime_error("Native30BD0 tail differs from complete live literal ABI/bank/cursor"+std::to_string(index));
        }
        ++index;
    }
    if(normalize_stats.admitted!=index || external_calls!=index || normalize_stats.renderer_admitted!=0)
        throw std::runtime_error("FP light graph did not exercise native and literal normalizer paths");
    std::cout<<"PASS "<<index<<" complete original FP light cases in both modes; "<<normalize_stats.admitted
        <<" native normalizer calls, no native ABI owner round trips\n";return 0;
}catch(const std::exception& error){std::cerr<<error.what()<<'\n';return 1;}
