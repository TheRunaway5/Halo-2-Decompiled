#include "halo2/mainmenu_bsp_frame.hpp"
#include "halo2/mainmenu_portal_state.hpp"
#include <algorithm>
#include <bit>
#include <fstream>
#include <iostream>
#include <limits>
#include <string>

using namespace halo2;
namespace {
void require(bool condition,const std::string &message) {
    if(!condition)throw FormatError(message);
}
template<class F>void rejected(F action,const char *message) {
    bool threw=false;
    try{action();}catch(const FormatError&){threw=true;}
    require(threw,message);
}
bool equal(BspAtmosphere::CameraLocation a,BspAtmosphere::CameraLocation b) {
    return a.leaf==b.leaf && a.cluster==b.cluster;
}
void boundary_checks() {
    BspAtmosphere bank;
    bank.nodes={{0,0xffffff,0x800000}};
    bank.planes={{1,0,0,0}};
    bank.leaves={1};bank.clusters.resize(2);
    const auto inside=mainmenu_camera_location(bank,{0,0,0},{-9,-7});
    require(inside.query_succeeded && equal(inside.current,{0,1}) &&
        equal(inside.retained,{0,1}),"inside source location must replace invalid history");
    // The original validates the two ranges, not their leaf->cluster relation.
    const auto retained=mainmenu_camera_location(bank,{-1,0,0},{0,0});
    require(!retained.query_succeeded && equal(retained.current,{0,0}) &&
        equal(retained.retained,{0,0}),"valid outside history must be retained exactly");
    for(const auto previous:{BspAtmosphere::CameraLocation{},
            BspAtmosphere::CameraLocation{1,0},BspAtmosphere::CameraLocation{0,2}}) {
        const auto result=mainmenu_camera_location(bank,{-1,0,0},previous);
        require(!result.query_succeeded && equal(result.current,{}) &&
            equal(result.retained,previous),"failed invalid history must clear only local location");
    }
    bank.leaves[0]=-1;
    const auto unowned=mainmenu_camera_location(bank,{1,0,0},{0,0});
    require(unowned.query_succeeded && equal(unowned.current,{0,-1}) &&
        equal(unowned.retained,{0,-1}),"source success depends on leaf presence, not cluster presence");
    bank.clusters[0].planar_designator=0;
    bank.clusters[1].planar_designator=-128;
    bank.fog_planes.push_back({0,{}});
    BspAtmosphere::FogPalette palette;palette.datum=0xe4310211;
    bank.fog_palette.push_back(palette);
    const std::vector<std::uint8_t> all{1,1},none{0,0};
    require(mainmenu_bsp_fog_admission(bank,palette.datum,{})==all,
        "matching source fog membership invariant differs");
    require(mainmenu_bsp_fog_admission(bank,0xffffffff,{})==all,
        "NONE source fog membership invariant differs");
    rejected([&]{mainmenu_bsp_fog_admission(bank,0x1234,{});},
        "differing override branches must require the actual source word");
    require(mainmenu_bsp_fog_admission(bank,0x1234,0)==none &&
        mainmenu_bsp_fog_admission(bank,0x1234,0x10)==all,
        "explicit source fog override word must select its actual branch");
    bank.fog_palette[0].datum^=1;
    rejected([&]{mainmenu_bsp_fog_admission(bank,palette.datum,{});},
        "changed authored fog datum must not bypass the invariant guard");
}
std::uint32_t read_word(std::istream &stream) {
    std::array<unsigned char,4> bytes{};
    stream.read(reinterpret_cast<char*>(bytes.data()),bytes.size());
    require(bool(stream),"truncated original mainmenu frame fixture");
    return std::uint32_t(bytes[0])|(std::uint32_t(bytes[1])<<8)|
        (std::uint32_t(bytes[2])<<16)|(std::uint32_t(bytes[3])<<24);
}
struct Words {
    std::span<const std::uint32_t> values;
    std::size_t at{};
    std::uint32_t take() {
        require(at<values.size(),"truncated original mainmenu frame record");
        return values[at++];
    }
    std::span<const std::uint32_t> take(std::size_t count) {
        require(count<=values.size()-at,"truncated original mainmenu frame bank");
        const auto out=values.subspan(at,count);at+=count;return out;
    }
    float scalar(){return std::bit_cast<float>(take());}
    Vec3 vector(){return {scalar(),scalar(),scalar()};}
    BspAtmosphere::CameraLocation location(){return {std::int32_t(take()),std::int32_t(take())};}
};
void compare(std::span<const std::uint32_t> actual,
    std::span<const std::uint32_t> expected,const std::string &name) {
    require(actual.size()==expected.size(),name+" word count differs");
    for(std::size_t i=0;i<actual.size();++i)if(actual[i]!=expected[i])
        throw FormatError(name+" word "+std::to_string(i)+" differs: "+
            std::to_string(actual[i])+" versus original "+std::to_string(expected[i]));
}
template<class T>void compare_object(const T &actual,
    std::span<const std::uint32_t> expected,const std::string &name) {
    const auto bank=std::bit_cast<std::array<std::uint32_t,sizeof(T)/4>>(actual);
    compare(bank,expected,name);
}
void compare_fog(const BspFogSample &sample,std::span<const std::uint32_t> original,
    const std::string &name) {
    require(original.size()==72,"original aggregate fog bank must contain72 words");
    const auto word=[&](std::size_t offset,std::uint32_t value) {
        require(original[offset/4]==value,name+" byte offset "+std::to_string(offset)+" differs");
    };
    const auto scalar=[&](std::size_t offset,float value){word(offset,std::bit_cast<std::uint32_t>(value));};
    const auto color=[&](std::size_t offset,Vec3 value){scalar(offset,value.x);scalar(offset+4,value.y);scalar(offset+8,value.z);};
    const auto atmospheric=[&](std::size_t offset,const AtmosphericFog &fog) {
        color(offset,fog.color);scalar(offset+12,fog.maximum_density);
        scalar(offset+16,fog.start);scalar(offset+20,fog.opaque);
    };
    word(0,sample.classification);
    atmospheric(0x1c,sample.primary);atmospheric(0x34,sample.secondary);
    color(0x5c,sample.sky_color_premultiplied);scalar(0x68,sample.sky_opacity);
    word(0x6c,sample.patchy_datum);word(0x9c,sample.planar_datum);
    if(sample.patchy) {
        color(0x70,sample.patchy->color);
        for(unsigned i=0;i<3;++i)scalar(0x7c+4*i,0);
        for(unsigned i=0;i<2;++i){scalar(0x88+4*i,sample.patchy->density[i]);scalar(0x90+4*i,sample.patchy->distance[i]);}
    }else for(unsigned i=0;i<10;++i)scalar(0x70+4*i,0);
    color(0xa0,sample.planar.color);scalar(0xac,sample.planar.maximum_density);
    scalar(0xb0,sample.planar.opaque_distance);scalar(0xb4,sample.planar.opaque_depth);
    for(unsigned i=0;i<4;++i)scalar(0xe0+4*i,sample.bloom[i]);
    scalar(0x108,sample.eye_height);scalar(0x10c,sample.eye_fraction);
    scalar(0x110,sample.eye_offset);
    require((original[0x114/4]&255)==unsigned(sample.planar_only),name+" plane-only flag differs");
    scalar(0x118,sample.transition);scalar(0x11c,sample.interpolation);
    // Unexposed original scratch/retained words are deliberately not compared.
}
std::vector<std::uint32_t> visibility_words(const SourcePortalVisibilitySet &visible) {
    std::vector<std::uint32_t> out{unsigned(visible.clusters.size())};
    for(const auto &cluster:visible.clusters) {
        out.push_back(std::uint16_t(cluster.source_cluster));
        for(const auto value:cluster.counts)out.push_back(std::uint16_t(value));
        for(const auto value:cluster.first_indices)out.push_back(std::uint16_t(value));
    }
    out.insert(out.end(),visible.sorted_cluster_indices.begin(),visible.sorted_cluster_indices.end());
    out.push_back(unsigned(visible.frustums.size()));
    for(const auto &frustum:visible.frustums) {
        const auto bank=std::bit_cast<std::array<std::uint32_t,sizeof(frustum)/4>>(frustum);
        // Source163DB0 does not write retained word3 at+0C.
        for(unsigned i=0;i<bank.size();++i)if(i!=3)out.push_back(bank[i]);
    }
    return out;
}
std::vector<std::uint32_t> admission_words(const SourceBspAdmission &admission) {
    std::vector<std::uint32_t> out;
    for(unsigned i=0;i<admission.cluster_entries.size();i+=4)
        out.push_back(unsigned(admission.cluster_entries[i])|
            (unsigned(admission.cluster_entries[i+1])<<8)|
            (unsigned(admission.cluster_entries[i+2])<<16)|
            (unsigned(admission.cluster_entries[i+3])<<24));
    out.insert(out.end(),admission.marked_clusters.begin(),admission.marked_clusters.end());
    for(const auto *queue:{&admission.clusters,&admission.instances}) {
        out.push_back(unsigned(queue->size()));
        for(const auto &record:*queue)
            out.insert(out.end(),{record.id,record.flags,record.other,record.association});
    }
    out.push_back(unsigned(admission.associations.size()));
    for(const auto &association:admission.associations)
        out.insert(out.end(),association.begin(),association.end());
    return out;
}
void camera_guards(const MainmenuBspFrameProducer &producer,Camera camera,
    const SourcePortalMaskState &portals) {
    const auto check=[&](const Camera &changed) {
        rejected([&]{producer.prepare({changed,{},portals,{true,true},{}});},
            "unsupported source camera context must be rejected");
    };
    auto changed=camera;changed.oriented=false;check(changed);
    changed=camera;changed.horizontal_field_of_view=false;check(changed);
    changed=camera;changed.projection_vertical_angle.reset();check(changed);
    changed=camera;changed.projection_clip_scale.x=1;check(changed);
    changed=camera;changed.projection_clip_offset.x=1;check(changed);
    changed=camera;changed.position.x=std::numeric_limits<float>::infinity();check(changed);
}
}
int main(int argc,char **argv) {
    try {
        boundary_checks();
        require(argc==3,"mainmenu BSP frame test requires original mainmenu map and joined original fixture");
        if(!std::filesystem::exists(argv[1]) || !std::filesystem::exists(argv[2]))return 77;
        Cache cache(argv[1]);MainmenuBspFrameProducer producer(cache,0);
        const auto atmosphere=cache.load_bsp_atmosphere(0);
        SourcePortalMaskState initialized;initialize_mainmenu_portal_state(initialized,cache,0);
        rejected([&]{MainmenuBspFrameProducer invalid(cache,cache.bsps().size());},
            "unsupported BSP index must be rejected");
        std::ifstream fixture(argv[2],std::ios::binary);
        std::array<char,8> magic{};fixture.read(magic.data(),magic.size());
        require(magic==std::array<char,8>{'H','2','M','B','F','0','0','1'},"unknown original mainmenu frame fixture schema");
        const auto count=read_word(fixture);
        require(count>=2996 && count<10000,"joined original mainmenu frame coverage is incomplete or invalid");
        unsigned inside=0,outside=0,cinematic_contexts=0;
        for(unsigned index=0;index<count;++index) {
            const auto size=read_word(fixture);
            require(size>=271 && size<100000,"invalid original mainmenu frame record size");
            std::vector<std::uint32_t> record(size);
            for(auto &word:record)word=read_word(fixture);
            Words data{record};Camera camera;
            camera.position=data.vector();camera.forward=data.vector();camera.up=data.vector();
            camera.projection_vertical_angle=data.scalar();
            camera.horizontal_field_of_view=true;camera.oriented=true;
            camera.projection_clip_scale={544.f/640.f,408.f/480.f};
            const auto previous=data.location();SourcePortalMaskState portals;
            for(auto &word:portals.words)word=data.take();
            const SourceBspShaderLodAuxiliary cinematic{bool(data.take()),bool(data.take())};
            const auto override_present=data.take(),override_word=data.take();
            const std::optional<std::uint32_t> flags=override_present?std::optional(override_word):std::nullopt;
            const auto expected_current=data.location(),expected_retained=data.location();
            const bool success=data.take(),modifier=data.take();
            const auto fog=data.take(72),render=data.take(48),clipping=data.take(111);
            const auto expected_visibility=data.take(data.take());
            const auto expected_admission=data.take(data.take());
            require(data.at==record.size(),"trailing original mainmenu frame record words");
            const auto name="original mainmenu frame case "+std::to_string(index);
            const auto location=mainmenu_camera_location(atmosphere,camera.position,previous);
            require(equal(location.current,expected_current) &&
                equal(location.retained,expected_retained) && location.query_succeeded==success,
                name+" current/retained location differs");
            if(index==0) {
                require(portals.words==initialized.words,"actual source portal initializer differs from joined original input");
                camera_guards(producer,camera,portals);
            }
            if(!success || expected_current.cluster<0) {
                rejected([&]{producer.prepare({camera,previous,portals,cinematic,flags});},
                    "outside original camera must remain an explicit unsupported producer boundary");
                ++outside;continue;
            }
            const auto frame=producer.prepare({camera,previous,portals,cinematic,flags});
            require(equal(frame.current_location,expected_current) &&
                equal(frame.retained_location,expected_retained) && frame.location_query_succeeded==success,
                name+" produced location differs");
            require(frame.fog.cluster==expected_current.cluster,name+" aggregate fog uses a different current cluster");
            require(frame.view_modifier==modifier,name+" actual source view modifier differs");
            require(frame.cinematic.present==cinematic.present && frame.cinematic.byte5==cinematic.byte5,
                name+" actual cinematic pointer/byte state was changed");
            compare_fog(frame.fog,fog,name+" fog");
            compare_object(frame.render_camera,render,name+" render camera");
            compare_object(frame.clipping_camera,clipping,name+" clipping camera");
            compare(visibility_words(frame.visibility),expected_visibility,name+" visibility");
            compare(admission_words(frame.admission),expected_admission,name+" BSP admission");
            const auto shader=frame.shader_view();
            require(&shader.camera==&frame.render_camera && &shader.admission==&frame.admission &&
                shader.outer_top==0 && shader.outer_bottom==480 && shader.local_players==1 &&
                shader.view_modifier==modifier,name+" resident shader-view handoff differs");
            cinematic_contexts|=1u<<(cinematic.present?(cinematic.byte5?2:1):0);
            ++inside;
        }
        require(fixture.peek()==std::char_traits<char>::eof(),"trailing original mainmenu frame fixture bytes");
        require(inside>=2996,"joined original inside-camera coverage is incomplete");
        require(outside>=4,"joined original outside-history control coverage is incomplete");
        require(cinematic_contexts==7,"joined original cinematic pointer/byte controls are incomplete");
        std::cout<<inside<<" joined original inside camera/fog/visibility/BSP banks and "<<outside
            <<" original outside-history rejection controls match; all three cinematic contexts checked\n";
        return 0;
    }catch(const std::exception &error){std::cerr<<error.what()<<'\n';return 1;}
}
