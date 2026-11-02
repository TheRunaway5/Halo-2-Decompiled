#pragma once
#include "halo2/cache.hpp"
#include "halo2/mcc_original_sound_runtime_delivery_tu15.hpp"
#include <map>
#include <nlohmann/json.hpp>

namespace halo2::test_original_sound {
inline void need(bool value,const char* why){if(!value)throw FormatError(why);}
struct Names {
    std::map<std::uint32_t,std::pair<std::uint32_t,std::string>> original;
    std::map<std::uint32_t,std::string> native;
    void bind(std::uint32_t old,std::uint32_t id,const std::string& name){
        const auto [i,added]=original.emplace(old,std::pair{id,name});need(added||i->second==std::pair{id,name},"Actual Cache import aliases original identities");
        const auto [n,unique]=native.emplace(id,name);need(unique||n->second==name,"Actual Cache import aliases native StringIDs");
    }
};
// Only actual resident Classic snd! roots and exact pitch/permutation import
// names may bind an externally admitted original catalog to this Cache.
struct Inputs {
    std::vector<OriginalSoundRuntimeBindingTu15> bindings;
    std::shared_ptr<Names> names=std::make_shared<Names>();
    MccOriginalSoundRuntimeInputTu15 view()const{
        const auto retained=names;
        return {bindings,{retained,[retained](auto old,auto label){const auto& n=retained->original.at(old);need(n.second==label,"Original import label changed");return n.first;},
            [retained](auto id){return retained->native.at(id);}}};
    }
    Inputs(const Cache& cache,const nlohmann::json& catalog,const std::string& map){
        for(const auto& entry:catalog.at("cases"))if(entry.at("map")==map){
            const auto bank=PreparedOriginalSoundBank::load(entry.at("directory").get<std::string>(),entry.at("sha256").get<std::string>());
            const auto& m=bank->metadata();const Tag* tag{};
            for(const auto& t:cache.tags())if(t.address&&t.group=="snd!"&&t.name==m.name){need(!tag,"Actual Cache original target path is ambiguous");tag=&t;}
            need(tag,"Actual Cache original target is missing");
            const Bytes snd(cache.read_tag_bytes(tag->address,28));need(!(snd.u16(0)&0x800u),"Actual Cache original target is not Classic");
            const auto& owner=cache.mcc_sound_gestalt_owner(tag->handle());const Bytes g(cache.read_tag_bytes(owner.address,120));
            const auto name=[&](unsigned index){need(index<g.u32(24),"Actual Cache name ordinal escapes its gestalt");
                const auto id=Bytes(cache.read_tag_bytes(g.u32(28)+index*4u,4)).u32(0);return std::pair{id,cache.string_id(id)};};
            std::map<std::string,std::uint32_t> ranges;
            const auto count=std::to_integer<unsigned>(snd.slice(10,1)[0]);
            need(std::uint64_t(snd.u16(8))+count<=g.u32(40),"Actual Cache pitch ranges escape their owner");
            for(unsigned n=0;n<count;++n){const auto at=g.u32(44)+(snd.u16(8)+n)*12u;const auto row=cache.read_tag_bytes(at,12);const auto named=name(Bytes(row).u16(0));
                need(ranges.emplace(named.second,at).second,"Actual Cache original pitch name is ambiguous");}
            for(const auto& pitch:m.pitch_ranges){need(pitch.import_name_id&&ranges.contains(pitch.name),"Actual Cache lacks the exact original pitch name");
                const Bytes range(cache.read_tag_bytes(ranges.at(pitch.name),12));const auto rn=name(range.u16(0));names->bind(*pitch.import_name_id,rn.first,rn.second);
                need(std::uint64_t(range.u16(8))+range.u16(10)<=g.u32(48),"Actual Cache permutations escape their owner");
                std::map<std::string,std::uint32_t> permutations;
                for(unsigned n=0;n<range.u16(10);++n){const Bytes row(cache.read_tag_bytes(g.u32(52)+(range.u16(8)+n)*16u,16));const auto named=name(row.u16(0));
                    need(permutations.emplace(named.second,named.first).second,"Actual Cache original permutation name is ambiguous");}
                for(const auto& p:pitch.permutations){need(p.import_name_id&&permutations.contains(p.name),"Actual Cache lacks the exact original permutation name");names->bind(*p.import_name_id,permutations.at(p.name),p.name);}
            }
            bindings.push_back({tag->handle(),bank});
        }
        need(!bindings.empty(),"Admitted original catalog has no selected-map bindings");
    }
};
}
