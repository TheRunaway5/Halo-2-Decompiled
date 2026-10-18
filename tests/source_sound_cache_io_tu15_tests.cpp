#include "halo2/source_sound_cache_io_tu15.hpp"
#include <algorithm>
#include <cstring>
#include <fstream>
#include <iostream>
#include <map>
#include <nlohmann/json.hpp>
#include <optional>
using namespace halo2;
using J=nlohmann::json;
namespace {
void require(bool value,const char *message){if(!value)throw std::runtime_error(message);}
struct Memory final:SourcePhysicsContactMemory {
    std::vector<std::byte> data;
    std::uint32_t cr3,cr4;
    std::optional<std::uint32_t> rejected_write;
    Memory(const char *path,const J &input):cr3(input["control_registers"]["CR3"]),cr4(input["control_registers"]["CR4"]) {
        std::ifstream file(path,std::ios::binary|std::ios::ate);require(bool(file),"Original RAM unavailable");
        data.resize(std::size_t(file.tellg()));require(data.size()==67108864,"Original RAM extent differs");
        file.seekg(0);file.read(reinterpret_cast<char *>(data.data()),data.size());require(bool(file),"Original RAM truncated");
    }
    std::uint32_t physical_word(std::uint32_t p) const {
        require(std::uint64_t(p)+4<=data.size(),"Physical word escapes");std::uint32_t v;std::memcpy(&v,data.data()+p,4);return v;
    }
    std::pair<std::uint32_t,bool> physical(std::uint32_t p) const {
        const auto directory=physical_word((cr3&0xfffff000u)+(p>>22)*4u);require(directory&1,"Original PDE absent");
        if(directory&128u){require(cr4&16u,"Original PSE absent");return {(directory&0xffc00000u)+(p&0x3fffffu),bool(directory&2u)};}
        const auto table=physical_word((directory&0xfffff000u)+((p>>12)&1023u)*4u);require(table&1,"Original PTE absent");
        return {(table&0xfffff000u)+(p&4095u),bool(table&directory&2u)};
    }
    void read(std::uint32_t p,std::span<std::byte> output) const override {
        while(!output.empty()){const auto [at,w]=physical(p);const auto n=std::min<std::size_t>(output.size(),4096-(p&4095));
            require(std::uint64_t(at)+n<=data.size(),"Original read escapes RAM");std::memcpy(output.data(),data.data()+at,n);p+=std::uint32_t(n);output=output.subspan(n);}
    }
    void write(std::uint32_t p,std::span<const std::byte> input) override {
        if(rejected_write && p==*rejected_write)throw std::runtime_error("Declared native backing failure");
        while(!input.empty()){const auto [at,w]=physical(p);require(w,"Original destination is not writable");
            const auto n=std::min<std::size_t>(input.size(),4096-(p&4095));require(std::uint64_t(at)+n<=data.size(),"Original write escapes RAM");
            std::memcpy(data.data()+at,input.data(),n);p+=std::uint32_t(n);input=input.subspan(n);}
    }
    std::uint32_t word(std::uint32_t p) const {std::array<std::byte,4>b;read(p,b);std::uint32_t v;std::memcpy(&v,b.data(),4);return v;}
    void word(std::uint32_t p,std::uint32_t v){std::array<std::byte,4>b;std::memcpy(b.data(),&v,4);write(p,b);}
    unsigned byte(std::uint32_t p) const {std::array<std::byte,1>b;read(p,b);return std::to_integer<unsigned>(b[0]);}
};
SourcePhysicsWorldRuntimeAbi abi(const J &input) {
    SourcePhysicsWorldRuntimeAbi a{};a.gp=input["gp"];a.xmm=input["xmm"];a.eflags=input["eflags"];a.fs_base=input["fs_base"];
    a.x87_status=input["x87_status"];a.floating_point=SourcePhysicsFactoryFloatingPoint{input["x87_control"],input["mxcsr"]};return a;
}
}
int main(int argc,char **argv) {
    try {
        if(argc!=5)throw std::invalid_argument("Expected original CPU JSON, RAM, result and native RAM output");
        J input;std::ifstream(argv[1])>>input;Memory initial(argv[2],input);
        const auto original=abi(input);
        const auto sp=original.gp[4],size=initial.word(sp+4),destination=initial.word(sp+8),done=initial.word(sp+16);
        const SourcePhysicsContactRuntimeCall call{0x21f4bdu,0x1a3b70u,false};J controls=J::array();
        for(const std::string kind:{"unrelated","target","frame","encoded","missing","read_failure","short_read","write_failure","receipt_failure"}) {
            Memory memory=initial;auto state=original;auto selected=call;unsigned reads=0,receipts=0;
            if(kind=="unrelated")selected.instruction=0x1301b0u;
            if(kind=="target")selected.target^=1u;
            if(kind=="frame")memory.word(sp+20,4);
            if(kind=="encoded")state.gp[1]^=1u;
            if(kind=="write_failure")memory.rejected_write=destination;
            SourceSoundCacheIoTu15Services services;
            services.read=[&](auto encoded,auto n){++reads;if(kind=="read_failure")throw std::runtime_error("Declared native read failure");return std::vector<std::byte>(kind=="short_read"?n-1:n);};
            services.retain_completed_read=[&](auto,auto,auto){++receipts;if(kind=="receipt_failure")throw std::runtime_error("Declared receipt failure");return 0xf8000001u;};
            if(kind=="missing")services.read={};
            bool threw=false,handled=false;try{handled=source_try_sound_cache_io_tu15(memory,selected,state,services);}catch(const std::exception&){threw=true;}
            if(kind=="unrelated")require(!handled&&!threw&&!reads&&!receipts,"Unrelated call was consumed");
            else require(threw,"Invalid/failed native owner did not reject");
            require(memory.byte(done)==initial.byte(done),"A rejected read published completion");
            require(state.gp==original.gp || (kind=="encoded"&&state.gp[4]==sp),"Rejected owner consumed its caller ABI");
            if(kind=="target"||kind=="frame"||kind=="encoded"||kind=="missing")require(!reads&&!receipts,"Malformed request reached I/O");
            if(kind!="receipt_failure")require(!receipts,"Failed read/write retained a receipt");
            controls.push_back({{"name",kind},{"passed",true},{"reads",reads},{"receipts",receipts}});
        }
        Memory memory=initial;auto state=original;std::map<std::uint32_t,std::array<std::uint32_t,3>> receipts;
        std::uint64_t next=0xf8000001u;unsigned reads=0;
        SourceSoundCacheIoTu15Services services;
        services.read=[&](auto encoded,auto n){
            ++reads;require(encoded>>30==2u,"Reference procedure requires its actual shared-file input");
            std::ifstream file(input["owned_file"].get<std::string>(),std::ios::binary);require(bool(file),"Bound original shared file unavailable");
            file.seekg(encoded&0x3fffffffu);std::vector<std::byte> bytes(n);
            file.read(reinterpret_cast<char *>(bytes.data()),n);require(file.gcount()==n,"Bound file read incomplete");return bytes;
        };
        services.retain_completed_read=[&](auto descriptor,auto buffer,auto n){
            const auto id=std::uint32_t(next++);require(receipts.emplace(id,std::array{descriptor,buffer,n}).second,"Native receipt reused");return id;
        };
        require(source_try_sound_cache_io_tu15(memory,call,state,services),"Actual sound-cache call was not handled");
        require(reads==1&&receipts.size()==1&&memory.byte(done)==1,"Actual read/receipt/completion differs");
        require(state.gp[4]==sp+28&&receipts.contains(state.gp[0]),"Actual native call ABI/receipt differs");
        for(unsigned i=1;i<8;++i)if(i!=4)require(state.gp[i]==original.gp[i],"Native boundary changed a retained register");
        const auto retained=receipts.at(state.gp[0]);require(retained==std::array{original.gp[3],destination,size},"Receipt lost actual descriptor/buffer/extent");
        J result={{"passed",true},{"negative_controls",controls},{"bytes",size},{"destination",destination},{"done",done},
            {"actual_reads",reads},{"retained_receipt",state.gp[0]},{"receipt",retained},{"post_GP",state.gp},
            {"expected_bytes_or_original_kernel_result_supplied",false},{"native_synchronous_IO_policy",true}};
        std::ofstream(argv[3])<<result.dump()<<'\n';
        std::ofstream out(argv[4],std::ios::binary);out.write(reinterpret_cast<const char*>(memory.data.data()),memory.data.size());require(bool(out),"Native RAM observation failed");
    } catch(const std::exception &error){std::cerr<<error.what()<<'\n';return 1;}
}
