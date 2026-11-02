#include "halo2/prepared_original_sound_bank.hpp"
#include "halo2/audio.hpp"
#include <algorithm>
#include <fstream>
#include <iostream>
#include <map>
#include <set>
#include <nlohmann/json.hpp>
#include <openssl/evp.h>

namespace {
using namespace halo2;using J=nlohmann::json;
void need(bool ok,const char* why){if(!ok)throw std::runtime_error(why);}
std::string sha(std::span<const std::byte> bytes){
    std::array<unsigned char,32> digest{};unsigned count{};
    need(EVP_Digest(bytes.data(),bytes.size(),digest.data(),&count,EVP_sha256(),nullptr)==1&&count==32,"Cannot hash native bank bytes");
    constexpr char digits[]="0123456789abcdef";std::string result;
    for(auto n:digest){result+=digits[n>>4];result+=digits[n&15];}return result;
}
struct Original {
    std::map<std::string,std::ifstream> files;std::uint64_t comparisons{};
    std::vector<std::byte> read(const std::string& path,std::uint64_t at,std::size_t size){
        auto [i,inserted]=files.try_emplace(path);
        if(inserted)i->second.open(path,std::ios::binary);
        auto& file=i->second;file.clear();file.seekg(std::streamoff(at));std::vector<std::byte> result(size);
        file.read(reinterpret_cast<char*>(result.data()),std::streamsize(size));need(file.gcount()==std::streamsize(size),"Original reference slice is truncated");return result;
    }
    void same(std::span<const std::byte> actual,const std::string& path,std::uint64_t at){
        const auto expected=read(path,at,actual.size());need(std::ranges::equal(actual,expected),"Loaded native metadata differs from literal original bytes");++comparisons;
    }
    void identifier(const std::optional<std::uint32_t>& id,const std::string& path,std::uint64_t at){
        need(id.has_value()&&*id==Bytes(read(path,at,4)).u32(0),"Loaded import StringID differs from its original owner");++comparisons;
    }
};
using Key=std::array<std::string,3>;
struct Validator {
    Original original;std::map<Key,J> waveforms;std::set<Key> decoded;
    std::uint64_t permutations{},chunks{},bounds{},absent_used{},promotions{};
    J banks=J::object();
    void pcm(const PreparedOriginalSoundBank& bank,unsigned pitch,unsigned permutation){
        const auto& m=bank.metadata();const auto& range=m.pitch_ranges[pitch];const auto& p=range.permutations[permutation];
        const Key key{m.name,range.name,p.name};const auto expected=waveforms.find(key);need(expected!=waveforms.end(),"Original named waveform is absent from the independent audit");
        const auto& e=bank.encoded(pitch,permutation);const auto& reference=expected->second;
        need(sha(e.data)==reference.at("original_encoded_sha256").get<std::string>()&&
             e.sample_rate==reference.at("original_rate")&&e.channels==reference.at("original_channels")&&
             e.codec==reference.at("original_codec")&&e.samples==reference.at("original_ugh_sample_size_word"),"Native encoded descriptor differs from independent original audit");
        if(decoded.insert(key).second){
            const auto samples=decode_sound(e);
            need(samples.frames()==reference.at("original_decoded_frames")&&
                 sha(std::as_bytes(std::span(samples.stereo)))==reference.at("original_pcm_sha256").get<std::string>(),"Native prepared bank changed same-PC original PCM");
        }
    }
    void check(const J& test,bool decode=true){
        const auto bank=PreparedOriginalSoundBank::load(test.at("directory").get<std::string>(),test.at("sha256").get<std::string>());
        const auto& m=bank->metadata();const auto& r=test.at("reference");const auto path=r.at("path").get<std::string>();
        original.same(m.sound,path,r.at("sound"));original.same(m.playback,path,r.at("playback"));
        original.same(m.scale,path,r.at("scale"));original.same(m.sound_class,path,r.at("sound_class"));
        if(r.contains("origin")){
            need(m.origin&&m.sound_classes,"Prepared native bank lost its original namespace/class owner");const auto& origin=r.at("origin");
            need(m.origin->map==origin.at("map").get<std::string>()&&m.origin->map_sha256==origin.at("map_sha256").get<std::string>()&&
                m.origin->sound.value==std::stoul(origin.at("sound_datum").get<std::string>(),nullptr,16)&&
                m.origin->gestalt.value==std::stoul(origin.at("gestalt_datum").get<std::string>(),nullptr,16)&&
                m.origin->classes.value==std::stoul(origin.at("class_datum").get<std::string>(),nullptr,16),"Original native bank changed its full namespace identity");
            need(r.at("sound_classes").size()==54,"Original native class reference is incomplete");
            for(unsigned n=0;n<54;++n)original.same((*m.sound_classes)[n],path,r.at("sound_classes")[n]);
        }
        need(m.name==test.at("name").get<std::string>()&&m.pitch_ranges.size()==r.at("pitch_ranges").size(),"Original sound identity or pitch extent changed");
        for(unsigned pitch=0;pitch<m.pitch_ranges.size();++pitch){
            const auto& range=m.pitch_ranges[pitch];const auto& ref=r.at("pitch_ranges")[pitch];
            original.same(range.literal,path,ref.at("literal"));original.same(range.interval,path,ref.at("interval"));
            original.identifier(range.import_name_id,path,ref.at("import_name"));
            if(ref.at("used").is_null()){need(!range.used,"Native bank invented absent original used storage");++absent_used;}
            else{need(range.used.has_value()&&range.used->size()==ref.at("used_bytes"),"Native bank lost used-state ownership");original.same(*range.used,path,ref.at("used"));}
            need(range.permutations.size()==ref.at("permutations").size(),"Native bank lost original permutations");
            for(unsigned n=0;n<range.permutations.size();++n){
                const auto& p=range.permutations[n];const auto& pr=ref.at("permutations")[n];
                original.same(p.literal,path,pr.at("literal"));original.identifier(p.import_name_id,path,pr.at("import_name"));
                need(p.chunks.size()==pr.at("chunks").size(),"Native bank lost literal chunks");
                std::size_t offset{};const auto& encoded=bank->encoded(pitch,n);
                for(unsigned k=0;k<p.chunks.size();++k){
                    const auto& c=p.chunks[k];const auto& cr=pr.at("chunks")[k];need(c.literal.has_value(),"Native bank omitted literal chunk DATA");
                    original.same(*c.literal,path,cr.at("literal"));
                    need(c.bytes==cr.at("bytes")&&c.sector_bytes==cr.at("sector_bytes")&&offset+c.bytes<=encoded.data.size(),"Native chunk extent changed");
                    const auto resource=cr.at("path").get<std::string>();
                    original.same(std::span(encoded.data).subspan(offset,c.bytes),resource,cr.at("offset"));
                    original.same(bank->sectors().subspan(c.offset,c.sector_bytes),resource,cr.at("offset"));
                    offset+=c.bytes;++chunks;
                }
                need(offset==encoded.data.size(),"Native bank has unowned permutation data");if(decode)pcm(*bank,pitch,n);++permutations;
            }
            bool refused=false;try{(void)bank->encoded(pitch,unsigned(range.permutations.size()));}catch(const FormatError&){refused=true;}need(refused,"Native permutation boundary was ignored");++bounds;
        }
        if(r.at("promotion").is_null())need(!m.promotion,"Native bank invented a promotion owner");
        else{
            need(m.promotion.has_value(),"Native bank lost its original promotion owner");const auto& p=*m.promotion;const auto& pr=r.at("promotion");
            original.same(p.literal,path,pr.at("literal"));need(p.rules.size()==pr.at("rules").size()&&p.counters.size()==pr.at("counters").size(),"Native promotion nested owners changed");
            for(unsigned n=0;n<p.rules.size();++n)original.same(p.rules[n],path,pr.at("rules")[n]);
            for(unsigned n=0;n<p.counters.size();++n)original.same(p.counters[n],path,pr.at("counters")[n]);++promotions;
        }
        bool refused=false;try{(void)bank->encoded(unsigned(m.pitch_ranges.size()),0);}catch(const FormatError&){refused=true;}need(refused,"Native pitch boundary was ignored");++bounds;
        auto& counts=banks[m.name];if(counts.is_null())counts=J{{"contexts",0},{"pitch_count",m.pitch_ranges.size()}};
        counts["contexts"]=counts.at("contexts").get<unsigned>()+1u;
    }
};
}
int main(int argc,char** argv){J report{{"passed",false},{"runtime_binding_verified",false},{"xbox_waveform_or_device_verified",false},{"raw_audio_retained",false}};try{
    need(argc==3,"Supply owned original-bank control catalog and bounded output");std::ifstream input(argv[1]);const auto plan=J::parse(input);
    std::ifstream wave_file(plan.at("waveforms").get<std::string>());const auto waves=J::parse(wave_file);need(waves.at("passed").get<bool>(),"Independent PCM audit did not pass");
    Validator v;for(const auto& row:waves.at("cases")){const auto key=row.at("key").get<Key>();auto [i,inserted]=v.waveforms.emplace(key,row);need(inserted||i->second==row,"Independent original waveform audit is ambiguous");}
    for(const auto& row:plan.at("cases")){report["active_context"]=J{{"map",row.at("map")},{"name",row.at("name")}};v.check(row);}
    report.erase("active_context");
    need(v.banks.size()==31&&v.decoded.size()==273,"Incomplete original destruction bank/waveform coverage");
    unsigned refusals{},fidelity{};
    for(const auto& row:plan.at("rejections")){
        bool rejected=false;try{(void)PreparedOriginalSoundBank::load(row.at("directory").get<std::string>(),row.at("sha256").get<std::string>());}catch(const std::exception&){rejected=true;}
        need(rejected,("Native bank admitted malformed control "+row.at("name").get<std::string>()).c_str());++refusals;
    }
    for(const auto& row:plan.at("fidelity_mutations")){
        Validator changed;changed.waveforms=v.waveforms;
        bool rejected=false;try{changed.check(row,false);}catch(const std::exception&){rejected=true;}
        need(rejected,"Independent original literal comparison missed a fidelity mutation");++fidelity;
    }
    unsigned legacy{};for(const auto& row:plan.at("legacy")){
        const auto bank=PreparedOriginalSoundBank::load(row.at("directory").get<std::string>(),row.at("sha256").get<std::string>());
        need(bank->metadata().pitch_ranges.size()==1&&!bank->metadata().promotion,"Legacy prepared format changed its single pitch/no-promotion contract");
        const auto& range=bank->metadata().pitch_ranges[0];need(!range.import_name_id,"Legacy bank invented an unavailable import StringID");
        if(row.at("schema")==1)need(!range.used,"V1 invented initial permutation storage");
        else need(range.used.has_value(),"V2 lost initial permutation storage");
        for(unsigned n=0;n<range.permutations.size();++n)v.pcm(*bank,0,n);++legacy;
    }
    report.update(J{{"passed",true},{"contexts",plan.at("cases").size()},{"banks",v.banks},{"unique_original_pcm_comparisons",v.decoded.size()},
        {"literal_byte_comparisons",v.original.comparisons},{"permutation_comparisons",v.permutations},{"chunks",v.chunks},{"bounds_refusals",v.bounds},
        {"absent_used_ranges",v.absent_used},{"promotion_contexts",v.promotions},{"malformed_input_refusals",refusals},{"fidelity_mutation_refusals",fidelity},{"legacy_schema_controls",legacy}});
}catch(const std::exception& error){report["error"]=error.what();}
if(argc==3){std::ofstream output(argv[2]);output<<report.dump(2)<<'\n';if(!output)return 2;}
std::cout<<report.value("error",std::string("Original bank literals, all pitch ranges and same-PC decoded waveforms match"))<<'\n';return report.at("passed").get<bool>()?0:1;}
