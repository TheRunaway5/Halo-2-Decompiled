#include "halo2/cache.hpp"
#include "halo2/mcc_script_layout.hpp"
#include "halo2/native_mcc_script_builtin.hpp"
#include <algorithm>
#include <array>
#include <fstream>
#include <iostream>
#include <map>
#include <nlohmann/json.hpp>
#include <set>

namespace {
using namespace halo2;
using Json=nlohmann::json;
constexpr bool admitted=HALO2_TEST_CAMERA_PROFILED;
void require(bool value,const char* why){if(!value)throw std::runtime_error(why);}
std::vector<std::byte> read(const std::string& path) {
    std::ifstream file(path,std::ios::binary|std::ios::ate);
    require(bool(file),"Missing owned camera projection input");const auto size=file.tellg();
    require(size>=0 && size<=8*1024*1024,"Camera projection bank exceeds test bound");
    std::vector<std::byte> result(static_cast<std::size_t>(size));file.seekg(0);
    file.read(reinterpret_cast<char*>(result.data()),result.size());
    require(bool(file),"Incomplete camera projection input");return result;
}
void put(std::span<std::byte> out,std::size_t at,unsigned value,unsigned size=4) {
    for(unsigned n=0;n<size;++n)out[at+n]=std::byte(value>>(n*8));
}
std::size_t offset(unsigned datum){return std::size_t(datum&0xffffu)*20;}
template<class F>void malformed(F&& action) {
    bool refused=false;try{action();}catch(const FormatError&){refused=true;}
    require(refused,"Malformed camera caller was admitted");
}
std::vector<std::string> names(const MccScriptLayout& layout) {
    std::set<std::string> unique;for(const auto& r:layout.requirements)unique.insert(r.name);
    return {unique.begin(),unique.end()};
}
void controls(const std::vector<std::byte>& bank,const std::vector<std::byte>& strings,
              const Json& call,unsigned pending_opcode,unsigned& count) {
    const auto root=call.at("root").get<unsigned>(),function=call.at("function").get<unsigned>();
    const auto arguments=call.at("arguments").get<std::array<unsigned,2>>();
    std::vector<std::byte> isolated(bank.size());
    for(const auto datum:{root,function,arguments[0],arguments[1]})
        std::copy_n(bank.begin()+offset(datum),20,isolated.begin()+offset(datum));
    const auto native=project_mcc_script_layout(isolated,strings,{});
    require(native.native_ready() && native.converted_calls==1 && native.rewritten_calls==1 &&
        Bytes(native.nodes).u16(offset(root)+2)==452,"Authored isolated camera call did not project");
    ++count;
    const auto blocked=[&](const auto& nodes,const auto& text,MccScriptLayoutNeed reason) {
        const auto result=project_mcc_script_layout(nodes,text,{});
        require(result.nodes==nodes && result.requirements.size()==1 &&
            result.requirements[0].need==reason,"Camera admission guard changed or accepted its input");
        malformed([&]{result.require_native_ready();});++count;
    };
    auto changed=isolated;
    put(changed,offset(function)+8,0xffffffffu);
    blocked(changed,strings,MccScriptLayoutNeed::ArgumentCount);
    changed=isolated;put(changed,offset(arguments[0])+8,0xffffffffu);
    blocked(changed,strings,MccScriptLayoutNeed::ArgumentCount);
    auto text=strings;text[Bytes(bank).u32(offset(function)+12)]=std::byte{'_'};
    blocked(isolated,text,MccScriptLayoutNeed::BuiltinIdentity);
    changed=isolated;put(changed,offset(root)+16,function^0x10000u);
    malformed([&]{project_mcc_script_layout(changed,strings,{});});++count;
    changed=isolated;put(changed,offset(function)+8,arguments[0]^0x10000u);
    malformed([&]{project_mcc_script_layout(changed,strings,{});});++count;
    changed=isolated;put(changed,offset(arguments[0])+8,arguments[0]);
    malformed([&]{project_mcc_script_layout(changed,strings,{});});++count;
    changed=isolated;put(changed,offset(root)+16,0x9000ffffu);
    malformed([&]{project_mcc_script_layout(changed,strings,{});});++count;
    changed=isolated;put(changed,offset(root)+2,pending_opcode,2);
    const auto pending_offset=text.size();const std::string pending("camera_pan\0",11);
    text=strings;text.insert(text.end(),reinterpret_cast<const std::byte*>(pending.data()),
        reinterpret_cast<const std::byte*>(pending.data()+pending.size()));
    put(changed,offset(function)+12,pending_offset);
    require(!mcc_script_builtin_signature(pending_opcode),"Unqualified camera_pan became profiled");
    blocked(changed,text,MccScriptLayoutNeed::UnknownBuiltin);
}
}
int main(int argc,char** argv)try {
    require(argc==3,"Expected owned source manifest and output directory");
    Json reference;std::ifstream(argv[1])>>reference;
    const auto signature=mcc_script_builtin_signature(454);
    if constexpr(admitted) {
        require(signature && signature->name=="camera_set_animation" && signature->xbox_opcode==452 &&
            signature->declared_return_type==4 && signature->xbox_flags==0 &&
            std::ranges::equal(signature->argument_types,std::array<std::uint16_t,2>{36,11}),
            "Private camera declaration differs from authenticated MCC/TU1.5 signatures");
    }else require(!signature,"Production camera declaration was already admitted");
    Json report={{"private_camera_profile",admitted},{"maps",Json::array()}};
    unsigned map_index=0,total_calls=0,total_controls=0;
    for(const auto& map:reference.at("maps")) {
        const auto nodes=read(map.at("nodes")),strings=read(map.at("strings")),scripts=read(map.at("scripts"));
        const auto preserved=nodes;
        const auto result=project_mcc_script_layout(nodes,strings,scripts,{true});
        require(nodes==preserved,"Camera projection modified borrowed authored syntax");
        const NativeMccScriptBuiltinProfile owned(result,strings,true);
        MccScriptLayout unowned;
        for(const auto& requirement:result.requirements)
            if(!owned.owns(requirement))unowned.requirements.push_back(requirement);
        auto expected=map.at("requirements").get<std::vector<std::string>>();
        const auto call_count=map.at("calls").size();
        if constexpr(admitted)if(call_count)std::erase(expected,"camera_set_animation");
        std::sort(expected.begin(),expected.end());
        if(names(unowned)!=expected)std::cerr<<map.at("name")<<" actual="<<Json(names(unowned)).dump()
            <<" expected="<<Json(expected).dump()<<'\n';
        require(names(unowned)==expected,"Projection changed unrelated installed-map requirements");
        if(call_count)malformed([&]{unowned.require_native_ready();});
        else require(unowned.native_ready(),"Camera-free Playground acquired requirements");
        std::set<std::size_t> allowed;
        unsigned controls_count=0;
        for(const auto& call:map.at("calls")) {
            const auto root=call.at("root").get<unsigned>();
            require(Bytes(nodes).u16(offset(root))==root>>16 && Bytes(nodes).u16(offset(root)+2)==454,
                "Camera caller lost its authored salt/opcode");
            require(Bytes(result.nodes).u16(offset(root)+2)==(admitted?452:454),
                "Camera projection did not select its correct original opcode");
            allowed.insert(offset(root)+2);allowed.insert(offset(root)+3);
            if constexpr(admitted)controls(nodes,strings,call,reference.at("pending_opcode"),controls_count);
        }
        const auto base=std::string(argv[2])+"/camera-"+std::to_string(map_index++);
        if constexpr(admitted) {
            const auto before=read(base+"-before.bin");
            require(before.size()==result.nodes.size(),"Before/after projection bank extents differ");
            unsigned changed_calls=0;
            for(std::size_t n=0;n<before.size();++n)
                require(before[n]==result.nodes[n] || allowed.contains(n),
                    "Private camera addition changed another opcode, salt, link, context or literal");
            for(const auto& call:map.at("calls")) {
                const auto at=offset(call.at("root").get<unsigned>())+2;
                require(Bytes(before).u16(at)==454 && Bytes(result.nodes).u16(at)==452,
                    "Before/after comparison lost an authored camera call");++changed_calls;
            }
            require(changed_calls==call_count,"Partial authored camera projection");
        }else {
            std::ofstream out(base+"-before.bin",std::ios::binary);
            out.write(reinterpret_cast<const char*>(result.nodes.data()),result.nodes.size());
            require(bool(out),"Could not write owned before-projection comparison");
        }
        auto truncated=nodes;truncated.pop_back();
        malformed([&]{project_mcc_script_layout(truncated,strings,scripts,{true});});++controls_count;
        report["maps"].push_back({{"name",map.at("name")},{"nodes",nodes.size()/20},
            {"camera_calls",call_count},{"requirements",names(unowned)},
            {"metadata_requirements",names(result)},{"native_owned_calls",owned.calls()},
            {"converted_calls",result.converted_calls},{"rewritten_calls",result.rewritten_calls},
            {"native_ready",unowned.native_ready()},{"guard_cases",controls_count}});
        total_calls+=call_count;total_controls+=controls_count;
    }
    require(map_index==3 && total_calls==110,"Actual E3 camera coverage changed");
    report["authored_camera_calls"]=total_calls;report["guard_cases"]=total_controls;report["passed"]=true;
    std::ofstream(std::string(argv[2])+(admitted?"/camera-after.json":"/camera-before.json"))<<report.dump(2)<<'\n';
    std::cout<<"PASS "<<(admitted?"private412":"production411")<<" metadata: "<<total_calls
        <<" authored E3 camera callers, "<<total_controls<<" guard cases; production admission unchanged\n";
}catch(const std::exception& error){std::cerr<<error.what()<<'\n';return 1;}
