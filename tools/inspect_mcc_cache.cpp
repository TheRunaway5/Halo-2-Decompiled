#include "halo2/cache.hpp"
#include "halo2/content.hpp"
#include "halo2/model.hpp"
#include "halo2/original_data.hpp"
#include "halo2/unit_animation_graph_metadata.hpp"
#include "halo2/mcc_audio.hpp"
#include "halo2/source_tag_metadata.hpp"
#include "halo2/sound_variation.hpp"
#include "halo2/tags/system.hpp"
#include "halo2/source_runtime_cache_tags.hpp"
#include <algorithm>
#include <iostream>

int main(int argc,char** argv) {
    try {
        if(argc!=3)throw halo2::FormatError("usage: inspect_mcc_cache MAP --geometry|--models|--content|--multiplayer-content|--materials|--animations|--animation-clips|--sounds");
        const halo2::Cache cache(argv[1]);
        if(!cache.is_mcc())throw halo2::FormatError("Expected a native MCC cache owner");
        std::cout<<cache.describe()<<'\n';
        std::cout<<"Engine operands: "<<cache.original_data()->source.string()<<'\n';
        std::cout<<"Multiplayer combat stores: "<<cache.multiplayer_balance_writes()
                 <<" netgame stores: "<<cache.multiplayer_netgame_writes()<<'\n';
        const std::string action=argv[2];
        if(action=="--geometry") {
            halo2::TagSystem tags(std::make_shared<const halo2::Cache>(argv[1]));
            for(std::size_t i=0;i<cache.bsps().size();++i) {
                tags.load_bsp(i);
                const auto view=tags.get({cache.bsps()[i].datum},halo2::tag_groups::scenario_structure_bsp);
                if(view.address().value<=0xffffffffull || tags.source_address(view.address(),view.info().size)!=view.source_address())
                    throw halo2::FormatError("Native MCC BSP address did not retain its checked 64-bit owner");
                std::cout<<"BSP "<<i<<" vertices: "<<cache.load_bsp(i,false).vertices.size()<<'\n';
            }
        } else if(action=="--content" || action=="--multiplayer-content") {
            for(std::size_t i=0;i<cache.bsps().size();++i) {
                const auto content=cache.load_content(i,action=="--multiplayer-content");
                std::cout<<content.describe()<<'\n';
                for(const auto& limitation:content.limitations)std::cout<<"Unresolved: "<<limitation<<'\n';
            }
        } else if(action=="--materials") {
            std::size_t count=0,passes=0,failed=0;
            for(const auto& tag:cache.tags())if(tag.group=="shad" && tag.address) {
                ++count;
                try {
                    const auto material=cache.load_material(tag.datum);passes+=material.passes.size();
                    if(!material.rendering_gaps.empty()) {
                        ++failed;if(failed<=30) {std::cout<<"Unresolved material "<<tag.name;
                            for(const auto& gap:material.rendering_gaps)std::cout<<"; "<<gap;std::cout<<'\n';}
                    }
                } catch(const std::exception& error) {
                    if(++failed<=30)std::cout<<"Unresolved material "<<tag.name<<": "<<error.what()<<'\n';
                }
            }
            std::cout<<"Material tags: "<<count<<" existing native passes: "<<passes<<" unresolved: "<<failed<<'\n';
            if(failed)return 2;
        } else if(action=="--models") {
            std::size_t count=0,vertices=0;
            for(const auto& tag:cache.tags())if(tag.group=="mode" && tag.address) {
                const auto model=cache.load_model(tag.datum,false);
                vertices+=model.mesh.vertices.size();++count;
            }
            std::cout<<"Models: "<<count<<" vertices: "<<vertices<<'\n';
        } else if(action=="--animations" || action=="--animation-clips") {
            std::size_t count=0,graphs=0,clips=0,decoded=0,resource_reads=0;
            for(const auto& tag:cache.tags())if(tag.group=="jmad" && tag.address) {
                ++graphs;
                try {
                    const auto metadata=cache.load_source_unit_animation_graph_metadata(tag.datum);
                    ++count;
                    if(action=="--animation-clips") {
                        const auto total=halo2::Bytes(metadata.header()).u32(0x2c);
                        for(unsigned n=0;n<total;++n) {
                            ++clips;
                            try {
                                const auto clip=cache.load_animation(tag.datum,n);(void)clip;
                                const auto descriptor=halo2::Bytes(metadata.header()).u32(0xb0)+n*20;
                                const auto resource=cache.load_source_runtime_animation_image(descriptor);
                                if(!resource.bytes.empty()) {
                                    const auto file=halo2::Bytes(resource.descriptor).u32(8);
                                    const auto bytes=cache.load_source_runtime_resource(file,(resource.bytes.size()+511)&~std::size_t(511));
                                    if(!std::ranges::equal(resource.bytes,std::span(bytes).first(resource.bytes.size())))
                                        throw halo2::FormatError("Native animation I/O changed codec bytes");
                                    ++resource_reads;
                                }
                                ++decoded;
                            }
                            catch(const halo2::FormatError& error) {
                                if(clips-decoded<=50)std::cout<<"Unresolved clip "<<tag.name<<" #"<<n<<": "<<error.what()<<'\n';
                            }
                        }
                    }
                } catch(const halo2::FormatError& error) {
                    std::cout<<"Unresolved graph "<<tag.name<<": "<<error.what()<<'\n';
                }
            }
            std::cout<<"Animation graph adapters: "<<count<<" / "<<graphs<<'\n';
            std::cout<<"Decoded animation clips: "<<decoded<<" / "<<clips<<'\n';
            if(action=="--animation-clips")std::cout<<"Owned animation sector reads: "<<resource_reads<<'\n';
            if(count!=graphs || clips!=decoded)return 2;
        } else if(action=="--sounds") {
            std::size_t total=0,loaded=0,decoded=0,frames=0,failed=0,non_opus=0;
            for(const auto& tag:cache.tags())if(tag.group=="snd!" && tag.address) {
                const halo2::Bytes sound(cache.tag_payload({tag.datum}));
                if(sound.u16(0)&0x800)continue; // Classic playback only.
                const auto pitches=cache.sound_pitch_range_count(tag.datum);
                for(unsigned p=0;p<pitches;++p) {
                    for(unsigned n=0;n<cache.sound_permutation_count(tag.datum,p);++n) {
                        ++total;
                        try {
                            const auto encoded=cache.load_sound(tag.datum,p,n);++loaded;
                            const auto pcm=halo2::decode_sound(encoded);frames+=pcm.frames();++decoded;
                            if(encoded.codec!=5)++non_opus;
                            if(n==0)(void)cache.load_sound_variation(tag.datum,p);
                        } catch(const std::exception& error) {
                            if(++failed<=100)std::cout<<"Unresolved sound "<<tag.name<<" #"<<p<<'/'<<n<<": "<<error.what()<<'\n';
                        }
                    }
                }
            }
            std::cout<<"Sound permutations: "<<loaded<<" / "<<total<<" decoded: "<<decoded<<" frames: "<<frames<<" other codecs: "<<non_opus<<" failures: "<<failed<<'\n';
            if(total!=loaded || failed)return 2;
        } else throw halo2::FormatError("Unknown MCC inspection action");
        return 0;
    } catch(const std::exception& error) {std::cerr<<error.what()<<'\n';return 1;}
}
