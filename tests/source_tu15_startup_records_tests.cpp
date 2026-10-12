#include "halo2/source_tu15_startup_records.hpp"
#include "halo2/source_runtime_image.hpp"
#include <algorithm>
#include <fstream>
#include <iostream>
#include <nlohmann/json.hpp>

namespace {
using J=nlohmann::json;
using M=halo2::SourceRuntimeMemory;
using A=halo2::SourcePhysicsContactRuntimeAbi;
struct SettingsOwnerRequired {};
void check(bool value,const std::string &why) { if(!value)throw std::runtime_error(why); }
std::vector<std::byte> bytes(const std::string &s) {
    check(!(s.size()&1),"Invalid byte capture");std::vector<std::byte> b;
    for(std::size_t i=0;i<s.size();i+=2)b.push_back(std::byte(std::stoul(s.substr(i,2),nullptr,16)));
    return b;
}
void put(M &m,std::uint32_t p,std::uint32_t v) {
    std::array<std::byte,4> b;
    for(unsigned i=0;i<4;++i)b[i]=std::byte(v>>(i*8));m.write(p,b);
}
A state(const J &j) {
    A a{};a.gp=j.at("gp").get<decltype(a.gp)>();a.xmm=j.at("xmm").get<decltype(a.xmm)>();
    a.eflags=j.at("eflags");a.fs_base=j.at("fs_base");a.x87_status=j.at("x87_status");
    a.floating_point=halo2::SourcePhysicsFactoryFloatingPoint{j.at("x87_control_word"),j.at("mxcsr")};return a;
}
void cpu_equal(const A &a,const J &j,std::uint32_t mask) {
    const auto b=state(j);
    check(a.gp==b.gp && a.xmm==b.xmm,"TU1.5 pending GP/XMM ABI differs");
    // TEST leaves AF undefined; the earlier missing-owner boundary follows SUB
    // and also checks its defined AF. Each caller supplies the exact mask.
    check(((a.eflags^b.eflags)&mask)==0 && a.fs_base==b.fs_base &&
        a.x87_depth==0 && a.x87_status==b.x87_status && !a.active_mmx && a.floating_point &&
        a.floating_point->x87_control_word==b.floating_point->x87_control_word &&
        a.floating_point->mxcsr==b.floating_point->mxcsr,"TU1.5 retained flags/FS/FP ABI differs");
}
void change_equal(const halo2::SourcePhysicsContactRecordStore &a,const J &b) {
    check(a.address==b.at("address") && a.before==bytes(b.at("before")) &&
        a.written==bytes(b.at("written")),"Independent primary record store differs");
}
void same_bank(M &m,std::uint32_t p,const std::vector<std::byte> &b) {
    check(std::ranges::equal(m.view(p,b.size()),b),"Retained source memory differs");
}
std::uint32_t word(std::span<const std::byte> b,std::size_t p) {
    std::uint32_t value=0;
    for(unsigned i=0;i<4;++i)value|=std::to_integer<std::uint32_t>(b[p+i])<<(i*8);
    return value;
}
void hardware_loader(const char *path,const halo2::SourceRuntimeKernelHardwareInfo &hardware) {
    auto memory=halo2::load_source_runtime_image(path,halo2::SourcePhysicsContactImage::TitleUpdate15,hardware);
    std::ifstream in(path,std::ios::binary|std::ios::ate);
    std::vector<std::byte> raw(static_cast<std::size_t>(in.tellg()));in.seekg(0);
    check(bool(in.read(reinterpret_cast<char *>(raw.data()),raw.size())),"Cannot read owned image");
    same_bank(memory,word(raw,0x104),{raw.begin(),raw.begin()+word(raw,0x108)});
    const auto table=word(raw,0x120)-word(raw,0x104),count=word(raw,0x11c);
    unsigned bindings=0;
    for(unsigned i=0;i<count;++i) {
        const auto p=table+i*56,va=word(raw,p+4),extent=word(raw,p+8),offset=word(raw,p+12),length=word(raw,p+16);
        std::vector<std::byte> expected(extent);
        std::copy_n(raw.begin()+offset,length,expected.begin());
        if(va<=0x41b66c && std::uint64_t(va)+length>=0x41b670) {
            check(word(expected,0x41b66c-va)==0x80000142,"Literal import ordinal differs");
            for(unsigned k=0;k<4;++k)expected[0x41b66c-va+k]=std::byte(hardware.address>>(k*8));
            ++bindings;
        }
        same_bank(memory,va,expected);
    }
    check(bindings==1,"Loader did not bind exactly one original data cell");
    same_bank(memory,hardware.address,{hardware.bytes.begin(),hardware.bytes.end()});
    for(const auto p:{0x41b66cu,hardware.address}) {
        bool rejected=false;try { memory.mutable_view(p,4); } catch(const std::runtime_error &) { rejected=true; }
        check(rejected,"Loader data lost read-only ownership");
    }
    const halo2::SourceProcessHeapDataImport request{0x2dc624,0x2e00d4,0x41b66c,322,8};
    auto owner=halo2::source_tu15_hardware_import_owner(hardware);owner(memory,request);
    for(unsigned fault=0;fault<7;++fault) {
        auto bad=request;auto resource=hardware;
        if(fault==0)bad.instruction^=1;
        if(fault==1)bad.actual_instruction^=1;
        if(fault==2)bad.cell+=4;
        if(fault==3)bad.ordinal+=1;
        if(fault==4)bad.bytes-=1;
        if(fault==5)resource.bytes[0]^=std::byte{1};
        if(fault==6)resource.address+=16;
        bool rejected=false;
        try { halo2::source_tu15_hardware_import_owner(resource)(memory,bad); }
        catch(const std::exception &) { rejected=true; }
        check(rejected,"Mismatched hardware resource/request admitted");
        same_bank(memory,hardware.address,{hardware.bytes.begin(),hardware.bytes.end()});
    }
    for(unsigned fault=0;fault<3;++fault) {
        auto bad=hardware;
        if(fault==0)bad.address=0x8000ffff;
        if(fault==1)bad.address=0xfffffffc;
        if(fault==2)bad.owner.clear();
        bool rejected=false;
        try { (void)halo2::load_source_runtime_image(path,halo2::SourcePhysicsContactImage::TitleUpdate15,bad); }
        catch(const std::exception &) { rejected=true; }
        check(rejected,"Invalid hardware input owner admitted by loader");
    }
}
}
int main(int argc,char **argv) {
    try {
        check(argc==3,"Supply TU1.5 startup fixture and owned English TU1.5 image");
        for(unsigned i=1;i<3;++i)if(!std::filesystem::exists(argv[i]))return 77;
#if !defined(__linux__)
        return 77;
#endif
        std::ifstream in(argv[1]);const auto f=J::parse(in);
        const bool hardware_supplied=f.at("hardware_data_supplied");
        check(f.at("parity_target")=="English TU1.5 only" && !f.at("older_image_executed").get<bool>() &&
            !f.at("captured_world_input_used").get<bool>() && !f.at("settings_result_supplied").get<bool>() &&
            !f.at("process_heap_installed").get<bool>() && !f.at("live_physics_parity").get<bool>(),"Wrong evidence scope");
        halo2::SourceRuntimeKernelHardwareInfo hardware{};
        if(hardware_supplied) {
            const auto &record=f.at("kernel_input").at("record");const auto input=bytes(record.at("hex"));
            check(input.size()==8 && record.at("bytes")==8 && !f.at("physical_xbox_proof").get<bool>(),"Wrong hardware input extent");
            hardware.address=record.at("address");std::copy(input.begin(),input.end(),hardware.bytes.begin());
            hardware.owner=f.at("kernel_input").at("reference").get<std::string>()+" export322 SHA256 "+record.at("sha256").get<std::string>();
            hardware_loader(argv[2],hardware);
        }
        std::size_t components=0,component_stores=0,cases=0,instructions=0,game_stores=0,record_stores=0,registry_stores=0;
        for(const auto &row:f.value("component_cases",J::array())) {
            M memory;const auto p=row.at("address").get<std::uint32_t>();
            const auto initial=bytes(row.at("initial_hex")),frame=bytes(row.at("frame"));
            const auto sp=row.at("frame_address").get<std::uint32_t>();
            memory.mount(p,initial,M::Access::ReadWrite,"declared patterned record input");
            memory.mount(sp,frame,M::Access::ReadWrite,"actual TU1.5 initializer frame input");
            A abi{};for(unsigned i=0;i<8;++i)abi.gp[i]=0x12340000u+i;abi.gp[4]=sp;
            for(unsigned i=0;i<8;++i)for(unsigned k=0;k<4;++k)abi.xmm[i][k]=0x76540000u+i*4+k;
            abi.eflags=0xcd7;abi.fs_base=0x6000000;abi.x87_depth=2;abi.x87_status=0x1800;
            abi.x87[0]=1.25L;abi.x87[1]=-0.5L;abi.floating_point=halo2::SourcePhysicsFactoryFloatingPoint{0x37f,0x3f80};
            const auto before=abi;std::size_t writes=0;
            halo2::SourcePhysicsContactRuntimeCall call{row.at("canonical_instruction"),row.at("target"),false};
            const auto observe=[&](const auto &actual) {
                check(writes<row.at("stores").size(),"Unexpected initializer store");
                check(actual.instruction==call.instruction,"Initializer source call attribution differs");
                change_equal(actual,row.at("stores").at(writes++));
            };
            check(halo2::source_try_tu15_startup_record_call(memory,call,abi,observe),"Initializer call was not owned");
            auto expected=before.gp;expected[4]+=frame.size();
            check(abi.gp==expected && abi.xmm==before.xmm && abi.eflags==before.eflags && abi.fs_base==before.fs_base &&
                abi.x87_depth==2 && abi.x87_status==before.x87_status && abi.x87[0]==before.x87[0] && abi.x87[1]==before.x87[1] &&
                abi.floating_point->x87_control_word==before.floating_point->x87_control_word &&
                abi.floating_point->mxcsr==before.floating_point->mxcsr,"Native initializer void ABI differs");
            check(writes==row.at("stores").size(),"Missing initializer stores");
            same_bank(memory,p,bytes(row.at("final_hex")));same_bank(memory,sp,frame);
            // Invalid caller frames must fail before changing record or CPU state.
            for(unsigned fault=0;fault<4;++fault) {
                memory.write(p,initial);memory.write(sp,frame);abi=before;auto bad=call;
                if(fault==0)put(memory,sp,0xbad00bad);
                if(fault==1)put(memory,sp+4,p+4);
                if(fault==2)put(memory,sp+8,0xbad00bad);
                if(fault==3)bad.tail=true;
                bool rejected=false;try { halo2::source_try_tu15_startup_record_call(memory,bad,abi); }
                catch(const std::runtime_error &) { rejected=true; }
                check(rejected && abi.gp==before.gp && abi.eflags==before.eflags,"Invalid initializer frame admitted");
                same_bank(memory,p,initial);
            }
            memory.write(sp,frame);abi=before;
            auto unknown=call;unknown.instruction^=1;
            check(!halo2::source_try_tu15_startup_record_call(memory,unknown,abi) && abi.gp==before.gp,"Unknown caller changed state");
            // Validate the full writable extent before the first partial store.
            for(bool read_only:{false,true}) {
                M missing;const auto extent=initial.size()-(read_only?0:1);
                missing.mount(p,std::span(initial).first(extent),read_only?M::Access::ReadOnly:M::Access::ReadWrite,"unavailable record extent");
                missing.mount(sp,frame,M::Access::ReadWrite,"actual frame");abi=before;bool rejected=false;
                try { halo2::source_try_tu15_startup_record_call(missing,call,abi); }
                catch(const std::exception &) { rejected=true; }
                check(rejected && abi.gp==before.gp,"Unavailable record owner admitted");
                same_bank(missing,p,std::vector<std::byte>(initial.begin(),initial.begin()+extent));
            }
            ++components;component_stores+=writes;
        }
        std::map<std::uint32_t,std::uint32_t> canonical;
        for(auto it=f.at("source_instructions").begin();it!=f.at("source_instructions").end();++it)
            canonical.emplace(std::stoul(it.key(),nullptr,16),it.value().at("canonical"));
        for(const auto &original:f.at("cases")) {
            auto row=original;
            if(!hardware_supplied) {
                // The sealed older fixture stopped before dereferencing encoded
                // ordinal322. Current platform ownership stops earlier, before
                // its MOV. Derive that state from the independent timer CALL
                // entry and declared preserving void ABI, not native output.
                const auto &last=row.at("platform_calls").back();
                check(last.at("target")==0x80000071 && last.at("return_address")==0x2e00d4,"Wrong preceding owner");
                auto after=last.at("before_abi");
                after["gp"][4]=after.at("gp").at(4).get<std::uint32_t>()+last.at("width").get<std::uint32_t>();
                row["after"]=after;
                auto &trace=row["instructions"];
                auto cut=std::find(trace.begin(),trace.end(),J(0x2e00d4));
                check(cut!=trace.end(),"Earlier fixture lacks hardware import load");
                trace.erase(cut,trace.end());
                auto &stores=row["stores"];
                stores.erase(std::remove_if(stores.begin(),stores.end(),[&](const auto &s) {
                    return s.at("trace_index").template get<std::size_t>()>trace.size();
                }),stores.end());
            }
            auto memory=hardware_supplied?
                halo2::load_source_runtime_image(argv[2],halo2::SourcePhysicsContactImage::TitleUpdate15,hardware):
                halo2::load_source_runtime_image(argv[2],halo2::SourcePhysicsContactImage::TitleUpdate15);
            memory.mount(f.at("stack_address"),bytes(f.at("initial_stack_hex")),M::Access::ReadWrite,"declared patterned source stack");
            memory.mount(0,std::vector<std::byte>(0x1000),M::Access::ReadWrite,"declared standalone FS exception-chain input");
            put(memory,0,0xffffffff);put(memory,row.at("stack_pointer"),0x3000000);
            halo2::SourceRuntimeHostVirtualMemory vm(memory);halo2::PhysicsPlatformLocks locks;
            halo2::SourceProcessShutdownRegistry registry(memory,f.at("primary_registry_owner").at("head"));
            auto abi=state(row.at("before"));std::size_t trace=0,stores=0,records=0,registrations=0;
            std::vector<J> expected_records,expected_registry;
            for(const auto &call:row.at("platform_calls"))for(const auto &change:call.at("stores")) {
                auto value=change;value["canonical_instruction"]=call.at("canonical_instruction");
                (call.at("target")==0x8000002f?expected_registry:expected_records).push_back(value);
            }
            halo2::SourcePhysicsContactRuntimeServices services;
            services.instruction=[&](auto p) {
                check(trace<row.at("instructions").size() && p==canonical.at(row.at("instructions").at(trace++)),"Fresh TU1.5 source trace differs");
            };
            services.required_owner=[&](auto &bank,const auto &call,auto &state,const auto &) {
                check(hardware_supplied && call.instruction==row.at("boundary").at("canonical_instruction") &&
                    call.target==0x2e0fe2 && !call.tail,"Unexpected unowned TU1.5 startup call");
                check(state.gp[4]==row.at("boundary").at("frame_address"),"Settings caller stack differs");
                check(&bank==&memory,"Settings caller lost its retained bank owner");
                same_bank(memory,state.gp[4],bytes(row.at("boundary").at("frame")));
                throw SettingsOwnerRequired{};
            };
            const auto observe=[&](const auto &a) {
                check(stores<row.at("stores").size(),"Unexpected source store");const auto &b=row.at("stores").at(stores++);
                check(a.instruction==b.at("canonical_instruction") && trace==b.at("trace_index"),"Source store order differs");change_equal(a,b);
            };
            const auto record_observe=[&](const auto &a) {
                check(records<expected_records.size(),"Unexpected native record store");const auto &b=expected_records.at(records++);
                check(a.instruction==b.at("canonical_instruction") && trace==b.at("trace_index"),"Native record call order differs");change_equal(a,b);
            };
            const auto shutdown_observe=[&](const auto &a) {
                check(registrations<expected_registry.size(),"Unexpected shutdown list store");const auto &b=expected_registry.at(registrations++);
                check(trace==b.at("trace_index"),"Shutdown registration order differs");
                check(a.address==b.at("address") && a.before==bytes(b.at("before")) && a.written==bytes(b.at("written")),"Independent registry expression differs");
            };
            const halo2::PhysicsPlatformLockServices forbidden{
                []()->std::uint32_t { throw std::runtime_error("Startup unexpectedly requested identity"); },
                [](auto) { throw std::runtime_error("Startup unexpectedly requested wait"); },
                [](auto) { throw std::runtime_error("Startup unexpectedly requested wake"); }};
            bool stopped=false;
            try { halo2::source_run_tu15_process_startup_records(memory,row.at("canonical_entry"),abi,{0x27f,0x1f80},
                services,vm,locks,forbidden,registry,observe,record_observe,shutdown_observe,
                hardware_supplied?halo2::source_tu15_hardware_import_owner(hardware):halo2::SourceProcessHeapDataImportOwner{}); }
            catch(const SettingsOwnerRequired &) { stopped=hardware_supplied; }
            catch(const halo2::SourceProcessHeapDataUnavailable &e) {
                check(!hardware_supplied && e.request.instruction==0x2dc624 && e.request.actual_instruction==0x2e00d4 &&
                    e.request.cell==0x41b66c && e.request.ordinal==322 && e.request.bytes==8,"Wrong missing data owner boundary");
                stopped=true;
            }
            check(stopped && trace==row.at("instructions").size() && stores==row.at("stores").size() &&
                records==expected_records.size() && registrations==expected_registry.size(),"Fresh startup did not retain its exact required-owner boundary");
            if(hardware_supplied)
                check(row.at("boundary").at("target")==0x2e0fe2 && row.at("instructions").back()==0x2e00fc,"Wrong required settings boundary");
            cpu_equal(abi,row.at("after"),hardware_supplied?0xcc5u:0xcd5u);
            for(const auto &bank:row.at("final_banks"))same_bank(memory,bank.at("address"),bytes(bank.at("hex")));
            ++cases;instructions+=trace;game_stores+=stores;record_stores+=records;registry_stores+=registrations;
        }
        std::cout<<"PASS TU1.5 fresh startup records: "<<cases<<" prefixes, "<<instructions<<" source instructions, "<<game_stores
            <<" source stores, "<<record_stores<<" record stores, "<<registry_stores<<" registry stores; "<<components
            <<" independent primary cases, "<<component_stores<<" component stores; "
            <<(hardware_supplied?"actual kernel322 input reaches required settings owner":"missing hardware322 stops before original load")<<'\n';
    } catch(const std::exception &e) { std::cerr<<e.what()<<'\n';return 1; }
}
