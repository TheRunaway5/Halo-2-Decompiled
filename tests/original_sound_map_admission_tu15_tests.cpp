#include "halo2/prepared_original_sound_catalog_tu15.hpp"
#include "halo2/original_data.hpp"
#include "halo2/source_runtime_mcc_preparation.hpp"
#include "halo2/source_runtime_resource_preparation.hpp"
#include "halo2/source_runtime_resources.hpp"
#include "halo2/mcc_sound_dat_resources_tu15.hpp"
#include "helpers/original_sound_preparation_inputs.hpp"
#include <algorithm>
#include <fstream>
#include <iostream>
#include <map>
#include <nlohmann/json.hpp>
#include <openssl/evp.h>
#include <set>
namespace {
using namespace halo2;using J=nlohmann::json;namespace fs=std::filesystem;
void need(bool value,const char* why){if(!value)throw FormatError(why);}
std::string hash(std::span<const std::byte> bytes){std::array<unsigned char,32> digest;unsigned count{};
    need(EVP_Digest(bytes.data(),bytes.size(),digest.data(),&count,EVP_sha256(),nullptr)==1&&count==32,"Cannot hash admitted map input");
    constexpr char hex[]="0123456789abcdef";std::string out;for(auto b:digest){out+=hex[b>>4];out+=hex[b&15];}return out;}
template<class F>void rejects(F&& call,unsigned& count){bool refused=false;try{call();}catch(const std::exception&){refused=true;}
    need(refused,"Mismatched or consumed original map resources were admitted");++count;}
struct PrefixReads {std::uint64_t routes{},bytes{},unaligned{},high_bits{},permutations{},encoded_chunks{};};
PrefixReads prefix_data(const Cache& cache,const MccSoundRuntimePlan& actual,const SourceRuntimeResources& resources,unsigned& controls) {
    need(bool(actual.original),"Prefix integration requires the complete joined original owner");const auto& parent=actual.original->parent();
    const auto row=[&](unsigned table,unsigned index,unsigned stride){const auto block=parent.blocks[table];need(index<std::uint32_t(block.count),"Prefix test selector escaped its actual table");
        return Bytes(std::span(parent.bytes).subspan(std::uint64_t(block.address)-parent.address+std::uint64_t(index)*stride,stride));};
    std::map<fs::path,std::ifstream> files;std::set<std::uint32_t> visited;PrefixReads count;std::optional<SourceSoundCacheReadTu15> selected;
    const auto dat=resources.sound_dat_owner();need(bool(dat),"Prefix PCM test requires the retained DAT owner");
    std::optional<MccSoundResourceRead> encoded_selected;
    for(const auto& sound:parent.sounds) {
        const auto bank=sound.file==MccSoundDataFile::English?MccResourceBank::EnglishSounds:
            sound.file==MccSoundDataFile::Remastered?MccResourceBank::RemasteredSounds:MccResourceBank::NeutralSounds;
        const auto path=cache.mcc_resource_bank_path(sound.sound,bank);auto [stream,added]=files.try_emplace(path);
        if(added){stream->second.open(path,std::ios::binary);need(bool(stream->second),"Independent prefix DAT source is missing");}
        const auto patch=std::ranges::find(parent.patches,sound.sound,&MccSoundRuntimePatch::owner);need(patch!=parent.patches.end(),"Prefix test lost its source sound root");const Bytes root(patch->after);
        for(unsigned pitch=0;pitch<std::to_integer<unsigned>(root.slice(10,1)[0]);++pitch){const auto range=row(4,root.u16(8)+pitch,12);
            for(unsigned p=0;p<range.u16(10);++p){const auto permutation=row(5,range.u16(8)+p,16);const auto expected=cache.load_sound(sound.sound.value,pitch,p);std::size_t at{};
                const auto retained=cache.load_sound(sound.sound.value,pitch,p,[&](const auto& request){
                    if(!encoded_selected)encoded_selected=request;++count.encoded_chunks;return dat->read_encoded(cache,request);});
                need(retained.name==expected.name&&retained.data==expected.data&&retained.codec==expected.codec&&retained.channels==expected.channels&&
                    retained.sample_rate==expected.sample_rate&&retained.samples==expected.samples&&retained.sound_class==expected.sound_class&&
                    retained.packet_chunks.size()==expected.packet_chunks.size(),"Retained prefix PCM parser changed encoded bytes or format");
                for(unsigned n=0;n<retained.packet_chunks.size();++n){const auto& a=retained.packet_chunks[n];const auto& b=expected.packet_chunks[n];
                    need(a.bytes==b.bytes&&a.pcm_bytes==b.pcm_bytes&&a.reset_after==b.reset_after,"Retained prefix parser changed decoder packet boundaries");}
                ++count.permutations;
                for(unsigned c=0;c<permutation.u16(14);++c){const auto index=permutation.u16(12)+c;const auto chunk=row(8,index,12);const auto raw=chunk.u32(4)&0x3fffffffu;
                    const SourceSoundCacheReadTu15 request{actual.blocks[8].address+index*12u,chunk.u32(0),chunk.u32(4),(raw+511u)&~511u};
                    const auto delivered=resources.sound_cache(request);need(delivered.size()==request.bytes&&at<=expected.data.size()&&raw<=expected.data.size()-at&&
                        std::ranges::equal(std::span(delivered).first(raw),std::span(expected.data).subspan(at,raw)),"Typed prefix DAT bytes differ from actual Cache audio");
                    std::vector<std::byte> sector(request.bytes);stream->second.clear();stream->second.seekg(std::streamoff(request.encoded_offset));
                    need(bool(stream->second.read(reinterpret_cast<char*>(sector.data()),sector.size()))&&delivered==sector,"Typed prefix read changed actual DAT sector padding");
                    if(visited.insert(index).second){++count.routes;count.bytes+=delivered.size();count.unaligned+=request.encoded_offset%512!=0;count.high_bits+=request.encoded_offset>>30!=0;}
                    at+=raw;if(!selected)selected=request;
                }need(at==expected.data.size(),"Typed prefix test omitted encoded data");
            }
        }
    }
    need(visited.size()==parent.chunks.size()&&selected,"Typed prefix test omitted a retained chunk");
    for(unsigned n=0;n<4;++n){auto bad=*selected;if(n==0)bad.descriptor=actual.blocks[8].address-12u;
        else if(n==1)bad.encoded_offset^=0x40000000u;else if(n==2)bad.size_flags^=0x80000000u;else bad.bytes+=512u;
        rejects([&]{(void)resources.sound_cache(bad);},controls);}
    need(encoded_selected&&count.encoded_chunks>=count.routes,"Prefix PCM test omitted encoded descriptors");
    for(unsigned n=0;n<6;++n){auto bad=*encoded_selected;if(n==0)bad.sound.value^=0x10000u;
        else if(n==1)bad.gestalt.value^=0x10000u;else if(n==2)++bad.descriptor;
        else if(n==3)bad.bank=bad.bank==MccResourceBank::EnglishSounds?MccResourceBank::NeutralSounds:MccResourceBank::EnglishSounds;
        else if(n==4)bad.file_offset^=0x40000000u;else ++bad.bytes;
        rejects([&]{(void)dat->read_encoded(cache,bad);},controls);}
    return count;
}
}
int main(int argc,char** argv){J report{{"passed",false},{"goal_complete",false},{"world_or_device_verified",false},{"retained_large_artifacts",J::array()}};
    if(argc!=6||!fs::is_regular_file(argv[1])||!fs::is_regular_file(argv[3]))return 77;
    try {
        std::ifstream input(argv[3],std::ios::binary);const std::string raw{std::istreambuf_iterator<char>(input),{}};
        need(hash(std::as_bytes(std::span(raw.data(),raw.size())))==argv[4],"Exporter catalog differs from its independent admission pin");
        const fs::path map=argv[1];auto cache=std::make_shared<const Cache>(map,map.parent_path(),OriginalData::native_tu15());
        const auto catalog=cache->original_sound_catalog();need(bool(catalog)&&catalog->original_context()==argv[2],"Normal Cache admission omitted its exact original sound catalog");
        const auto digest=hash(cache->source_runtime_cache_bytes());
        const test_original_sound::Inputs expected(*cache,J::parse(raw),argv[2]);const auto actual=catalog->input(*cache);
        need(actual.bindings.size()==expected.bindings.size(),"Normal map admission changed its original bank set");
        std::uint64_t routes{};
        for(unsigned n=0;n<actual.bindings.size();++n){const auto& a=actual.bindings[n];const auto& b=expected.bindings[n];
            need(a.sound==b.sound&&a.bank->metadata().sound==b.bank->metadata().sound&&std::ranges::equal(a.bank->sectors(),b.bank->sectors()),"Normal map admission changed original metadata or resource sectors");
            for(unsigned pitch=0;pitch<a.bank->metadata().pitch_ranges.size();++pitch)for(unsigned p=0;p<a.bank->metadata().pitch_ranges[pitch].permutations.size();++p){
                const auto& x=a.bank->encoded(pitch,p);const auto& y=b.bank->encoded(pitch,p);
                need(x.data==y.data&&x.samples==y.samples&&x.codec==y.codec&&x.sample_rate==y.sample_rate&&x.channels==y.channels,"Normal map admission changed an original encoded route");++routes;}
        }
        unsigned controls{};PrefixReads prefix;std::unique_ptr<SourceRuntimeResources> prepared_resources;
        {
            SourceRuntimeResourcePreparation prepared(cache);
            auto metadata=take_prepared_mcc_world_metadata(*cache,0x90000000u,0x60000000u);
            need(metadata&&metadata->sound.original&&metadata->sound.original->bindings().size()==expected.bindings.size(),"Normal countdown preparation lost original sound metadata");
            prepared_resources=std::make_unique<SourceRuntimeResources>(*cache);
            prepared_resources->require_sound_plan(metadata->sound);
            prefix=prefix_data(*cache,metadata->sound,*prepared_resources,controls);
            need(!prepared_resources->take_mcc_world_metadata(0x90000000u,0x60000000u),"Prepared resources acquired a second canonical plan");
            rejects([&]{(void)take_prepared_mcc_world_metadata(*cache,0x90000000u,0x60000000u);},controls);
            rejects([&]{SourceRuntimeResources again(*cache);},controls);
        }
        SourceRuntimeResources direct(*cache);
        rejects([&]{(void)direct.take_mcc_world_metadata(0x90001000u,0x60000000u);},controls);
        rejects([&]{(void)direct.take_mcc_world_metadata(0x90000000u,0x60001000u);},controls);
        auto metadata=direct.take_mcc_world_metadata(0x90000000u,0x60000000u);
        need(metadata&&metadata->sound.original&&direct.identity()==prepared_resources->identity(),"Direct and countdown paths split original resource identity");
        direct.require_sound_plan(metadata->sound);
        need(direct.original_sound_owner()==metadata->delivery.original_sound_owner()&&
            direct.original_sound_owner()!=prepared_resources->original_sound_owner(),"Direct path did not create its own complete paired owner");
        rejects([&]{prepared_resources->require_sound_plan(metadata->sound);},controls);
        rejects([&]{(void)direct.take_mcc_world_metadata(0x90000000u,0x60000000u);},controls);
        need(hash(cache->source_runtime_cache_bytes())==digest,"Normal original sound admission changed Cache bytes");
        report.update(J{{"passed",true},{"map",argv[2]},{"original_bindings",actual.bindings.size()},{"encoded_routes",routes},{"negative_controls",controls},
            {"normal_cache_catalog_admitted",true},{"countdown_preparation_automatic",true},{"direct_resources_metadata_paired",true},
            {"equal_resource_identity_distinct_paired_owners",true},{"raw_cache_unchanged",true}});
        report.update(J{{"mcc_prefix_dat_routes_verified",true},{"mcc_prefix_dat_routes",prefix.routes},{"mcc_prefix_sector_bytes",prefix.bytes},
            {"mcc_prefix_unaligned_offsets",prefix.unaligned},{"mcc_prefix_high_offsets",prefix.high_bits},
            {"mcc_prefix_pcm_same_dat_owner_verified",true},{"mcc_prefix_pcm_permutations",prefix.permutations},{"mcc_prefix_pcm_chunk_reads",prefix.encoded_chunks}});
    }catch(const std::exception& error){report["error"]=error.what();}
    std::ofstream output(argv[5]);output<<report.dump(2)<<'\n';if(!output)return 2;std::cout<<report.dump()<<'\n';return report.at("passed").get<bool>()?0:1;
}
