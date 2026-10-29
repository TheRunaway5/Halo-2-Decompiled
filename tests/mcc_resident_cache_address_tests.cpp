#include "halo2/cache.hpp"
#include "halo2/physics_live_world_tu15.hpp"
#include "halo2/menu_world_source_pass.hpp"
#include "halo2/source_shadow_raw_draw.hpp"
#include "halo2/source_tag_metadata.hpp"
#include "halo2/source_shadow_scene.hpp"
#include "halo2/source_object_functions_tu15.hpp"
#include "halo2/source_game_variant.hpp"
#include "halo2/model_selection.hpp"
#include "halo2/content.hpp"
#include <algorithm>
#include <iostream>
#include <thread>

namespace {
using namespace halo2;
void require(bool condition,const char* message){if(!condition)throw FormatError(message);}
template<class F> void rejects(F&& action) {
    try {action();}catch(const FormatError&){return;}
    throw FormatError("Resident cache ownership test accepted a foreign or incomplete address");
}
unsigned native_shadow_bitmap_frames(const Cache& cache,PhysicsLiveWorldTu15& world) {
    const Tag *banshee{},*biped{},*shader{};
    for(const auto& tag:cache.tags()) {
        if(tag.group=="vehi" && tag.name=="objects\\vehicles\\banshee\\banshee")banshee=&tag;
        if(tag.group=="bipd" && tag.name.find("masterchief")!=std::string::npos)biped=&tag;
        if(tag.group=="shad" && tag.name=="objects\\vehicles\\banshee\\shaders\\banshee_oil_cab")shader=&tag;
    }
    if(!banshee || !shader)return 0; // A map without this material does not cover this seam.
    require(biped && !cache.spawns().empty(),"Native shader input test lacks its actual actor/spawn definitions");
    const auto definition=cache.load_initial_model_definition(banshee->datum);
    const auto actor_model=cache.load_initial_model_definition(biped->datum);
    world.initialize_map();world.configure_session(source_game_variant_default(0),1,30,0x12345678u);
    const auto position=cache.spawns().front().position;
    const std::array objects{world.create_biped(biped->datum,position,{1,0,0}),
        world.create_biped(biped->datum,position,{1,0,0})};
    require(objects[0]!=0xffffffffu && objects[1]!=0xffffffffu && objects[0]!=objects[1],
        "Actual actor births lost their independent full native identities");
    // Test the explicit material-input seam on two real native actors. It
    // supplies no geometry draw and makes no Banshee framebuffer assertion.
    Mesh mesh;const std::array models{definition.render_model,actor_model.render_model};
    SourceShadowScene scene(cache,mesh,world.source_bsp_index(),models);
    const auto pass=load_menu_world_source_pass(cache,shader->datum,{2,10,0,0});
    auto authored=cache.load_material(shader->datum);authored.selected_shader_lod=2;
    auto materials=prepare_shadow_material(cache,authored,SourceShadowPhase::Generate,true,false,.5f);
    const auto selected=std::find_if(materials.begin(),materials.end(),[&](const auto& material) {
        const auto& m=material.material;const auto& p=m.passes.at(m.forward_pass_index);
        return p.template_pass==pass.template_pass() && p.implementation==0;
    });require(selected!=materials.end(),"Actual Banshee cab shadow implementation is absent");
    std::array completed{*selected,*selected};
    const std::array owners{MenuWorldSourceOwner{MenuWorldSourceOwnerKind::dynamic_object,
        actor_model.render_model,objects[0]&65535u,objects[0]},
        MenuWorldSourceOwner{MenuWorldSourceOwnerKind::dynamic_object,
        actor_model.render_model,objects[1]&65535u,objects[1]}};
    constexpr auto change_to_heretic=0x11001e4du;
    require(cache.string_id(change_to_heretic)=="change_to_heretic","MCC Banshee function namespace changed");
    source_object_function_set_tu15(world,objects[0],change_to_heretic,0.f,0.f);
    source_object_function_set_tu15(world,objects[1],change_to_heretic,1.f,0.f);
    const std::array receipts{scene.complete_bitmap_frames(world,pass,owners[0],completed[0]),
        scene.complete_bitmap_frames(world,pass,owners[1],completed[1])};
    require(completed[0].material.texture_frames[0]==0 && completed[1].material.texture_frames[0]==1,
        "Two real native actor overrides shared one shadow image selection");
    for(unsigned n=0;n<2;++n) {
        require(receipts[n].matches(completed[n],pass,owners[n]),"Native shadow image receipt lost its actual owner");
        require(!receipts[n].matches(completed[1-n],pass,owners[1-n]),"Native shadow image receipt crossed model owners");
    }
    completed[1].material.texture_frames[0]=0;
    require(!receipts[1].matches(completed[1],pass,owners[1]),"Native shadow receipt admitted a changed frame");
    completed[1].material.texture_frames[0]=1;
    auto stale=owners[0];stale.generation=*stale.generation^0x10000u;
    rejects([&]{(void)scene.complete_bitmap_frames(world,pass,stale,completed[0]);});
    return 5;
}
}
int main(int argc,char** argv)try {
    using namespace halo2;
    if(argc!=3 || !*argv[1] || !std::filesystem::is_regular_file(argv[1]))return 77;
    const Cache cache(argv[1]);if(!cache.is_mcc())return 77;
    std::filesystem::create_directories(argv[2]);
    PhysicsLiveWorldTu15 world(cache,argv[2]);auto& memory=world.memory();
    const auto word=[&](std::uint32_t p){return Bytes(memory.view(p,4)).u32(0);};
    const auto table=word(0x4f0414);
    const auto& bsp=cache.bsps().at(world.source_bsp_index());
    const auto* bsp_tag=cache.find_tag({bsp.datum},tag_groups::scenario_structure_bsp);
    require(bsp_tag && bsp_tag->address && bsp_tag->size>=0x160,"Shadow BSP lacks its real salted tag root");
    const auto native_bsp=memory.source_address(world.resident_cache_address(cache,bsp_tag->address,bsp_tag->size),bsp_tag->size);
    const auto bsp_entry=table+(bsp.datum&65535u)*16u;
    require(word(0x4ecc18u)==native_bsp && word(bsp_entry)==tag_groups::scenario_structure_bsp.value &&
        word(bsp_entry+4u)==bsp.datum && word(bsp_entry+8u)==native_bsp,
        "Native current BSP differs from the actual Cache tag allocation/full salt");
    // The raw-resource header view can be a different Cache address from the
    // actual sbsp tag root. Neither is a canonical native pointer by itself.
    const auto metadata=cache.load_source_bsp_metadata(world.source_bsp_index());
    const Bytes bsp_root(metadata.bytes_at(bsp_tag->address,0x160));
    const Bytes resource_root(metadata.bytes_at(bsp.memory_address+16u,0x160));
    for(const auto offset:{0x9cu,0xa0u,0xa4u,0xa8u,0x138u,0x13cu,0x140u,0x144u})
        require(resource_root.u32(offset)==bsp_root.u32(offset),
            "Legacy raw BSP geometry view differs from its actual complete tag owner");
    std::optional<MenuWorldSourcePass> shadow_pass;
    for(unsigned n=0;n<bsp_root.u32(0xa4);++n) {
        const auto shader=Bytes(metadata.bytes_at(bsp_root.u32(0xa8)+n*32u+12u,4)).u32(0);
        if(shader==0xffffffffu)continue;
        try{shadow_pass=load_menu_world_source_pass(cache,shader,{0,10,0,0});break;}
        catch(const FormatError&){}
    }
    require(bool(shadow_pass),"Actual BSP supplies no original fixed shadow implementation");
    const auto pass_owner=source_shadow_raw_pass_owner_tu15(world,cache,*shadow_pass);
    const auto implementation=memory.source_address(world.resident_cache_address(cache,
        shadow_pass->implementation_address(),shadow_pass->implementation_record().size()),shadow_pass->implementation_record().size());
    require(pass_owner.matches(*shadow_pass,implementation),"Exact shadow pass capsule lost its canonical pointer");
    require(!pass_owner.matches(*shadow_pass,implementation^4u),"Shadow pass capsule accepted a different descriptor address");
    const auto preimage=memory.view(implementation,1).front();const std::array altered{preimage^std::byte{1}};
    memory.write(implementation,altered);
    rejects([&]{(void)source_shadow_raw_pass_owner_tu15(world,cache,*shadow_pass);});
    memory.write(implementation,std::span(&preimage,1));
    require(source_shadow_raw_pass_owner_tu15(world,cache,*shadow_pass).matches(*shadow_pass,implementation),
        "Restored actual shadow implementation no longer matches its full preimage");
    unsigned roots{},banks{};
    for(const auto& tag:cache.select_tags({.group=tag_groups::bitmap,.require_address=true})) {
        require(tag.size>=0x4c,"Bitmap root lacks its complete MCC descriptor");
        const auto native=world.resident_cache_address(cache,tag.address,tag.size);
        require(native.value>UINT32_MAX,"Resident cache lookup narrowed its actual host pointer");
        const auto logical=memory.source_address(native,tag.size);
        const auto entry=table+tag.handle().index()*16;
        require(word(entry)==tag_groups::bitmap.value && word(entry+4)==tag.datum &&
            word(entry+8)==logical && word(entry+12)==tag.size,"Resident bitmap mapping differs from its typed directory");
        const Bytes root(cache.tag_payload(tag.handle()));const auto count=root.u32(0x44),first=root.u32(0x48);
        if(count) {
            const auto extent=std::size_t(count)*0xa8;
            const auto bank=world.resident_cache_address(cache,first,extent);
            require(memory.source_address(bank,extent)==word(logical+0x48),"Resident bitmap bank differs from its exact source allocation");
            require(std::ranges::equal(memory.view(bank,extent),cache.read_tag_bytes(first,extent)),
                "Resident bitmap bytes differ from their source preimage");
            ++banks;
        }
        rejects([&]{(void)world.resident_cache_address(cache,tag.address,0);});
        rejects([&]{(void)world.resident_cache_address(cache,tag.address+tag.size-1,2);});
        ++roots;
    }
    require(roots && banks,"Owned map supplied no resident bitmap owners");
    const Cache foreign(argv[1]);const auto* first=cache.query_tag({.group=tag_groups::bitmap,.require_address=true});
    const auto foreign_pass=load_menu_world_source_pass(foreign,shadow_pass->shader(),shadow_pass->choice());
    require(!pass_owner.matches(foreign_pass,implementation),"Shadow pass capsule accepted identical bytes from a foreign Cache");
    rejects([&]{(void)source_shadow_raw_pass_owner_tu15(world,foreign,*shadow_pass);});
    rejects([&]{(void)source_shadow_raw_pass_owner_tu15(world,foreign,foreign_pass);});
    require(first,"Bitmap directory unexpectedly disappeared");
    rejects([&]{(void)world.resident_cache_address(foreign,first->address,first->size);});
    rejects([&]{(void)world.resident_cache_address(cache,0xffffffffu,2);});
    bool rejected_thread{};
    std::thread other([&]{try {(void)world.resident_cache_address(cache,first->address,first->size);}
        catch(const FormatError&){rejected_thread=true;}});other.join();
    require(rejected_thread,"Resident cache lookup accepted a foreign executor");
    // Real Lockout banks with identical24-byte headers must retain distinct
    // immutable mappings even if writable runtime descriptors are redirected.
    const Tag *headlight{},*generic{};
    for(const auto& tag:cache.tags()) {
        if(tag.group!="bitm")continue;
        if(tag.name=="effects\\bitmaps\\lens_flares\\headlight_lens_flare")headlight=&tag;
        if(tag.name=="effects\\bitmaps\\lens_flares\\flares_generic")generic=&tag;
    }
    bool matching_prefix_control{};
    if(headlight && generic) {
        const Bytes a(cache.tag_payload(headlight->handle())),b(cache.tag_payload(generic->handle()));
        if(a.u32(0x44) && b.u32(0x44) && a.u32(0x48)!=b.u32(0x48) &&
           std::ranges::equal(cache.read_tag_bytes(a.u32(0x48),24),cache.read_tag_bytes(b.u32(0x48),24))) {
            const auto extent=std::size_t(a.u32(0x44))*0xa8;
            const auto correct=memory.source_address(world.resident_cache_address(cache,a.u32(0x48),extent),extent);
            const auto wrong=memory.source_address(world.resident_cache_address(cache,b.u32(0x48),0xa8),0xa8);
            const auto root=memory.source_address(world.resident_cache_address(cache,headlight->address,headlight->size),headlight->size);
            std::array<std::byte,4> redirect{};for(unsigned n=0;n<4;++n)redirect[n]=std::byte(wrong>>(8*n));
            memory.write(root+0x48,redirect);
            require(word(root+0x48)==wrong && correct!=wrong &&
                memory.source_address(world.resident_cache_address(cache,a.u32(0x48),extent),extent)==correct,
                "Matching-prefix redirect changed the retained admitted relocation");
            for(unsigned n=0;n<4;++n)redirect[n]=std::byte(correct>>(8*n));memory.write(root+0x48,redirect);
            matching_prefix_control=true;
        }
    }
    std::cout<<"PASS resident Cache ownership: roots="<<roots<<" banks="<<banks
        <<" matching_prefix_control="<<matching_prefix_control<<" shadow_canonical_pass_controls=5"
        <<" shadow_bitmap_native_controls="<<native_shadow_bitmap_frames(cache,world)<<'\n';return 0;
}catch(const std::exception& error){std::cerr<<error.what()<<'\n';return 1;}
