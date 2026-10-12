#include "halo2/reference_player.hpp"
#include "halo2/content.hpp"
#include "halo2/model.hpp"
#include "halo2/shader_animation.hpp"
#include "halo2/vertex_program.hpp"
#include <bit>
#include <chrono>
#include <fstream>
#include <iomanip>
#include <iostream>
#include <sstream>
#include <set>

namespace {
using namespace halo2;
void require(bool value,const char *message) {if(!value)throw FormatError(message);}
auto bits(Vec3 value) {return std::bit_cast<std::array<std::uint32_t,3>>(value);}
std::array<unsigned,256> constant_components(const Material &material) {
    std::array<unsigned,256> result{};const auto code=decode_vertex_program(material.vertex_program);
    for(unsigned n=0;n<code.count;++n) {
        const auto &t=code.instructions[n];const unsigned mac=(t[1]>>21)&15,ilu=(t[1]>>25)&7;
        const unsigned reg=(t[3]>>20)&15,mm=(t[3]>>24)&15,im=(t[3]>>16)&15;
        unsigned om=(t[3]>>12)&15;const unsigned output=(t[3]>>3)&15;
        // Constant operands feeding a disabled texture stage are inherited
        // state, rather than active fragment inputs (armsbase c20.xy).
        if(output>=9 && output<13 && ((material.forward_program.words[54]>>(5*(output-9)))&31)==0)om=0;
        const unsigned mw=(mac && !(ilu && reg==1)?mm:0)|((t[3]&4)?0:om);
        const unsigned iw=(ilu?im:0)|((t[3]&4)?om:0);
        std::array<unsigned,3> components{};
        const auto axes=[](unsigned mask){unsigned a=0;for(unsigned i=0;i<4;++i)if(mask&(8>>i))a|=1<<i;return a;};
        if(mac && mw) {
            if(mac>=5 && mac<=7){components[0]=mac==7?15:7;components[1]=mac==5?7:15;}
            else if(mac==8){if(mw&4){components[0]|=2;components[1]|=2;}if(mw&2)components[0]|=4;if(mw&1)components[1]|=8;}
            else {components[0]=axes(mw);if(mac==2 || mac>=4)components[1]=axes(mw);if(mac==3 || mac==4)components[2]=axes(mw);}
        }
        if(ilu && iw)components[2]|=ilu==1?axes(iw):ilu==7?11:1;
        for(unsigned which=0;which<3;++which) {
            const unsigned mux=which==0?(t[2]>>26)&3:which==1?(t[2]>>11)&3:(t[3]>>28)&3;
            if(mux!=3)continue;
            const unsigned swizzle=which==0?t[1]:which==1?t[2]>>17:t[2]>>2;
            for(unsigned axis=0;axis<4;++axis)if(components[which]&(1<<axis))
                result[(t[1]>>13)&255]|=1<<((swizzle>>(6-2*axis))&3);
        }
    }
    return result;
}
const auto fixture=std::filesystem::path(__FILE__).parent_path()/"fixtures/retail_ivory_player.reference";
void retained() {
    const auto s=load_reference_player_state(fixture);
    require(s.map=="cyclotron" && s.pose_frame==32 && s.unit_datum==0xe2c60057 && s.weapon_definition==0xe68b0516,"original reference identity changed");
    require(bits(s.camera_position)==std::array<std::uint32_t,3>{0xc0a3696d,0xc0b02c10,0x3f2147aa},"original camera words changed");
    require(bits(s.root_position)==std::array<std::uint32_t,3>{0xc0a3696d,0xc0b02c10,0x3c23d620},"original physics root words changed");
    require(s.packed_colors==std::array<std::uint32_t,4>{0xfb9bc9,0x416c8f,0xbe2c2c,0xfb9bc9},"original context color words changed");
    require(bits(s.colors[0])==std::array<std::uint32_t,3>{0x3f7bfbfd,0x3f1b9b9c,0x3f49c9cb},"source131BB0 color float words differ");
    require(std::bit_cast<std::uint32_t>(s.colors[0].x)!=s.object_color_words[0],"runtime float bank incorrectly used as shader color");
    require(s.lighting_words[20]==0xffff,"84-byte lighting flags lost");
    const auto raw=s.root_lighting.source_values();
    for(unsigned i=0;i<20;++i)require(std::bit_cast<std::uint32_t>(raw[i])==s.lighting_words[i],"retained lighting float words changed");
    require(bits(s.first_person_lighting.primary_direction)==std::array<std::uint32_t,3>{0xbecfe36d,0xbe1df6a8,0xbf669739},"original GPU c26 words changed");
    require(bits(s.first_person_lighting.secondary_direction)==std::array<std::uint32_t,3>{0x3ec836ae,0x3e419989,0x3f669739},"original GPU c27 words changed");
    require(bits(s.first_person_lighting.primary_direction)==bits(s.root_lighting.primary_direction),"lighting draw does not corroborate retained state");
    require(pack_object_lighting_rgb(s.first_person_lighting.primary_color)==0x191b1d &&
            pack_object_lighting_rgb(s.first_person_lighting.secondary_color)==0x191b1e &&
            pack_object_lighting_rgb(s.first_person_lighting.ambient)==0x0f1112,"FP GPU lighting RGB bytes differ");
    require(s.aiming_word==0x03010707 && source_aim_blend_weight(s.aiming)==1,"retained aiming counter differs from original");
    require(s.retained_context_specular_brightness==0 && s.retained_context_ambient_brightness==0,"retained final context brightness changed");
    require(s.owner_sphere && bits(s.owner_sphere->center)==std::array<std::uint32_t,3>{0xc0a3ae17,0xc0b05c37,0x3ebb4240} &&
            std::bit_cast<std::uint32_t>(s.owner_sphere->radius)==0x3ee147ae,"original owner bounds words changed");
    require(bits(s.owner_sphere->center)!=bits(s.root_position),"physics position substituted for cached owner bounds");
    require(s.devices.empty(),"Ivory fixture unexpectedly restored a device");
}
void owner_spheres() {
    std::ifstream input(fixture.parent_path()/"retail_player_owner_spheres.tsv");
    require(bool(input),"original BDEF0 owner sphere fixture missing");
    std::string line;unsigned count=0;
    while(std::getline(input,line)) {
        if(line.empty() || line[0]=='#')continue;
        std::istringstream row(line);std::string map;std::uint32_t handle,definition;
        std::array<std::uint32_t,18> source{};std::array<std::uint32_t,4> expected{};
        row>>map>>std::hex>>handle>>definition;
        for(auto&v:source)row>>v;for(auto&v:expected)row>>v;
        require(bool(row),"original BDEF0 owner sphere fixture truncated");
        std::string trailing;require(!(row>>trailing),"original BDEF0 owner sphere fixture has trailing data");
        const auto real=[](auto v){return std::bit_cast<float>(v);};
        ShadowCasterDefinition bounds;bounds.bounding_radius=real(source[0]);
        bounds.bounding_offset={real(source[1]),real(source[2]),real(source[3])};
        Transform node;node.scale=real(source[5]);
        for(unsigned i=0;i<3;++i)node.basis[i]={real(source[6+i*3]),real(source[7+i*3]),real(source[8+i*3])};
        node.position={real(source[15]),real(source[16]),real(source[17])};
        const auto sphere=source_shadow_sphere(bounds,node,real(source[4]));
        const auto result=bits(sphere.center);
        for(unsigned i=0;i<3;++i)require(result[i]==expected[i],"native BDEF0 differs from original retained player sphere");
        require(std::bit_cast<std::uint32_t>(sphere.radius)==expected[3],"native BDEF0 radius differs from original retained player sphere");
        const auto player=load_reference_player_state(fixture.parent_path()/(map=="cyclotron"?"retail_ivory_player.reference":"retail_zanzibar_player.reference"));
        require(player.map==map && player.unit_datum==handle && player.unit_definition==definition && player.owner_sphere,
                "retained owner sphere identity differs from player fixture");
        require(bits(player.owner_sphere->center)==result && std::bit_cast<std::uint32_t>(player.owner_sphere->radius)==expected[3],
                "reference player owner sphere differs from independently frozen BDEF0 fixture");
        ++count;
    }
    require(count==2,"original owner sphere coverage incomplete");
}
void malformed() {
    std::ifstream in(fixture);const std::string good((std::istreambuf_iterator<char>(in)),{});
    require(bool(in),"original fixture unavailable");
    const auto path=std::filesystem::temp_directory_path()/std::filesystem::path("halo2-reference-player-"+std::to_string(std::chrono::steady_clock::now().time_since_epoch().count())+".txt");
    struct Remove {std::filesystem::path path;~Remove(){std::error_code error;std::filesystem::remove(path,error);}} cleanup{path};
    const auto reject=[&](const std::string &text) {
        {std::ofstream out(path);out<<text;}
        bool rejected=false;try{load_reference_player_state(path);}catch(const FormatError&){rejected=true;}
        require(rejected,"malformed reference fixture admitted");
    };
    const auto replace=[&](const std::string &old,const std::string &fresh) {
        auto text=good;const auto at=text.find(old);require(at!=std::string::npos,"malformed fixture replacement missing");text.replace(at,old.size(),fresh);reject(text);
    };
    reject(good.substr(0,good.find("lighting")));
    reject(good+"unknown 0\n");reject(std::string(16385,'x'));
    replace("H2REFERENCE_PLAYER 3","H2REFERENCE_PLAYER 4");
    replace("owner_sphere C0A3AE17","owner_sphere 7FC00000");
    replace("3EE147AE","BEE147AE");
    replace("3EE147AE","00000000");
    replace("3EE147AE","7F800000");
    {auto missing=good;const auto at=missing.find("owner_sphere");missing.erase(at,missing.find('\n',at)-at+1);reject(missing);}
    replace("map cyclotron","map ../cyclotron");
    replace("E2C60057","FFFFFFFF");
    replace("C0A3696D","7FC00000");
    replace("00FB9BC9","00FB9BCA");
    replace("03010707","03010708");
    replace("draw_light_directions BECFE36D","draw_light_directions BECFE36E");
    std::ifstream device_input(fixture.parent_path()/"retail_zanzibar_player.reference");
    const std::string device_good((std::istreambuf_iterator<char>(device_input)),{});require(bool(device_input),"Zanzibar device fixture missing");
    const auto change_device=[&](const std::string&old,const std::string&fresh) {
        auto text=device_good;const auto at=text.find(old);require(at!=std::string::npos,"device rejection replacement missing");
        text.replace(at,old.size(),fresh);reject(text);
    };
    reject(device_good.substr(0,device_good.find("devices")));
    change_device("devices 2","devices 65");
    change_device("EC0A0A95 7 5","EC0A0A95 7 0");
    change_device("E2840015","E27F0010");
    change_device("EBE40A6F 7 0","EBE40A6F 6 0");
    change_device("3F5CB371","7FC00000");
    change_device("3F5CB371","3F800000");
    const auto legacy=[&](std::string text,const char*version,bool remove_devices) {
        text.replace(text.find("H2REFERENCE_PLAYER 3"),20,std::string("H2REFERENCE_PLAYER ")+version);
        const auto owner=text.find("owner_sphere");text.erase(owner,text.find('\n',owner)-owner+1);
        if(remove_devices){const auto devices=text.find("devices");text.erase(devices);}
        {std::ofstream out(path);out<<text;}
        const auto state=load_reference_player_state(path);
        require(!state.owner_sphere,"legacy reference inferred an unrecorded owner sphere");
        require(state.devices.size()==(remove_devices?0:2),"legacy reference devices changed");
    };
    legacy(good,"1",true);legacy(device_good,"2",false);
}
void shader_bindings(const std::filesystem::path &map,const std::filesystem::path &source_fixture=fixture,
                     const std::filesystem::path &source_bindings=std::filesystem::path(__FILE__).parent_path()/"fixtures/retail_fp_bindings.tsv") {
    const auto s=load_reference_player_state(source_fixture);Cache cache(map);const auto content=cache.load_content(0,true);
    const auto primary=pack_object_lighting_rgb(s.first_person_lighting.primary_color);
    const auto secondary=pack_object_lighting_rgb(s.first_person_lighting.secondary_color);
    const auto ambient=pack_object_lighting_rgb(s.first_person_lighting.ambient);
    const std::array ids{content.first_person_hands,content.weapons.at(s.weapon_definition).first_person_model};
    unsigned lighting_passes=0,color_passes=0,environment_passes=0;
    unsigned compared_rows=0;
    std::set<std::string> compared_families;
    for(auto id:ids) {
        auto model=cache.load_model(id,true);
        for(auto &material:model.mesh.materials) {
            if(!prepare_dynamic_material(cache,material,s.first_person_lighting,true))continue;
            const auto animate=[&](auto &self,Material &m)->void {
                animate_forward_material(m,double(s.pose_frame)/30.0,{});
                animate_material_bitmaps(m,double(s.pose_frame)/30.0,{});
                if(m.environment_pass)self(self,*m.environment_pass);
                for(auto &pass:m.world_overlays_before)self(self,*pass);
                for(auto &pass:m.world_overlays_after)self(self,*pass);
            };
            animate(animate,material);
            apply_object_colors(material,id==content.first_person_hands?s.colors:cache.load_object_colors(s.weapon_definition));
            apply_object_lighting(material,s.first_person_lighting);
            const auto check=[&](const Material &m) {
                const auto reads=constant_components(m);
                // Renderer applies source camera-color extern0 perview, after
                // retaining the material's authored program and object bank.
                auto pixel=m.forward_program;
                std::array<ShaderVector,256> colors{};
                colors[0]={s.camera_forward.x,s.camera_forward.y,s.camera_forward.z,0};
                std::vector<ShaderRemap> camera_bindings;
                const auto &active=m.passes.at(m.forward_pass_index);
                for(const auto &r:active.pixel_externs)if(r.source==0 && r.component==4)camera_bindings.push_back(r);
                remap_pixel_externs(pixel,{},colors,camera_bindings,active.fragments,m.shader_switches);
                std::ifstream gold(source_bindings);
                require(bool(gold),"original active FP binding fixture missing");
                std::string line;
                while(std::getline(gold,line)) {
                    if(line.empty() || line[0]=='#')continue;
                    std::istringstream row(line);std::string material_leaf,pass_leaf;char kind;unsigned reg;
                    row>>material_leaf>>pass_leaf>>kind>>reg;
                    if(!m.name.ends_with("\\"+material_leaf) || !m.passes.at(m.forward_pass_index).name.ends_with("\\"+pass_leaf))continue;
                    ++compared_rows;compared_families.insert(material_leaf+":"+pass_leaf);
                    row>>std::hex;
                    if(kind=='V') {
                        for(unsigned i=0;i<4;++i) {
                            std::uint32_t word;row>>word;
                            const auto native=std::bit_cast<std::uint32_t>(m.program_constants[1][reg-18][i]);
                            if(!(reads[reg]&(1<<i)))continue;
                            if(native!=word && !((native&0x7fffffff)==0 && (word&0x7fffffff)==0)) {
                                std::cerr<<m.name<<" c"<<std::dec<<reg<<'.'<<i<<" original "<<std::hex<<word<<" native "<<native<<std::dec<<'\n';
                                throw FormatError("production active FP vertex constant differs from captured GPU");
                            }
                        }
                    } else {
                        std::uint32_t word;row>>word;
                        const auto index=reg<16?10+(reg&1)*8+reg/2:43+reg-16;
                        if(pixel.words[index]!=word) {
                            std::cerr<<m.name<<" "<<pass_leaf<<" P"<<std::dec<<reg<<" original "<<std::hex<<word<<" native "<<pixel.words[index]<<std::dec<<'\n';
                            throw FormatError("production active FP pixel constant differs from captured GPU");
                        }
                    }
                    require(bool(row),"original active FP binding fixture truncated");
                }
                if(m.forward_vertex==6) {
                    // Shader constant bank starts at c18, so c26/c27 are8/9.
                    for(unsigned i=0;i<3;++i) {
                        require(std::bit_cast<std::uint32_t>(m.program_constants[1][8][i])==s.draw_light_direction_words[i],"production primary direction differs from original FP GPU");
                        require(std::bit_cast<std::uint32_t>(m.program_constants[1][9][i])==s.draw_light_direction_words[3+i],"production secondary direction differs from original FP GPU");
                    }
                    // GPU Pshconst4/5 are stage2 C0/C1, original definition
                    // words12/20. RGB is identical for all four lit passes.
                    require((m.forward_program.words[12]&0xffffff)==primary &&
                            (m.forward_program.words[20]&0xffffff)==secondary,"production FP light colors differ from captured GPU");
                    if(m.passes.at(m.forward_pass_index).name.ends_with("lightmap_tex_detail") ||
                       m.passes.at(m.forward_pass_index).name.ends_with("old_lightmap_tangentspace"))
                        require(m.forward_program.words[15]==(0xc0000000|ambient),"production FP ambient constant differs from captured GPU");
                    ++lighting_passes;
                } else if(m.passes.at(m.forward_pass_index).name.ends_with("\\default_texture_two_change_color_zwrite")) {
                    require(m.forward_program.words[11]==s.packed_colors[0] &&
                            m.forward_program.words[12]==s.packed_colors[1],"production hand profile bank differs from original FP GPU");
                    ++color_passes;
                } else if(m.passes.at(m.forward_pass_index).name.ends_with("\\default_environment_map_tangent_space")) {
                    ++environment_passes;
                }
            };
            check(material);if(material.environment_pass)check(*material.environment_pass);
            for(const auto &pass:material.world_overlays_before)check(*pass);
            for(const auto &pass:material.world_overlays_after)check(*pass);
            if(material.name.ends_with("\\smg_painted_metal")) {
                const auto &lit=material.environment_pass?*material.environment_pass:material;
                const auto &pass=lit.passes.at(lit.forward_pass_index);
                require(pass.program.fragment_offsets[1]==5 && pass.program.fragment_offsets[2]==6,
                        "retained paint fragment layout changed");
                auto rebound=pass.program,broken=pass.program;
                broken.fragment_offsets={};
                auto correct_constants=lit.program_constants,broken_constants=correct_constants;
                apply_object_lighting_bindings(rebound,correct_constants,pass,s.first_person_lighting,
                    lit.shader_switches,lit.specular_brightness,lit.ambient_brightness);
                apply_object_lighting_bindings(broken,broken_constants,pass,s.first_person_lighting,
                    lit.shader_switches,lit.specular_brightness,lit.ambient_brightness);
                // Original848 keeps ambient in stage5 C0 and scalar alpha in
                // stage6 C1. Reconstructing from words alone loses the layout
                // and exactly reproduces the faulty native1808 GPU bank.
                require(rebound.words[10]==0xc0000000 && rebound.words[15]==(0xc0000000|ambient) &&
                        rebound.words[24]!=0,"source per-view paint rebinding changed");
                require(broken.words[10]==rebound.words[15] && broken.words[18]==rebound.words[24] &&
                        broken.words[15]==0xc0000000 && broken.words[24]==0,
                        "zero-offset runtime failure no longer reproduced");
            }
        }
    }
    require(lighting_passes==4 && color_passes==1 && environment_passes==1,"reference FP shader coverage incomplete");
    require(compared_rows==70 && compared_families.size()==7,"original active FP binding coverage incomplete");
}
void probe(const std::filesystem::path &path) {
    const auto s=load_reference_player_state(path);std::cout<<std::hex;
    for(const auto c:s.colors)for(auto word:bits(c))std::cout<<word<<' ';
    for(auto c:s.colors)std::cout<<pack_object_lighting_rgb(c)<<' ';
    for(auto word:s.draw_light_direction_words)std::cout<<word<<' ';
    std::cout<<std::bit_cast<std::uint32_t>(source_aim_blend_weight(s.aiming))<<'\n';
}
}
void captured_clear() {
    const auto path=fixture.parent_path()/"retail_zanzibar_view_clear.reference";
    const auto state=halo2::load_reference_player_state(path);
    require(state.view_clear.has_value(),"captured original initialization absent");
    const auto &clear=*state.view_clear;
    require(clear.transition_byte==0 && clear.x87_control_word==0x023f,"actual source clear context changed");
    const auto events=halo2::source_view_initialization_plan(clear.header,clear.transition_byte,
        clear.x87_control_word,{0,0,640,480,0,0x3f800000});
    require(events.size()==2 && events[1].words==std::array<std::uint32_t,6>{0xf3,0x00475b63,0x3f800000,0,0,0},
        "captured original D3D clear operands changed");
    std::ifstream input(path);const std::string good((std::istreambuf_iterator<char>(input)),{});
    const auto temp=std::filesystem::temp_directory_path()/std::filesystem::path("halo2-clear-admission-"+std::to_string(std::chrono::steady_clock::now().time_since_epoch().count())+".txt");
    struct Remove {std::filesystem::path path;~Remove(){std::error_code error;std::filesystem::remove(path,error);}} cleanup{temp};
    for(const auto &bad:std::array<std::pair<std::string,std::string>,4>{{
        {"view_clear_transition 00000000","view_clear_transition 00000001"},
        {"view_clear_x87 0000023F","view_clear_x87 0000023E"},
        {"view_clear_header 00000000ffff00000200","view_clear_header 00000100ffff00000200"},
        {"view_clear_header 00000000ffff00000200","view_clear_header 00000000ffff00000000"}}}) {
        auto text=good;const auto at=text.find(bad.first);require(at!=std::string::npos,"clear rejection replacement absent");
        text.replace(at,bad.first.size(),bad.second);{std::ofstream output(temp);output<<text;}
        bool rejected=false;try{halo2::load_reference_player_state(temp);}catch(const halo2::FormatError&){rejected=true;}
        require(rejected,"captured clear admitted unsupported resource/history state");
    }
}

int main(int argc,char **argv) {
    try {
        if(argc==3 && std::string(argv[1])=="--probe"){probe(argv[2]);return 0;}
        retained();owner_spheres();malformed();captured_clear();if(argc==2) {
            shader_bindings(argv[1]);
            const auto fixtures=std::filesystem::path(__FILE__).parent_path()/"fixtures";
            const auto zanzibar=load_reference_player_state(fixtures/"retail_zanzibar_player.reference");
            require(zanzibar.map=="zanzibar" && zanzibar.pose_frame==56 &&
                    zanzibar.weapon_definition==0xe514039f && zanzibar.unit_definition==0xedf10c6b,
                    "Zanzibar source player fixture identity changed");
            require(zanzibar.packed_colors==std::array<std::uint32_t,4>{0xfb9bc9,0x416c8f,0xbe2c2c,0xfb9bc9} &&
                    zanzibar.aiming_word==0x03010707,"Zanzibar retained rendering inputs changed");
            shader_bindings(std::filesystem::path(argv[1]).parent_path()/"zanzibar.map",
                            fixtures/"retail_zanzibar_player.reference",fixtures/"retail_zanzibar_fp_bindings.tsv");
        }
        std::cout<<"Original player reference words and rejection fixtures passed\n";return 0;
    }catch(const std::exception &error){std::cerr<<error.what()<<'\n';return 1;}
}
