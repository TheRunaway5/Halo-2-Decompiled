#include "halo2/native_tu15_world_memory.hpp"
#include "halo2/native_tu15_renderer_startup_data.hpp"
#include "halo2/native_tu15_physics_branch_data.hpp"
#include "halo2/physics_live_support_tu15.hpp"
#include "halo2/source_cinematic_shadow_control_tu15.hpp"
#include <algorithm>
#include <iostream>
#include <stdexcept>
#include <ranges>
#include <tuple>
#include <string_view>

namespace {
using namespace halo2;
#include "fixtures/native_tu15_vehicle_render_operands.original.inc"
#include "fixtures/native_tu15_vehicle_read_operands.original.inc"
#include "fixtures/native_tu15_vehicle_closure_operands.original.inc"
void check(bool condition,const char* message) {
    if(!condition)throw std::runtime_error(message);
}
std::uint32_t word(const SourceRuntimeMemory& memory,std::uint32_t address) {
    std::uint32_t value=0;const auto bytes=memory.view(address,4);
    for(unsigned i=0;i<4;++i)value|=std::to_integer<std::uint32_t>(bytes[i])<<(i*8);
    return value;
}
}
int main(int argc,char** argv)try {
    constexpr std::uint32_t tick=0x60212000u;
    SourceRuntimeKernelTickCount input{tick,{std::byte{0x78},std::byte{0x56},std::byte{0x34},std::byte{0x12}},
        "world memory test retained clock"};
    auto memory=native_tu15_world_memory(input);
    // Independently authenticated original45F290..45F2BF, covering all24
    // bitmap formats admitted by138780's original selector bound.
    std::uint64_t bitmap_depth_hash=14695981039346656037ull;
    check(memory.is_read_only(0x45f290u,48),"Complete bitmap bit-depth table is not immutable");
    for(const auto byte:memory.view(0x45f290u,48))
        bitmap_depth_hash=(bitmap_depth_hash^std::to_integer<unsigned char>(byte))*1099511628211ull;
    check(bitmap_depth_hash==0x30844cf11e590c09ull,"Bitmap bit-depth inputs differ from all24 original formats");
    for(const auto offset:{0u,22u,47u}) {
        SourceRuntimeMemory foreign;
        foreign.mount(0x45f290u+offset,std::array<std::byte,1>{},
            SourceRuntimeMemory::Access::ReadWrite,"foreign bitmap table overlap");
        const auto before=foreign.checkpoint();bool rejected=false;
        try{mount_native_tu15_renderer_startup_data(foreign);}
        catch(const std::invalid_argument&){rejected=true;}
        check(rejected&&foreign.checkpoint()==before,"Bitmap table overlap caused partial startup publication");
    }
    // Independent original fixture e926ad8121e9e0f0... authenticates these
    // 132 bytes; test actual cold composition, not only the standalone mount.
    // Table order is the existing draw reader's blend_operands[0..2].
    std::uint64_t contrail_hash=14695981039346656037ull;
    for(const auto address:{0x45c438u,0x45c408u,0x45c3d8u}) {
        check(memory.is_read_only(address,44),"Cold contrail blend data lacks its exact immutable owner");
        for(const auto byte:memory.view(address,44))
            contrail_hash=(contrail_hash^std::to_integer<unsigned char>(byte))*1099511628211ull;
    }
    check(contrail_hash==0xcaf5ef873c341710ull,"Cold contrail blend data differs from33 original words");
    // Independent original geometry-dispatch fixture/proof, not blend state.
    constexpr std::array geometry_targets{0x430ceu,0x431c8u,0x431c8u,0x436dfu,0x432bdu};
    check(memory.is_read_only(0x436f0u,20),"Cold contrail geometry dispatch lacks immutable coverage");
    for(unsigned n=0;n<geometry_targets.size();++n)
        check(word(memory,0x436f0u+n*4u)==geometry_targets[n],"Cold contrail geometry dispatch differs from five original words");
    for(const auto&[address,bytes,writable]:vehicle_closure_operands) {
        std::vector<std::byte> actual(bytes.size());memory.read(address,actual);
        check(std::ranges::equal(actual,std::as_bytes(bytes)),"Original vehicle closure input bytes differ");
        for(unsigned n=0;n<bytes.size();++n)check(memory.is_read_only(address+n,1)!=writable,
            "Original vehicle closure input lost section access");
    }
    check(word(memory,0x1d574u)==0x1d4fdu && word(memory,0x1d578u)==0x1d4f6u &&
        word(memory,0x138be4u)==0x13887cu,"Original raw CPU bitmap dispatch operands differ");
    check(memory.is_read_only(0x1d574u,8)&&memory.is_read_only(0x1d59eu,2)&&
        memory.is_read_only(0x45f2a4u,2)&&memory.is_read_only(0x138be4u,4)&&
        !memory.is_read_only(0x40db36u,2),"Original raw CPU bitmap section access differs");
    check(memory.view(0x1d59eu,2)[0]==std::byte{5}&&memory.view(0x1d59eu,2)[1]==std::byte{6}&&
        memory.view(0x40db36u,2)[0]==std::byte{0xa1}&&memory.view(0x40db36u,2)[1]==std::byte{0xa1}&&
        memory.view(0x45f2a4u,2)[0]==std::byte{32}&&memory.view(0x45f2a4u,2)[1]==std::byte{},
        "Original raw CPU bitmap literal selector/flags/bit depth differs");
    check(std::ranges::all_of(memory.view(0x505e84u,8),[](auto b){return b==std::byte{};})&&
        std::ranges::all_of(memory.view(0x505e90u,0xd0c),[](auto b){return b==std::byte{};}),
        "Cold contrail destinations contain a fabricated completed value");
    check(!memory.is_read_only(0x505e84u,8)&&!memory.is_read_only(0x505e90u,0xd0c)&&
        memory.is_read_only(0x46a2fcu,4)&&word(memory,0x46a2fcu)==0x47e7aaacu,
        "Original contrail startup storage/scale operand differs");
    check(word(memory,0x46a168u)==0x3d8f5c29u&&word(memory,0x46a170u)==0x4b000000u&&
        memory.is_read_only(0x46a168u,4)&&memory.is_read_only(0x46a170u,4),
        "Original contrail interpolation/quantization operands differ");
    check(word(memory,0x4f2d74u)==0&&word(memory,0x4f2d78u)==0&&
        !memory.is_read_only(0x4f2d74u,8),"Original model-cache policy/counter cold fields differ");
    check(memory.view(0x48c754u,1)[0]==std::byte{1}&&!memory.is_read_only(0x48c754u,1),
        "Original130F90 geometry-cache pressure-warning setting lost its authored writable byte");
    check(word(memory,0x411568u)==0&&!memory.is_read_only(0x411568u,4),
        "Original405801 cold SDK device cell lost its loader-zero writable owner");
    constexpr std::array lens_dispatch{0x2f5bau,0x2f2ccu,0x2f3e2u,0x2f3feu,0x2f50au};
    for(unsigned i=0;i<lens_dispatch.size();++i)
        check(word(memory,0x2f5c8u+i*4)==lens_dispatch[i]&&memory.is_read_only(0x2f5c8u+i*4,4),
            "Original2F2C5 lens draw dispatch table lacks its exact immutable targets");
    check(memory.view(0x4c6894u,16u*12u).size()==192&&!memory.is_read_only(0x4c6894u,192)&&
        std::ranges::all_of(memory.view(0x4c6894u,192),[](auto b){return b==std::byte{};}),
        "Original sixteen-entry cinematic shadow destination differs");
    {
        auto shadow=native_tu15_world_memory(input);
        shadow.mount(0x64000000u,std::vector<std::byte>(4096),SourceRuntimeMemory::Access::ReadWrite,"declared shadow-control test stack");
        const auto put=[&](unsigned at,unsigned value){std::array<std::byte,4>bytes{};for(unsigned i=0;i<4;++i)bytes[i]=std::byte(value>>(i*8));shadow.write(at,bytes);};
        const auto call=[&](unsigned object,bool enabled,unsigned marker){
            put(0x64000800u,0x65000000u);put(0x64000804u,0x3ecccccdu);
            SourcePhysicsWorldRuntimeAbi abi{};abi.gp[3]=enabled;abi.gp[6]=object;abi.gp[7]=marker;abi.gp[4]=0x64000800u;abi.eflags=0x202;
            source_run_cinematic_shadow_control_tu15(shadow,0x30e10u,abi,{0x27f,0x1f80},{});
            check(abi.gp[4]==0x64000808u,"Original shadow caller did not return");
        };
        for(unsigned n=0;n<16;++n){call(n+1,true,n+100);const auto at=0x4c6894u+n*12u;
            check(word(shadow,0x4c6954u)==n+1&&word(shadow,at)==n+1&&word(shadow,at+4)==n+100&&word(shadow,at+8)==0x3ecccccdu,
                "Original shadow insertion escaped or changed the cold owner");}
        call(17,true,999);check(word(shadow,0x4c6954u)==16&&word(shadow,0x4c6894u+15*12u)==16,
            "Original shadow capacity guard changed");
        call(1,false,0);check(word(shadow,0x4c6954u)==15&&word(shadow,0x4c6894u)==16,
            "Original shadow removal did not move its actual last entry");
    }
    constexpr std::array vehicle_dispatch{0xf25dfu,0xf25f0u,0xf2649u,0xf2600u,0xf2610u,0xf2625u,0xf2635u};
    check(memory.is_read_only(0xf2bfcu,28),"Original vehicle dispatch table lacks immutable backing");
    for(unsigned i=0;i<vehicle_dispatch.size();++i)check(word(memory,0xf2bfcu+i*4)==vehicle_dispatch[i],
        "Original vehicle dispatch target differs");
    for(const auto&[address,bytes]:vehicle_read_operands) {
        std::vector<std::byte> actual(bytes.size());memory.read(address,actual);
        check(std::ranges::equal(actual,std::as_bytes(bytes)),"Original vehicle literal read bytes differ");
        for(unsigned n=0;n<bytes.size();++n)check(memory.is_read_only(address+n,1),
            "Original vehicle literal read lost immutable ownership");
    }
    for(const auto&[address,bits]:vehicle_render_operands) {
        check(word(memory,address)==bits,"Original vehicle render scalar cold input differs");
        const bool writable=address==0x473e1cu||address==0x473e20u||address==0x473e34u;
        check(memory.is_read_only(address,4)!=writable,"Vehicle operand lost its original section access");
    }

    // Actual180C77/180C82 select row1 in the MCC Cyclotron world. All four
    // fields are offline-verified TU1.5 .rdata, also consumed by182BB0.
    constexpr std::array placement_row1{0x42200000u,0x42dc0000u,0x3fc00000u,1u};
    for(unsigned i=0;i<placement_row1.size();++i)
        check(word(memory,0x44eb40u+i*4)==placement_row1[i],
            "Native original placement selector-one parameters differ");
    check(memory.is_read_only(0x44eb40u,16),"Placement selector-one row became writable");
    const auto dispatch=[&](std::uint32_t address,const auto& expected) {
        for(unsigned i=0;i<expected.size();++i) {
            check(memory.is_read_only(address+i*4,4),
                "Original dispatch table lacks complete immutable ownership");
            check(word(memory,address+i*4)==expected[i],
                "Native dispatch identity differs from the offline TU1.5 table");
        }
    };
    // Actual MCC player/vehicle/lift runs reached these original data tables;
    // constructors, consumers and extents are pinned in v779/v780 audits.
    dispatch(0x461420u,std::array{
        0x24dc20u,0x2e1270u,0x2e12a0u,0x2e9250u,0x2e4d60u,0x31ede0u,
        0x2e8120u,0x2e90e0u,0x2e81e0u,0x2e9140u,0x2e7d10u,0x306bb0u,
        0x2e7d90u,0x3070f0u,0x2e7e10u,0x31d520u});
    dispatch(0x41ca9cu,std::array{
        0x2fc540u,0x2f16f0u,0x2f13f0u,0x2f10b0u,
        0x2f0f10u,0x2f0da0u,0x2e38c0u,0x2f1850u});
    dispatch(0x15cfa8u,std::array{
        0x15cefeu,0x15cf16u,0x15cf2eu,0x15cf46u,0x15cf5eu,0x15cf76u,0x15cf8eu});
    dispatch(0x1e0184u,std::array{
        0x1dfc35u,0x1dfc9cu,0x1dfd9eu,0x1dff3eu,0x1dffb4u,0x1e001cu});
    dispatch(0x41ca8cu,std::array{0x2f0a50u,0x30ba80u});
    dispatch(0x41ca94u,std::array{0x2f0a20u,0x30ba90u});
    dispatch(0x462060u,std::array{
        0x1f2c10u,0x2e1270u,0x2e12a0u,0x306a80u,0x2e4d60u,0x31cae0u,
        0x306de0u,0x2e9400u,0x306bc0u,0x306ed0u,0x306b90u,0x306bb0u});
    dispatch(0x41ea3cu,std::array{
        0x31cea0u,0x2e1270u,0x2e12a0u,0x31e1e0u,0x2e4d60u,0x31e2e0u,
        0x31e2f0u,0x325d80u,0xa3850u,0x31e360u,0x31e330u});
    dispatch(0x41e79cu,std::array{0x31d700u,0x2e4d60u,0x325aa0u,0x2e4d60u});
    dispatch(0x45f8c0u,std::array{0x1848a0u,0x185190u});
    dispatch(0x461f64u,std::array{
        0x24dc20u,0x2e1270u,0x2e12a0u,0x308530u,0x2e4d60u,0x308540u,
        0x308550u,0x2e9400u,0x1f7da0u,0x2e4ea0u,0x1f34d0u,0x1f3550u});
    dispatch(0x41e458u,std::array{
        0x3235c0u,0x3240d0u,0x323e30u,0x323f50u,0x324230u,0x323420u,
        0x31d520u,0x323860u,0x3238a0u,0x323b80u,0x323b80u,0x323bb0u,
        0x323c10u,0x323c50u,0x323c50u,0x3238b0u,0x323900u,0x323950u,
        0x3239a0u,0x323b20u,0x3239f0u,0x323a00u,0x323a10u,0x323a70u,
        0x323c80u,0x323d10u,0x323d80u,0x323d50u,0x323e00u,0x324410u,0x323ab0u});
    // Actual MCC v820 reached31D5A4's slot7 after original31D68A installed
    // this interface. Offline v826 bounds its eleven methods before null.
    dispatch(0x41e910u,std::array{
        0x31cea0u,0x2e1270u,0x2e12a0u,0x31d410u,0x2e4d60u,0x31d510u,
        0x31ede0u,0x31d710u,0x31d520u,0x31d930u,0x31d630u});
    check(word(memory,0x46aae8u)==0x3fd55556u && memory.is_read_only(0x46aae8u,4) &&
        word(memory,0x46a1f4u)==0x3f555555u && memory.is_read_only(0x46a1f4u,4),
        "Original F9083/E892C fixed arithmetic inputs lack their exact readonly bytes");
    check(word(memory,0xe985cu)==0 && memory.is_read_only(0xe985cu,4),
        "Authored null branch identity was replaced or lacks readonly ownership");
    // The live movement failure read byte10 of original115BCF's selector,
    // after real native frames and ticks. Check its actual factory ownership.
    constexpr std::array<unsigned,12> movement_selector{0,0,2,2,2,2,2,2,2,2,1,1};
    for(unsigned i=0;i<movement_selector.size();++i) {
        check(memory.is_read_only(0x115e10u+i,1),"Original movement selector is not immutable");
        check(std::to_integer<unsigned>(memory.view(0x115e10u+i,1).front())==movement_selector[i],
            "Original movement selector byte differs");
    }
    // Partial shared owners must survive, and neither writable bytes nor
    // differing readonly bytes may be accepted or overwritten by the profile.
    for(unsigned fault=0;fault<2;++fault) {
        SourceRuntimeMemory conflicting;
        const std::array value{std::byte(fault?1:0)};
        conflicting.mount(0x115e1au,value,fault?SourceRuntimeMemory::Access::ReadWrite:
            SourceRuntimeMemory::Access::ReadOnly,"isolated conflicting selector owner");
        const auto before=conflicting.checkpoint();bool rejected=false;
        try{mount_native_tu15_physics_branch_data(conflicting);}catch(const std::invalid_argument&){rejected=true;}
        check(rejected && conflicting.checkpoint().regions==before.regions,
            "Physics branch profile changed or admitted a conflicting owner");
    }
    SourceRuntimeMemory partial;
    partial.mount(0x115e1au,std::array{std::byte{1}},SourceRuntimeMemory::Access::ReadOnly,
        "isolated compatible selector owner");
    mount_native_tu15_physics_branch_data(partial);
    const auto retained=partial.checkpoint();
    const auto at=std::ranges::find(retained.regions,0x115e1au,&SourceRuntimeMemory::CheckpointRegion::address);
    check(at!=retained.regions.end() && at->owner=="isolated compatible selector owner" && at->bytes.size()==1,
        "Physics branch profile replaced a compatible partial owner");
    std::array<std::byte,12> selector_read{};partial.read(0x115e10u,selector_read);
    for(unsigned i=0;i<selector_read.size();++i)check(std::to_integer<unsigned>(selector_read[i])==movement_selector[i],
        "Selector read across shared owners differs");
    mount_native_tu15_physics_branch_data(partial);
    check(partial.checkpoint().regions==retained.regions,"Physics branch profile is not idempotent");
    // Exact TU1.5 C3300 class tables, including the transparent opening's
    // class-zero path into C2A80. These are authored operands, not fade results.
    constexpr std::array<std::uint32_t,25> light_fade_tables{
        0x41a00000u,0x41700000u,0x41200000u,0x40a00000u,0u,
        0x41700000u,0x41200000u,0x40e00000u,0x40400000u,0u,
        0x40200000u,0x40000000u,0x3fc00000u,0x3f800000u,0x3f000000u,
        0x40200000u,0x40000000u,0x3fc00000u,0x3f800000u,0x3f000000u,
        0u,0u,0u,0u,0u};
    for(unsigned i=0;i<light_fade_tables.size();++i)
        check(word(memory,0x44a6d8u+i*4)==light_fade_tables[i],
            "Native original effect-light class table differs");
    check(memory.is_read_only(0x44a6d8u,100),"Effect-light authored tables became writable");
    check(word(memory,0x474c80u)==0x44ab1cu&&word(memory,0x44ab1cu)==0u&&
        word(memory,0x44ab20u)==0x3f800000u&&word(memory,0x44ab24u)==0u,
        "Original1FEB9 default screen-vector owner differs");
    check(memory.is_read_only(0x474c80u,4)&&memory.is_read_only(0x44ab1cu,12),
        "Original screen-vector operands became writable");
    constexpr std::array camera_tables{
        std::pair{0x16fd30u,std::array{0x16f6f4u,0x16fa29u,0x16fbf2u,0x16fc79u}},
        std::pair{0x16f33cu,std::array{0x16f247u,0x16f21bu,0x16f29cu,0x16f265u}}};
    for(const auto&[address,words]:camera_tables) {
        check(memory.is_read_only(address,16),"Original camera dispatch table lacks its immutable owner");
        for(unsigned i=0;i<words.size();++i)check(word(memory,address+i*4)==words[i],
            "Original bounded camera dispatch identity differs");
    }
    check(word(memory,0x474c2cu)==0x44aad8u,"Original17C84C lens color record binding differs");
    for(const auto&[address,bits]:std::array{
        std::pair{0x46a250u,0x4b7fffffu},std::pair{0x46a268u,0x3c008081u},
        std::pair{0x46a384u,0x3c800000u},std::pair{0x46a51cu,0x3fb504f3u},
        std::pair{0x46a9b4u,0x4f000000u}})
        check(word(memory,address)==bits&&memory.is_read_only(address,4),
            "Original literal lens-query/draw operand lacks immutable exact-byte backing");
    for(unsigned n=0;n<4;++n)check(word(memory,0x44aad8u+n*4)==0x3f800000u,
        "Original lens RGBA default operand differs");
    check(memory.is_read_only(0x474c2cu,4)&&memory.is_read_only(0x44aad8u,4)&&memory.is_read_only(0x44aadcu,12),
        "Original lens default record lacks immutable ownership");
    check(word(memory,0x41b774)==tick && word(memory,tick)==0x12345678u,
        "Native tick-count data was not bound to its actual owner");
    check(memory.is_read_only(0x41b774,4) && !memory.is_read_only(tick,4),
        "Tick-count binding or volatile data access differs");
    check(memory.view(word(memory,0x474c70u),12).size()==12,
        "Original scale/color vector cannot be read by its complete native owner");
    input.bytes.fill(std::byte{});
    check(word(memory,tick)==0x12345678u,"World memory borrowed expired caller bytes");
    for(const auto cell:{0x56ee28u,0x4f3218u,0x4f3cd8u,0x51df54u,
                         0x5020c4u,0x4f54f0u,0x51df44u,0x515ec0u,
                         0x56ebacu,0x56e684u,0x502088u,0x56b170u})
        check(!word(memory,cell),
            "Cold composition fabricated a completed constructor result");
    for(const auto cell:{0x4f0430u,0x4922a8u,0x48d480u})
        check(memory.view(cell,1).front()==std::byte{},
            "Cold composition fabricated successful initialization");
    bool missing=false;
    try{(void)memory.view(0x10000,4);}catch(const std::out_of_range&){missing=true;}
    check(missing,"Cold composition included an executable header");
    missing=false;
    try{(void)memory.view(0x13a700,4);}catch(const std::out_of_range&){missing=true;}
    check(missing,"Cold composition included executable instructions");
    std::size_t regions=0,bytes=0;
    const auto checkpoint=memory.checkpoint();
    const auto type_owner=std::ranges::find(checkpoint.regions,0x5020b4u,
        &SourceRuntimeMemory::CheckpointRegion::address);
    check(type_owner!=checkpoint.regions.end() && type_owner->bytes.size()==15u &&
        type_owner->access==SourceRuntimeMemory::Access::ReadWrite &&
        type_owner->owner=="native TU1.5 cold complete 1E7690 object-type runtime flags" &&
        !type_owner->page_bytes && type_owner->committed.empty(),
        "Complete native object-type runtime owner is absent or has an invalid schema");
    check(std::ranges::all_of(type_owner->bytes,[](auto value){return value==std::byte{};}),
        "Native object-type runtime flags were warmed before the source writer");
    check(std::ranges::none_of(checkpoint.regions,[](const auto&region){return region.address==0x5020b8u;}),
        "Old one-byte member remained separately mounted inside the complete array");
    native_tu15_validate_type_runtime_restore(memory,checkpoint);
    unsigned rejected_type_schemas{};
    for(unsigned fault=0;fault<9;++fault) {
        auto invalid=checkpoint;
        const auto owner=std::ranges::find(invalid.regions,0x5020b4u,
            &SourceRuntimeMemory::CheckpointRegion::address);
        switch(fault) {
        case 0:invalid.regions.erase(owner);break;
        case 1:owner->bytes.pop_back();break;
        case 2:owner->bytes.push_back(std::byte{});break;
        case 3:owner->access=SourceRuntimeMemory::Access::ReadOnly;break;
        case 4:owner->owner+=" invalid";break;
        case 5:owner->page_bytes=4096;break;
        case 6:owner->committed={1};break;
        case 7:owner->address=0x5020b8u;owner->bytes={std::byte{}};
            owner->owner="native TU1.5 cold 102A54 scene placement policy byte";break;
        case 8:owner->address+=1;break;
        }
        bool rejected=false;
        try {native_tu15_validate_type_runtime_restore(memory,invalid);}
        catch(const std::exception&error) {
            rejected=std::string(error.what())=="TU1.5 authority lacks its complete object-type runtime owner";
        }
        check(rejected,"Native restore admitted a missing, malformed or old partial type-runtime owner");
        check(memory.checkpoint()==checkpoint,"Rejected type-runtime schema mutated the current factory bank");
        ++rejected_type_schemas;
    }
    // This isolated whole-section topology has no native15-byte declaration;
    // the surrounding authority keeps its existing offline-image admission.
    SourceRuntimeMemory whole_section;
    whole_section.mount(0x502000u,std::vector<std::byte>(4096),
        SourceRuntimeMemory::Access::ReadWrite,"isolated whole-section schema control");
    native_tu15_validate_type_runtime_restore(whole_section,whole_section.checkpoint());
    std::cout<<"Native object-type runtime restore schemas: "<<rejected_type_schemas
        <<" rejected, complete factory owner accepted; offline whole-section route preserved\n";

    for(const auto& region:checkpoint.regions) {
        check(!region.owner.empty(),"World declaration has no retained owner identity");
        check(memory.native_address(region.address,region.bytes.size()).value>UINT32_MAX,
            "World data was narrowed to a 32-bit host address");
        ++regions;bytes+=region.bytes.size();
    }
    // Optional offline audit compares individually declared data against the
    // authenticated TU1.5 reference. The production factory accepts no path.
    if(argc==2) {
        auto original=load_source_runtime_image(argv[1],SourcePhysicsContactImage::TitleUpdate15);
        for(const auto& region:checkpoint.regions) {
            if(region.address==tick || region.address==0x41b774)continue;
            if(!std::ranges::equal(memory.view(region.address,region.bytes.size()),
                original.view(region.address,region.bytes.size()))) {
                std::cerr<<"Reference differs at 0x"<<std::hex<<region.address<<" "<<region.owner<<'\n';
                return 1;
            }
        }
    }else check(argc==1,"Expected only optional offline reference path");
    const auto level_owner=std::ranges::find(checkpoint.regions,0x4fadb8u,
        &SourceRuntimeMemory::CheckpointRegion::address);
    check(level_owner!=checkpoint.regions.end() && level_owner->bytes.size()==0x208u &&
        level_owner->access==SourceRuntimeMemory::Access::ReadWrite &&
        level_owner->owner=="source 64 level-handle pairs and two pool constructor bindings",
        "Complete level-handle source owner is absent or has a partial schema");
    check(std::ranges::all_of(level_owner->bytes,[](auto value){return value==std::byte{};}),
        "Cold level-handle backing fabricated a source constructor result");
    unsigned bounded_name_cases{};
    for(const auto& [address,name,extent]:std::array{
        std::tuple{0x4607ecu,std::string_view{"level handles"},16u},
        std::tuple{0x4607d0u,std::string_view{"multiplayer level handles"},28u}}) {
        check(memory.is_read_only(address,extent),"Level-handle name extent is not immutable");
        const auto owner=std::ranges::find(checkpoint.regions,address,
            &SourceRuntimeMemory::CheckpointRegion::address);
        check(owner!=checkpoint.regions.end() && owner->bytes.size()==extent,
            "Level-handle pool name has an inexact owned extent");
        check(memory.view(0x4607ecu+15u,1).front()==std::byte{},
            "Level-handle original post-NUL DWORD byte differs");
        bool read_only{};try{memory.mutable_view(address,1);}catch(const std::exception&){read_only=true;}
        check(read_only,"Authored pool name accepted a write");
        constexpr std::uint32_t stack=0x61008000u,destination=0x61010000u,stop=0x70000000u;
        for(const auto count:std::array{0u,1u,unsigned(name.size()+1u),32u}) {
            SourceRuntimeMemory call_bank;call_bank.restore(checkpoint);
            call_bank.mount(stack-4096u,std::vector<std::byte>(8192),SourceRuntimeMemory::Access::ReadWrite,
                "isolated bounded pool-name actual caller stack");
            call_bank.mount(destination,std::vector<std::byte>(64,std::byte{0xa5}),SourceRuntimeMemory::Access::ReadWrite,
                "isolated bounded pool-name caller destination");
            const auto put=[&](auto p,auto value){std::array<std::byte,4> raw{};
                for(unsigned i=0;i<4;++i)raw[i]=std::byte(value>>(8*i));call_bank.write(p,raw);};
            put(stack,stop);put(stack+4,destination);put(stack+8,address);put(stack+12,count);
            SourcePhysicsWorldRuntimeAbi abi{};abi.gp[4]=stack;abi.eflags=0x202;
            source_run_physics_live_support_tu15(call_bank,0x32a4e0u,abi,{0x27f,0x1f80},{});
            check(abi.gp[0]==destination && abi.gp[4]==stack+4,
                "Actual bounded pool-name copy returned a different caller frame");
            const auto output=call_bank.view(destination,64);
            for(unsigned i=0;i<64;++i) {
                const auto expected=i>=count?std::byte{0xa5}:i<name.size()?std::byte(name[i]):std::byte{};
                check(output[i]==expected,"Actual bounded pool-name copy differs from source string/zero padding");
            }
            ++bounded_name_cases;
        }
    }
    std::cout<<"Native bounded level-handle name controls: "<<bounded_name_cases<<" cases\n";
    for(const auto& bad:std::array{
        SourceRuntimeKernelTickCount{0x1000,{},"invalid low clock"},
        SourceRuntimeKernelTickCount{0xffffffffu,{},"invalid wrapped clock"},
        SourceRuntimeKernelTickCount{tick,{},""}}) {
        bool rejected=false;
        try{(void)native_tu15_world_memory(bad);}catch(const std::invalid_argument&){rejected=true;}
        check(rejected,"Invalid native tick-count owner was admitted");
    }
    std::cout<<"Native TU1.5 world cold declarations: "<<regions<<" regions, "<<bytes<<" bytes\n";
    return 0;
}catch(const std::exception& error){std::cerr<<error.what()<<'\n';return 1;}
