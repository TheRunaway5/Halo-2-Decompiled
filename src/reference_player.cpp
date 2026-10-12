#include "halo2/reference_player.hpp"
#include <algorithm>
#include <bit>
#include <charconv>
#include <cmath>
#include <fstream>
#include <sstream>
#include <set>

namespace halo2 {
namespace {
std::uint32_t hexadecimal(std::istream &in) {
    std::string text;if(!(in>>text))throw FormatError("truncated reference player word");
    if(text.starts_with("0x") || text.starts_with("0X"))text.erase(0,2);
    if(text.empty() || text.size()>8)throw FormatError("invalid reference player word");
    std::uint32_t value{};const auto result=std::from_chars(text.data(),text.data()+text.size(),value,16);
    if(result.ec!=std::errc{} || result.ptr!=text.data()+text.size())throw FormatError("invalid reference player hexadecimal word");
    return value;
}
void field(std::istream &in,const char *expected) {
    std::string actual;if(!(in>>actual) || actual!=expected)throw FormatError(std::string("reference player field missing: ")+expected);
}
float real(std::uint32_t word) {
    const auto value=std::bit_cast<float>(word);
    if(!std::isfinite(value))throw FormatError("nonfinite reference player float");
    return value;
}
Vec3 vector(std::span<const std::uint32_t> words) {return {real(words[0]),real(words[1]),real(words[2])};}
void sha256(std::istream &in,std::string &value) {
    if(!(in>>value) || value.size()!=64 || !std::all_of(value.begin(),value.end(),[](char c){return(c>='0'&&c<='9')||(c>='a'&&c<='f');}))
        throw FormatError("invalid reference player source hash");
}
template<std::size_t N> void words(std::istream &in,std::array<std::uint32_t,N> &out) {for(auto &word:out)word=hexadecimal(in);}
unsigned decimal(std::istream &in,unsigned maximum) {
    std::string text;if(!(in>>text))throw FormatError("truncated reference player integer");
    unsigned value{};const auto r=std::from_chars(text.data(),text.data()+text.size(),value);
    if(r.ec!=std::errc{} || r.ptr!=text.data()+text.size() || value>maximum)
        throw FormatError("invalid reference player integer");
    return value;
}
} // namespace

ReferencePlayerState load_reference_player_state(const std::filesystem::path &path) {
    std::ifstream input(path,std::ios::binary|std::ios::ate);
    if(!input)throw FormatError("reference player fixture missing");
    const auto size=input.tellg();if(size<0 || size>16384)throw FormatError("reference player fixture size exceeds limit");
    std::string text(std::size_t(size),'\0');input.seekg(0);input.read(text.data(),std::streamsize(text.size()));
    if(!input)throw FormatError("reference player fixture read failed");
    std::istringstream in(text);ReferencePlayerState state;
    field(in,"H2REFERENCE_PLAYER");std::string version;in>>version;
    if(version!="1" && version!="2" && version!="3" && version!="4")throw FormatError("unsupported reference player fixture version");
    field(in,"map");in>>state.map;
    if(state.map.empty() || state.map.size()>64 || !std::all_of(state.map.begin(),state.map.end(),[](char c){return(c>='a'&&c<='z')||(c>='0'&&c<='9')||c=='_';}))throw FormatError("invalid reference player map identity");
    field(in,"map_sha256");sha256(in,state.map_sha256);
    field(in,"executable_sha256");sha256(in,state.executable_sha256);
    field(in,"pose_frame");std::string frame;in>>frame;
    const auto number=std::from_chars(frame.data(),frame.data()+frame.size(),state.pose_frame);
    if(number.ec!=std::errc{} || number.ptr!=frame.data()+frame.size() || state.pose_frame>10000)throw FormatError("invalid reference player pose frame");
    field(in,"unit");state.unit_datum=hexadecimal(in);state.unit_definition=hexadecimal(in);
    field(in,"weapon");state.weapon_datum=hexadecimal(in);state.weapon_definition=hexadecimal(in);
    for(auto datum:{state.unit_datum,state.unit_definition,state.weapon_datum,state.weapon_definition})
        if((datum&0xffffu)==0xffffu)throw FormatError("reference player object or tag datum is NONE");
    std::array<std::uint32_t,11> camera{};field(in,"camera");words(in,camera);
    state.camera_position=vector(std::span(camera).subspan(0,3));state.camera_forward=vector(std::span(camera).subspan(3,3));state.camera_up=vector(std::span(camera).subspan(6,3));
    state.camera_vertical_angle=real(camera[9]);state.camera_scale=real(camera[10]);
    const auto norm=[](Vec3 v){return(v.x*v.x+v.y*v.y)+v.z*v.z;};
    if(norm(state.camera_forward)<.0001f || norm(state.camera_up)<.0001f || state.camera_vertical_angle<=0 || state.camera_vertical_angle>=3.14159265358979323846f || state.camera_scale<=0)throw FormatError("invalid reference player camera");
    std::array<std::uint32_t,6> root{};field(in,"root");words(in,root);
    state.root_position=vector(std::span(root).subspan(0,3));state.root_velocity=vector(std::span(root).subspan(3,3));
    if(version=="3" || version=="4") {
        std::array<std::uint32_t,4> sphere{};field(in,"owner_sphere");words(in,sphere);
        state.owner_sphere=ShadowCasterSphere{vector(std::span(sphere).subspan(0,3)),real(sphere[3])};
        if(state.owner_sphere->radius<=0)throw FormatError("reference owner sphere has nonpositive radius");
    }
    field(in,"object_rgb");words(in,state.object_color_words);
    for(auto word:state.object_color_words)if(const auto c=real(word);c<0 || c>1)throw FormatError("reference player object color outside unit range");
    field(in,"object_packed");words(in,state.packed_colors);
    constexpr float scale=1.f/255.f;
    for(unsigned i=0;i<4;++i) {
        const auto p=state.packed_colors[i];if(p&0xff000000u)throw FormatError("reference player packed color has nonzero high byte");
        if(pack_object_lighting_rgb(vector(std::span(state.object_color_words).subspan(i*3,3)))!=p)
            throw FormatError("reference player packed and runtime colors disagree");
        state.colors[i]={float((p>>16)&255)*scale,float((p>>8)&255)*scale,float(p&255)*scale};
    }
    field(in,"lighting");words(in,state.lighting_words);
    const auto light=std::span(state.lighting_words);
    state.root_lighting={vector(light.subspan(0,3)),vector(light.subspan(3,3)),real(light[6]),real(light[7]),vector(light.subspan(8,3)),vector(light.subspan(11,3)),vector(light.subspan(14,3)),vector(light.subspan(17,3))};
    field(in,"draw_light_directions");words(in,state.draw_light_direction_words);
    for(unsigned i=0;i<3;++i)
        if(state.draw_light_direction_words[i]!=state.lighting_words[11+i] ||
           state.draw_light_direction_words[3+i]!=state.lighting_words[17+i])
            throw FormatError("reference player lighting draw and retained record disagree");
    state.first_person_lighting=state.root_lighting;
    state.first_person_lighting.primary_direction=vector(std::span(state.draw_light_direction_words).subspan(0,3));
    state.first_person_lighting.secondary_direction=vector(std::span(state.draw_light_direction_words).subspan(3,3));
    field(in,"retained_context_brightness");state.retained_context_specular_brightness=real(hexadecimal(in));state.retained_context_ambient_brightness=real(hexadecimal(in));
    field(in,"aiming");state.aiming_word=hexadecimal(in);
    const auto aim=state.aiming_word;state.aiming={std::bit_cast<std::int8_t>(std::uint8_t(aim)),std::bit_cast<std::int8_t>(std::uint8_t(aim>>8)),std::bit_cast<std::int8_t>(std::uint8_t(aim>>16)),std::uint8_t(aim>>24)};
    if(state.aiming.count<0 || state.aiming.count>126 || state.aiming.value<0 || state.aiming.value>state.aiming.count || state.aiming.curve<0 || state.aiming.curve>4 || state.aiming.flags>3)throw FormatError("invalid reference player aiming counter");
    if(version!="1") {
        field(in,"devices");const auto count=decimal(in,64);
        std::set<unsigned> placements,handles;
        for(unsigned i=0;i<count;++i) {
            field(in,"device");ReferenceDeviceState device;
            device.source_object_datum=hexadecimal(in);device.definition=hexadecimal(in);device.render_model=hexadecimal(in);
            device.object_type=decimal(in,11);device.scenario_placement=decimal(in,65534);
            device.source_object_flags=hexadecimal(in);words(in,device.words);
            if(device.object_type!=7 || (device.source_object_datum&0xffffu)==0xffffu ||
                (device.definition&0xffffu)==0xffffu || (device.render_model&0xffffu)==0xffffu ||
                !placements.insert(device.scenario_placement).second || !handles.insert(device.source_object_datum).second)
                throw FormatError("invalid or duplicate reference device identity");
            const auto values=std::span(device.words);
            device.position=real(values[0]);device.power=real(values[1]);
            device.root_position=vector(values.subspan(2,3));device.root_forward=vector(values.subspan(5,3));
            device.root_up=vector(values.subspan(8,3));device.root_scale=real(values[11]);
            const auto f=norm(device.root_forward),u=norm(device.root_up);
            const auto dot=(device.root_forward.x*device.root_up.x+device.root_forward.y*device.root_up.y)+device.root_forward.z*device.root_up.z;
            if(device.position<0 || device.position>=1 || device.power<0 || device.power>1 || device.root_scale<=0 ||
                !std::isfinite(f) || !std::isfinite(u) || !std::isfinite(f*u-dot*dot) || f*u-dot*dot<.0001f)
                throw FormatError("invalid reference device channel or root transform");
            state.devices.push_back(device);
        }
    }
    if(version=="4") {
        SourceViewClearState clear;field(in,"view_clear_header");std::string raw;in>>raw;
        if(raw.size()!=clear.header.size()*2)throw FormatError("reference clear header size differs");
        for(std::size_t i=0;i<clear.header.size();++i) {
            unsigned byte{};const auto start=raw.data()+i*2;
            const auto parsed=std::from_chars(start,start+2,byte,16);
            if(parsed.ec!=std::errc{} || parsed.ptr!=start+2)throw FormatError("invalid reference clear header byte");
            clear.header[i]=std::byte(byte);
        }
        field(in,"view_clear_transition");const auto transition=hexadecimal(in);
        field(in,"view_clear_x87");const auto control=hexadecimal(in);
        if(transition>255 || control>65535)throw FormatError("invalid reference clear context");
        clear.transition_byte=std::uint8_t(transition);clear.x87_control_word=std::uint16_t(control);
        const auto events=source_view_initialization_plan(clear.header,clear.transition_byte,
            clear.x87_control_word,{0,0,640,480,0,0x3f800000});
        // Color-only/no-clear needs original prior attachment history. A
        // target18 or temporary viewport also requires its actual resource
        // service; reject these before this fixture reaches a renderer.
        if(events.size()!=2 || events[0].words!=std::array<std::uint32_t,6>{0,1,0,0,0,0} ||
           events[1].kind!=SourceClearEvent::Kind::clear || events[1].words[0]!=0xf3)
            throw FormatError("reference clear needs unsupported source resource or prior history");
        state.view_clear=clear;
    }
    std::string trailing;if(in>>trailing)throw FormatError("trailing reference player fixture fields");
    return state;
}
} // namespace halo2
