#include "halo2/content.hpp"
#include "halo2/damage_feedback.hpp"
#include "halo2/model.hpp"
#include "halo2/renderer.hpp"
#include "halo2/shader_animation.hpp"
#include "halo2/source_cpu_skin_inputs.hpp"
#include "halo2/source_shield_model_shaders.hpp"
#include "halo2/source_transparent_queue.hpp"
#include "halo2/source_tag_metadata.hpp"
#include "halo2/world_material.hpp"
#include <algorithm>
#include <fstream>
#include <iostream>
#include <map>

using namespace halo2;
namespace {
bool validation=true;
void check(bool value,const char *message){if(!value)throw FormatError(message);}
struct Image {unsigned width{},height{};std::vector<unsigned char> rgb;};
Image read(const std::filesystem::path &path) {
    std::ifstream in(path,std::ios::binary);std::string magic;Image result;unsigned maximum;
    in>>magic>>result.width>>result.height>>maximum;in.get();
    check(magic=="P6"&&maximum==255,"Invalid shield inspection image");
    result.rgb.resize(std::size_t(result.width)*result.height*3);
    in.read(reinterpret_cast<char *>(result.rgb.data()),result.rgb.size());
    check(bool(in),"Truncated shield inspection image");return result;
}
Image capture(SDL_Window *window,const Mesh &mesh,const Camera &camera,const std::filesystem::path &path,
    const SourceTransparentDrawBank *bank=nullptr,bool first_person=false) {
    Renderer renderer(window,mesh,validation);renderer.scene_clear_color({.025f,.035f,.045f});
    ModelView view;view.camera=camera;view.width=640;view.height=480;view.vertices=mesh.vertices;view.full_surface=true;
    if(bank) {
        if(first_person){view.vertices={};view.foreground=mesh.vertices;view.foreground_transparent=*bank;}
        else view.world_transparent=*bank;
    }
    for(unsigned i=0;i<4;++i)
        if(renderer.draw(camera,path,{},false,{},{},{},{&view,1}))return read(path);
    throw FormatError("Shield inspection did not submit");
}
Image capture_queue(SDL_Window *window,const Mesh &mesh,const Camera &camera,
    const std::filesystem::path &path,bool first_person) {
    //Controlled completed callback services, not a live collector/PVS claim.
    //Repeated keys remain separate callbacks, with an ignored occupied prefix
    //and a null callback inside the drained range.
    std::map<std::array<unsigned,3>,std::vector<Vertex>> parts;
    for(const auto &v:mesh.vertices)parts[{v.material,v.source_section,v.source_part}].push_back(v);
    SourceTransparentQueue queue;std::vector<SourceTransparentDrawBinding> bindings(2);
    queue.allocate()->callback=0xffffffffu;queue.allocate()->callback=0xffffffffu;
    queue.begin_first_person_suffix();
    unsigned index=0;
    for(const auto &[key,vertices]:parts) {
        SourceTransparentPartInput p;p.part_type=1;p.selected_template_layers=0x8000;
        p.selected_shader=mesh.materials.at(key[0]).source_shader_datum;p.draw_token=0x100000+index;
        p.camera_position=camera.position;p.camera_forward=camera.forward;
        p.center=camera.position-camera.forward*float(index++);
        auto *c=queue.append_part(p);check(c,"Shield callback allocation failed");
        bindings.push_back(SourceTransparentGeometryDraw{p.selected_shader,p.draw_token,vertices,{},first_person?0x800u:0u});
        if(index==1){queue.allocate()->callback=0;bindings.emplace_back();}
    }
    SourceTransparentDrawBank bank{queue.commands(),queue.first(),bindings};
    const auto result=capture(window,mesh,camera,path,&bank,first_person);
    //A nonnull unsupported callback must never turn into a missing draw.
    SourceTransparentQueue unsupported;unsupported.allocate()->callback=0x41cc0;
    const std::array<SourceTransparentDrawBinding,1> missing{};
    const SourceTransparentDrawBank invalid{unsupported.commands(),0,missing};bool rejected=false;
    try{capture(window,mesh,camera,{},&invalid);}catch(const FormatError &e) {
        rejected=std::string(e.what()).find("callback service is unresolved")!=std::string::npos;
    }
    check(rejected,"Shield renderer silently skipped an unsupported source callback");
    return result;
}
unsigned differences(const Image &a,const Image &b) {
    check(a.width==b.width&&a.height==b.height&&a.rgb.size()==b.rgb.size(),"Shield inspection extent changed");
    unsigned result=0;for(unsigned i=0;i<a.rgb.size();++i)result+=a.rgb[i]!=b.rgb[i];return result;
}
void animate(Mesh &mesh,const Cache &cache,std::uint32_t biped,float recent,float shield=.625f,
    float camouflage=0.f) {
    const auto functions=cache.load_object_functions(biped);DamageFeedbackState feedback;feedback.shield.visible=recent;
    for(auto &material:mesh.materials) {
        bind_object_camouflage(material,camouflage);
        check(animate_forward_material(material,.375,[&](auto name) {
            return evaluate_object_function(functions,*material.animation.tables,name,0,{},
                [&](auto requested)->std::optional<FunctionValue> {
                    if(const auto value=damage_feedback_input(feedback,1,shield,false,requested))
                        return FunctionValue{value->value,value->active};
                    return {};
                }).value;
        }),"Shield inspection animation was not bound");
    }
}
}
int main(int argc,char **argv) {
    try {
        check(argc==2 || (argc==3 && std::string(argv[2])=="--no-validation"),
            "Supply shield inspection output directory and optional --no-validation");
        validation=argc==2;
        const auto root=std::filesystem::path(HALO2_SOURCE_DIR),directory=std::filesystem::path(argv[1]);
        const auto path=root/"assets/disc/maps/lockout.map";if(!std::filesystem::exists(path))return 77;
        std::filesystem::create_directories(directory);Cache cache(path);const auto content=cache.load_content(0,true);
        check(SDL_Init(SDL_INIT_VIDEO)==0,"Shield inspection SDL initialization failed");
        auto *window=SDL_CreateWindow("Original shield material inspection",0,0,640,480,SDL_WINDOW_VULKAN|SDL_WINDOW_HIDDEN);
        check(window,"Shield inspection window unavailable");unsigned cases=0,changed=0,local_controls=0;
        for(unsigned representation:{2u,3u})for(bool first_person:{false,true})for(bool overshield:{false,true}) {
            const auto &r=content.player_representations.at(representation);
            auto model=cache.load_model(first_person?r.hands:r.biped.model,false);
            std::vector<BonePose> locals;for(const auto &bone:model.bones)locals.push_back(bone.bind);
            const auto hand=std::find_if(model.bones.begin(),model.bones.end(),[&](const auto &bone){return cache.string_id(bone.name)=="l_hand";});
            check(hand!=model.bones.end(),"Shield inspection left hand node missing");
            auto &q=locals[std::size_t(hand-model.bones.begin())].rotation;const auto old=q;
            constexpr float s=.3826834323650898f,k=.9238795325112867f;
            q={old[0]*k+old[3]*s,old[1]*k+old[2]*s,old[2]*k-old[1]*s,old[3]*k-old[0]*s};
            const auto &vitality=r.biped.vitality;
            const auto shader=overshield?
                (first_person?vitality.overshield_first_person_shader:vitality.overshield_shader):
                (first_person?vitality.shield_damaged_first_person_shader:vitality.shield_damaged_shader);
            const auto header=cache.load_source_tag_metadata(r.biped.model);
            const auto mode=first_person?r.hands:Bytes(header.bytes_at(header.source_address(),8)).u32(4);
            SourceShieldModelShaders bank(cache,model,mode,shader);
            Mesh mesh;mesh.vertices=pose_model_locals(model,locals);
            SourceModelShaderView shader_view;shader_view.shader=source_shader_lod_view(0,480,false,false);shader_view.part_shader_multiplier=1;
            bank.select(mesh.vertices,{10000,255},shader_view);std::map<unsigned,unsigned> slots;
            for(auto &vertex:mesh.vertices) {
                const auto [slot,inserted]=slots.try_emplace(vertex.material,unsigned(mesh.materials.size()));
                if(inserted)mesh.materials.push_back(model.mesh.materials.at(vertex.material));
                vertex.material=slot->second;
            }
            Vec3 low{INFINITY,INFINITY,INFINITY},high{-INFINITY,-INFINITY,-INFINITY};
            for(const auto &v:mesh.vertices){low=minimum(low,v.position);high=maximum(high,v.position);}
            const auto center=(low+high)*.5f;const float radius=length(high-low)*.5f;
            Camera camera;camera.oriented=true;camera.position=center+Vec3{radius*2.5f,0,0};
            camera.forward={-1,0,0};camera.up={0,0,1};camera.field_of_view=60;
            const auto name=std::string(representation==2?"spartan":"elite")+(first_person?"-fp":"-world")+
                (overshield?"-overshield":"-damaged");
            animate(mesh,cache,r.biped.datum,0,1);const auto zero=capture(window,mesh,camera,directory/(name+"-zero.ppm"));
            animate(mesh,cache,r.biped.datum,.8f,overshield?2.f:.625f);const auto hit=capture(window,mesh,camera,directory/(name+"-hit.ppm"));
            const auto repeated=capture(window,mesh,camera,directory/(name+"-repeat.ppm"));
            check(differences(hit,repeated)==0,"Shield renderer retained nondeterministic object state");
            const auto ordered=capture_queue(window,mesh,camera,directory/(name+"-ordered-world.ppm"),false);
            const auto ordered_fp=capture_queue(window,mesh,camera,directory/(name+"-ordered-fp.ppm"),true);
            check(differences(hit,ordered)==0 && differences(hit,ordered_fp)==0,
                "Shield callback consumer changed authored RGB or duplicated a regular layer15 draw");
            const auto response=differences(zero,hit);check(response>0,"Actual shield input did not affect rendered RGB");changed+=response;
            Mesh control=mesh;for(auto &material:control.materials)material.world_vertex_input_transform=SourceWorldVertexInputTransform::Preserve;
            const auto wrong=capture(window,control,camera,directory/(name+"-posed-noise-control.ppm"));
            const auto local=differences(hit,wrong);check(local>0,"Shield renderer did not consume retained local noise coordinates");local_controls+=local;
            if(overshield) {
                std::vector<std::array<std::uint32_t,60>> programs;
                for(const auto &material:mesh.materials)programs.push_back(material.forward_program.words);
                animate(mesh,cache,r.biped.datum,.8f,2,1.f);
                bool remapped=false;
                for(unsigned n=0;n<mesh.materials.size();++n)
                    remapped|=programs[n]!=mesh.materials[n].forward_program.words;
                check(remapped,"Overshield ignored its owner's original scalar37 program remap");
                const auto cloaked=capture(window,mesh,camera,directory/(name+"-camouflage.ppm"));
                // Exact original19CA0/17420 bank comparisons certify this
                // input. A changed alpha constant need not change RGB under
                // the authored ONE/ONE blend, especially on the FP shader.
                std::cout<<name<<": scalar37 program remap, "<<differences(hit,cloaked)<<" camouflage RGB bytes\n";
                animate(mesh,cache,r.biped.datum,.8f,2,0);
                const auto restored=capture(window,mesh,camera,directory/(name+"-restored.ppm"));
                check(differences(hit,restored)==0,"Overshield retained another owner's camouflage scalar");
            }
            check(hit.width==640&&hit.height==480,"Shield inspection ignored its drawable extent");
            std::cout<<name<<": "<<response<<" input-response and "<<local<<" local-coordinate control RGB bytes\n";++cases;
        }
        SDL_DestroyWindow(window);SDL_Quit();
        check(cases==8,"Incomplete damaged/overshield species/view coverage");
        std::cout<<cases<<" actual authored shield materials rendered deterministically, "<<changed<<" input-response and "<<local_controls
                 <<" local-coordinate control RGB bytes. Ordered world/FP callback consumers match inspection RGB, retain prefix/null semantics and reject missing group services. Controlled bind channels plus left-hand rotation and bounds camera; no original gameplay-frame or live collector admission claim.\n";
        return 0;
    } catch(const std::exception &e){std::cerr<<e.what()<<'\n';SDL_Quit();return 1;}
}
