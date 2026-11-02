#include "halo2/native_tu15_camera_view_layout.hpp"
#include "halo2/native_tu15_camera_view_layout_binding.hpp"
#include <algorithm>
#include <bit>
#include <fstream>
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
struct Memory final:SourcePhysicsContactMemory {
    std::map<std::uint32_t,std::vector<std::byte>> banks;
    std::string phase;unsigned ordinal{};mutable unsigned read_attempts{};unsigned write_attempts{};
    mutable std::optional<std::pair<std::uint32_t,std::size_t>> failed_access;
    bool recording{},raw_stores{};mutable Json reads=Json::array(),read_values=Json::array();
    std::vector<SourcePhysicsContactRecordStore> stores;
    Json changes=Json::array();unsigned published{};
    void change_inputs(){for(const auto &change:changes)if(change.at("after_store")==published){const auto raw=bytes(change.at("bytes"));const auto at=change.at("address").get<std::uint32_t>();const auto found=locate(at,raw.size());std::copy(raw.begin(),raw.end(),banks.at(found->first).begin()+at-found->first);}++published;}
    std::map<std::uint32_t,std::vector<std::byte>>::const_iterator locate(std::uint32_t address,std::size_t size)const {
        auto found=banks.upper_bound(address);
        if(found==banks.begin()){failed_access={{address,size}};throw std::runtime_error("Missing camera view layout storage");}--found;
        if(std::uint64_t(address-found->first)+size>found->second.size()){failed_access={{address,size}};throw std::runtime_error("Camera view layout storage extent differs");}return found;
    }
    void read(std::uint32_t address,std::span<std::byte> out)const override {
        if(phase=="before_read" && read_attempts++==ordinal)throw Prefix{};
        const auto found=locate(address,out.size());
        if(recording){reads.push_back(Json::array({address,out.size()}));
            read_values.push_back(hex(std::span(found->second).subspan(address-found->first,out.size())));}
        std::copy_n(found->second.begin()+address-found->first,out.size(),out.begin());
    }
    void write(std::uint32_t address,std::span<const std::byte> values)override {
        if(phase=="before_write" && write_attempts++==ordinal)throw Prefix{};
        const auto found=locate(address,values.size());
        if(raw_stores)stores.push_back({0,address,
            {found->second.begin()+address-found->first,found->second.begin()+address-found->first+values.size()},
            {values.begin(),values.end()}});
        std::copy(values.begin(),values.end(),banks.at(found->first).begin()+address-found->first);change_inputs();
    }
};
std::uint32_t word(const Memory &memory,std::uint32_t address) {
    std::array<std::byte,4> raw{};memory.read(address,raw);std::uint32_t value{};
    for(unsigned n=0;n<4;++n)value|=std::to_integer<std::uint32_t>(raw[n])<<(8u*n);return value;
}
struct TileArguments final:NativeTu15CameraViewTileArguments {
    const Memory &memory;std::uint32_t sp;
    TileArguments(const Memory &m,std::uint32_t s):memory(m),sp(s){}
    std::int32_t count()const override{return std::bit_cast<std::int32_t>(word(memory,sp+4u));}
    std::int32_t split_mode()const override{return std::bit_cast<std::int32_t>(word(memory,sp+8u));}
    std::uint32_t tile_output()const override{return word(memory,sp+12u);}
};
struct RectangleArguments final:NativeTu15CameraViewRectangleArguments {
    const Memory &memory;std::uint32_t sp;
    RectangleArguments(const Memory &m,std::uint32_t s):memory(m),sp(s){}
    std::uint32_t grid()const override{return word(memory,sp+4u);}
    std::uint32_t span()const override{return word(memory,sp+8u);}
    std::uint32_t rectangle_output()const override{return word(memory,sp+12u);}
};
}
int main(int argc,char **argv)try {
    check(argc==2,"Supply independent original view layout fixture");std::ifstream stream(argv[1]);const auto fixture=Json::parse(stream);
    check(fixture.at("image_sha256")=="21b207e7efdacaf6124a0822837f8be570766a42505057f7fd2d2ba8f409c4fd"&&
        fixture.at("backend")=="physical-unchanged-i386-offline"&&fixture.at("instruction_patches").empty()&&
        !fixture.at("supplied_original_outcomes").get<bool>(),"View layout reference lacks physical original authority");
    std::size_t count{},reads{},stores{},prefixes{},faults{},bindings{},divides{};
    for(const auto &row:fixture.at("cases")) {
        const auto name=row.at("name").get<std::string>();const auto entry=row.at("entry").get<unsigned>();
        check(row.at("physical_verified").get<bool>()&&row.at("live_x87_unchanged").get<bool>(),name+": physical evidence missing");
        check(row.at("before_fp")==row.at("after_fp"),name+": integer-only original changed FP policy/status");
        for(bool binding:{false,true})for(bool reading:{false,true}) {
            Memory memory;for(const auto &bank:row.at("banks"))memory.banks.emplace(bank.at("address"),bytes(bank.at("before")));
            memory.changes=row.at("input_changes");const auto sp=row.at("gp")[4].get<std::uint32_t>();
            std::vector<std::byte> stack(16);const std::array<std::uint32_t,4> words{0x3000000u,row.at("matrix"),row.at("source"),row.at("output_argument")};
            for(unsigned n=0;n<4;++n)for(unsigned k=0;k<4;++k)stack[n*4+k]=std::byte(words[n]>>(k*8u));
            memory.banks.emplace(sp,stack);const auto fault=row.value("fault",Json::object());
            memory.phase=fault.value("phase",std::string{});memory.ordinal=fault.value("ordinal",0u);
            if(reading&&memory.phase=="after_write")continue;
            memory.recording=reading;memory.raw_stores=!reading&&(memory.phase=="before_read"||memory.phase=="before_write"||row.value("actual_write_fault",false));
            SourcePhysicsContactRecordObserver observe;std::vector<SourcePhysicsContactRecordStore> events;unsigned observed{};
            if(!reading&&!memory.raw_stores)observe=[&](const auto &event){events.push_back(event);if(memory.phase=="after_write"&&observed++==memory.ordinal)throw Prefix{};};
            SourcePhysicsWorldRuntimeAbi abi{};abi.gp=row.at("gp").get<std::array<std::uint32_t,8>>();abi.eflags=0x602u;
            abi.floating_point=SourcePhysicsFactoryFloatingPoint{row.at("before_fp")[0],row.at("before_fp")[2]};abi.x87_status=row.at("before_fp")[1];
            const auto original=abi;const auto host_mxcsr=_mm_getcsr();std::uint16_t host_cw{},host_sw{};
            asm volatile("fnstcw %0\n\tfnstsw %1":"=m"(host_cw),"=m"(host_sw));bool stopped{};
            try {
                if(binding){native_tu15_camera_view_layout_run(memory,entry,entry==0x2f5e0u?0x229b44u:entry==0x2f620u?0x229b5fu:0x229b7du,abi,observe);++bindings;}
                else if(entry==0x2f5e0u)native_tu15_camera_view_grid_publish(memory,row.at("count"),row.at("mode"),row.at("grid"),observe);
                else if(entry==0x2f620u){TileArguments argument(memory,sp);native_tu15_camera_view_tile(memory,row.at("ordinal"),row.at("grid"),row.at("span"),argument,observe);}
                else{RectangleArguments argument(memory,sp);native_tu15_camera_view_rectangles(memory,row.at("tile"),row.at("inset"),argument,observe);}
            }catch(const NativeTu15CameraViewLayoutDivideError &divide) {
                check(row.at("boundary").at("phase")=="integer_divide"&&divide.original_site()==row.at("boundary").at("instruction"),name+": layout divide phase differs");
                const auto divisor_register=divide.original_site()==0x2f66au?6u:divide.original_site()==0x2f675u?7u:divide.original_site()==0x2f684u?1u:5u;
                check(std::uint32_t(divide.dividend())==row.at("boundary_gp")[0]&&std::uint32_t(divide.divisor())==row.at("boundary_gp")[divisor_register],
                    name+": layout divide operands differ");stopped=true;++divides;
            }catch(const Prefix &){stopped=true;}
            catch(const std::runtime_error &error) {
                if(!(row.value("actual_read_fault",false)||row.value("actual_write_fault",false))||!memory.failed_access)throw std::runtime_error(name+": "+error.what());
                check(memory.failed_access->first==row.at("boundary").at("address")&&memory.failed_access->second==row.at("boundary").at("bytes"),name+": layout actual fault differs");stopped=true;++faults;
            }
            std::uint16_t actual_cw{},actual_sw{};asm volatile("fnstcw %0\n\tfnstsw %1":"=m"(actual_cw),"=m"(actual_sw));
            check(actual_cw==host_cw&&actual_sw==host_sw&&_mm_getcsr()==host_mxcsr,name+": layout host FP leaked");
            check(stopped==!row.at("completed").get<bool>(),name+": layout completion differs");
            if(binding) {
                for(unsigned n=0;n<8;++n)if(n!=4)check(abi.gp[n]==original.gp[n],name+": layout fabricated dead register state");
                check(abi.gp[4]==sp+(stopped?0u:entry==0x2f5e0u?4u:16u),name+": layout SP differs");
                check(abi.eflags==original.eflags&&abi.x87_status==original.x87_status&&abi.floating_point->mxcsr==original.floating_point->mxcsr&&
                    abi.floating_point->x87_control_word==original.floating_point->x87_control_word,name+": layout changed unrelated ABI state");
            }
            for(const auto &bank:row.at("banks"))check(memory.banks.at(bank.at("address"))==bytes(bank.at("after")),name+": layout retained bytes differ");
            if(reading) {
                Json expected=Json::array();for(const auto &event:row.at("reads"))expected.push_back(Json::array({event.at("address"),event.at("bytes")}));
                if(row.at("boundary").is_object()&&row.at("boundary").contains("pending_rmw_read")) {
                    const auto &pending=row.at("boundary").at("pending_rmw_read");
                    check(memory.phase=="before_write"&&pending.at("bytes")==2&&
                        pending.at("instruction")==row.at("boundary").at("instruction"),name+": unqualified layout RMW cut");
                    expected.push_back(Json::array({pending.at("address"),pending.at("bytes")}));
                    check(memory.read_values.size()==expected.size()&&memory.read_values.back()==pending.at("value"),
                        name+": exact pending layout RMW operand differs");
                }
                if(binding&&!stopped)expected.push_back(Json::array({sp,4}));
                check(memory.reads==expected,name+": layout ordered reads differ native="+memory.reads.dump()+" original="+expected.dump());reads+=memory.reads.size();
            }else {
                if(memory.raw_stores)events=memory.stores;check(events.size()==row.at("writes").size(),name+": layout store count differs");
                for(std::size_t n=0;n<events.size();++n){const auto &a=events[n];const auto &b=row.at("writes")[n];
                    check((memory.raw_stores||a.instruction==b.at("instruction"))&&a.address==b.at("address")&&hex(a.before)==b.at("before").get<std::string>()&&
                        hex(a.written)==b.at("written").get<std::string>(),name+": layout ordered store differs");}
                stores+=events.size();
            }
            prefixes+=stopped;
        }
        if(entry==0x2f5e0u&&row.at("completed").get<bool>()) {
            const auto grid=native_tu15_camera_view_grid(row.at("count"),row.at("mode"));
            std::array<std::byte,2> x{std::byte(grid.columns),std::byte(grid.columns>>8u)},
                y{std::byte(grid.rows),std::byte(grid.rows>>8u)};
            check(hex(x)==row.at("writes")[0].at("written").get<std::string>()&&hex(y)==row.at("writes")[1].at("written").get<std::string>(),
                name+": typed private grid value differs");
        }
        ++count;
    }
    check(count&&prefixes&&faults&&divides&&bindings,"Layout qualification matrix incomplete");
    {
        Memory memory;SourcePhysicsWorldRuntimeAbi abi{};
        for(const auto entry:{0x2f5e0u,0x2f620u,0x2f690u}) {
            bool rejected{};try{native_tu15_camera_view_layout_run(memory,entry,0,abi);}catch(const std::invalid_argument&){rejected=true;}
            check(rejected,"Layout foreign caller admitted");
        }
        check(memory.reads.empty()&&memory.stores.empty(),"Layout admission touched storage");
    }
    for(const auto entry:{0x2f5e0u,0x2f620u,0x2f690u}) {
        const auto found=std::ranges::find_if(fixture.at("cases"),[&](const auto &row){return row.at("entry")==entry&&row.at("completed").template get<bool>();});
        check(found!=fixture.at("cases").end(),"Layout lacks real completed RET seed");const auto &row=*found;
        Memory memory;for(const auto &bank:row.at("banks"))memory.banks.emplace(bank.at("address"),bytes(bank.at("before")));
        SourcePhysicsWorldRuntimeAbi abi{};abi.gp=row.at("gp").get<std::array<std::uint32_t,8>>();const auto original=abi;
        const auto sp=abi.gp[4];std::vector<std::byte> args(12);const std::array<std::uint32_t,3> values{row.at("matrix"),row.at("source"),row.at("output_argument")};
        for(unsigned n=0;n<3;++n)for(unsigned k=0;k<4;++k)args[n*4+k]=std::byte(values[n]>>(k*8u));
        memory.banks.emplace(sp+4u,args);bool failed{};
        try{native_tu15_camera_view_layout_run(memory,entry,entry==0x2f5e0u?0x229b44u:entry==0x2f620u?0x229b5fu:0x229b7du,abi);}
        catch(const std::runtime_error&){failed=true;}
        check(failed&&memory.failed_access==std::pair<std::uint32_t,std::size_t>{sp,4}&&abi.gp==original.gp,
            "Layout RET failure lost its completed state/transport boundary");
        for(const auto &bank:row.at("banks"))check(memory.banks.at(bank.at("address"))==bytes(bank.at("after")),"Layout RET failure rolled back publication");
    }
    std::cout<<count<<" physical layout cases, "<<reads<<" reads, "<<stores<<" stores, "<<prefixes<<" prefixes, "
        <<divides<<" actual divides and "<<bindings<<" completed ABI transports passed\n";return 0;
}catch(const std::exception &error){std::cerr<<error.what()<<'\n';return 1;}
