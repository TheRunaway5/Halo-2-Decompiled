#include "halo2/menu_world_vertex_catalog.hpp"
#include "menu_world_vertex_catalog.generated.hpp"
#include <filesystem>
#include <iostream>

using namespace halo2;
int main(int argc,char** argv) {try {
    if(argc!=2)throw FormatError("hologram catalog test requires original opening map");
    if(!std::filesystem::exists(argv[1]))return 77;
    const Cache cache(argv[1]);
    // Original Regret HLMT override E87106FC resolves through its sole
    // layer21 to the original E87406FF program. Runtime catalog lookup uses
    // the complete bytes/declaration, never these datum values or a name.
    constexpr std::uint32_t vertex=0xe87406ffu;
    unsigned programs{},instructions{};
    for(unsigned cls=0;cls<7;++cls) {
        const auto bytes=cache.load_vertex_shader_code(vertex,cls);
        const auto declaration=cache.load_source_vertex_declaration(vertex,cls);
        const auto& entry=menu_world_vertex_catalog_lookup(
            generated_menu_world_vertex_catalog::entries,bytes,declaration);
        if(entry.abi!=menu_world_vertex_catalog_abi || entry.class_index!=cls ||
           entry.program_bytes!=bytes.size() || entry.spirv.empty())
            throw FormatError("original hologram program lost its own embedded class");
        instructions+=decode_vertex_program(bytes).count;++programs;
        // Changing a literal word must not select the original program's
        // compiled module through its unchanged declaration or source datum.
        auto changed=bytes;changed.at(4)^=std::byte{1};
        bool rejected=false;
        try{(void)menu_world_vertex_catalog_lookup(
            generated_menu_world_vertex_catalog::entries,changed,declaration);}
        catch(const FormatError&){rejected=true;}
        if(!rejected)throw FormatError("changed hologram token selected an existing module");
    }
    std::cout<<"PASS original hologram catalog: "<<programs<<" class0..6 programs / "
        <<instructions<<" instructions; 7 changed-token controls; no live rendering claim\n";
    return 0;
}catch(const std::exception& error){std::cerr<<error.what()<<'\n';return 1;}}
