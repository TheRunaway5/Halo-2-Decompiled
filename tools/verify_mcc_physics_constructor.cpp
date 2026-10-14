#include "halo2/physics_world_runtime.hpp"
#include "halo2/physics_live_support_tu15.hpp"
#include <nlohmann/json.hpp>
#include <algorithm>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <map>
#include <set>
#include <sstream>

namespace {
using Buffer=std::vector<std::byte>;
using Json=nlohmann::json;
void check(bool value,const std::string& message){if(!value)throw std::runtime_error(message);}
Buffer hex(const std::string& value) {
    check(value.size()%2==0,"Invalid original operand extent");Buffer result;
    for(std::size_t i=0;i<value.size();i+=2)result.push_back(std::byte(std::stoul(value.substr(i,2),nullptr,16)));
    return result;
}
Buffer read_file(const std::filesystem::path& path) {
    std::ifstream stream(path,std::ios::binary|std::ios::ate);check(bool(stream),"Missing reference packet: "+path.string());
    const auto size=stream.tellg();check(size>=0 && size<=128*1024*1024,"Excessive reference packet");
    Buffer result(static_cast<std::size_t>(size));stream.seekg(0);
    check(bool(stream.read(reinterpret_cast<char*>(result.data()),std::streamsize(result.size()))),"Truncated reference packet");return result;
}
void word(Buffer& bytes,std::size_t at,std::uint32_t value){for(unsigned i=0;i<4;++i)bytes.at(at+i)=std::byte(value>>(8*i));}
struct Memory final:halo2::SourcePhysicsContactMemory {
    struct Region {Buffer bytes;bool writable;};std::map<std::uint32_t,Region> regions;
    std::set<std::uint32_t> rebound;
    mutable std::set<std::uint32_t> written;
    bool binding=false;
    auto at(std::uint32_t address,std::size_t size)const {
        auto i=regions.upper_bound(address);
        if(i!=regions.begin()) {
            --i;if(std::uint64_t(address)-i->first+size<=i->second.bytes.size())return i;
        }
        std::ostringstream s;s<<"Constructor requested undeclared backing at "<<std::hex<<address<<" size "<<size;
        throw std::runtime_error(s.str());
    }
    void read(std::uint32_t address,std::span<std::byte> out)const override {
        const auto i=at(address,out.size());std::copy_n(i->second.bytes.begin()+address-i->first,out.size(),out.begin());
    }
    void write(std::uint32_t address,std::span<const std::byte> in)override {
        const auto i=at(address,in.size());check(i->second.writable,"Constructor modified an immutable program operand");
        auto& region=regions.at(i->first);std::copy(in.begin(),in.end(),region.bytes.begin()+address-i->first);
        if(binding)for(std::size_t byte=0;byte<in.size();++byte)written.insert(address+byte);
    }
};
}
int main(int argc,char** argv)try {
    check(argc==2,"usage: verify_mcc_physics_constructor ORIGINAL_CONSTRUCTOR_REPORT_JSON");
    std::ifstream file(argv[1]);const auto report=Json::parse(file);
    check(report.at("schema")=="H2_MCC_PHYSICS_TU15_CONSTRUCTOR_ORACLE_V1","Wrong original comparison report");
    check(report.at("reference_xbe_sha256")=="21b207e7efdacaf6124a0822837f8be570766a42505057f7fd2d2ba8f409c4fd","Wrong original update reference");
    const auto group=report.value("tag_group",std::string("phmo"));
    check(group=="phmo" || group=="coll" || group=="sbsp","Unsupported constructor comparison group");
    const bool collision=group!="phmo";
    std::size_t cases=0,phases=0,stores=0;
    for(const auto& row:report.at("rows")) {
        if(row.contains("duplicate_of"))continue;
        const auto base=row.at("address").get<std::uint32_t>(),datum=row.at("datum").get<std::uint32_t>();
        const std::filesystem::path packet=row.at("packet_path").get<std::string>();
        Memory memory;memory.regions.emplace(base,Memory::Region{read_file(packet),true});
        if(row.contains("runtime_pointer_offsets"))for(const auto& offset:row.at("runtime_pointer_offsets"))
            for(unsigned byte=0;byte<4;++byte)memory.rebound.insert(base+offset.get<std::uint32_t>()+byte);
        check(memory.regions.at(base).bytes.size()==row.at("bytes").get<std::size_t>(),"Changed reference packet extent");
        Buffer global(4);word(global,0,0x70000000u);memory.regions.emplace(0x4f0414u,Memory::Region{std::move(global),false});
        Buffer directory(0x100000);const auto at=(datum&65535u)*16;
        word(directory,at,group=="coll"?0x636f6c6cu:group=="sbsp"?0x73627370u:0x70686d6fu);word(directory,at+4,datum);word(directory,at+8,base);
        word(directory,at+12,row.at("bytes"));memory.regions.emplace(0x70000000u,Memory::Region{std::move(directory),false});
        for(const auto& phase:row.at("phases"))for(const auto& read:phase.at("program_reads")) {
            const auto address=read.at("address").get<std::uint32_t>();const auto bytes=hex(read.at("bytes"));
            if(memory.regions.contains(address))check(memory.regions.at(address).bytes==bytes,"Original immutable operand changed between phases");
            else memory.regions.emplace(address,Memory::Region{bytes,false});
        }
        for(const auto& phase:row.at("phases")) {
            memory.binding=phase.at("clear")==0;memory.written.clear();
            const auto size=row.at("bytes").get<std::size_t>();
            Buffer stack(0x10000);word(stack,0x8000,0x73000000u);word(stack,0x8004,phase.at("clear"));
            memory.regions.insert_or_assign(0x72000000u,Memory::Region{std::move(stack),true});
            halo2::SourcePhysicsWorldRuntimeAbi state{};state.gp=phase.at("initial_gp").get<std::array<std::uint32_t,8>>();
            state.eflags=0x202u;state.floating_point=halo2::SourcePhysicsFactoryFloatingPoint{0x27f,0x1f80};
            std::vector<halo2::SourcePhysicsContactRecordStore> actual;
            halo2::SourcePhysicsWorldRuntimeServices services;
            services.required_owner=[&](auto& m,const auto& call,auto& abi,const auto& observer) {
                if(halo2::source_physics_live_support_tu15_owns_entry(call.target)) {
                    halo2::source_run_physics_live_support_tu15(m,call.target,abi,{0x27f,0x1f80},services,observer);
                    return;
                }
                std::ostringstream s;s<<"Unowned constructor callee "<<std::hex<<call.target<<" at "<<call.instruction;
                throw std::runtime_error(s.str());
            };
            const auto observe=[&](const auto& store){
                    if(store.address>=base && std::uint64_t(store.address)-base<size)actual.push_back(store);
                };
            if(collision)halo2::source_run_physics_live_support_tu15(memory,group=="coll"?0x21a580u:0x171120u,state,{0x27f,0x1f80},services,observe);
            else halo2::source_run_physics_world_runtime(memory,0x1ee380u,state,halo2::SourcePhysicsContactImage::TitleUpdate15,
                {0x27f,0x1f80},services,observe);
            const auto expected=read_file(packet.parent_path()/phase.at("after_file").get<std::string>());
            const auto name=row.at("path").get<std::string>()+" clear="+phase.at("clear").dump();
            if(memory.binding)check(std::includes(memory.written.begin(),memory.written.end(),
                memory.rebound.begin(),memory.rebound.end()),name+": native constructor did not replace every tool pointer");
            check(memory.regions.at(base).bytes==expected,name+": native constructed metadata differs");
            check(state.gp==phase.at("returned_gp").get<std::array<std::uint32_t,8>>(),name+": native returned registers differ");
            check((state.eflags&0xcd5u)==(phase.at("returned_eflags").get<std::uint32_t>()&0xcd5u),name+": native returned defined flags differ");
            const auto& writes=phase.at("stores");check(actual.size()==writes.size(),name+": native ordered store count differs");
            for(std::size_t i=0;i<actual.size();++i) {
                const auto& a=actual[i];const auto& e=writes[i];
                // The shared native world body reports canonical TU1.1 code
                // IDs; live TU1.5 callees report actual TU1.5 IDs. Those are
                // diagnostic labels. Compare the complete ordered data stores
                // without inventing a PC mapping between the two label spaces.
                check((!collision || a.instruction==e.at("instruction")) && a.address==e.at("address") &&
                      a.before==hex(e.at("before")) && a.written==hex(e.at("written")),
                      name+": native original store differs at "+std::to_string(i)+
                      " native PC="+std::to_string(a.instruction)+" original PC="+e.at("instruction").dump()+
                      " native address="+std::to_string(a.address)+" original address="+e.at("address").dump()+
                      " beforeEqual="+std::to_string(a.before==hex(e.at("before")))+
                      " writtenEqual="+std::to_string(a.written==hex(e.at("written"))));
            }
            ++phases;stores+=actual.size();
        }
        ++cases;
    }
    std::cout<<"TU1.5 native physics constructors: "<<cases<<" unique packets, "<<phases<<" bind/release phases, "<<stores<<" exact ordered stores passed\n";return 0;
}catch(const std::exception& error){std::cerr<<error.what()<<'\n';return 1;}
