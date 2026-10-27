#pragma once
// Explicit offline maintenance only. Follow the same complete typed metadata
// owners as Cache::load_particle_material, without opening any bitmap resource.
#include "halo2/cache.hpp"
#include "halo2/render_program.hpp"
#include <nlohmann/json.hpp>
#include <set>
#include <tuple>

namespace halo2 {
template<class Emit>
nlohmann::json emit_native_particle_fragment_inventory(const Cache& cache,Emit emit) {
    using json=nlohmann::json;
    struct Block {std::uint32_t count{},address{};std::vector<std::byte> storage;};
    const auto block=[&](const Bytes& parent,std::size_t offset,std::size_t stride) {
        Block result;result.count=parent.u32(offset);result.address=parent.u32(offset+4);
        if(result.count>65536 || (result.count&&!result.address))
            throw FormatError("Particle fragment inventory block has invalid ownership");
        if(result.count) {
            const auto raw=cache.read_tag_bytes(result.address,std::size_t(result.count)*stride);
            result.storage.assign(raw.begin(),raw.end());
        }
        return result;
    };
    const auto typed=[&](std::uint32_t datum,const char* group)->const Tag& {
        const auto found=cache.query_tag({.handle=TagHandle{datum},.group=TagGroup::from_string(group),.require_address=true});
        if(!found)throw FormatError("Particle fragment inventory lacks a typed resident dependency");
        return *found;
    };
    const auto range=[](const Bytes& bytes,std::size_t offset,const Block& owner) {
        const ShaderRange result{bytes.u16(offset)};
        if(result.first()>owner.count || result.count()>owner.count-result.first())
            throw FormatError("Particle fragment inventory range escapes its typed owner");
        return result;
    };
    json report={{"particle_owners",0},{"template_definitions",0},{"pass_selections",0},
                 {"complete_programs",0},{"classes",json::object()},{"rejected",json::array()}};
    for(const auto& tag:cache.tags()) {
        if((tag.group!="prt3"&&tag.group!="PRTM")||!tag.address)continue;
        report["particle_owners"]=report["particle_owners"].template get<unsigned>()+1;
        json alias={{"kind","particle-pass"},{"group",tag.group},{"name",tag.name},{"datum",tag.datum}};
        try {
            const auto raw=cache.tag_payload(tag.handle());const Bytes root(raw);
            const bool model=tag.group=="PRTM";
            const unsigned classification=model?1u:root.u16(4)+7u;
            alias["vertex_class"]=classification;
            const auto class_key=std::to_string(classification);
            report["classes"][class_key]=report["classes"].value(class_key,0u)+1;
            // PRTM owns a shad reference; prt3 embeds its shader and pp block.
            const auto shader=model?cache.tag_payload(typed(root.u32(0x1c),"shad").handle()):raw;
            const auto pp=block(Bytes(shader),model?0x20:0x8c,124);
            if(!pp.count)throw FormatError("Particle fragment owner has no complete cooked definition");
            const auto stem_datum=Bytes(pp.storage).u32(0);
            const auto& stem=typed(stem_datum,"stem");alias["template_datum"]=stem_datum;alias["template"]=stem.name;
            const auto templates=block(Bytes(cache.tag_payload(stem.handle())),0x58,40);
            if(!templates.count)throw FormatError("Particle template has no complete cooked definition");
            // Exact pixel controls are selected by pass implementation. The
            // template remap changes only the18 dynamic color data words.
            std::set<std::tuple<std::uint32_t,unsigned>> emitted;
            for(unsigned definition=0;definition<templates.count;++definition) {
                report["template_definitions"]=report["template_definitions"].template get<unsigned>()+1;
                const Bytes table(templates.storage);const auto base=definition*40;
                const auto lods=block(table,base,10),layers=block(table,base+8,2),passes=block(table,base+16,10),
                           implementations=block(table,base+24,6);
                const Bytes lod_bytes(lods.storage),layer_bytes(layers.storage),pass_bytes(passes.storage);
                for(unsigned lod=0;lod<lods.count;++lod) {
                    const auto lr=range(lod_bytes,lod*10,layers);
                    for(unsigned layer=0;layer<lr.count();++layer) {
                        const auto pr=range(layer_bytes,(lr.first()+layer)*2,passes);
                        for(unsigned pass=0;pass<pr.count();++pass) {
                            const auto offset=(pr.first()+pass)*10;
                            const auto pass_datum=pass_bytes.u32(offset+4);
                            const auto& spas=typed(pass_datum,"spas");
                            const auto selected=range(pass_bytes,offset+8,implementations);
                            const auto processed=block(Bytes(cache.tag_payload(spas.handle())),0x1c,88);
                            if(!processed.count)throw FormatError("Particle pass has no processed source definition");
                            const auto impl=block(Bytes(processed.storage),0,306);
                            if(selected.count()>impl.count)throw FormatError("Particle implementation choice exceeds its pass definition");
                            const Bytes records(impl.storage);
                            for(unsigned implementation=0;implementation<selected.count();++implementation) {
                                report["pass_selections"]=report["pass_selections"].template get<unsigned>()+1;
                                if(!emitted.emplace(pass_datum,implementation).second)continue;
                                PixelProgram program;
                                for(unsigned word=0;word<60;++word)program.words[word]=records.u32(implementation*306+6+word*4);
                                auto owner=alias;owner["lod"]=lod;owner["layer"]=layer;owner["layer_pass"]=pass;
                                owner["pass_datum"]=pass_datum;owner["pass"]=spas.name;owner["implementation"]=implementation;
                                owner["vertex_datum"]=records.u32(implementation*306+256);
                                owner["complete_control_words"]=60;
                                emit(program,owner);
                                report["complete_programs"]=report["complete_programs"].template get<unsigned>()+1;
                            }
                        }
                    }
                }
            }
        } catch(const std::exception& error) {
            alias["error"]=error.what();report["rejected"].push_back(std::move(alias));
        }
    }
    report["metadata_only"]=true;report["bitmap_resources_opened"]=0;
    report["complete"]=report["rejected"].empty();return report;
}
} // namespace halo2
