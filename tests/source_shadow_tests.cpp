#include "halo2/source_shadow.hpp"
#include "halo2/model.hpp"
#include "halo2/transform.hpp"
#include "halo2/content.hpp"
#include "halo2/scenery.hpp"
#include <cmath>
#include <iomanip>
#include <iostream>

using namespace halo2;
namespace {void require(bool v,const char*m){if(!v)throw std::runtime_error(m);}}
int main(int argc,char**argv){try{
    std::cout<<std::setprecision(9);
    if(argc==3 && std::string_view(argv[1])=="--object-idle-probe"){
        const Cache cache(argv[2]);std::uint32_t datum;
        while(std::cin>>datum){const auto idle=cache.load_initialized_object_idle(datum);
            std::cout<<(idle?idle->name:"NONE")<<'\n';}
        return 0;
    }
    if(argc==2 && (std::string_view(argv[1])=="--frame-probe" || std::string_view(argv[1])=="--uniform-probe")){
        const bool uniform=std::string_view(argv[1])=="--uniform-probe";
        ShadowCasterSphere root;Vec3 direction;float opacity,fade;unsigned count;
        while(std::cin>>root.center.x>>root.center.y>>root.center.z>>root.radius
          >>direction.x>>direction.y>>direction.z>>opacity>>fade>>count){
            std::vector<ShadowCasterSphere> hierarchy(count);
            for(auto&s:hierarchy)std::cin>>s.center.x>>s.center.y>>s.center.z>>s.radius;
            const auto frame=source_shadow_frame(root,hierarchy,direction,opacity,fade);
            if(frame){
                if(uniform){bool feature;std::cin>>feature;const auto state=source_shadow_uniforms(*frame,feature);
                    for(const auto&v:state.projection)for(auto x:v)std::cout<<x<<' ';
                    for(const auto&v:state.constants)for(auto x:v)std::cout<<x<<' ';
                }else for(auto x:frame->source_values())std::cout<<x<<' ';
                std::cout<<'\n';}
            else std::cout<<"none\n";
        }return 0;
    }
    if(argc==2 && std::string_view(argv[1])=="--fade-probe"){
        ShadowCasterDefinition d;float metric,radius;bool force;
        while(std::cin>>d.model_present>>d.render_lod_count){
            for(auto&v:d.render_cutoffs)std::cin>>v;
            std::cin>>metric>>radius>>force;
            std::cout<<source_shadow_distance_fade(d,metric,radius,force)<<'\n';
        }
        return 0;
    }
    if(argc==2 && std::string_view(argv[1])=="--root-fade-probe"){
        unsigned mode,type,kind,byte;bool no_shadow,dying;float elapsed,camo;
        while(std::cin>>mode>>type>>kind>>byte>>no_shadow>>dying>>elapsed>>camo){
            const auto state=source_shadow_root_fade({std::uint16_t(mode),std::uint8_t(type),std::uint8_t(kind),
                std::uint8_t(byte),no_shadow,dying,elapsed,camo});
            std::cout<<state.eligible<<' '<<state.fade<<'\n';
        }return 0;
    }
    if(argc==2 && std::string_view(argv[1])=="--sphere-probe"){
        ShadowCasterDefinition definition;Transform node0;float object_scale;
        while(std::cin>>definition.bounding_radius>>definition.bounding_offset.x>>definition.bounding_offset.y
            >>definition.bounding_offset.z>>object_scale>>node0.scale){
            for(auto&v:node0.basis)std::cin>>v.x>>v.y>>v.z;
            std::cin>>node0.position.x>>node0.position.y>>node0.position.z;
            const auto sphere=source_shadow_sphere(definition,node0,object_scale);
            std::cout<<sphere.center.x<<' '<<sphere.center.y<<' '<<sphere.center.z<<' '<<sphere.radius<<'\n';
        }return 0;
    }
    if(argc==2 && std::string_view(argv[1])=="--object-transform-probe"){
        Vec3 position,forward,up;Transform parent;float scale;bool mirror,attached,parent_mirror;
        while(std::cin>>position.x>>position.y>>position.z>>forward.x>>forward.y>>forward.z
            >>up.x>>up.y>>up.z>>scale>>mirror>>attached>>parent_mirror>>parent.scale){
            for(auto&v:parent.basis)std::cin>>v.x>>v.y>>v.z;
            std::cin>>parent.position.x>>parent.position.y>>parent.position.z;
            const auto frame=source_object_transform(position,forward,up,scale,mirror,attached?&parent:nullptr,parent_mirror);
            std::cout<<frame.scale;
            for(auto v:frame.basis)std::cout<<' '<<v.x<<' '<<v.y<<' '<<v.z;
            std::cout<<' '<<frame.position.x<<' '<<frame.position.y<<' '<<frame.position.z<<'\n';
        }return 0;
    }
    if(argc==2 && std::string_view(argv[1])=="--coverage-probe"){
        Vec3 center;float radius,scale;ShaderVector plane;
        while(std::cin>>center.x>>center.y>>center.z>>radius>>plane[0]>>plane[1]>>plane[2]>>plane[3]>>scale)
            std::cout<<source_shadow_coverage(center,radius,plane,scale)<<'\n';
        return 0;
    }
    if(argc==2 && (std::string_view(argv[1])=="--attachment-frame-probe" ||
        std::string_view(argv[1])=="--attachment-unnamed-probe")){
        ObjectAttachmentFrame current;Transform parent,child;
        const auto read=[](Transform&t){std::cin>>t.scale;
            for(auto&v:t.basis)std::cin>>v.x>>v.y>>v.z;
            std::cin>>t.position.x>>t.position.y>>t.position.z;};
        while(std::cin>>current.position.x>>current.position.y>>current.position.z
            >>current.forward.x>>current.forward.y>>current.forward.z>>current.up.x>>current.up.y>>current.up.z){
            read(parent);read(child);
            const auto f=source_object_attachment_frame(current,parent,child,
                std::string_view(argv[1])=="--attachment-frame-probe");
            for(auto v:{f.position,f.forward,f.up})std::cout<<v.x<<' '<<v.y<<' '<<v.z<<' ';
            std::cout<<'\n';
        }return 0;
    }
    if(argc==2 && std::string_view(argv[1])=="--attachment-local-probe"){
        ObjectAttachmentFrame world;Transform parent;
        while(std::cin>>world.position.x>>world.position.y>>world.position.z
            >>world.forward.x>>world.forward.y>>world.forward.z>>world.up.x>>world.up.y>>world.up.z
            >>parent.scale){
            for(auto&v:parent.basis)std::cin>>v.x>>v.y>>v.z;
            std::cin>>parent.position.x>>parent.position.y>>parent.position.z;
            const auto frame=source_object_attachment_local_frame(world,parent);
            for(auto v:{frame.position,frame.forward,frame.up})std::cout<<v.x<<' '<<v.y<<' '<<v.z<<' ';
            std::cout<<'\n';
        }return 0;
    }
    if(argc==2 && std::string_view(argv[1])=="--selection-probe"){
        SourceShadowSelectionCache cache;int frame;unsigned preferred,count;float acceleration;bool bypass;
        while(std::cin>>frame>>preferred>>acceleration>>bypass>>count){
            std::vector<SourceShadowCandidate> candidates(count);
            for(auto&c:candidates)std::cin>>c.flags>>c.identity>>c.owner>>c.requested_fade>>c.coverage;
            const auto output=source_shadow_select(cache,candidates,frame,preferred,acceleration,bypass);
            std::cout<<output.size();
            for(unsigned i=0;i<cache.count;++i){const auto&e=cache.entries[i];const auto&c=e.candidate;
                std::cout<<' '<<c.flags<<' '<<c.identity<<' '<<c.owner<<' '<<c.requested_fade<<' '<<c.coverage
                    <<' '<<e.slope<<' '<<e.fade<<' '<<e.last_seen_frame<<' '<<e.created_frame;
            }std::cout<<'\n';
        }return 0;
    }
    ShadowCasterDefinition d;d.model_present=true;d.render_lod_count=2;d.render_cutoffs[2]=10;
    require(source_shadow_distance_fade(d,10,1,false)==1,"near source shadow fade");
    require(source_shadow_distance_fade(d,17,1,false)==0,"far source shadow fade");
    require(source_shadow_distance_fade_byte(d,13.5f,1,false)==127,"truncated source shadow fade byte");
    require(source_shadow_distance_fade(d,13.5f,1,false)==127.f*0.003921568859368563f,"source byte fade scaling");
    require(source_shadow_distance_fade(d,100,1,true)==1,"forced source shadow fade");
    d.model_present=false;require(source_shadow_distance_fade(d,0,1,false)==0,"missing source model shadow fade");
    ShadowCasterSphere root{{0,0,0},1};
    require(!source_shadow_frame(root,{},Vec3{0,0,-1},0.09f,1),"source shadow opacity cutoff");
    const auto a=source_shadow_frame(root,{},Vec3{0,0,-1},1,1);
    require(a && a->width==2 && a->near_offset==1 && a->far_offset==2,"source shadow root bounds");
    const std::array child{ShadowCasterSphere{{0,2,3},0.5f}};
    const auto b=source_shadow_frame(root,child,Vec3{0,0,-1},1,1);
    require(b && b->transverse_radius==2.5f && b->axial_radius==3.5f,"source attachment-expanded shadow bounds");
    SourceShadowSelectionCache selection;
    const std::array candidates{SourceShadowCandidate{0,1,-1,1,.25f},SourceShadowCandidate{0,2,-1,1,.75f}};
    auto selected=source_shadow_select(selection,candidates,1,1,.005f,false);
    require(selected.size()==2 && selected[0].identity==2 && selected[0].requested_fade==1 &&
        selected[1].requested_fade==.995f,"original coverage order and retained fading slot");
    selected=source_shadow_select(selection,candidates,2,1,.005f,false);
    require(selected[1].requested_fade==.985f,"original fade acceleration history");
    selected=source_shadow_select(selection,{},3,1,.005f,false);
    require(selected.empty() && selection.count==0,"missing source candidates remove retained records");
    const std::array maps{"ascension","beavercreek","burial_mounds","coagulation","colossus","cyclotron","foundation","headlong","lockout","midship","waterworks","zanzibar","containment","warlock","deltatap","turf","backwash","dune","elongation","gemini","triplicate"};
    unsigned definitions=0,generators=0,receivers=0,attachments=0,weapons=0,scenery=0,animated_scenery=0;
    for(unsigned mi=0;mi<maps.size();++mi){
        const auto path=std::filesystem::path(mi<12?"assets/disc/maps":"assets/map-pack/maps")/(std::string(maps[mi])+".map");
        Cache cache(path);
        for(const auto&object:cache.load_content(0,true).objects)if(object.type==6 &&
            scenery_automatic_creation_allowed(object,2) && (object.origin_bsp<0 || object.origin_bsp==0)){
            const auto model=cache.load_model(object.definition,false,object.variant);
            const auto initialized=cache.load_initialized_object_idle(object.definition);
            ++scenery;
            if(model.animation==0xffffffffu){
                require(!initialized,"source graphless scenery acquired an initialized idle");
                continue;
            }
            // All 73 authored graph placements have exact named idle index0.
            // scenery-initialized-idle-original.py also runs the complete
            // original controller constructor before this named override.
            const auto named=cache.find_animation_name(model.animation,"idle");
            require(bool(named),"source scenery named idle missing");
            const auto expected=cache.load_named_animation(model.animation,*named);
            require(initialized && initialized->name==expected.name &&
                initialized->frame_count==expected.frame_count && initialized->nodes==expected.nodes,
                "source scenery constructor/named idle selection changed");
            ++animated_scenery;
        }
        for(const auto&w:cache.tags())if(w.group=="weap" &&
            (w.name.ends_with("\\smg") || w.name.ends_with("\\battle_rifle") || w.name.ends_with("\\sniper_rifle"))){
            const auto model=cache.load_model(w.datum,false);
            const auto initialized=cache.load_initialized_object_idle(w.datum);
            if(w.name.ends_with("\\battle_rifle"))
                require(model.animation==0xffffffffu && !initialized,"source BR third-person initialized idle changed");
            else {
                require(model.animation!=0xffffffffu,"source held weapon animation graph missing");
                require(!cache.find_animation_name(model.animation,"idle"),"source initial named idle unexpectedly exists");
                if(w.name.ends_with("\\smg")){
                    const auto idle=cache.load_action(model.animation,"any","any","any","idle");
                    const auto dual=cache.load_action(model.animation,"any","dual","any","idle");
                    require(idle.frame_count==1 && dual.frame_count==5,"source SMG default/dual idle action changed");
                    require(initialized && initialized->frame_count==1 && initialized->nodes==idle.nodes &&
                        initialized->frames.size()==idle.frames.size(),"source SMG initialized idle selection changed");
                }else{
                    bool missing=false;try{(void)cache.load_action(model.animation,"any","any","any","idle");}
                    catch(const FormatError&){missing=true;}
                    require(missing,"source sniper third-person idle unexpectedly exists");
                    require(!initialized,"source sniper initialized idle unexpectedly exists");
                    require(cache.load_action(model.animation,"any","any","any","fire_1").frame_count==5,
                        "source sniper third-person fire action changed");
                }
            }
            ++weapons;
        }
        for(const auto&t:cache.tags())if(t.group=="bipd" && (t.name=="objects\\characters\\masterchief\\masterchief_mp" || t.name=="objects\\characters\\elite\\elite_mp")){
            const auto def=cache.load_object_shadow_definition(t.datum);
            require(def.model_present && def.mode==0 && def.bounding_radius==0.44f,"source multiplayer biped caster definition");
            require(def.fade_end>=def.fade_start,"authored model shadow fade bounds");++definitions;
            const auto body=cache.load_model(def.model);
            for(const auto&w:cache.tags())if(w.group=="weap" &&
                (w.name.ends_with("\\smg") || w.name.ends_with("\\battle_rifle") || w.name.ends_with("\\sniper_rifle")))
                for(unsigned hand=0;hand<2;++hand){
                    const auto attachment=cache.load_weapon_attachment(t.datum,w.datum,hand);
                    require(attachment.parent.size()==1 && attachment.parent[0].region==-1,
                        "source player hand marker unexpectedly needs a variant permutation filter");
                    require(attachment.child_marker==0 && attachment.child.empty() && attachment.preferred_child.empty(),
                        "source SMG/BR/sniper unnamed attachment changed");
                    const auto selected=select_weapon_attachment(attachment,{},{});
                    require(selected.parent.node<body.bones.size(),"source attachment parent node outside body");
                    require(!selected.named_child && selected.child.node==0 && selected.child.local.scale==1,
                        "source weapon unnamed marker fallback changed");
                    ++attachments;
                }
            for(const auto&material:body.mesh.materials){
                const auto generated=prepare_shadow_material(cache,material,SourceShadowPhase::Generate,true,false,.75f);
                const auto received=prepare_shadow_material(cache,material,SourceShadowPhase::Apply,true,false,.75f);
                require(prepare_shadow_material(cache,material,SourceShadowPhase::Generate,true,true,.75f).empty(),
                    "source geometry modifier failed to suppress generation");
                for(const auto&pass:generated){
                    const auto&p=pass.material.passes.at(pass.material.forward_pass_index);
                    require(p.implementation==0 && pass.material.vertex_program==p.vertex_code,
                        "source model generation implementation or microcode changed");
                    require(pass.material.forward_raster.color_write==0x1010101 && pass.material.forward_raster.depth_enable==0,
                        "source model generation target writes changed");++generators;
                }
                for(const auto&pass:received){
                    const auto&p=pass.material.passes.at(pass.material.forward_pass_index);
                    require(p.implementation==4 && pass.external_targets[0]==10,
                        "source receiver target or implementation changed");
                    require(pass.material.forward_blend==9 && pass.material.forward_raster.color_write==0x10101 &&
                        pass.material.forward_raster.depth_function==0x202 && !pass.material.forward_raster.depth_write,
                        "source receiver blend or world depth changed");++receivers;
                }
            }
        }
    }
    require(definitions==42,"all 21 maps retain Chief/Elite shadow definitions");
    require(attachments==252,"all 21 Chief/Elite SMG/BR/sniper attachment pairs missing");
    require(weapons==63,"all 21 held weapon animation definitions missing");
    require(scenery==278 && animated_scenery==73,"all 21 admitted Slayer scenery idle paths missing");
    require(generators && receivers,"source biped shadow pass coverage missing");
    std::cout<<"42 original multiplayer caster definitions, "<<generators<<" generators, "<<receivers
        <<" receivers, "<<attachments<<" weapon marker pairs, "<<weapons<<" weapon animation definitions, "
        <<scenery<<" scenery idle paths and source shadow bounds/fade checks passed\n";
    return 0;
}catch(const std::exception&e){std::cerr<<e.what()<<'\n';return 1;}}
