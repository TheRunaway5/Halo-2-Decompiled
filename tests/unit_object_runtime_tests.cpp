#include "halo2/unit_object_runtime.hpp"
#include "halo2/source_runtime_memory.hpp"
#include <filesystem>
#include <fstream>
#include <iostream>
#include <map>
#include <optional>
#include <sstream>
#include <stdexcept>

using namespace halo2;
namespace {
void check(bool b,const std::string &s){if(!b)throw std::runtime_error(s);}
std::uint32_t number(const std::string &s){return std::stoul(s,nullptr,16);}
std::vector<std::byte> bytes(const std::string &s){
    check(s.size()%2==0,"byte extent");std::vector<std::byte>b;
    for(std::size_t i=0;i<s.size();i+=2)b.push_back(std::byte(number(s.substr(i,2))));return b;
}
using Store=SourcePhysicsContactRecordStore;
struct Boundary {
    std::uint32_t instruction{},address{},size{};
    std::array<std::uint32_t,8> gp{};
    std::array<std::array<std::uint32_t,4>,8> xmm{};
};
struct Case {
    std::string name;std::uint32_t entry{},result{},return_mask{};std::int32_t esp_delta{};
    SourceUnitAnimationRuntimeAbi abi{};SourceRuntimeMemory memory;
    std::vector<Store> stores;
    std::map<std::uint32_t,std::vector<std::byte>> before,after;
    std::optional<Boundary> missing,callee;
};
}
int main(int argc,char **argv){std::string running;try{
    check(argc==2,"Supply original Unit construction fixture");
    if(!std::filesystem::exists(argv[1]))return 77;
    std::ifstream input(argv[1]);std::vector<Case>cases;
    for(std::string line;std::getline(input,line);){
        if(line.empty()||line[0]=='#')continue;
        std::istringstream in(line);std::string kind,t;in>>kind;
        auto n=[&]{check(bool(in>>t),"missing numeric field");return number(t);};
        if(kind=="C"){
            Case c;in>>c.name;c.entry=n();c.result=n();in>>c.esp_delta;
            for(auto &x:c.abi.gp)x=n();c.abi.eflags=2;cases.push_back(std::move(c));continue;
        }
        check(!cases.empty(),"missing fixture case");auto &c=cases.back();
        if(kind=="M"||kind=="A"){
            const auto p=n();in>>t;auto b=bytes(t);
            if(kind=="M"){
                c.memory.mount(p,b,SourceRuntimeMemory::Access::ReadWrite,"original construction input");
                c.before.emplace(p,b);
            }else c.after.emplace(p,std::move(b));
        }else if(kind=="W"){
            Store s;s.instruction=n();s.address=n();in>>t;s.before=bytes(t);in>>t;s.written=bytes(t);c.stores.push_back(std::move(s));
        }else if(kind=="D"||kind=="B"){
            Boundary b;b.instruction=n();b.address=n();if(kind=="D")b.size=n();
            for(auto &x:b.gp)x=n();if(kind=="D")c.missing=b;else c.callee=b;
        }else if(kind=="X"){
            auto &b=c.missing?*c.missing:*c.callee;for(auto &lane:b.xmm)for(auto &x:lane)x=n();
        }else if(kind=="R")c.return_mask=n();
        else check(false,"unknown original fixture record "+kind);
    }
    check(cases.size()==14,"missing original construction controls");
    std::size_t total=0,backing_bytes=0,returned=0,missing=0,callees=0,partial=0;
    for(auto &c:cases){
        running=c.name;std::size_t cursor=0;std::uint32_t last=0;const auto esp=c.abi.gp[4];
        auto observe=[&](const Store &s){
            check(cursor<c.stores.size(),running+" extra store");const auto &expected=c.stores[cursor++];
            if(s!=expected){std::ostringstream msg;msg<<running<<" store "<<cursor<<" actual "<<std::hex<<s.instruction<<':'<<s.address<<" expected "<<expected.instruction<<':'<<expected.address;throw std::runtime_error(msg.str());}
        };
        SourceUnitAnimationRuntimeServices services;
        services.instruction=[&](auto p){last=p;};
        services.required_owner=[&](auto &,const auto &call,auto &abi,const auto &){
            check(c.callee && call.instruction==c.callee->instruction&&call.target==c.callee->address&&abi.gp==c.callee->gp,running+" unexpected external owner");
            throw std::runtime_error("expected external owner");
        };
        bool stopped=false;
        try{source_run_unit_object_runtime_1_1(c.memory,c.entry,c.abi,{0x27f,0x1f80},services,observe);}
        catch(const std::out_of_range &){
            check(c.missing&&last==c.missing->instruction,running+" unexpected missing backing instruction");
            // The memory adapter itself refuses the missing original read.
            // No callback supplies a resource, successful result or final state.
            bool absent=false;try{c.memory.view(c.missing->address,c.missing->size);}catch(const std::out_of_range &){absent=true;}
            check(absent,running+" original unavailable backing was fabricated");stopped=true;++missing;
        }catch(const std::runtime_error &e){
            if(!c.callee||std::string(e.what())!="expected external owner")throw;stopped=true;
        }
        check(stopped==bool(c.missing||c.callee),running+" original boundary differs");
        if(stopped){const auto &b=c.missing?*c.missing:*c.callee;
            check(c.abi.gp==b.gp,running+" original boundary GP/stack differs");
            check(c.abi.xmm==b.xmm,running+" original boundary XMM differs");
        }else{check((c.abi.gp[0]&c.return_mask)==(c.result&c.return_mask),running+" original return differs");++returned;}
        check(c.abi.gp[4]==esp+c.esp_delta,running+" original stack ownership differs");
        // MXCSR bits0..5 are sticky arithmetic exception status, not controls.
        // The Unicorn reference does not establish Xbox exception status.
        check(c.abi.floating_point&&(c.abi.floating_point->mxcsr&~0x3fu)==0x1f80&&c.abi.floating_point->x87_control_word==0x27f,running+" original FP controls differ");
        check(cursor==c.stores.size(),running+" missing stores");
        for(const auto &[p,b]:c.after){std::vector<std::byte>a(b.size());c.memory.read(p,a);check(a==b,running+" original final backing differs at "+std::to_string(p));backing_bytes+=b.size();}
        total+=cursor;
        if(c.callee){
            ++callees;
            const auto clone=[&]{SourceRuntimeMemory m;for(const auto &[p,b]:c.before)m.mount(p,b,SourceRuntimeMemory::Access::ReadWrite,"required-owner failure control");return m;};
            auto absent_memory=clone();auto absent=c.abi;
            // Restore the declared root call, not the boundary ABI.
            absent.gp={0x11111111,0x22222222,0x33333333,0x44444444,esp,0x66666666,0x77777777,0x88888888};
            absent.xmm={};absent.mmx={};absent.eflags=2;absent.x87={};absent.x87_depth=0;absent.x87_status=0;absent.fs_base=0;absent.floating_point.reset();
            const auto root=absent;bool rejected=false;
            try{source_run_unit_object_runtime_1_1(absent_memory,c.entry,absent,{0x27f,0x1f80},{});}
            catch(const std::runtime_error &e){rejected=std::string(e.what())=="Required original Unit construction callee unavailable";}
            check(rejected&&absent.gp==c.callee->gp,running+" missing lower owner altered original boundary");
            auto partial_memory=clone();auto interrupted=root;SourceUnitAnimationRuntimeAbi changed{};
            SourceUnitAnimationRuntimeServices injected;
            injected.required_owner=[&](auto &,const auto &,auto &a,const auto &){
                for(unsigned i=0;i<8;++i){a.gp[i]^=0x13579bdfu+i;a.mmx[i]=0x1234567887654321ull+i;a.x87[i]=static_cast<long double>(i)+0.25L;for(unsigned j=0;j<4;++j)a.xmm[i][j]=0x3f000000u+i*4+j;}
                a.eflags=0x8d7;a.fs_base=0x12340000;a.x87_depth=3;a.x87_status=0x2100;changed=a;
                throw std::runtime_error("partial owner failure");
            };
            rejected=false;
            try{source_run_unit_object_runtime_1_1(partial_memory,c.entry,interrupted,{0x27f,0x1f80},injected);}
            catch(const std::runtime_error &e){rejected=std::string(e.what())=="partial owner failure";}
            check(rejected&&interrupted.gp==changed.gp&&interrupted.mmx==changed.mmx&&interrupted.xmm==changed.xmm&&interrupted.x87==changed.x87&&interrupted.x87_depth==changed.x87_depth&&interrupted.x87_status==changed.x87_status&&interrupted.eflags==changed.eflags&&interrupted.fs_base==changed.fs_base,running+" partial lower-owner ABI was lost");
            ++partial;
        }
    }
    std::cout<<returned<<" original construction/query returns, "<<missing<<" original resource-backing stops, "<<callees<<" required-owner stops, "<<total<<" ordered stores, "<<backing_bytes<<" compared bytes; "<<partial<<" absent/partial-owner controls; zero supplied lower results and zero replayed calls\n";
    return 0;
}catch(const std::exception &e){std::cerr<<running<<' '<<e.what()<<'\n';return 1;}}
