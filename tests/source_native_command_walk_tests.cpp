#include "halo2/native_tu15_command_walk.hpp"
#include "halo2/physics_live_world_tu15.hpp"
#include "halo2/original_data.hpp"
#include "halo2/source_live_transparent_groups_tu15.hpp"
#include <bit>
#include <cstdlib>
#include <iostream>
#include <openssl/sha.h>
using namespace halo2;
namespace {
void check(bool v,const char* s){if(!v)throw FormatError(s);}
void equal(const SourceRuntimeMemory::Checkpoint& a,const SourceRuntimeMemory::Checkpoint& b) {
    check(a.regions.size()==b.regions.size(),"Native command walk changed World topology");
    for(std::size_t i=0;i<a.regions.size();++i) {
        if(a.regions[i].address==0x60000000u)continue;
        if(a.regions[i]!=b.regions[i]) {
            for(std::size_t n=0;n<a.regions[i].bytes.size();++n)if(a.regions[i].bytes[n]!=b.regions[i].bytes[n])
                throw FormatError("Native command walk differs at "+std::to_string(a.regions[i].address+n)+" in "+a.regions[i].owner);
        }
    }
}
void restore_bytes(PhysicsLiveWorldTu15& world,const SourceRuntimeMemory::Checkpoint& saved) {
    // Keep every actual native allocation/lease in place. Replacing World
    // topology with a bare memory checkpoint would retire those owners.
    const auto current=world.memory().checkpoint();check(saved.regions.size()==current.regions.size(),"Command services changed fixture topology");
    for(std::size_t i=0;i<saved.regions.size();++i) {
        const auto& a=saved.regions[i];const auto& b=current.regions[i];
        check(a.address==b.address&&a.owner==b.owner&&a.access==b.access&&a.page_bytes==b.page_bytes&&a.committed==b.committed&&a.bytes.size()==b.bytes.size(),
            "Command services replaced a native fixture lease");
        if(a.bytes==b.bytes)continue;
        check(a.access==SourceRuntimeMemory::Access::ReadWrite,"Command services changed a readonly fixture bank");
        for(std::size_t n=0;n<a.bytes.size();) {
            if(a.bytes[n]==b.bytes[n]){++n;continue;}
            const auto first=n;while(n<a.bytes.size()&&a.bytes[n]!=b.bytes[n])++n;
            world.write_source_call_input(a.address+std::uint32_t(first),std::span(a.bytes).subspan(first,n-first));
        }
    }
}
}
int main(int argc,char** argv)try {
    check(argc==3,"Supply TU15_XBE OPENING_MAP");
    if(!std::filesystem::is_regular_file(argv[1])||!std::filesystem::is_regular_file(argv[2]))return 77;
    const auto* scratch=std::getenv("HALO2_TEST_SCRATCH");check(scratch,"Command walk requires owned scratch");
    Cache cache(argv[2],std::filesystem::path(argv[2]).parent_path(),OriginalData::load_xbe(argv[1]));
    PhysicsLiveWorldTu15 world(cache,argv[1],std::filesystem::path(scratch)/"saves");auto& m=world.memory();
    constexpr auto data=0x1000000u,ctx=data,pool=data+0x4000u,rows=data+0x4800u,
        tags=data+0x5000u,model=data+0x6000u,sections=data+0x7000u,geom=data+0x8000u,
        parts=data+0x9000u,materials=data+0xa000u,body=data+0xb000u,mask=data+0xc000u,
        shader=data+0xd000u,records=data+0xe000u,queue=data+0x10000u,units=0x4c6958u;
    check(m.find_unowned_range(data,data+0x20000u,0x20000u,4096)==data,"Command fixture overlaps another owner");
    m.mount(data,std::vector<std::byte>(0x20000),SourceRuntimeMemory::Access::ReadWrite,"native-command-declared-inputs");
    const auto initial=m.checkpoint();
    struct Restore {PhysicsLiveWorldTu15& world;const SourceRuntimeMemory::Checkpoint& initial;
        ~Restore(){restore_bytes(world,initial);world.memory().release(data);}} restore{world,initial};
    const auto put=[&](std::uint32_t at,std::uint32_t value,unsigned size=4) {
        std::array<std::byte,4> bytes{};for(unsigned i=0;i<4;++i)bytes[i]=std::byte(value>>(8*i));
        world.write_source_call_input(at,std::span(bytes).first(size));
    };
    put(0x4cd3a0,ctx);put(ctx+0x14,ctx+0x3000);put(0x4f0414,tags);put(tags+8,model);
    put(0x4ecbd0,pool);put(pool+0x44,rows);put(rows+8,body);put(0x4f3218,model);put(model+8,3);
    put(model+0x28,sections);put(model+0x64,materials);put(sections+0x34,geom);put(sections+0x2c,0xffff,2);
    put(geom,3);put(geom+4,parts);
    put(materials+0xc,1u);put(tags+16u,0x73686164u);put(tags+24u,shader);put(tags+40u,shader+0x100u);
    put(shader+0x24u,shader+0x60u);put(shader+0x3cu,5,2);put(shader+0x60u,2);
    put(shader+0x15cu,shader+0x200u);put(shader+0x200u,1);put(shader+0x204u,shader+0x220u);put(shader+0x222u,4u);
    for(unsigned bank=0;bank<3;++bank) {
        const auto at=0x4ce268u+24u*bank;put(at,16);put(at+0x14u,records+0x200u*bank);
    }
    put(body+0x50,std::bit_cast<std::uint32_t>(1.f));put(body+0x60,std::bit_cast<std::uint32_t>(2.f));
    put(body+0x70,std::bit_cast<std::uint32_t>(3.f));
    for(unsigned i=0;i<3;++i) {
        put(parts+72*i,1,2);
        put(parts+72*i+0xa,i*31,2);put(parts+72*i+0xc,2,2);
        put(parts+72*i+0x2c,std::bit_cast<std::uint32_t>(i?3.f:0.f));
    }
    for(unsigned i=0;i<4;++i) {
        put(units+32*i+4,0);put(units+32*i+8,0);put(units+32*i+12,31u<<4);
        put(units+32*i+0x14,mask);put(units+32*i+0x18,1u<<16);
    }
    put(0x4c2aa8u,queue);put(0x4ce1ccu,1);
    put(0x4cdd4cu,std::bit_cast<std::uint32_t>(1.75f));put(0x4cdd4cu+32u,3);
    unsigned compared{},positive_bank_frames{},section_frames{};
    for(unsigned mode:{0u,1u})for(unsigned visible:{0u,1u})for(unsigned selector:{0u,1u,2u}) {
        put(ctx+0x2a78,mode);put(0x4cd39c,mode?1:4);put(0x4cd398,4);
        put(0x515f1c,0);put(0x515f20,0);put(0x4d2f1c,0,2);
        for(unsigned i=0;i<4;++i) {
            put(units+32*i,i==3?0x400u:0x77u);put(0x4cd6f6u+i,255,1);put(0x4cd3a4u+i,13+i*60,1);
        }
        for(unsigned i=0;i<3;++i)put(mask+4*i,visible?0xffffffffu:0u);
        const auto before=m.checkpoint();auto original=world.prepare_source_call();original.gp[0]=selector;
        world.run_actual(0x40e80,original);const auto expected=m.checkpoint();restore_bytes(world,before);
        const auto actual=world.run_native_command_frame_tu15(selector);equal(expected,m.checkpoint());
        check(actual.control_word==original.floating_point->x87_control_word&&actual.status==original.x87_status&&actual.mxcsr==original.floating_point->mxcsr,
            "Native command frame changed complete World FP controls/status");++compared;
        const auto count=[&](unsigned b) {return Bytes(m.view(0x4ce26cu+24u*b,4)).u32(0);};
        if(visible) {
            check(mode?(count(1)||count(2)):count(0)>0,"Whole World command fixture did not exercise a bank append");
            ++positive_bank_frames;
        }
        if(!mode) {
            check(Bytes(m.view(0x4c2aa0u,4)).u32(0)==1u&&Bytes(m.view(queue+0x20u,4)).u32(0)==0x423b0u,
                "Whole World command fixture did not exercise a kind-3 pool append");
            ++section_frames;
        }
    }
    std::cout<<"PASS "<<compared<<" whole40E80 non-stack World-bank and FP comparisons; "<<positive_bank_frames<<
        " positive command bank frames and "<<section_frames<<" positive transparent section frames\n";
    // Kind-1 seeds 0 and 2 overlap. The intervening kind-3 row touches
    // their edge and joins the first camouflage group, then queues itself.
    put(ctx+0x2a78u,0);put(0x4cd39cu,4);put(0x4cd398u,4);put(0x4ce1ccu,3);
    for(unsigned section=0;section<3;++section) {
        const auto address=0x4cdd4cu+36u*section;
        const std::array values{4.f-float(section),float(section),-float(section),1.f,
            section==1?1.f:0.f,section==1?2.f:1.f,0.f,1.f};
        for(unsigned i=0;i<values.size();++i)put(address+4u*i,std::bit_cast<std::uint32_t>(values[i]));
        put(address+32u,section==1?3u:1u);
    }
    {
        const auto before=m.checkpoint();auto original=world.prepare_source_call();original.gp[0]=0;
        world.run_actual(0x40e80u,original);const auto expected=m.checkpoint();restore_bytes(world,before);
        const auto actual=world.run_native_command_frame_tu15();equal(expected,m.checkpoint());
        check(actual.control_word==original.floating_point->x87_control_word&&actual.status==original.x87_status&&
            actual.mxcsr==original.floating_point->mxcsr,"Native42090 changed whole World FP controls/status");
        const auto word=[&](std::uint32_t at){return Bytes(m.view(at,4)).u32(0);};
        check(word(0x4c2aa0u)==3u&&word(queue+0x20u)==0x41cb0u&&word(queue+0x90u)==0x41cb0u&&
            word(queue+0x100u)==0x423b0u&&(word(queue+0x24u)&0xffffffu)==0x010002u&&
            (word(queue+0x94u)&0xffffu)==0x0201u,"Whole World camouflage fixture lost its two groups and section append");
    }
    std::cout<<"PASS two camouflage groups and one section append through the whole World command frame\n";
    // The actual 423B0 group consumer owns bank0 and the retained prefix.
    // Layer 10 emits records without invoking an unrelated descriptor sort.
    put(0x4c2aa0u,1);put(0x4c2aa8u,queue);put(queue+0x20u,0x423b0u);put(queue+0x28u,0);
    put(0x4ce1ccu,2);put(0x4cdd4cu+32u,3);put(0x4cdd4cu+36u+32u,3);
    put(0x4cd39cu,4);put(0x4cd398u,4);put(ctx+0x2a78u,0);put(ctx+0xa5cu,1,2);
    put(shader+0x222u,0x400u);
    for(unsigned i=0;i<4;++i) {
        const auto unit=units+32u*i,object_context=data+0x11000u+0x80u*i;
        put(unit,0x77u);put(unit+0xcu,i<2?0u:31u<<4);put(unit+0x10u,object_context);
        put(object_context+0x76u,i==1?255u:128u,1);
    }
    for(unsigned section:{0u,1u}) {
        put(queue+0x24u,section);put(0x4ce26cu,0);put(0x4ce270u,0x80u);put(0x4ce274u,0);
        const auto before=m.checkpoint();
        put(0x4ce278u,0x80u);put(0x4ce270u,0);put(0x4ce274u,0);
        auto original=world.prepare_source_call(std::array{0u,0xffffffcfu,section,0xffffffffu,0u,0xffffffffu,0u,1u});
        world.run_actual(0x44c80u,original);
        const auto expected_count=Bytes(m.view(0x4ce26cu,4)).u32(0);
        const auto expected_layers=Bytes(m.view(0x4ce270u,4)).u32(0);
        check(section?expected_count==0:expected_count==6,"Genuine44C80 group fixture lost its positive/negative control");
        put(0x4ce274u,0);put(0x4ce270u,0x80u);
        const auto expected=m.checkpoint();restore_bytes(world,before);
        const auto groups=source_live_transparent_groups_tu15(world);
        check(groups.size()==1&&groups[0].commands.size()==expected_count&&groups[0].layers==expected_layers,
            "Native44C80 normal group consumer lost its actual output range");
        equal(expected,m.checkpoint());
    }
    std::cout<<"PASS positive and empty 44C80 groups through the actual 423B0 collection seam\n";

    // Exercise the original material callback with a declared ordinary shad
    // template, pass and implementation. Layer 5 also admits on unit bit 13.
    constexpr auto definition=shader+0x100u,template_tables=shader+0x300u,pass=shader+0x500u,
        implementation=shader+0x600u,shader_bank=0x4ce268u;
    put(definition+0x5cu,template_tables);
    put(template_tables+4u,shader+0x340u);put(template_tables+0xcu,shader+0x360u);
    put(template_tables+0x14u,shader+0x390u);
    put(shader+0x360u+2u,0x200u,2);put(shader+0x360u+10u,0x200u,2);put(shader+0x390u+4u,3);
    put(tags+56u,pass);put(pass+0x20u,shader+0x5c0u);put(shader+0x5c4u,implementation);
    put(implementation,1u,2);put(implementation+0x100u,4u);
    put(tags+72u,shader+0x800u);put(shader+0x804u,16u);put(shader+0x808u,shader+0x840u);
    for(unsigned i=0;i<16;++i)put(shader+0x84cu+28u*i,0x10u);
    put(shader+0x24u,shader+0x60u);put(shader+0x60u,2);
    for(unsigned i=0;i<4;++i) {
        put(units+32u*i,0x77u|(i%2?0x2000u:0u));put(units+32u*i+0xcu,0);
        put(units+32u*i+0x10u,0);
    }
    put(shader_bank+4u,8);put(shader_bank+12u,1);
    for(unsigned i=0;i<8;++i) {
        const auto record=records+24u*i;
        put(record,0x4cbe0u);put(record+8u,1u);put(record+12u,(i%4)<<8);
        put(record+16u,i==4?0u:0x22u);put(record+20u,0u);
    }
    for(unsigned layer:{1u,5u,6u}) {
        auto caller=world.prepare_source_call();
        const auto stack=caller.gp[4]-0xf01cu,output=stack+0x1cu;
        put(stack,Bytes(m.view(caller.gp[4],4)).u32(0));
        put(stack+4u,shader_bank);put(stack+8u,0);put(stack+12u,layer);put(stack+16u,output);
        caller.gp[4]=stack;caller.gp[0]=output;caller.gp[3]=layer;caller.gp[5]=0;caller.gp[7]=shader_bank;
        for(unsigned i=0;i<42;++i)put(output+i*4u,0xc77e0000u+i);
        const auto before=m.checkpoint();
        if(layer==5u) {
            const auto initial=caller;
            world.run_command_selection_tu15(caller);
            check(caller.gp[0]==4u&&caller.gp[4]==stack+20u&&caller.xmm==initial.xmm&&
                caller.x87_status==initial.x87_status&&caller.floating_point->mxcsr==initial.floating_point->mxcsr&&
                caller.floating_point->x87_control_word==initial.floating_point->x87_control_word,
                "Native3BAA0 layer-5 result lost its physical count, stack or FP");
            const auto rows=m.view(output,168u);
            std::array<unsigned char,SHA256_DIGEST_LENGTH> digest{};
            SHA256(reinterpret_cast<const unsigned char*>(rows.data()),rows.size(),digest.data());
            // Unchanged XBE 3BAA0 with genuine 4CBE0 and 3BBC0, 2185 instructions.
            constexpr std::array<unsigned char,32> physical{
                0xd4,0xc6,0x85,0x2d,0xe2,0x1e,0x5a,0xaf,0xfb,0x6b,0x18,0xd5,0x88,0xed,0x85,0x8a,
                0x66,0x8f,0x50,0x36,0x50,0x1a,0xa0,0x46,0x94,0x8a,0xd8,0x76,0x45,0xf3,0xda,0x1e};
            check(digest==physical,"Native3BAA0 layer-5 descriptors differ from the physical XBE");
            equal(before,m.checkpoint());
            continue;
        }
        auto original=caller;
        world.run_actual(0x3baa0u,original);
        check(original.gp[0]==(layer==1?6u:0u),"3BAA0 original fixture lost accepted or masked commands");
        const auto expected=m.checkpoint();
        const auto descriptors=m.view(output,168u);
        const std::vector<std::byte> expected_rows(descriptors.begin(),descriptors.end());
        constexpr std::array material_order{2u,3u,5u,6u,7u,1u};
        for(unsigned i=0;i<original.gp[0];++i)
            check(Bytes(descriptors).u16(24u*i)==material_order[i],
                "Original3BAA0 diverged from the pinned physical tie order");
        restore_bytes(world,before);
        world.run_command_selection_tu15(caller);
        check(caller.gp[0]==original.gp[0]&&caller.gp[4]==original.gp[4]&&caller.xmm==original.xmm&&
            caller.x87_status==original.x87_status&&caller.floating_point->mxcsr==original.floating_point->mxcsr&&
            caller.floating_point->x87_control_word==original.floating_point->x87_control_word,
            "Native3BAA0 changed count, stack or FP at the caller boundary");
        check(std::equal(expected_rows.begin(),expected_rows.end(),m.view(output,168u).begin()),
            "Native3BAA0 changed accepted descriptors or rejected slot residue");
        equal(expected,m.checkpoint());
    }
    std::cout<<"PASS material and layer-5 order plus masked commands through the original selector\n";
}catch(const std::exception& e){std::cerr<<e.what()<<'\n';return 1;}
