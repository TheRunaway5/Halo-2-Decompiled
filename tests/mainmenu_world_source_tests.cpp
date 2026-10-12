#include "halo2/mainmenu_world_source.hpp"
#include <algorithm>
#include <bit>
#include <cmath>
#include <cstring>
#include <fstream>
#include <iostream>
#include <limits>
#include <set>

using namespace halo2;
namespace {
void require(bool value,const std::string &message) {if(!value)throw FormatError(message);}
template<class F>void invariant_rejected(F action,const char *message) {
    bool caught=false;try{action();}catch(const FormatError &){caught=true;}
    require(caught,message);
}
std::uint32_t word(std::istream &stream) {
    std::array<unsigned char,4> bytes{};
    require(bool(stream.read(reinterpret_cast<char *>(bytes.data()),4)),"short joined original frame fixture");
    return std::uint32_t(bytes[0])|(std::uint32_t(bytes[1])<<8)|
        (std::uint32_t(bytes[2])<<16)|(std::uint32_t(bytes[3])<<24);
}
struct Words {
    std::span<const std::uint32_t> data;std::size_t at{};
    std::uint32_t take() {require(at<data.size(),"short original frame record");return data[at++];}
    std::span<const std::uint32_t> take(std::size_t count) {
        require(count<=data.size()-at,"short original frame bank");
        const auto result=data.subspan(at,count);at+=count;return result;
    }
    float scalar(){return std::bit_cast<float>(take());}
    Vec3 vector(){return {scalar(),scalar(),scalar()};}
    BspAtmosphere::CameraLocation location(){return {std::int32_t(take()),std::int32_t(take())};}
};
struct Case {
    unsigned index{};Camera camera;BspAtmosphere::CameraLocation previous;
    SourcePortalMaskState portals;SourceBspShaderLodAuxiliary cinematic;
    std::optional<std::uint32_t> flags;
    std::vector<SourceBspAdmissionRecord> original_instances;
};
std::vector<Case> cases(const std::filesystem::path &path) {
    std::ifstream input(path,std::ios::binary);std::array<char,8> magic{};
    require(bool(input.read(magic.data(),8)) && magic==std::array<char,8>{'H','2','M','B','F','0','0','1'},
        "unknown joined original frame fixture");
    const auto count=word(input);require(count>=2996 && count<10000,"incomplete original title fixture");
    const std::set<unsigned> wanted{0,181,293,840,2990};std::vector<Case> result;
    for(unsigned index=0;index<count;++index) {
        const auto size=word(input);require(size>=271 && size<100000,"invalid original frame record extent");
        if(!wanted.contains(index)) {input.seekg(std::streamoff(size)*4,std::ios::cur);require(bool(input),"short skipped original frame record");continue;}
        std::vector<std::uint32_t> record(size);for(auto &value:record)value=word(input);
        Words data{record};Case value;value.index=index;
        value.camera.position=data.vector();value.camera.forward=data.vector();value.camera.up=data.vector();
        value.camera.projection_vertical_angle=data.scalar();value.camera.horizontal_field_of_view=true;
        value.camera.oriented=true;value.camera.projection_clip_scale={544.f/640.f,408.f/480.f};
        value.previous=data.location();for(auto &mask:value.portals.words)mask=data.take();
        value.cinematic={bool(data.take()),bool(data.take())};
        const auto has_flags=data.take(),flags=data.take();if(has_flags)value.flags=flags;
        (void)data.location();(void)data.location();
        require(data.take()!=0,"selected canonical original title case must be inside the BSP");
        (void)data.take();(void)data.take(72+48+111);
        (void)data.take(data.take());const auto admission=data.take(data.take());
        require(data.at==record.size(),"trailing original frame record");
        Words queue{admission};(void)queue.take(128+16);
        const auto clusters=queue.take();(void)queue.take(std::size_t(clusters)*4);
        const auto instances=queue.take();
        for(unsigned i=0;i<instances;++i) {
            const auto id=queue.take(),flags=queue.take(),other=queue.take(),association=queue.take();
            require(flags<=65535 && association<=65535,"original queue record exceeds retained fields");
            value.original_instances.push_back({id,std::uint16_t(flags),std::uint16_t(association),other});
        }
        result.push_back(std::move(value));
    }
    require(result.size()==wanted.size(),"missing chosen canonical original title cases");return result;
}
std::vector<std::byte> bytes(const std::filesystem::path &path) {
    std::ifstream input(path,std::ios::binary|std::ios::ate);
    require(bool(input),"original raw draw fixture is missing");const auto size=input.tellg();
    require(size>=0 && size<1024*1024,"invalid original raw draw fixture size");
    std::vector<std::byte> result(static_cast<std::size_t>(size));input.seekg(0);
    require(bool(input.read(reinterpret_cast<char *>(result.data()),size)),"short original raw draw fixture");return result;
}
MenuWorldSourceConstants view_constants(const MainmenuBspAdmissionFrame &frame,const Camera &camera) {
    MenuWorldSourceConstants result;for(auto &vector:result)vector.fill(std::bit_cast<float>(0x7fc12345u));
    const auto view=source_view_shader_bank(frame.render_camera,1024,camera.position,{0,0,640,480,1,false,false,0},false);
    const auto fog=bsp_fog_constants(frame.fog,camera.position,camera.forward);
    std::copy(view.begin(),view.end(),result.begin());std::copy(fog.begin(),fog.end(),result.begin()+12);
    return result;
}
bool bits(const ShaderVector &a,const ShaderVector &b) {
    return std::bit_cast<std::array<std::uint32_t,4>>(a)==std::bit_cast<std::array<std::uint32_t,4>>(b);
}
const MainmenuWorldSourceCandidate *find(const MainmenuWorldSourceFrame &frame,unsigned instance,unsigned part) {
    const auto found=std::find_if(frame.candidates.begin(),frame.candidates.end(),[&](const auto &value) {
        return value.original.geometry==-2-int(instance) && value.original.part==part;
    });
    return found==frame.candidates.end()?nullptr:&*found;
}
const MainmenuWorldSourceRange &single(const MainmenuWorldSourceCandidate &candidate) {
    require(candidate.ranges.size()==1,"fully-contained baseline must retain one original draw range");
    return candidate.ranges.front();
}
bool has_rejection(const MainmenuWorldSourceFrame &frame,MainmenuWorldSourceReject reason) {
    const auto found=frame.rejected.find(reason);return found!=frame.rejected.end() && found->second;
}
void packet(const MenuWorldSourceBinding &binding,const Material &material,
            const MenuWorldSourceConstants &view) {
    for(unsigned i=0;i<18;++i)require(bits(binding.constants()[i],view[i]),"raw packet changed actual source view/fog bank");
    for(unsigned i=0;i<16;++i)
        require(bits(binding.constants()[18+i],material.program_constants[1][i]),"raw packet used stale or wrong selected-pass constants");
    require(std::bit_cast<std::uint32_t>(binding.constants()[191][0])==0x7fc12345u,
        "raw packet replaced an unconsumed caller NaN");
    for(const auto index:binding.consumed_constants())for(const auto value:binding.constants()[index])
        require(std::isfinite(value),"raw candidate retained a missing consumed constant");
    require(binding.external_attribute_registers().empty(),"raw candidate lost an external attribute rejection");
}
void census(const MainmenuWorldSourceFrame &prepared,const SourceBspResidentPlan &resident,
            const MainmenuBspAdmissionFrame &frame,std::span<const Material> materials,
            const MenuWorldSourceConstants &view,const MainmenuBspRangeFrame *prefix=nullptr) {
    std::size_t accounted=prepared.candidates.size();for(const auto &[reason,count]:prepared.rejected) {
        require(!mainmenu_world_source_reject_name(reason).empty() && count,"invalid raw rejection audit");accounted+=count;
    }
    require(accounted==resident.parts().size(),"raw preparation lost a catalogue entry without a rejection reason");
    const auto selected=resident.select(frame.shader_view(),frame.cinematic);std::optional<std::size_t> prior;
    std::map<unsigned,std::shared_ptr<const MenuWorldSourceSection>> sections;
    for(const auto &candidate:prepared.candidates) {
        require(!prior || *prior<candidate.catalogue_index,"raw producer reordered resident catalogue");prior=candidate.catalogue_index;
        require(candidate.original==resident.parts().at(candidate.catalogue_index).original,
            "raw producer discarded original material/lightmap/owner/section/part key");
        const auto &decision=selected.decisions.at(candidate.catalogue_index);
        const auto &binding=selected.bindings.at(candidate.catalogue_index);
        require(decision.state==SourceBspPartDecision::accepted && decision.selection && binding.material &&
            candidate.selected_material==*binding.material && decision.selection->instance_alpha==255 &&
            binding.stipple && *binding.stipple==16,"raw candidate bypassed selected LOD/alpha/stipple admission");
        require(candidate.original.geometry<=-2,"raw producer admitted a cluster or dynamic owner");
        const auto instance=unsigned(-std::int64_t(candidate.original.geometry)-2);
        const auto *record=frame.admission.instance(instance);
        require(record,"raw producer lost its actual queue owner");
        const MainmenuBspPartRanges *expected_ranges=nullptr;
        if(prefix) {
            const auto *unit=prefix->unit_for_geometry(candidate.original.geometry);
            require(unit && unit->section==candidate.original.section,"raw prefix lost completed range owner");
            expected_ranges=&unit->parts.at(candidate.original.part);
            require(candidate.ranges.size()==expected_ranges->ranges.size(),"raw prefix changed ordered source submission count");
        } else {
            require(record->flags&0x800,"raw producer substituted complete part without queue0800");
            require(candidate.ranges.size()==1,"fully-contained census changed its original draw range count");
        }
        for(unsigned range_index=0;range_index<candidate.ranges.size();++range_index) {
        const auto &range=candidate.ranges[range_index];
        const auto &draw=range.base.draw();const auto &material=materials[candidate.selected_material];
        if(expected_ranges)require(draw.range && draw.index_range()==expected_ranges->ranges[range_index],
            "raw prefix expanded or reordered an original source range");
        else require(!draw.range,"whole-contained route unexpectedly supplied a filtered range");
        require(draw.part==candidate.original.part && draw.geometry->section()==candidate.original.section &&
            draw.geometry->parts().at(draw.part).shader==material.source_shader_datum &&
            draw.topology==MenuWorldSourceTopology::triangle_list,"raw owner/resource/shader/topology mismatch");
        require(range.base.owner().kind==MenuWorldSourceOwnerKind::bsp_instance &&
            range.base.owner().ordinal==instance && !range.base.owner().generation,
            "raw producer invented or conflated the BSP owner generation");
        const auto [owner,inserted]=sections.emplace(draw.geometry->section(),draw.geometry);
        require(inserted || owner->second==draw.geometry,"immutable source section was not shared by definition");
        require(range.base_pass.shader()==material.source_shader_datum &&
            range.base_pass.choice()==MenuWorldSourcePassChoice{material.selected_shader_lod,3,0,3} &&
            range.base.vertex_shader()==range.base_pass.vertex_shader(),"raw base did not use actual classification3 tuple");
        packet(range.base,material,view);
        const auto expected=bsp_planar_fog_implementation(frame.fog,
            frame.admission.geometry_contains_fog(candidate.original.geometry));
        if(range.fog) {
            const auto &fog=*range.fog;
            require(expected && fog.implementation==*expected && fog.material &&
                fog.binding.owner()==range.base.owner() && fog.binding.draw().geometry==draw.geometry &&
                fog.binding.draw().part==draw.part && fog.binding.draw().object_rows==draw.object_rows &&
                fog.binding.draw().range==draw.range &&
                fog.pass.choice()==MenuWorldSourcePassChoice{material.selected_shader_lod,16,0,*expected},
                "raw fog did not pair the actual source implementation and base owner");
            packet(fog.binding,*fog.material,view);
        }
        }
    }
}
}
int main(int argc,char **argv) {try {
    require(argc==4,"mainmenu raw producer test needs mainmenu.map, joined original frame fixture and Stage3 directory");
    for(unsigned i=1;i<4;++i)if(!std::filesystem::exists(argv[i]))return 77;
    const Cache cache(argv[1]);auto mesh=cache.load_bsp(0);
    const SourceBspResidentPlan resident(cache,mesh,0,SourceBspResidentLighting::classification3);
    prepare_bsp_planar_fog(cache,mesh,cache.load_bsp_atmosphere(0));
    MainmenuWorldSourcePlan plan(cache,resident);MainmenuBspFrameProducer producer(cache,0);
    MainmenuBspRangeProducer range_producer(cache,0);
    invariant_rejected([&]{MainmenuWorldSourcePlan invalid(cache,resident,cache.bsps().size());},"raw producer accepted nonexistent BSP");
    const auto original_cases=cases(argv[2]);std::optional<MainmenuBspAdmissionFrame> initial;
    std::optional<MenuWorldSourceConstants> initial_view;std::size_t candidates{},fog_pairs{};
    bool actual_partial=false;std::size_t prefix_candidates{},prefix_empty{},prefix_calls{};
    std::optional<MainmenuBspRangeFrame> initial_ranges;
    for(const auto &test:original_cases) {
        const auto frame=producer.prepare({test.camera,test.previous,test.portals,test.cinematic,test.flags});
        for(const auto &expected:test.original_instances) {
            const auto *actual=frame.admission.instance(expected.id);
            require(actual && actual->flags==expected.flags && actual->association==expected.association && actual->other==expected.other,
                "canonical source instance queue differs from retained original record");
        }
        require(frame.admission.instances.size()==test.original_instances.size(),"canonical source queue gained an instance");
        const auto view=view_constants(frame,test.camera),preserved=view;
        const auto prepared=plan.prepare(frame,mesh.materials,view);census(prepared,resident,frame,mesh.materials,view);
        const auto ranges=range_producer.prepare(frame);
        require(ranges.complete,"canonical camera/frame range prefix incomplete: "+ranges.diagnostic);
        const auto prefix=plan.prepare_bsp_prefix(frame,ranges,mesh.materials,view);
        census(prefix,resident,frame,mesh.materials,view,&ranges);
        for(const auto &candidate:prefix.candidates) {
            ++prefix_candidates;prefix_empty+=candidate.ranges.empty();prefix_calls+=candidate.ranges.size();
        }
        require(std::memcmp(view.data(),preserved.data(),sizeof(view))==0,"raw preparation mutated caller source constants");
        candidates+=prepared.candidates.size();for(const auto &value:prepared.candidates)
            for(const auto &range:value.ranges)fog_pairs+=range.fog.has_value();
        if(test.index==0) {
            initial=frame;initial_view=view;initial_ranges=ranges;const auto *regression=find(prepared,58,2);
            require(regression,"canonical original1335 regression owner was not prepared");
            const auto &range=single(*regression);
            require(range.fog && range.base_pass.shader()==0xe777054du &&
                range.base.vertex_shader()==0xe1cc0057u && regression->original.section==52,
                "canonical original1335 instance58/definition52/part2 regression was not prepared");
            const auto original_program=bytes(std::filesystem::path(argv[3])/"event-1335-source1-program.bin");
            require(std::equal(original_program.begin(),original_program.end(),range.base.program_bytes().begin(),range.base.program_bytes().end()),
                "original1335 actual class1 bytes differ from prepared base");
            const auto raw_indices=bytes(std::filesystem::path(argv[3])/"event-1335-indices.bin");const Bytes indices(raw_indices);
            require(range.base.draw().indices().size()==522 && raw_indices.size()==522*4,
                "original1335 whole part range changed");
            for(unsigned i=0;i<522;++i)require(range.base.draw().indices()[i]==indices.u32(i*4),"raw producer changed original1335 indices");
            const auto captured=bytes(std::filesystem::path(argv[3])/"event-1335-constants192.bin");const Bytes bank(captured);
            for(const unsigned first:{50u,170u})for(unsigned row=0;row<3;++row)for(unsigned component=0;component<4;++component)
                require(std::bit_cast<std::uint32_t>(range.base.constants()[first+row][component])==bank.u32((first+row)*16+component*4),
                    "raw producer scaled owner rows or changed original1335 compression");
        }
        if(test.index==293) {
            const auto *record=frame.admission.instance(58);
            require(record && !(record->flags&0x800) && !find(prepared,58,2) &&
                has_rejection(prepared,MainmenuWorldSourceReject::partial_instance),
                "actual original partial instance58 must not become a whole raw part");actual_partial=true;
            const auto *part=find(prefix,58,2);
            require(part && single(*part).base.draw().index_range()==MenuWorldSourceIndexRange{702,522} &&
                single(*part).fog && single(*part).fog->binding.draw().index_range()==MenuWorldSourceIndexRange{702,522},
                "original partial293 prefix did not retain exact702/522 base and fog partners");
        }
    }
    require(initial && initial_view && initial_ranges && candidates && fog_pairs && actual_partial && prefix_empty,
        "canonical raw producer test coverage is empty");
    const auto baseline=plan.prepare(*initial,mesh.materials,*initial_view);
    const auto *target=find(baseline,58,2);require(target && single(*target).fog,"original base/fog control missing");
    const auto material_index=target->selected_material;
    auto incomplete_ranges=*initial_ranges;incomplete_ranges.complete=false;
    const auto no_partial=plan.prepare_bsp_prefix(*initial,incomplete_ranges,mesh.materials,*initial_view);
    require(no_partial.candidates.empty() && has_rejection(no_partial,MainmenuWorldSourceReject::incomplete_bsp_ranges),
        "incomplete BSP prefix leaked whole-part or partial draw claims");
    auto wrong_owner=*initial_ranges;
    auto *range_owner=const_cast<MainmenuBspRangeUnit *>(wrong_owner.unit_for_geometry(-60));
    require(range_owner,"original prefix control owner unavailable");range_owner->section^=1;
    invariant_rejected([&]{plan.prepare_bsp_prefix(*initial,wrong_owner,mesh.materials,*initial_view);},
        "raw prefix swallowed a different source owner section");
    auto partial=*initial;auto *record=const_cast<SourceBspAdmissionRecord *>(partial.admission.instance(58));
    record->flags&=~0x800u;const auto excluded=plan.prepare(partial,mesh.materials,*initial_view);
    require(!find(excluded,58,2) && has_rejection(excluded,MainmenuWorldSourceReject::partial_instance),"0800-clear control admitted whole source part");
    auto absent=*initial;std::erase_if(absent.admission.instances,[](const auto &entry){return entry.id==58;});
    require(!find(plan.prepare(absent,mesh.materials,*initial_view),58,2),"absent owner queue admitted raw part");
    // An explicit coverage-only negative control uses the independently proved
    // source alpha service. It is not a newly classified live frame.
    auto faded=*initial;const auto authored=cache.load_source_bsp_admission(0).instances.at(58);
    const auto coverage=source_shader_instance_coverage(authored.center,authored.radius,
        source_shader_coverage_camera(faded.render_camera));
    faded.render_camera.viewport_half_metrics.y*=28.f/coverage;
    const auto alpha=source_shader_instance_alpha(source_shader_instance_coverage(authored.center,authored.radius,
        source_shader_coverage_camera(faded.render_camera)));
    require(alpha>0 && alpha<255,"bounded source fade control did not produce partial alpha");
    const auto fade_result=plan.prepare(faded,mesh.materials,*initial_view);
    require(!find(fade_result,58,2) && has_rejection(fade_result,MainmenuWorldSourceReject::instance_fade),"raw route discarded actual instance fade");
    auto missing=*initial_view;missing[0].fill(std::numeric_limits<float>::quiet_NaN());
    const auto missing_result=plan.prepare(*initial,mesh.materials,missing);
    require(!find(missing_result,58,2) && has_rejection(missing_result,MainmenuWorldSourceReject::missing_constant),"raw route accepted missing consumed c0");
    auto &material=mesh.materials[material_index];const auto old=material.program_constants[1][0];
    material.program_constants[1][0]={3.25f,-1.5f,.125f,7};
    const auto animated=plan.prepare(*initial,mesh.materials,*initial_view);
    require(find(animated,58,2) && bits(single(*find(animated,58,2)).base.constants()[18],material.program_constants[1][0]),
        "raw base retained stale selected animated c18");material.program_constants[1][0]=old;
    // These explicit negative controls change the native selected-family
    // preconditions, not the original assets or a claimed live source frame.
    const auto vertex=material.forward_vertex;material.forward_vertex=4;
    const auto unsupported_base=plan.prepare(*initial,mesh.materials,*initial_view);
    require(!find(unsupported_base,58,2) &&
        has_rejection(unsupported_base,MainmenuWorldSourceReject::unsupported_base_pass),
        "unsupported selected base family did not retain a fallback reason");material.forward_vertex=vertex;
    const auto selected_lod=material.selected_shader_lod;material.selected_shader_lod^=1;
    invariant_rejected([&]{plan.prepare(*initial,mesh.materials,*initial_view);},
        "raw route swallowed accepted selected-LOD identity mismatch");material.selected_shader_lod=selected_lod;
    const auto shader=material.source_shader_datum;material.source_shader_datum^=1;
    invariant_rejected([&]{plan.prepare(*initial,mesh.materials,*initial_view);},"raw route swallowed source shader identity mismatch");material.source_shader_datum=shader;
    const auto pass_index=material.forward_pass_index;material.forward_pass_index=0xffffffffu;
    invariant_rejected([&]{plan.prepare(*initial,mesh.materials,*initial_view);},"raw route swallowed selected base pass mismatch");material.forward_pass_index=pass_index;
    const auto fog_index=single(*target).fog->implementation;const auto fog_child=material.world_planar_passes[fog_index],fallback=material.planar_fog_pass;
    require(bool(fog_child),"selected source fog control must use the actual implementation child");
    const auto child_constant=fog_child->program_constants[1][0];
    fog_child->program_constants[1][0]={-2.5f,6.75f,.375f,9};
    const auto animated_fog=plan.prepare(*initial,mesh.materials,*initial_view);
    const auto *animated_fog_target=find(animated_fog,58,2);
    require(animated_fog_target && single(*animated_fog_target).fog &&
        single(*animated_fog_target).fog->material==fog_child &&
        bits(single(*animated_fog_target).fog->binding.constants()[18],fog_child->program_constants[1][0]) &&
        bits(single(*animated_fog_target).base.constants()[18],material.program_constants[1][0]),
        "raw fog reused the base material bank or a stale selected child");
    fog_child->program_constants[1][0]=child_constant;
    const auto child_blend=fog_child->forward_blend;fog_child->forward_blend=1;
    const auto unsupported_fog=plan.prepare(*initial,mesh.materials,*initial_view);
    require(!find(unsupported_fog,58,2) &&
        has_rejection(unsupported_fog,MainmenuWorldSourceReject::unsupported_fog_pass),
        "raw base survived an unsupported required paired fog family");fog_child->forward_blend=child_blend;
    const auto child_shader=fog_child->source_shader_datum;
    fog_child->source_shader_datum=material.source_shader_datum^1;
    invariant_rejected([&]{plan.prepare(*initial,mesh.materials,*initial_view);},
        "raw route swallowed populated fog-child source shader mismatch");fog_child->source_shader_datum=child_shader;
    material.world_planar_passes[fog_index].reset();material.planar_fog_pass.reset();
    const auto unpaired=plan.prepare(*initial,mesh.materials,*initial_view);
    require(!find(unpaired,58,2) && has_rejection(unpaired,MainmenuWorldSourceReject::missing_fog_material),"raw base survived without required matching fog child");
    material.world_planar_passes[fog_index]=fog_child;material.planar_fog_pass=fallback;
    // Check all original4CBF0 layer16 implementation branches from the actual
    // queue bit, without inferring membership from a child or submerged test.
    for(unsigned implementation=0;implementation<3;++implementation) {
        auto controlled=*initial;auto *queue=const_cast<SourceBspAdmissionRecord *>(controlled.admission.instance(58));
        controlled.fog.classification=3;controlled.fog.planar_only=implementation==2;
        controlled.fog.eye_fraction=0;controlled.fog.planar_flags=0;
        if(implementation==1)queue->flags&=~0x400u;else queue->flags|=0x400u;
        const auto prepared=plan.prepare(controlled,mesh.materials,*initial_view);const auto *found=find(prepared,58,2);
        require(found && single(*found).fog && single(*found).fog->implementation==implementation &&
            single(*found).fog->material==material.world_planar_passes[implementation],"raw fog ignored original queue/predicate implementation branch");
    }
    auto noncontaining=*initial;auto *queue=const_cast<SourceBspAdmissionRecord *>(noncontaining.admission.instance(58));
    queue->flags&=~0x400u;noncontaining.fog.classification=3;noncontaining.fog.planar_only=true;
    const auto nofog=plan.prepare(noncontaining,mesh.materials,*initial_view);
    require(find(nofog,58,2) && !single(*find(nofog,58,2)).fog,"raw fog invented a noncontaining planar-only pass");
    std::cout<<original_cases.size()<<" canonical original title cases / "<<candidates<<" raw owner/base candidates / "<<fog_pairs
        <<" paired fog passes; "<<prefix_candidates<<" BSP-prefix claims / "<<prefix_calls<<" calls / "<<prefix_empty
        <<" complete-empty parts; original1335 bytes, actual partial293 base/fog ranges, queue/alpha/animated banks/unsupported families/identity controls; CPU only\n";
    return 0;
}catch(const std::exception &error){std::cerr<<error.what()<<'\n';return 1;}}
