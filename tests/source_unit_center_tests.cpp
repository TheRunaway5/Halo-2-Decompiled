#include "halo2/source_unit_center.hpp"
#include <bit>
#include <cfenv>
#include <fstream>
#include <iostream>
#include <sstream>
#include <limits>
#if defined(__i386__) || defined(__x86_64__) || defined(_M_IX86) || defined(_M_X64)
#include <xmmintrin.h>
#endif

using namespace halo2;
namespace {
using Words=std::vector<std::uint32_t>;
using Curve=std::array<std::uint8_t,1024>;
void require(bool value,const char *message){if(!value)throw FormatError(message);}
std::uint32_t bits(float f){return std::bit_cast<std::uint32_t>(f);}
void append(Words &w,Vec3 v){w.insert(w.end(),{bits(v.x),bits(v.y),bits(v.z)});}
Words run(const Words &in,const Curve &curve) {
    unsigned at=0;
    const auto word=[&](){return in.at(at++);};
    const auto scalar=[&](){return std::bit_cast<float>(word());};
    const auto vector=[&](){const auto x=scalar(),y=scalar(),z=scalar();return Vec3{x,y,z};};
    const auto mode=word();require(mode<=5,"invalid unit center mode");
    SourceUnitEyeState u;u.object=0x10001;
    u.parent=word();u.related=word();u.player=word();u.type=std::uint8_t(word());
    u.flags10a=std::uint8_t(word());u.state3dc=std::uint8_t(word());u.flag3f8=std::uint8_t(word());
    u.flags348=std::uint16_t(word());u.seat=std::int16_t(word());u.scale=scalar();u.crouch=scalar();
    u.forward=vector();u.up=vector();u.node_zero_position=vector();u.head.initialized=std::uint8_t(word());u.head.local=vector();
    auto &d=u.definition;d.standing_height=scalar();d.crouching_height=scalar();
    d.pitch_minimum=scalar();d.pitch_maximum=scalar();d.correction_scale=vector();d.minimum_forward=scalar();
    const auto root=vector(),attachment=vector(),position=vector();
    SourceUnitEyeState related,parent;
    related.seat=std::int16_t(word());parent.type=std::uint8_t(word());
    const SourceUnitEyeSeat seat{word(),word()};
    const std::array seats{seat};d.seats=seats;parent.definition.seats=seats;
    SourceUnitEyeMarker primary,fallback;
    primary.count=std::uint16_t(word());primary.position=vector();
    fallback.count=std::uint16_t(word());fallback.position=vector();
    for(auto &c:u.offset_channels){c.datum=word();c.index=std::int16_t(word());c.state=std::uint8_t(word());c.flags=std::uint8_t(word());}
    const auto aim=vector();
    std::array<SourceUnitEyeContact,2> contacts;
    for(auto &c:contacts){c.hit=word()!=0;c.fraction=scalar();c.point=vector();}
    require(at==in.size(),"unit center input layout drifted");
    Words trace;unsigned contact=0;
    SourceUnitEyeServices s;
    s.object=[&](std::uint32_t owner)->const SourceUnitEyeState&{
        trace.insert(trace.end(),{3,owner});
        require(owner==0x10002 || owner==0x10003,"unexpected eye lookup");
        return owner==0x10002?parent:related;
    };
    s.root=[&](auto owner){trace.insert(trace.end(),{1,owner});return root;};
    s.attachment_position=[&](auto owner){trace.insert(trace.end(),{2,owner});return attachment;};
    s.marker=[&](auto owner,auto name){trace.insert(trace.end(),{4,owner,name});
        return mode==2 && name==0x04000095u?fallback:primary;};
    s.clamp_aim=[&](auto owner,Vec3 value){trace.insert(trace.end(),{6,owner});append(trace,value);return aim;};
    s.contact=[&](Vec3 origin,Vec3 delta){trace.push_back(7);append(trace,origin);append(trace,delta);
        require(contact<2,"extra eye contact");return contacts[contact++];};
    Words out;
    if(mode==0){const auto b=source_unit_horizontal_basis(u.forward,u.up);
        out={bits(b.forward.x),bits(b.forward.y),bits(b.left.x),bits(b.left.y)};}
    else if(mode==1)append(out,source_unit_perpendicular_up(u.forward));
    else if(mode==2){source_update_unit_head_offset(u,s);out.push_back(u.head.initialized);append(out,u.head.local);}
    else if(mode==3)append(out,source_unit_observer_position(u,position,u.forward,u.up,curve));
    else if(mode==4)append(out,source_unit_center(u,s,curve));
    else out.push_back(source_unit_head_offset_suppressed(u.offset_channels));
    out.push_back(std::uint32_t(trace.size()));out.insert(out.end(),trace.begin(),trace.end());return out;
}
Curve read_curve(std::istream &stream) {
    std::string line;
    while(std::getline(stream,line)) {
        if(line.empty() || line.front()=='#')continue;
        std::istringstream row(line);std::string tag;row>>tag;require(tag=="curve","missing unit center curve");
        Curve curve;std::uint32_t value;
        for(unsigned i=0;i<256;++i){require(bool(row>>std::hex>>value),"truncated unit center curve");
            for(unsigned b=0;b<4;++b)curve[4*i+b]=std::uint8_t(value>>(8*b));}
        require(!(row>>tag),"extra unit center curve data");return curve;
    }
    throw FormatError("missing unit center curve");
}
Words words(std::string_view text){Words result;std::istringstream row{std::string(text)};
    std::uint32_t value;while(row>>std::hex>>value)result.push_back(value);require(row.eof(),"invalid unit center words");return result;}
}
int main(int argc,char **argv){try {
    if(argc>=2 && (std::string_view(argv[1])=="--probe" || std::string_view(argv[1])=="--fixture")) {
        const bool fixture=std::string_view(argv[1])=="--fixture";
        std::ifstream file;if(fixture){require(argc==3,"missing unit center fixture path");file.open(argv[2]);require(bool(file),"missing unit center fixture");}
        auto &stream=fixture?static_cast<std::istream&>(file):std::cin;const auto curve=read_curve(stream);
        std::string line;unsigned count=0;
        while(std::getline(stream,line)) {
            if(line.empty() || line.front()=='#')continue;
            const auto split=line.find('|');require(!fixture || split!=std::string::npos,"missing unit center expectation");
            const auto result=run(words(std::string_view(line).substr(0,split)),curve);
            if(fixture)require(result==words(std::string_view(line).substr(split+1)),"original unit center fixture mismatch");
            else {for(auto value:result)std::cout<<std::hex<<value<<' ';std::cout<<'\n';}
            ++count;
        }
        require(count>0,"empty unit center input");if(fixture)std::cout<<count<<" original unit center cases matched\n";return 0;
    }
    SourceUnitEyeState u;SourceUnitEyeServices s;Curve curve{};
    source_update_unit_head_offset(u,s); // NONE player must not request services.
    bool rejected=false;try{source_unit_center(u,s,curve);}catch(const FormatError&){rejected=true;}
    require(rejected,"missing unit root service accepted");
    rejected=false;try{source_unit_horizontal_basis({std::numeric_limits<float>::quiet_NaN(),0,0},{});}
    catch(const FormatError&){rejected=true;}require(rejected,"nonfinite source basis accepted");
    const auto previous=std::fegetround();std::fesetround(FE_DOWNWARD);rejected=false;
    try{source_unit_horizontal_basis({1,0,0},{});}catch(const FormatError&){rejected=true;}
    std::fesetround(previous);require(rejected,"wrong rounding mode accepted");
#if defined(__i386__) || defined(__x86_64__) || defined(_M_IX86) || defined(_M_X64)
    const auto mxcsr=_mm_getcsr();_mm_setcsr(mxcsr|0x8000u);rejected=false;
    try{source_unit_horizontal_basis({1,0,0},{});}catch(const FormatError&){rejected=true;}
    _mm_setcsr(mxcsr);require(rejected,"FTZ mode accepted");
    _mm_setcsr((mxcsr&~0x6000u)|0x4000u);rejected=false;
    try{source_unit_horizontal_basis({1,0,0},{});}catch(const FormatError&){rejected=true;}
    _mm_setcsr(mxcsr);require(rejected,"independent SSE rounding mode accepted");
#endif
#if (defined(__i386__) || defined(__x86_64__)) && !defined(_MSC_VER)
    std::uint16_t original_cw{},after_cw{};asm volatile("fnstcw %0":"=m"(original_cw));
    u.definition.pitch_minimum=.1f;u.definition.pitch_maximum=1.4f;
    source_unit_observer_position(u,{0,0,1},{.8f,0,-.6f},{.6f,0,.8f},curve);
    asm volatile("fnstcw %0":"=m"(after_cw));require(original_cw==after_cw,"unit pitch changed caller x87 control");
#endif
    u.parent=0x10002;u.seat=1;s.attachment_position=[](auto){return Vec3{};};
    s.object=[&](auto)->const SourceUnitEyeState&{return u;};rejected=false;
    try{source_unit_center(u,s,curve);}catch(const FormatError&){rejected=true;}require(rejected,"invalid seat accepted");
    std::cout<<"unit center service, finite operand, rounding and seat guards passed\n";return 0;
}catch(const std::exception &e){std::cerr<<e.what()<<'\n';return 1;}}
