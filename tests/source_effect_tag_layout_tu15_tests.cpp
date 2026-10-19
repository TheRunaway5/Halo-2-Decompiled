#include "halo2/source_effect_tag_layout_tu15.hpp"
#include "halo2/cache.hpp"
#include <array>
#include <iostream>

namespace {using namespace halo2;
void check(bool value,const char* why){if(!value)throw FormatError(why);}
void put(std::span<std::byte> bytes,unsigned at,std::uint32_t value,unsigned size=4) {
    for(unsigned n=0;n<size;++n)bytes[at+n]=std::byte(value>>(n*8u));
}
struct Fixture {
    Tag definition{"cont","retained authored effect",0xe1230042u,0x120000u,0xf0};
    Tag bitmap{"bitm","retained authored bitmap",0xe4560081u,0x140000u,0x4c};
    std::array<std::byte,0xf0> root{};
    std::array<std::byte,0x4c> image{};
    std::array<std::byte,128> states{};
    Fixture() {
        put(root,0x18,4,2);put(root,0x30,tag_groups::bitmap.value);
        put(root,0x34,bitmap.datum);put(root,0x60,1,2);put(root,0x64,1,2);
        put(root,0x66,3,2);put(root,0x68,1,2);put(root,0x6a,0x1234,2);
        put(root,0x8c,0xffffffffu);put(root,0xe8,2);put(root,0xec,0x160000u);
        put(image,0x44,7);
        for(unsigned n=0;n<states.size();++n)states[n]=std::byte(n*17u);
    }
    auto input()const{return SourceEffectTagInputTu15{definition,root,bitmap,image};}
};
void portable() {
    Fixture f;const auto root=f.root;const auto image=f.image;const auto states=f.states;
    const auto q=qualify_source_contrail_tag_layout_tu15(f.input(),f.states);
    check(q.definition==f.definition.handle() && q.bitmap==f.bitmap.handle() &&
        q.render_type==4 && q.properties==1 && q.blend==3 && q.fade==1 &&
        q.sampler_flags==0x1234 && q.bitmap_images==7 && q.point_states.count==2 &&
        q.point_states.address==0x160000u,"Literal contrail fields were moved or rewritten");
    check(f.root==root && f.image==image && f.states==states,"Qualifier mutated source banks");
    const auto reject=[&](auto change) {
        Fixture bad;change(bad);const auto before=bad.root;const auto bitmap=bad.image;const auto points=bad.states;
        bool failed=false;try{(void)qualify_source_contrail_tag_layout_tu15(bad.input(),bad.states);}
        catch(const FormatError&){failed=true;}
        check(failed,"Malformed or differently owned contrail was admitted");
        check(bad.root==before && bad.image==bitmap && bad.states==points,"Rejected qualifier mutated inputs");
    };
    reject([](auto&v){v.definition.datum=0xffffffffu;});
    reject([](auto&v){v.definition.group="deca";});
    reject([](auto&v){--v.definition.size;});
    reject([](auto&v){v.definition.address=0xffffff80u;});
    reject([](auto&v){v.bitmap.datum^=0x10000u;});
    reject([](auto&v){v.bitmap.group="shad";});
    reject([](auto&v){put(v.root,0x30,tag_groups::decal.value);});
    reject([](auto&v){put(v.root,0x60,0,2);});
    reject([](auto&v){put(v.root,0x8c,v.bitmap.datum);});
    reject([](auto&v){put(v.root,0x18,6,2);});
    reject([](auto&v){put(v.root,0x66,11,2);});
    reject([](auto&v){put(v.root,0x68,3,2);});
    reject([](auto&v){put(v.root,0xe8,3);});
    reject([](auto&v){put(v.root,0xe8,0xffffffffu);});
    reject([](auto&v){put(v.root,0xec,0);});
    reject([](auto&v){put(v.root,0xec,0xfffffff0u);});
    reject([](auto&v){put(v.image,0x44,0);});
    // Literal original layer6 is an unsupported error path, not an alias for5.
    f.definition.group="deca";f.definition.size=0xac;
    put(f.root,4,5,2);put(f.root,8,tag_groups::decal.value);put(f.root,0xc,0xe7890043u);
    put(f.root,0x88,tag_groups::bitmap.value);put(f.root,0x8c,f.bitmap.datum);
    const auto input=SourceEffectTagInputTu15{f.definition,std::span(f.root).first(0xac),f.bitmap,f.image};
    const auto d=qualify_source_decal_tag_layout_tu15(input);
    check(d.layer==5 && d.next_decal.value==0xe7890043u && d.bitmap==f.bitmap.handle() &&
        d.bitmap_images==7,"Literal decal fields or full chain salt changed");
    for(unsigned layer:{6u,0xffffu}) {
        put(f.root,4,layer,2);bool failed=false;
        try{(void)qualify_source_decal_tag_layout_tu15(input);}catch(const FormatError&){failed=true;}
        check(failed,"Unsupported decal layer was admitted");
    }
    put(f.root,4,5,2);put(f.root,8,tag_groups::bitmap.value);
    bool chain_failed=false;try{(void)qualify_source_decal_tag_layout_tu15(input);}
    catch(const FormatError&){chain_failed=true;}check(chain_failed,"A non-decal chain owner was admitted");
    put(f.root,8,tag_groups::decal.value);put(f.image,0x44,129);
    bool image_failed=false;try{(void)qualify_source_decal_tag_layout_tu15(input);}
    catch(const FormatError&){image_failed=true;}check(image_failed,"Decal retained image-index overflow was admitted");
    std::array<std::byte,436> fake{};const std::uint16_t declaration[]{0,3,9,14};
    bool failed=false;try{qualify_source_effect_vertex_program_tu15(tag_groups::contrail,0,fake,declaration);}
    catch(const FormatError&){failed=true;}check(failed,"A PC/fabricated program was accepted as original code");
}
}
int main(int argc,char**)try {
    if(argc!=1)return 2;portable();
    std::cout<<"PASS portable effect tag layout qualification; no Cache, World or source publication\n";return 0;
}catch(const std::exception& e){std::cerr<<e.what()<<'\n';return 1;}
