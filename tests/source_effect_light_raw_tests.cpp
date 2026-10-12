#include "halo2/menu_world_source_binding.hpp"
#include "halo2/menu_world_source_pass.hpp"
#include "halo2/menu_world_vertex_catalog.hpp"
#include "halo2/effect_light_draw.hpp"
#include "halo2/effect_light.hpp"
#include "halo2/world_material.hpp"
#include <algorithm>
#include <bit>
#include <cstring>
#include <fstream>
#include <iostream>
#include <map>
#include <sstream>
using namespace halo2;
namespace {
void require(bool ok,const char* why){if(!ok)throw FormatError(why);}
std::vector<std::byte> read(const std::filesystem::path& path) {
    std::ifstream f(path,std::ios::binary|std::ios::ate);require(bool(f),"original sphere raw fixture missing");const auto n=f.tellg();require(n>=0 && n<100000,"original sphere raw fixture extent");std::vector<std::byte> b(static_cast<std::size_t>(n));f.seekg(0);if(n)require(bool(f.read(reinterpret_cast<char*>(b.data()),n)),"original sphere raw fixture truncated");return b;
}
}
int main(int argc,char** argv)try {
    require(argc==3,"sphere raw test needs Lockout map and original binding directory");const Cache cache(argv[1]);const std::filesystem::path dir=argv[2];std::ifstream manifest(dir/"manifest.tsv");std::string line;require(bool(std::getline(manifest,line)) && line=="H2_SPHERE_RAW_BINDINGS_V1","sphere raw manifest schema");
    using Key=std::pair<unsigned,unsigned>;std::map<Key,std::shared_ptr<const MenuWorldSourceSection>> sections;unsigned draws=0,clusters=0,instances=0,program_bytes=0,retained=0,streams=0,raw_packets=0,raw_controls=0;std::vector<unsigned> order;
    const auto light_tag=std::ranges::find_if(cache.tags(),[](const auto& t){return t.group=="ligh"&&t.name=="effects\\objects\\weapons\\grenade\\frag_grenade\\grenade_frag_explosion";});require(light_tag!=cache.tags().end(),"sphere raw material light missing");const auto light=load_effect_light(cache,light_tag->datum);
    EffectLightUniformInput light_input;light_input.position={-2,-1,3};light_input.forward={1,0,0};light_input.up={0,0,1};light_input.left={0,-1,0};light_input.radius=2;light_input.diffuse={1,.5f,.25f};light_input.brightness=1;const auto light_uniforms=source_effect_light_uniforms(light_input);
    while(std::getline(manifest,line)) {
        unsigned command,kind,section,instance,part,shader,lod,implementation,vertex,classification;std::string key,extra;std::istringstream row(line);require(bool(row>>command>>kind>>section>>instance>>part>>shader>>lod>>implementation>>vertex>>classification>>key) && !(row>>extra),"sphere raw manifest row");require(kind<2 && classification==kind,"sphere raw actual class/owner corridor");order.push_back(command);
        const auto identity=Key{kind,section};if(!sections.contains(identity))sections.emplace(identity,std::make_shared<const MenuWorldSourceSection>(load_menu_world_source_bsp_section(cache,0,kind?MenuWorldSourceSectionKind::bsp_instance_definition:MenuWorldSourceSectionKind::bsp_cluster,section)));
        const auto& geometry=sections.at(identity);require(part<geometry->parts().size() && geometry->parts()[part].shader==shader,"sphere raw actual part shader differs from Cache");
        const auto pass=load_menu_world_source_pass(cache,shader,{lod,12,0,1});require(pass.vertex_shader()==vertex && pass.implementation_address()==implementation,"sphere raw selected source pass differs");
        MenuWorldSourceConstants bank;for(unsigned i=0;i<192;++i)for(unsigned j=0;j<4;++j)bank[i][j]=std::bit_cast<float>(0x3f000000u+i*4+j);bank[50][0]=std::bit_cast<float>(0x80000000u);const auto before=bank;
        MenuWorldSourceOwner owner;MenuWorldSourceDraw draw;
        if(kind) {
            const auto source=load_menu_world_source_bsp_owner(cache,0,instance);require(source.section==section,"sphere raw instance definition differs");const auto original=read(dir/("command-"+std::to_string(command)+"-palette.bin"));require(original.size()==sizeof(source.rows) && std::memcmp(original.data(),source.rows.data(),sizeof(source.rows))==0,"sphere raw instance c50 rows differ from RAM");
            owner={MenuWorldSourceOwnerKind::bsp_instance,source.bsp_datum,instance,{}};draw=menu_world_source_draw(geometry,part,MenuWorldSourceTopology::triangle_list,std::span(&source.rows,1));++instances;
        } else {owner={MenuWorldSourceOwnerKind::bsp_cluster,geometry->owner_datum(),section,{}};draw=menu_world_source_cluster_draw(geometry,part,MenuWorldSourceTopology::triangle_list);++clusters;}
        const auto binding=menu_world_source_binding_class(cache,owner,draw,vertex,classification,bank);const auto code=read(dir/(key+".tokens.bin")),declaration=read(dir/(key+".declaration.bin"));
        require(std::ranges::equal(binding.program_bytes(),code),"sphere raw Cache VRTX differs from RAM");require(binding.declaration().class_index==classification && declaration.size()==binding.declaration().semantics.size()*2 && std::memcmp(declaration.data(),binding.declaration().semantics.data(),declaration.size())==0,"sphere raw Cache declaration differs from RAM");
        require(binding.external_attribute_registers().empty(),"sphere raw selected unbound external attribute");(void)menu_world_vertex_shader_source(binding.program_bytes(),binding.declaration());
        for(unsigned i=0;i<192;++i) {
            if(i>=170 && i<173)require(std::memcmp(binding.constants()[i].data(),geometry->compression_rows().data()+(i-170)*4,16)==0,"sphere raw compression constants differ");
            else if(kind && i>=50 && i<53)require(std::memcmp(binding.constants()[i].data(),draw.object_rows[0].data()+(i-50)*4,16)==0,"sphere raw c50 upload differs");
            else {require(std::memcmp(binding.constants()[i].data(),before[i].data(),16)==0,"sphere raw changed retained view/owner constant");retained+=4;}
        }
        require(std::memcmp(bank.data(),before.data(),sizeof(bank))==0,"sphere raw mutated caller bank");program_bytes+=code.size();streams+=binding.inputs().size();++draws;
        bool rejected=false;try{(void)menu_world_source_binding_class(cache,owner,draw,vertex,1-classification,bank);}catch(const FormatError&){rejected=true;}require(rejected,"sphere raw accepted a mismatched palette/class owner");
        auto authored=cache.load_material(shader);authored.selected_shader_lod=lod;
        const auto materials=prepare_effect_light_diffuse_material(cache,authored,light,light_uniforms);require(materials.size()==1,"sphere raw completed material cardinality");const auto& material=materials[0];
        const auto completed=source_effect_light_raw_constants(bank,material);
        for(unsigned i=0;i<192;++i)if(i<18 || i>=40)require(std::memcmp(completed[i].data(),bank[i].data(),16)==0,"sphere raw material upload changed unrelated owner/view constants");
        const auto submitted=menu_world_source_binding_class(cache,owner,draw,vertex,classification,completed);
        MenuWorldRasterContext raster;raster.viewport_reset({640,480});raster.game_initialize_offset();raster.layer_begin(MenuWorldRasterLayer::sphere_diffuse_12);raster.entry_begin({false,false,false});const auto raster_packet=raster.snapshot({MenuWorldRasterPrimitive::ordinary_triangles,MenuWorldRasterPolygon::fill,std::array<std::int32_t,2>{1,1},true});raster.entry_end();
        const auto packet=source_effect_light_raw_draw(7,material,pass,submitted,raster_packet,{0,16777215,.0000152587890625f,1},{1,1});
        require(packet.material_slot==7 && packet.program.words==material.forward_program.words && packet.source.owner()==owner && packet.source.draw().indices().size()==draw.indices().size(),"sphere raw completed draw lost source identity");++raw_packets;
        auto wrong=completed;wrong[31][0]+=1.f;const auto unbound=menu_world_source_binding_class(cache,owner,draw,vertex,classification,wrong);
        rejected=false;try{(void)source_effect_light_raw_draw(7,material,pass,unbound,raster_packet,{0,16777215,.0000152587890625f,1},{1,1});}catch(const FormatError&){rejected=true;}require(rejected,"sphere raw accepted stale c31 light constants");++raw_controls;
    }
    require(order==std::vector<unsigned>({12,11,10,5,6,7,2,9,3,4,8,0,1}) && draws==13 && clusters==10 && instances==3,"sphere raw original sorted command scope changed");
    std::cout<<"PASS "<<draws<<" original sorted raw bindings / "<<clusters<<" class0 clusters / "<<instances<<" class1 instances / "<<program_bytes<<" literal program bytes / "<<retained<<" retained constant words / "<<streams<<" raw stream bindings / "<<raw_packets<<" completed light draw packets / "<<raw_controls<<" stale-bank controls; residency and GPU submission separate\n";
}catch(const std::exception& e){std::cerr<<e.what()<<'\n';return 1;}
