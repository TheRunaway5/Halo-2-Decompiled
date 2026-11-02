#include "halo2/native_tu15_camera_projection.hpp"
#include "halo2/native_tu15_camera_projection_binding.hpp"
#include <algorithm>
#include <bit>
#include <fstream>
#include <cstring>
#include <iostream>
#include <map>
#include <nlohmann/json.hpp>
#include <optional>
#include <stdexcept>
#include <xmmintrin.h>

namespace {
using namespace halo2;using Json=nlohmann::json;
void check(bool value,const std::string &message){if(!value)throw std::runtime_error(message);}
std::vector<std::byte> bytes(const std::string &text) {
    std::vector<std::byte> out;
    for(std::size_t n=0;n<text.size();n+=2)out.push_back(std::byte(std::stoul(text.substr(n,2),nullptr,16)));
    return out;
}
std::string hex(std::span<const std::byte> values) {
    constexpr char digits[]="0123456789abcdef";std::string out;
    for(auto value:values){const auto v=std::to_integer<unsigned>(value);out+=digits[v>>4];out+=digits[v&15];}return out;
}
struct Prefix {};
struct HostX87 {
    alignas(16) std::array<std::byte,108> original{},seeded{};
    HostX87() {
        const std::array<std::byte,10> value{std::byte(0),std::byte(0),std::byte(0),std::byte(0),
            std::byte(0),std::byte(0),std::byte(0),std::byte(0xc0),std::byte(0xff),std::byte(0x3f)};
        asm volatile("fnsave %0\n\tfninit\n\tfldt %2\n\tfldz\n\tfnsave %1\n\tfrstor %1"
            :"=m"(original),"=m"(seeded):"m"(value):"memory","st","st(1)");
    }
    ~HostX87(){asm volatile("frstor %0"::"m"(original):"memory","st","st(1)");}
    void verify()const {
        alignas(16) std::array<std::byte,108> actual{};
        asm volatile("fnsave %0\n\tfrstor %0":"=m"(actual)::"memory","st","st(1)");
        check(actual==seeded,"Occupied host x87 context was not restored");
    }
};
struct Memory final:SourcePhysicsContactMemory {
    std::map<std::uint32_t,std::vector<std::byte>> banks;
    std::string phase;unsigned ordinal{};mutable unsigned read_attempts{};unsigned write_attempts{};
    mutable std::optional<std::pair<std::uint32_t,std::size_t>> failed_access;
    bool recording{},raw_stores{};mutable Json reads=Json::array(),accesses=Json::array();
    std::vector<SourcePhysicsContactRecordStore> stores;
    Json changes=Json::array();unsigned published{};
    void change_inputs(){for(const auto &change:changes)if(change.at("after_store")==published){const auto raw=bytes(change.at("bytes"));const auto at=change.at("address").get<std::uint32_t>();const auto found=locate(at,raw.size());std::copy(raw.begin(),raw.end(),banks.at(found->first).begin()+at-found->first);}++published;}
    std::map<std::uint32_t,std::vector<std::byte>>::const_iterator locate(std::uint32_t address,std::size_t size)const {
        auto found=banks.upper_bound(address);
        if(found==banks.begin()){failed_access={{address,size}};throw std::runtime_error("Missing camera projection storage");}--found;
        if(std::uint64_t(address-found->first)+size>found->second.size()){failed_access={{address,size}};throw std::runtime_error("Render camera packet storage extent differs");}return found;
    }
    void read(std::uint32_t address,std::span<std::byte> out)const override {
        if(phase=="before_read" && read_attempts++==ordinal)throw Prefix{};
        const auto found=locate(address,out.size());
        if(recording){reads.push_back(Json::array({address,out.size()}));accesses.push_back(Json::array({"read",address,out.size()}));}
        std::copy_n(found->second.begin()+address-found->first,out.size(),out.begin());
    }
    void write(std::uint32_t address,std::span<const std::byte> values)override {
        if(phase=="before_write" && write_attempts++==ordinal)throw Prefix{};
        const auto found=locate(address,values.size());if(recording)accesses.push_back(Json::array({"write",address,values.size()}));
        if(raw_stores)stores.push_back({0,address,
            {found->second.begin()+address-found->first,found->second.begin()+address-found->first+values.size()},
            {values.begin(),values.end()}});
        std::copy(values.begin(),values.end(),banks.at(found->first).begin()+address-found->first);change_inputs();
    }
};
}
int main(int argc,char **argv)try {
    check(argc==2,"Supply independent original camera projection fixture");std::ifstream stream(argv[1]);const auto fixture=Json::parse(stream);
    check(fixture.at("image_sha256")=="21b207e7efdacaf6124a0822837f8be570766a42505057f7fd2d2ba8f409c4fd"&&fixture.at("backend")=="physical-unchanged-i386-offline"&&fixture.at("instruction_patches").empty()&&!fixture.at("supplied_original_outcomes").get<bool>(),"Camera projection original authority differs");
    const auto witness=std::ranges::find_if(fixture.at("cases"),[](const auto &row){return row.at("name")=="fault_seed_inverse_fp_store";});
    check(witness!=fixture.at("cases").end(),"Missing independently recovered pending-store witness");
    const auto write=std::ranges::find_if(witness->at("writes"),[](const auto &event){return event.at("instruction")==0x144194u;});
    check(write!=witness->at("writes").end(),"Pending-store witness did not reach actual publication");
    const auto ordinal=std::distance(witness->at("writes").begin(),write);
    for(const auto phase:{"before_write","after_write"}) {
        const auto name=std::string("fault_seed_inverse_fp_store_")+phase+"_"+std::to_string(ordinal);
        const auto cut=std::ranges::find_if(fixture.at("cases"),[&](const auto &row){return row.at("name")==name;});
        check(cut!=fixture.at("cases").end()&&cut->at("after_fp")[2]==0x1f80u,"Original pending-store FP witness differs");
    }
    std::size_t count{},reads{},stores{},prefixes{},faults{},bindings{},sparse_boundaries{};
    for(const auto &row:fixture.at("cases")) {
        const auto name=row.at("name").get<std::string>();const auto entry=row.at("entry").get<std::uint32_t>();
        check(row.at("physical_verified").get<bool>(),name+": physical evidence missing");
        const auto incoming_mask=row.at("x87_occupied").get<unsigned>();
        const bool dense=(incoming_mask&(incoming_mask+1u))==0;
        for(unsigned transport=0;transport<(entry==0x2fd70u?4u:2u);++transport)for(bool reading:{false,true}) {
            const bool binding=transport!=0;if(binding&&!dense)continue;
            Memory memory;for(const auto &bank:row.at("banks"))memory.banks.emplace(bank.at("address"),bytes(bank.at("before")));
            memory.changes=row.at("input_changes");const auto sp=row.at("gp")[4].get<std::uint32_t>();
            std::vector<std::byte> stack(12);const std::array<std::uint32_t,3> words{0x3000000u,row.at("matrix"),row.at("source")};
            for(unsigned n=0;n<3;++n)for(unsigned k=0;k<4;++k)stack[n*4+k]=std::byte(words[n]>>(8u*k));memory.banks.emplace(sp,stack);
            const auto fault=row.value("fault",Json::object());memory.phase=fault.value("phase",std::string{});memory.ordinal=fault.value("ordinal",0u);
            if(reading&&memory.phase=="after_write")continue;
            memory.recording=reading;memory.raw_stores=!reading&&(memory.phase=="before_read"||memory.phase=="before_write"||row.value("actual_write_fault",false));
            SourcePhysicsContactRecordObserver observe;std::vector<SourcePhysicsContactRecordStore> events;unsigned observed{};
            if(!reading&&!memory.raw_stores)observe=[&](const auto &event){events.push_back(event);if(memory.phase=="after_write"&&observed++==memory.ordinal)throw Prefix{};};
            NativeTu15PhysicsFrameFp fp{row.at("before_fp")[0],row.at("before_fp")[1],row.at("before_fp")[2]};
            NativeTu15CameraProjectionX87State state;state.occupied=incoming_mask;
            for(unsigned n=0;n<8;++n){const auto raw=bytes(row.at("live_x87")[n]);std::copy(raw.begin(),raw.end(),state.values[n].begin());}
            SourcePhysicsWorldRuntimeAbi abi{};abi.gp=row.at("gp").get<std::array<std::uint32_t,8>>();abi.eflags=0x202;
            abi.floating_point=SourcePhysicsFactoryFloatingPoint{fp.control_word,fp.mxcsr};abi.x87_status=fp.status;
            if(dense) {while(abi.x87_depth<8&&(incoming_mask&(1u<<abi.x87_depth)))++abi.x87_depth;for(unsigned n=0;n<abi.x87_depth;++n)std::memcpy(&abi.x87[n],state.values[n].data(),10);}
            const auto before_gp=abi.gp;const auto host_mxcsr=_mm_getcsr();std::uint16_t host_cw{},host_sw{};asm volatile("fnstcw %0\n\tfnstsw %1":"=m"(host_cw),"=m"(host_sw));
            bool stopped{},abi_boundary{};
            try {
                if(binding){
                    const auto caller=entry==0x2fb50u?0x2fd79u:entry==0x144090u?0x2ffe9u:transport==1?0x13c9bu:transport==2?0x2bab7u:0x2bafcu;
                    native_tu15_camera_projection_run(memory,entry,caller,abi,observe);++bindings;
                }else if(entry==0x2fb50u)native_tu15_camera_projection_program_publish(memory,row.at("camera"),row.at("bounds"),row.at("destination"),fp,state,observe);
                else if(entry==0x144090u)native_tu15_camera_inverse_frame(memory,row.at("frame"),row.at("destination"),fp,observe);
                else native_tu15_camera_projection(memory,row.at("camera"),row.at("bounds"),row.at("destination"),fp,state,observe);
            }catch(const NativeTu15CameraProjectionX87AbiRequired &required) {
                state=required.state();fp=required.floating_point();abi_boundary=true;stopped=false;++sparse_boundaries;
                if(required.cause()) {
                    try{std::rethrow_exception(required.cause());}
                    catch(const Prefix&){stopped=true;}
                    catch(const std::runtime_error&){check(memory.failed_access.has_value()&&
                        memory.failed_access->first==row.at("boundary").at("address")&&memory.failed_access->second==row.at("boundary").at("bytes"),
                        name+": sparse projection boundary lost actual source fault");stopped=true;++faults;}
                }
                check((state.occupied&(unsigned(state.occupied)+1u))!=0,name+": spurious sparse ABI boundary");
            }catch(const Prefix &){stopped=true;}
            catch(const std::runtime_error &error) {
                if(!(row.value("actual_read_fault",false)||row.value("actual_write_fault",false))||!memory.failed_access)throw std::runtime_error(name+": "+error.what());
                check(memory.failed_access->first==row.at("boundary").at("address")&&memory.failed_access->second==row.at("boundary").at("bytes"),name+": actual camera projection fault differs");stopped=true;++faults;
            }
            std::uint16_t actual_cw{},actual_sw{};asm volatile("fnstcw %0\n\tfnstsw %1":"=m"(actual_cw),"=m"(actual_sw));
            check(host_cw==actual_cw&&host_sw==actual_sw&&host_mxcsr==_mm_getcsr(),name+": host FP leaked");
            check(stopped==!row.at("completed").get<bool>(),name+": camera projection completion differs");
            if(binding) {
                fp.control_word=abi.floating_point->x87_control_word;fp.status=abi.x87_status;fp.mxcsr=abi.floating_point->mxcsr;
                if(!abi_boundary){state.occupied=std::uint8_t((1u<<abi.x87_depth)-1u);for(unsigned n=0;n<abi.x87_depth;++n)std::memcpy(state.values[n].data(),&abi.x87[n],10);}
                for(unsigned n=0;n<8;++n)if(n!=4)check(abi.gp[n]==before_gp[n],name+": dead GP fabricated");
                const unsigned cleanup=4;check(abi.gp[4]==sp+((stopped||abi_boundary)?0:cleanup),name+": camera projection RET differs");
                check(abi.eflags==0x202,name+": unrelated flags changed");
            }
            check(fp.control_word==row.at("after_fp")[0]&&fp.status==row.at("after_fp")[1]&&fp.mxcsr==row.at("after_fp")[2],name+": camera projection FP differs native="+Json::array({fp.control_word,fp.status,fp.mxcsr}).dump()+" original="+row.at("after_fp").dump());
            const auto &expected_fp=row.at("after_x87");check(state.occupied==expected_fp.at("occupied"),name+": camera projection x87 tags differ native="+std::to_string(state.occupied)+" original="+expected_fp.at("occupied").dump());
            for(unsigned n=0;n<8;++n)if(state.occupied&(1u<<n))check(hex(state.values[n])==expected_fp.at("values")[n].get<std::string>(),name+": camera projection live x87 value differs lane="+std::to_string(n));
            for(const auto &bank:row.at("banks"))check(memory.banks.at(bank.at("address"))==bytes(bank.at("after")),name+": retained camera projection bytes differ");
            if(reading) {
                Json expected=Json::array(),accesses=row.at("accesses");for(const auto &event:row.at("reads"))expected.push_back(Json::array({event.at("address"),event.at("bytes")}));
                if(binding&&!stopped&&!abi_boundary){expected.push_back(Json::array({sp,4}));accesses.push_back(Json::array({"read",sp,4}));}
                check(memory.reads==expected,name+": camera projection ordered reads differ native="+memory.reads.dump()+" original="+expected.dump());
                check(memory.accesses==accesses,name+": camera projection read/write interleaving differs");reads+=memory.reads.size();
            }else {
                if(memory.raw_stores)events=memory.stores;check(events.size()==row.at("writes").size(),name+": camera projection store count differs");
                for(std::size_t n=0;n<events.size();++n){const auto &a=events[n];const auto &b=row.at("writes")[n];check((memory.raw_stores||a.instruction==b.at("instruction"))&&a.address==b.at("address")&&hex(a.before)==b.at("before").get<std::string>()&&hex(a.written)==b.at("written").get<std::string>(),name+": camera projection ordered store differs");}stores+=events.size();
            }prefixes+=stopped;
        }
        if(entry==0x2fb50u&&row.at("completed").get<bool>()&&row.at("destination")==0x1000300u&&row.at("input_changes").empty()) {
            Memory memory;for(const auto &bank:row.at("banks"))memory.banks.emplace(bank.at("address"),bytes(bank.at("before")));
            NativeTu15PhysicsFrameFp fp{row.at("before_fp")[0],row.at("before_fp")[1],row.at("before_fp")[2]};
            NativeTu15CameraProjectionX87State state;state.occupied=incoming_mask;
            for(unsigned n=0;n<8;++n){const auto raw=bytes(row.at("live_x87")[n]);std::copy(raw.begin(),raw.end(),state.values[n].begin());}
            const auto program=native_tu15_camera_projection_program(memory,row.at("camera"),row.at("bounds"),fp,state);
            const std::array<std::uint32_t,16> values{program.bounds[0],program.bounds[1],program.bounds[2],program.bounds[3],
                program.width,program.height,program.half_width,program.half_height,program.center_x,program.center_y,
                program.horizontal,program.vertical,program.clipping[0],program.clipping[1],program.clipping[2],program.clipping[3]};
            std::vector<std::byte> raw(64);for(unsigned n=0;n<16;++n)for(unsigned k=0;k<4;++k)raw[n*4+k]=std::byte(values[n]>>(k*8u));
            const auto data=std::ranges::find_if(row.at("banks"),[](const auto &bank){return bank.at("address")==0x1000000u;});
            check(data!=row.at("banks").end(),"Projection value source bank missing");const auto expected=bytes(data->at("after"));
            check(std::equal(raw.begin(),raw.end(),expected.begin()+0x300),name+": typed private projection program differs");
            check(fp.control_word==row.at("after_fp")[0]&&fp.status==row.at("after_fp")[1]&&fp.mxcsr==row.at("after_fp")[2],name+": typed program FP differs");
            check(state.occupied==row.at("after_x87").at("occupied"),name+": typed program x87 validity differs");
            for(unsigned n=0;n<8;++n)if(state.occupied&(1u<<n))check(hex(state.values[n])==row.at("after_x87").at("values")[n].get<std::string>(),name+": typed program older x87 differs");
        }
        ++count;
    }
    check(count&&prefixes&&faults&&bindings,"Camera projection matrix incomplete");
    for(const auto entry:{0x2fb50u,0x2fd70u,0x144090u}) {
        const auto found=std::ranges::find_if(fixture.at("cases"),[&](const auto &row){
            return row.at("entry")==entry&&row.at("completed").template get<bool>()&&
                row.at("x87_occupied")==0&&row.at("input_changes").empty();});
        check(found!=fixture.at("cases").end(),"Missing ordinary return-failure case");const auto &row=*found;
        Memory memory;for(const auto &bank:row.at("banks"))memory.banks.emplace(bank.at("address"),bytes(bank.at("before")));
        SourcePhysicsWorldRuntimeAbi abi{};abi.gp=row.at("gp").get<std::array<std::uint32_t,8>>();abi.eflags=0x202;
        abi.floating_point=SourcePhysicsFactoryFloatingPoint{row.at("before_fp")[0],row.at("before_fp")[2]};abi.x87_status=row.at("before_fp")[1];
        const auto before_gp=abi.gp;const auto caller=entry==0x2fb50u?0x2fd79u:entry==0x144090u?0x2ffe9u:0x2bab7u;
        HostX87 host;bool failed{};
        try{native_tu15_camera_projection_run(memory,entry,caller,abi);}
        catch(const std::runtime_error&){failed=memory.failed_access==std::make_pair(before_gp[4],std::size_t(4));}
        host.verify();check(failed&&abi.gp==before_gp&&abi.eflags==0x202,"Actual RET fault was fabricated or consumed");
        for(const auto &bank:row.at("banks"))check(memory.banks.at(bank.at("address"))==bytes(bank.at("after")),"RET fault lost completed projection state");
        check(abi.floating_point->x87_control_word==row.at("after_fp")[0]&&abi.x87_status==row.at("after_fp")[1]&&abi.floating_point->mxcsr==row.at("after_fp")[2],"RET fault lost completed FP state");
        check(std::uint8_t((1u<<abi.x87_depth)-1u)==row.at("after_x87").at("occupied"),"RET fault lost completed x87 validity");
        for(unsigned n=0;n<abi.x87_depth;++n)check(hex(std::span(reinterpret_cast<const std::byte*>(&abi.x87[n]),10))==row.at("after_x87").at("values")[n].get<std::string>(),"RET fault lost completed x87 value");
    }
    {
        Memory memory;SourcePhysicsWorldRuntimeAbi abi{};bool entry{},caller{},policy{};
        try{native_tu15_camera_projection_run(memory,0,0,abi);}catch(const std::invalid_argument&){entry=true;}
        try{native_tu15_camera_projection_run(memory,0x2fd70u,0,abi);}catch(const std::invalid_argument&){caller=true;}
        try{native_tu15_camera_projection_run(memory,0x2fd70u,0x2bab7u,abi);}catch(const std::invalid_argument&){policy=true;}
        check(entry&&caller&&policy&&memory.reads.empty()&&memory.stores.empty(),"Projection admission touched storage");
        check(native_tu15_camera_projection_owns_entry(0x2fb50u)&&native_tu15_camera_projection_owns_entry(0x2fd70u)&&
            !native_tu15_camera_projection_owns_entry(0x144090u)&&!native_tu15_camera_projection_owns_entry(0),"Projection public ownership differs");
        check(native_tu15_camera_projection_owns_call(0x2fb50u,0x2fd79u)&&
            native_tu15_camera_projection_owns_call(0x2fd70u,0x13c9bu)&&native_tu15_camera_projection_owns_call(0x2fd70u,0x2bab7u)&&
            native_tu15_camera_projection_owns_call(0x2fd70u,0x2bafcu)&&!native_tu15_camera_projection_owns_call(0x144090u,0x2ffe9u)&&
            !native_tu15_camera_projection_owns_call(0x2fd70u,0)&&!native_tu15_camera_projection_owns_call(0,0x2bab7u),"Projection call admission differs");
    }
    std::cout<<count<<" physical default cases, "<<reads<<" ordered reads, "<<stores<<" stores, "<<prefixes<<" prefixes, "<<bindings<<" complete ABI transports, "<<sparse_boundaries<<" explicit sparse ABI boundaries passed\n";return 0;
}catch(const std::exception &error){std::cerr<<error.what()<<'\n';return 1;}
