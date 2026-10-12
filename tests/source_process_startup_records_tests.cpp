#include "halo2/source_process_heap_platform.hpp"
#include "halo2/source_runtime_image.hpp"
#include <algorithm>
#include <fstream>
#include <iostream>
#include <nlohmann/json.hpp>
#include <sstream>

namespace {
using J=nlohmann::json;
using M=halo2::SourceRuntimeMemory;
using A=halo2::SourcePhysicsContactRuntimeAbi;
void check(bool ok,const std::string &why) { if(!ok)throw std::runtime_error(why); }
J load(const std::filesystem::path &p) { std::ifstream in(p);return J::parse(in); }
std::vector<std::byte> bytes(const std::string &s) {
    check(!(s.size()&1),"Invalid captured bytes");std::vector<std::byte> out;
    for(std::size_t i=0;i<s.size();i+=2)out.push_back(std::byte(std::stoul(s.substr(i,2),nullptr,16)));
    return out;
}
void put(M &m,std::uint32_t p,std::uint32_t v) {
    std::array<std::byte,4> b;for(unsigned i=0;i<4;++i)b[i]=std::byte(v>>(i*8));m.write(p,b);
}
A abi(const J &row) {
    A a{};a.gp=row.at("gp").get<decltype(a.gp)>();a.eflags=row.at("eflags");a.x87_status=row.at("x87_status");
    a.floating_point=halo2::SourcePhysicsFactoryFloatingPoint{row.at("x87_control"),row.at("mxcsr")};
    check(row.at("x87_tag")==0xffff,"Capture requires an owned nonempty x87 stack");
    for(unsigned i=0;i<8;++i)for(unsigned k=0;k<4;++k)
        a.xmm[i][k]=std::stoul(row.at("xmm").at(i).get<std::string>().substr(24-k*8,8),nullptr,16);
    return a;
}
void same(const A &actual,const J &expected) {
    const auto e=abi(expected);
    check(actual.gp==e.gp && actual.xmm==e.xmm && ((actual.eflags^e.eflags)&0xcd5u)==0 &&
        actual.fs_base==0 && actual.x87_depth==0 && actual.x87_status==e.x87_status && !actual.active_mmx &&
        actual.floating_point && actual.floating_point->x87_control_word==e.floating_point->x87_control_word &&
        actual.floating_point->mxcsr==e.floating_point->mxcsr,"Pending original startup ABI differs");
}
void store(const halo2::SourceProcessStartupRecordStore &actual,const J &expected) {
    check(actual.address==expected.at("address") && actual.before==bytes(expected.at("before")) &&
        actual.written==bytes(expected.at("written")),"Ordered primary initializer store differs");
}
}
int main(int argc,char **argv) {
    try {
        check(argc==6,"Supply standalone and original-prefix captures, both images and paired report");
        for(int i=1;i<6;++i)if(!std::filesystem::exists(argv[i]))return 77;
#if !defined(__linux__)
        return 77;
#endif
        std::size_t standalone_cases=0,standalone_writes=0,prefix_cases=0,prefix_writes=0,source_writes=0;
        const auto standalone=load(argv[1]);
        for(const auto &row:standalone.at("cases")) {
            const auto arguments=row.at("arguments").get<std::vector<std::uint32_t>>();const auto p=arguments[0];
            const auto initial=bytes(row.at("before_hex"));M m;
            std::vector<std::byte> guarded(16,std::byte{0xc7});guarded.insert(guarded.end(),initial.begin(),initial.end());
            guarded.insert(guarded.end(),16,std::byte{0xc7});m.mount(p-16,guarded,M::Access::ReadWrite,"explicit dirty standalone record and guards");
            std::size_t n=0;
            const auto observe=[&](const auto &change) {
                check(n<row.at("ordered_record_writes").size(),"Unexpected native initializer store");
                store(change,row.at("ordered_record_writes").at(n++));
            };
            if(row.at("ordinal")==107)halo2::source_initialize_process_dpc(m,p,arguments[1],arguments[2],observe);
            else halo2::source_initialize_process_timer(m,p,arguments[1],observe);
            check(n==row.at("ordered_record_writes").size() &&
                std::ranges::equal(m.view(p,initial.size()),bytes(row.at("after_hex"))),"Standalone dirty record differs");
            check(std::ranges::equal(m.view(p-16,16),std::span(guarded).first(16)) &&
                std::ranges::equal(m.view(p+initial.size(),16),std::span(guarded).last(16)),"Standalone guard bytes changed");
            ++standalone_cases;standalone_writes+=n;
        }
        const auto report=load(argv[5]);std::array<std::map<std::uint32_t,std::uint32_t>,2> canonical;
        for(const auto &owner:report.at("owners"))if(owner.at("old_entry")=="002dc5ff") {
            std::array<std::vector<std::uint32_t>,2> sites;
            for(const auto &listing:owner.at("listings")) {
                const std::string name=listing.at("file");const unsigned index=name.ends_with("-u5.asm")?1:0;
                std::ifstream in(std::filesystem::path(argv[5]).parent_path()/name);std::string line;
                while(std::getline(in,line)) { std::istringstream s(line);std::uint32_t p;s>>std::hex>>p;sites[index].push_back(p); }
            }
            check(sites[0].size()==34 && sites[1].size()==34,"Incomplete paired setup body");
            for(unsigned i=0;i<34;++i)for(unsigned image=0;image<2;++image)canonical[image][sites[0][i]]=sites[image][i];
        }
        const auto prefixes=load(argv[2]);
        for(const auto &row:prefixes.at("cases"))for(const bool empty_owner:{false,true}) {
            const bool later=row.at("image")=="u5";
            const auto image=later?halo2::SourcePhysicsContactImage::TitleUpdate15:halo2::SourcePhysicsContactImage::MapPack11;
            auto m=halo2::load_source_runtime_image(argv[later?4:3],image);
            m.mount(0x2000000,std::vector<std::byte>(0x10000),M::Access::ReadWrite,"explicit source prefix stack");
            auto a=abi(row.at("before_abi"));put(m,a.gp[4],0x3000000);
            const auto &calls=row.at("initializer_calls");
            for(const auto &call:calls) {
                const auto p=call.at("ordered_record_writes").at(0).at("address").get<std::uint32_t>();
                const auto match=std::find_if(standalone.at("cases").begin(),standalone.at("cases").end(),[&](const auto &r) {
                    return r.at("image")==row.at("image") && r.at("pattern")==row.at("dirty_pattern") && r.at("ordinal")==call.at("ordinal");
                });
                check(match!=standalone.at("cases").end(),"Dirty prefix input capture missing");m.write(p,bytes(match->at("before_hex")));
            }
            halo2::SourceRuntimeHostVirtualMemory vm(m);halo2::PhysicsPlatformLocks locks;
            std::size_t trace=0,writes=0,record_writes=0,owner_calls=0;bool stopped=false;
            std::vector<J> expected_records;for(const auto &call:calls)for(const auto &w:call.at("ordered_record_writes"))expected_records.push_back(w);
            halo2::SourcePhysicsContactRuntimeServices services;
            services.instruction=[&](auto p) {
                check(trace<row.at("source_trace").size() && canonical[later].at(p)==row.at("source_trace").at(trace++),"Original dirty prefix trace differs");
            };
            services.required_owner=[](auto &,const auto &,auto &,const auto &) { throw std::runtime_error("Unexpected unprovided startup callee"); };
            const auto source_observe=[&](const auto &actual) {
                check(writes<row.at("source_ordered_stores").size(),"Unexpected dirty prefix source store");
                const auto &expected=row.at("source_ordered_stores").at(writes++);
                check(canonical[later].at(actual.instruction)==expected.at("instruction") && actual.address==expected.at("address") &&
                    actual.before==bytes(expected.at("before")) && actual.written==bytes(expected.at("written")),"Dirty prefix source store differs");
            };
            const auto records=[&](const auto &actual) {
                check(record_writes<expected_records.size(),"Unexpected dirty prefix native store");store(actual,expected_records.at(record_writes++));
            };
            halo2::SourceProcessHeapDataImportOwner data;
            if(empty_owner)data=[&](auto &,const auto &) { ++owner_calls; };
            try {
                halo2::source_run_process_heap_with_native_platform(m,0x2dc5ff,a,image,{0x27f,0x1f80},services,vm,locks,{},
                    source_observe,{},{},nullptr,{},records,data);
            } catch(const halo2::SourceProcessHeapDataUnavailable &e) {
                const auto &expected=row.at("boundary");
                check(!empty_owner && e.request.instruction==0x2dc624 && e.request.actual_instruction==expected.at("instruction") &&
                    e.request.cell==expected.at("import_cell") && e.request.ordinal==322 && e.request.bytes==8,"Imported data boundary differs");stopped=true;
            } catch(const std::out_of_range &) { check(empty_owner && owner_calls==1,"Unrelated inaccessible data");stopped=true; }
            check(stopped && trace==row.at("source_trace").size() && writes==row.at("source_ordered_stores").size() &&
                record_writes==expected_records.size(),"Incomplete dirty prefix comparison");same(a,row.at("after_abi"));
            check(std::ranges::equal(m.view(later?0x56edc0:0x566ea0,28),bytes(row.at("DPC_final_hex"))) &&
                std::ranges::equal(m.view(later?0x56ede0:0x566ec0,40),bytes(row.at("timer_final_hex"))),"Dirty prefix retained records differ");
            const auto pending=bytes(row.at("boundary").at("pending_stack_hex"));
            check(std::ranges::equal(m.view(a.gp[4],pending.size()),pending),"Dirty prefix pending stack differs");
            ++prefix_cases;prefix_writes+=record_writes;source_writes+=writes;
        }
        std::cout<<standalone_cases<<" dirty primary initializer cases / "<<standalone_writes<<" stores; "<<prefix_cases
            <<" original dirty prefixes / "<<prefix_writes<<" initializer and "<<source_writes
            <<" source stores; absent and empty data owners stop before hardware load\n";
    } catch(const std::exception &e) { std::cerr<<e.what()<<'\n';return 1; }
}
