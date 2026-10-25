// Read-only canonical metadata inventory; build only in the owned cohort.
#include "halo2/cache.hpp"
#include "halo2/source_tag_metadata.hpp"
#include "halo2/original_data.hpp"
#include <fstream>
#include <iostream>
#include <nlohmann/json.hpp>
using namespace halo2;
int main(int argc,char **argv)try {
    if(argc!=3||!std::getenv("HALO2_TEST_SCRATCH"))throw FormatError("Supply MAP REPORT under owned scratch");
    Cache cache(argv[1],std::filesystem::path(argv[1]).parent_path(),OriginalData::native_tu15());
    nlohmann::json result{{"map",argv[1]},{"projectile_attachments",nlohmann::json::array()},
        {"effect_lens_parts",nlohmann::json::array()},{"lights_with_lens",nlohmann::json::array()},
        {"projected_lights",nlohmann::json::array()},
        {"object_effect_attachments",nlohmann::json::array()},{"unresolved",nlohmann::json::array()}};
    const auto rows=[&](const Bytes &b,unsigned at,unsigned stride) {
        const auto count=b.u32(at),address=b.u32(at+4);
        if(count>65536)throw FormatError("Inventory block exceeds original bounds");
        return count?Bytes(cache.read_tag_bytes(address,std::size_t(count)*stride)):Bytes(std::span<const std::byte>{});
    };
    for(const auto &tag:cache.tags())if(tag.address&&(tag.group=="proj"||tag.group=="effe"||tag.group=="ligh"||
        tag.group=="scen"||tag.group=="mach"||tag.group=="ctrl"||tag.group=="lifi"||tag.group=="bloc"))try {
        const auto m=cache.load_source_tag_metadata(tag.datum);const Bytes b(m.bytes_at(m.source_address(),m.size()));
        if(tag.group=="proj") {
            const auto a=rows(b,0x94,24);
            for(unsigned n=0;n<b.u32(0x94);++n) {
                const auto at=n*24,group=a.u32(at),definition=a.u32(at+4);
                if(definition==0xffffffffu||(group!=0x6c656e73u&&group!=0x4d475332u))continue;
                result["projectile_attachments"].push_back({{"projectile",tag.name},{"projectile_identity",tag.datum},
                    {"attachment",n},{"group",group},{"definition",definition},{"marker",a.u32(at+8)},
                    {"color",a.u16(at+12)},{"function",a.u32(at+16)},{"object_flags",b.u16(2)}});
            }
        }else if(tag.group=="effe") {
            const auto events=rows(b,0x14,0x38);
            for(unsigned e=0;e<b.u32(0x14);++e) {
                const auto parts=rows(events,e*0x38+0x18,0x38);
                for(unsigned p=0;p<events.u32(e*0x38+0x18);++p) {
                    const auto at=p*0x38;
                    if(parts.u32(at+8)!=0x6c656e73u||parts.u32(at+16)==0xffffffffu)continue;
                    result["effect_lens_parts"].push_back({{"effect",tag.name},{"effect_identity",tag.datum},
                        {"event",e},{"part",p},{"lens",parts.u32(at+16)},{"environment",parts.u16(at)},
                        {"location",parts.u16(at+4)},{"flags",parts.u16(at+6)},
                        {"scale_a",parts.u32(at+0x30)},{"scale_b",parts.u32(at+0x34)}});
                    const auto index=parts.u16(at+4);
                    if(index<b.u32(0xc)) {
                        const Bytes names(cache.read_tag_bytes(b.u32(0x10),std::size_t(b.u32(0xc))*4));
                        const auto name=names.u32(index*4);auto &row=result["effect_lens_parts"].back();
                        row["canonical_name"]=name;row["canonical_name_text"]=cache.string_id(name);
                        const Bytes native(cache.tag_payload(tag.handle()));
                        const Bytes native_names(cache.read_tag_bytes(native.u32(0x10),std::size_t(native.u32(0xc))*4));
                        row["native_name"]=native_names.u32(index*4);row["native_name_text"]=cache.string_id(native_names.u32(index*4));
                    }
                }
            }
        }else if(tag.group=="ligh") {
            if(b.u32(0x94)!=0xffffffffu)result["lights_with_lens"].push_back({{"light",tag.name},{"identity",tag.datum},{"lens",b.u32(0x94)}});
            if(b.u16(4))result["projected_lights"].push_back({{"light",tag.name},{"identity",tag.datum},{"type",b.u16(4)},{"flags",b.u32(0)}});
        }else {
            const auto a=rows(b,0x94,24);
            for(unsigned n=0;n<b.u32(0x94);++n) {
                const auto at=n*24;
                if(a.u32(at)!=0x65666665u||a.u32(at+4)==0xffffffffu)continue;
                result["object_effect_attachments"].push_back({{"object",tag.name},{"object_identity",tag.datum},
                    {"group",tag.group},{"slot",n},{"effect",a.u32(at+4)},{"marker",a.u32(at+8)},
                    {"function_a",a.u32(at+16)},{"function_b",a.u32(at+20)}});
            }
        }
    }catch(const FormatError &e){result["unresolved"].push_back({{"tag",tag.name},{"error",e.what()}});}
    result["runtime_admission_or_pixels_verified"]=false;
    std::ofstream(argv[2])<<result.dump(2)<<'\n';
    std::cout<<"Read actual canonical projectile, effect and light references\n";return 0;
}catch(const std::exception &e){std::cerr<<e.what()<<'\n';return 1;}
