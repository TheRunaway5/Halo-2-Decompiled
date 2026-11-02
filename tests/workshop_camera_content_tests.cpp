#include "halo2/cache.hpp"
#include "halo2/content.hpp"
#include "halo2/model.hpp"
#include <algorithm>
#include <array>
#include <bit>
#include <cmath>
#include <fstream>
#include <iostream>
#include <nlohmann/json.hpp>
#include <set>

namespace {
using namespace halo2;
void need(bool value,const char* why){if(!value)throw FormatError(why);}
std::array<std::uint32_t,8> bits(const BonePose& pose) {
    const std::array values{pose.translation.x,pose.translation.y,pose.translation.z,
        pose.rotation[0],pose.rotation[1],pose.rotation[2],pose.rotation[3],pose.scale};
    std::array<std::uint32_t,8> result;
    for(unsigned n=0;n<values.size();++n){need(std::isfinite(values[n]),"Camera content sample is nonfinite");
        result[n]=std::bit_cast<std::uint32_t>(values[n]);}
    return result;
}
}
int main(int argc,char** argv)try {
    need(argc==6,"Supply MAP SHARED_MAPS PREPARED_ROOT EXPECTED_CALLS REPORT");
    Cache cache(argv[1],argv[2],{},argv[3]);need(cache.is_mcc(),"Camera content input is not MCC");
    const auto* scenario_tag=cache.find_tag(cache.scenario_tag());need(scenario_tag,"Camera content has no scenario");
    const Bytes scenario(cache.read_tag_bytes(scenario_tag->address,0x240));
    const auto syntax=cache.read_tag_bytes(scenario.u32(0x23c),std::size_t(scenario.u32(0x238))*20);
    const std::vector<std::byte> original(syntax.begin(),syntax.end());
    const Bytes strings(cache.read_tag_bytes(scenario.u32(0x1b4),scenario.u32(0x1b0)));
    const auto row=[&](unsigned datum){
        need(datum!=null_datum && (datum&65535)<syntax.size()/20,"Camera content caller escapes syntax");
        const Bytes node(syntax.subspan(std::size_t(datum&65535)*20,20));
        need(node.u16(0)==datum>>16 && node.u16(6)!=0xbaba,"Camera content caller has a stale salt");return node;
    };
    nlohmann::json report={{"scope","Actual prepared E3 camera graph/name/resource decoding and samples; no native HS/frame/GPU or mission acceptance"},
        {"calls",nlohmann::json::array()}};
    unsigned calls{},samples{},moving{},errors{};std::set<std::pair<unsigned,unsigned>> clips;
    for(unsigned index=0;index<syntax.size()/20;++index) {
        const Bytes call(syntax.subspan(index*20,20));
        if(!call.u16(0)||call.u16(6)==0xbaba||(call.u16(6)&7)||call.u16(2)!=454)continue;
        ++calls;nlohmann::json record={{"root",(unsigned(call.u16(0))<<16)|index}};
        try {
            const auto function=row(call.u32(16));
            need((function.u16(6)&1)&&function.u16(4)==2,"Camera content has no function-name identity");
            const auto text=function.u32(12);need(text<strings.size(),"Camera function name escapes strings");
            need(strings.string(text,strings.size()-text)=="camera_set_animation","Camera function identity changed");
            const auto graph_arg=row(function.u32(8)),name_arg=row(graph_arg.u32(8));
            need(graph_arg.u16(4)==36 && name_arg.u16(4)==11 && name_arg.u32(8)==null_datum &&
                (graph_arg.u16(6)&5)==1 && (name_arg.u16(6)&5)==1,"Camera content lost its two literal operands");
            const auto graph=graph_arg.u32(16),name=name_arg.u32(16);
            const auto* tag=cache.find_tag({graph});need(tag&&tag->group=="jmad"&&tag->address,"Camera graph is absent");
            record["graph"]=graph;record["graph_name"]=tag->name;record["animation_name"]=cache.string_id(name);
            record["animation_name_id"]=name;
            const auto clip_index=cache.find_local_animation_index(graph,name);need(clip_index.has_value(),"Authored camera clip is absent");
            const Bytes metadata(cache.read_tag_bytes(tag->address,0xb4));
            need(*clip_index<metadata.u32(0x2c),"Camera clip index escapes graph metadata");
            const Bytes declaration(cache.read_tag_bytes(metadata.u32(0x30)+*clip_index*0x6c,0x6c));
            need(declaration.u32(0)==name,"Camera clip lost its authored name ID");
            const auto clip=cache.load_named_animation(graph,name);
            need(clip.source_graph==graph && clip.source_index==*clip_index &&
                clip.frame_count==declaration.u16(0x14) && clip.nodes.size()==metadata.u32(0xc) &&
                !clip.nodes.empty(),"Decoded camera clip differs from its authored graph/index/frame/node metadata");
            record["clip_index"]=*clip_index;record["frame_count"]=clip.frame_count;record["node_count"]=clip.nodes.size();
            const auto first=bits(sample_animation(clip,0,0));bool moved=false;
            for(const auto frame:std::array{0.f,.25f,1.f,float(clip.frame_count-1)*.5f,float(clip.frame_count-1)}) {
                const auto pose=bits(sample_animation(clip,0,frame));moved|=pose!=first;++samples;
            }
            const auto replay=cache.load_named_animation(graph,name);
            need(bits(sample_animation(replay,0,.25f))==bits(sample_animation(clip,0,.25f)),
                "Repeated camera resource load changed an authored sample");
            clips.emplace(graph,name);moving+=moved;record["sampled_motion"]=moved;record["passed"]=true;
        }catch(const std::exception& error){record["passed"]=false;record["error"]=error.what();++errors;}
        report["calls"].push_back(std::move(record));
    }
    need(calls==std::stoul(argv[4]),"Camera resource test lost authored callers");
    need(std::ranges::equal(original,cache.read_tag_bytes(scenario.u32(0x23c),syntax.size())),
        "Camera resource decoding modified authored syntax");
    report["authored_calls"]=calls;report["distinct_clips"]=clips.size();report["samples"]=samples;
    report["moving_calls"]=moving;report["errors"]=errors;report["passed"]=errors==0;
    std::ofstream(argv[5])<<report.dump(2)<<'\n';
    std::cout<<(errors?"FAIL":"PASS")<<" actual E3 camera resources: "<<calls<<" callers, "<<clips.size()
        <<" distinct clips, "<<samples<<" samples, "<<errors<<" errors\n";
    return errors?1:0;
}catch(const std::exception& error){std::cerr<<error.what()<<'\n';return 1;}
