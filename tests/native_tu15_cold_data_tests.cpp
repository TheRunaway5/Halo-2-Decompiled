#include "halo2/native_tu15_world_memory.hpp"
#include "helpers/native_campaign_perception_default_ownership.hpp"
#include "halo2/source_network_session_startup_tu15.hpp"
#include "halo2/native_tu15_multiplayer_data.hpp"
#include "halo2/original_data.hpp"
#include <algorithm>
#include <array>
#include <iostream>
#include <stdexcept>

using namespace halo2;
namespace {
void check(bool value,const char* message) {
    if(!value)throw std::runtime_error(message);
}
const SourceRuntimeMemory::CheckpointRegion& region(
    const SourceRuntimeMemory::Checkpoint& checkpoint,std::uint32_t address) {
    const auto found=std::ranges::find(checkpoint.regions,address,
        &SourceRuntimeMemory::CheckpointRegion::address);
    if(found==checkpoint.regions.end())throw std::runtime_error("Cold data lost its exact declared owner");
    return *found;
}
template<class Call>void rejects(Call call,const char* message) {
    bool rejected=false;
    try{call();}catch(const std::out_of_range&){rejected=true;}
    check(rejected,message);
}
void submitted_view_storage(const SourceRuntimeMemory& cold) {
    // Original2B480's complete startup destination already contains the
    // submitted-view serial, current viewport and four cluster/leaf pairs.
    // These fields must share its backing rather than mount new subregions.
    constexpr std::uint32_t base=0x4c6854u;
    constexpr std::size_t bytes=0x40u;
    const auto checkpoint=cold.checkpoint();const auto& bank=region(checkpoint,base);
    check(bank.bytes.size()==bytes && bank.access==SourceRuntimeMemory::Access::ReadWrite &&
        bank.owner=="source startup33..44 loader-zeroed constructor destination" &&
        !bank.page_bytes && bank.committed.empty() &&
        std::ranges::all_of(bank.bytes,[](auto byte){return byte==std::byte{};}),
        "Submitted-view fields lost their complete original startup owner");
    const auto native=cold.native_address(base,bytes);
    for(const auto [address,length]:std::array{std::pair{0x4c6858u,4u},
            std::pair{0x4c6870u,0x24u}}) {
        cold.validate_mutable_view(address,length);
        check(cold.native_address(address,length).value==native.value+address-base,
            "Submitted-view field detached from original startup backing");
    }
    SourceRuntimeMemory live;live.restore(checkpoint);
    std::array<std::byte,bytes> values{};
    for(unsigned i=0;i<values.size();++i)values[i]=std::byte((i*37u+11u)&255u);
    live.write(base,values);
    SourceRuntimeMemory restored;restored.restore(live.checkpoint());
    check(std::ranges::equal(restored.view(base,bytes),values) &&
        region(restored.checkpoint(),base).owner==bank.owner,
        "Submitted-view live fields or original owner were lost on checkpoint restore");
    std::cout<<"PASS submitted-view storage: complete cold64B RW startup owner, shared serial/viewport "
        "backing and live checkpoint contents\n";
}
void view_angle_scalar(SourceRuntimeMemory& memory) {
    constexpr std::array expected{std::byte{0xc3},std::byte{0xf5},std::byte{0x48},std::byte{0x3f}};
    const auto checkpoint=memory.checkpoint();const auto& bank=region(checkpoint,0x45c55cu);
    check(bank.bytes.size()==4 && bank.access==SourceRuntimeMemory::Access::ReadOnly &&
        bank.owner=="source2F7E0 authored view-angle scalar" &&
        std::ranges::equal(memory.view(0x45c55cu,4),expected),
        "Cold world lost the exact original immutable view-angle scalar");
    bool rejected=false;
    try { memory.write(0x45c55cu,std::array<std::byte,4>{}); }
    catch(const std::exception&) { rejected=true; }
    check(rejected,"Original immutable view-angle scalar admitted a write");
    SourceRuntimeMemory restored;restored.restore(checkpoint);
    check(restored.is_read_only(0x45c55cu,4) &&
        std::ranges::equal(restored.view(0x45c55cu,4),expected),
        "Cold checkpoint restore lost the original view-angle scalar");
}
void arena_view_flag(const SourceRuntimeMemory& memory) {
    const auto checkpoint=memory.checkpoint();const auto& bank=region(checkpoint,0x4f2968u);
    check(bank.bytes.size()==1 && bank.access==SourceRuntimeMemory::Access::ReadWrite &&
        bank.owner=="native TU1.5 cold arena state-change view flag" &&
        memory.view(0x4f2968u,1)[0]==std::byte{},
        "Cold world lost the complete original loader-zeroed arena/view flag");
    SourceRuntimeMemory live;live.restore(checkpoint);
    const auto backing=live.native_address(0x4f2968u,1);
    live.write(0x4f2968u,std::array{std::byte{0xa5}});
    check(live.native_address(0x4f2968u,1).value==backing.value &&
        live.view(0x4f2968u,1)[0]==std::byte{0xa5},
        "Live arena/view flag lost its mutable native backing");
    SourceRuntimeMemory restored;restored.restore(live.checkpoint());
    check(!restored.is_read_only(0x4f2968u,1) &&
        restored.view(0x4f2968u,1)[0]==std::byte{0xa5} &&
        region(restored.checkpoint(),0x4f2968u).owner==bank.owner,
        "Checkpoint restore reset or detached the live arena/view flag");
}
void animation_mask_scratch(const SourceRuntimeMemory &cold) {
    const auto saved=cold.checkpoint();const auto &bank=region(saved,0x56e060u);
    check(bank.bytes.size()==32 && bank.access==SourceRuntimeMemory::Access::ReadWrite &&
        bank.owner=="native TU1.5 animation node-mask intersection scratch" &&
        !bank.page_bytes && bank.committed.empty() &&
        std::ranges::all_of(bank.bytes,[](auto byte){return byte==std::byte{};}),
        "Native animation mask scratch lost its authentic cold extent, access or owner");
    cold.validate_mutable_view(0x56e060u,32);
    rejects([&]{(void)cold.view(0x56e060u,33);},"Animation mask scratch borrowed its adjacent owner");
    SourceRuntimeMemory live;live.restore(saved);const auto native=live.native_address(0x56e060u,32);
    std::array<std::byte,32> mask{};
    for(unsigned i=0;i<mask.size();++i)mask[i]=std::byte((i*37u+11u)&255u);
    live.write(0x56e060u,mask);
    check(live.native_address(0x56e060u,32)==native && live.source_address(native,32)==0x56e060u,
        "Native mask scratch write replaced its actual source/native backing");
    const auto written=live.checkpoint();
    native_tu15_validate_type_runtime_restore(cold,written);
    SourceRuntimeMemory restored;restored.restore(written);
    check(restored.checkpoint()==written && std::ranges::equal(restored.view(0x56e060u,32),mask),
        "Native mask scratch contents or ownership were lost during checkpoint recovery");
    native_tu15_validate_type_runtime_restore(cold,saved);
    const auto rejects_topology=[&](auto corrupt) {
        auto incoming=written;corrupt(incoming);
        bool rejected=false;
        try{native_tu15_validate_type_runtime_restore(cold,incoming);}
        catch(const FormatError&){rejected=true;}
        check(rejected && cold.checkpoint()==saved,
            "Native restore admitted or applied missing/foreign animation mask workspace");
    };
    rejects_topology([](auto& incoming){std::erase_if(incoming.regions,
        [](const auto& bank){return bank.address==0x56e060u;});});
    const auto alter_bank=[&](auto alter) {
        rejects_topology([&](auto& incoming){alter(*std::ranges::find(incoming.regions,0x56e060u,
            &SourceRuntimeMemory::CheckpointRegion::address));});
    };
    alter_bank([](auto& bank){bank.owner="foreign animation workspace";});
    alter_bank([](auto& bank){bank.bytes.pop_back();});
    alter_bank([](auto& bank){bank.bytes.push_back(std::byte{});});
    alter_bank([](auto& bank){bank.access=SourceRuntimeMemory::Access::ReadOnly;});
    alter_bank([](auto& bank){++bank.address;});
    std::cout<<"PASS native animation mask workspace: exact cold32B RW owner, live native writes and "
        "checkpoint recovery, missing/foreign/extent/access/address restore rejection\n";
}
void name_mapping_operands(const SourceRuntimeMemory& cold) {
    constexpr auto owner="native TU1.5 original name token and private glyph operands";
    // Full raw-byte fingerprints from the independently authenticated export,
    // including original pointers, high halfwords and unused glyph DWORDs.
    // Cryptographic source provenance is retained in the exporter receipt.
    const auto fingerprint=[](std::span<const std::byte> bytes){
        std::uint64_t value=14695981039346656037ull;
        for(auto byte:bytes)value=(value^std::to_integer<unsigned>(byte))*1099511628211ull;
        return value;
    };
    struct Expected {std::uint32_t address;std::size_t bytes;std::uint64_t fingerprint;bool read_only;};
    constexpr std::array expected{
        Expected{0x455e48u,144,0x9da7c5881483ccb0ull,true},
        Expected{0x47c91cu,1572,0xb5505c436ed79d23ull,false},
        Expected{0x4639d0u,22,0xc303fee9d894a315ull,true},
        Expected{0x4639e8u,22,0xf9d0b3e202bd1531ull,true},
        Expected{0x463a00u,22,0x6742ab12c671a299ull,true},
        Expected{0x463a18u,22,0x1f944de8665500edull,true},
        Expected{0x463a30u,22,0x5c740e0466598725ull,true},
        Expected{0x463a48u,22,0x0b9e4da40bcf9adcull,true}};
    const auto saved=cold.checkpoint();
    for(const auto& row:expected){const auto& bank=region(saved,row.address);
        check(bank.owner==owner&&bank.bytes.size()==row.bytes&&!bank.page_bytes&&bank.committed.empty()&&
            (bank.access==SourceRuntimeMemory::Access::ReadOnly)==row.read_only&&
            fingerprint(bank.bytes)==row.fingerprint,
            "Name operand cold owner lost exact raw bytes, extent or original access");
        check(cold.source_address(cold.native_address(row.address,row.bytes),row.bytes)==row.address,
            "Name operand lost exact canonical/native address ownership");
    }
    const auto read=[&](std::uint32_t address,unsigned bytes){std::array<std::byte,4> raw{};
        cold.read(address,std::span(raw).first(bytes));std::uint32_t value{};
        for(unsigned i=0;i<bytes;++i)value|=std::to_integer<std::uint32_t>(raw[i])<<(8*i);return value;};
    const auto image=OriginalData::native_tu15();
    check(image&&image->full_scoreboard_name_mappings.has_value(),"Compiled native name operands absent");
    const auto& mapping=*image->full_scoreboard_name_mappings;
    check(mapping.tokens.size()==18&&mapping.glyphs.size()==131,"Compiled source name mapping extent differs");
    for(unsigned i=0;i<18;++i){const auto pointer=read(0x455e48u+i*8u,4);const auto& token=mapping.tokens[i];
        check(read(0x455e4cu+i*8u,2)==token.glyph,"Raw token glyph differs from independent compiled operands");
        for(unsigned j=0;j<token.text.size();++j)
            check(read(pointer+j*2u,2)==token.text[j],"Canonical token pointer lost its actual UTF16 bytes");
        check(read(pointer+std::uint32_t(token.text.size()*2u),2)==0,"Canonical token lost its original UTF16 terminator");
    }
    for(unsigned i=0;i<131;++i)check(read(0x47c91cu+i*12u,4)==mapping.glyphs[i].glyph&&
        read(0x47c920u+i*12u,4)==mapping.glyphs[i].original_handler,
        "Raw private glyph callback/key differs from independent compiled operands");
    SourceRuntimeMemory restored;restored.restore(saved);
    for(const auto& row:expected)if(row.read_only){bool refused=false;
        try{restored.write(row.address,std::array{std::byte{0xff}});}catch(const std::runtime_error&){refused=true;}
        check(refused&&restored.checkpoint()==saved,"Name token/string admitted a write or partially changed storage");
    }
    // Original glyph dispatch records live in writable .data. Preserve that
    // policy without inventing a callback result or a warm profile owner.
    const auto backing=restored.native_address(0x47c91cu,1572);
    restored.write(0x47c924u,std::array{std::byte{0xa7}});
    check(restored.native_address(0x47c91cu,1572)==backing,
        "Name glyph write replaced its live native backing");
    const auto live=restored.checkpoint();restored.restore(live);
    check(restored.checkpoint()==live&&restored.view(0x47c924u,1)[0]==std::byte{0xa7},
        "Name glyph writable state did not survive exact checkpoint restore");
    SourceRuntimeMemory foreign;foreign.mount(0x455e48u,std::array<std::byte,4>{},
        SourceRuntimeMemory::Access::ReadOnly,"foreign truncated name owner");
    const auto before=foreign.checkpoint();bool refused=false;
    try{mount_native_tu15_multiplayer_data(foreign);}catch(const std::invalid_argument&){refused=true;}
    check(refused&&foreign.checkpoint()==before,"Name operand conflict partially published unrelated provider banks");
}
void saved_session_storage(const SourceRuntimeMemory& cold) {
    constexpr std::uint32_t base=0x4d6098u;
    constexpr std::size_t bytes=0x144u;
    constexpr auto owner="network saved session startup state";
    const auto checkpoint=cold.checkpoint();
    const auto& saved=region(checkpoint,base);
    check(saved.owner==owner && saved.access==SourceRuntimeMemory::Access::ReadWrite &&
        saved.bytes.size()==bytes && !saved.page_bytes && saved.committed.empty() &&
        std::ranges::all_of(saved.bytes,[](auto byte){return byte==std::byte{};}),
        "Cold saved-session destination lost its complete bytes, owner or mutable policy");
    cold.validate_mutable_view(base,bytes);
    rejects([&]{(void)cold.view(base,bytes+1);},
        "Saved-session owner admitted a view beyond its declared extent");

    SourceRuntimeMemory live;
    mount_native_tu15_network_saved_session_storage(live);
    const auto native=live.native_address(base,bytes);
    check(live.source_address(native,bytes)==base,
        "Saved-session bank lost its complete source/native backing");
    // Test arbitrary live values across the whole destination, including the
    // initialized byte. This does not supply a real startup result or run53240.
    std::array<std::byte,bytes> values{};
    for(std::size_t i=0;i<values.size();++i)values[i]=std::byte((i*37u+11u)&255u);
    values[0x140]=std::byte{0xa7};
    live.write(base,values);
    const auto before=live.checkpoint();
    mount_native_tu15_network_saved_session_storage(live);
    check(live.checkpoint()==before && live.native_address(base,bytes)==native,
        "Repeated saved-session admission reset live values or replaced native backing");
    mount_native_tu15_network_session_data(live);
    const auto full=live.checkpoint();
    check(region(full,base)==region(before,base) &&
        std::ranges::equal(live.view(base,bytes),values) &&
        live.native_address(base,bytes)==native,
        "Full network provider replaced the existing saved-session owner or live state");
    SourceRuntimeMemory restored;
    restored.restore(full);
    const auto restored_native=restored.native_address(base,bytes);
    mount_native_tu15_network_saved_session_storage(restored);
    check(restored.checkpoint()==full && restored.native_address(base,bytes)==restored_native,
        "Restored saved-session owner failed exact non-destructive reuse");

    const auto rejects_owner=[&](auto mount) {
        SourceRuntimeMemory foreign;
        mount(foreign);
        const auto prior=foreign.checkpoint();
        bool rejected=false;
        try{mount_native_tu15_network_saved_session_storage(foreign);}
        catch(const std::invalid_argument&){rejected=true;}
        check(rejected && foreign.checkpoint()==prior,
            "Saved-session admission accepted or changed a foreign/partial owner");
    };
    for(const auto access:{SourceRuntimeMemory::Access::ReadOnly,SourceRuntimeMemory::Access::ReadWrite}) {
        rejects_owner([&](auto& memory){memory.mount(base,values,access,"foreign saved-session owner");});
    }
    rejects_owner([&](auto& memory){memory.mount(base,values,SourceRuntimeMemory::Access::ReadOnly,owner);});
    rejects_owner([&](auto& memory){memory.mount(base,std::span(values).first(bytes-1),
        SourceRuntimeMemory::Access::ReadWrite,owner);});
    rejects_owner([&](auto& memory){memory.mount(base+1,std::span(values).subspan(1),
        SourceRuntimeMemory::Access::ReadWrite,owner);});
    rejects_owner([&](auto& memory){
        memory.mount(base,std::span(values).first(0x140),SourceRuntimeMemory::Access::ReadWrite,owner);
        memory.mount(base+0x140,std::span(values).subspan(0x140),SourceRuntimeMemory::Access::ReadWrite,owner);
    });
    std::cout<<"PASS cold saved-session owner: complete mutable destination, whole-provider and checkpoint reuse, "
        "stable live native backing, foreign/access/partial/split owner rejection; startup unexecuted\n";
}
}

int main()try {
    // The same explicit, retained platform dependency as the full cold-world
    // regression. This is not a live clock, warm world or imported game asset.
    const SourceRuntimeKernelTickCount clock{0x60212000u,
        {std::byte{0x78},std::byte{0x56},std::byte{0x34},std::byte{0x12}},
        "cold-data regression retained clock"};
    for(const auto& invalid:std::array{
        SourceRuntimeKernelTickCount{0x5ffffffcu,clock.bytes,clock.owner},
        SourceRuntimeKernelTickCount{0xfffffffdu,clock.bytes,clock.owner},
        SourceRuntimeKernelTickCount{clock.address,clock.bytes,{}}}) {
        bool rejected=false;
        try{(void)native_tu15_world_memory(invalid);}catch(const std::invalid_argument&){rejected=true;}
        check(rejected,"Cold factory admitted an invalid retained platform dependency");
    }
    auto memory=native_tu15_world_memory(clock);
    view_angle_scalar(memory);
    arena_view_flag(memory);
    halo2::test::perception_default_ownership(memory);
    submitted_view_storage(memory);
    animation_mask_scratch(memory);
    name_mapping_operands(memory);
    saved_session_storage(memory);
    const auto checkpoint=memory.checkpoint();
    check(std::ranges::equal(memory.view(clock.address,4),clock.bytes),
        "Cold factory changed the supplied retained clock bytes");
    check(!memory.is_read_only(clock.address,4) &&
        region(checkpoint,clock.address).owner==clock.owner,
        "Cold factory replaced the supplied clock ownership/access");
    constexpr std::array binding{std::byte{0},std::byte{0x20},std::byte{0x21},std::byte{0x60}};
    check(memory.is_read_only(0x41b774u,4) &&
        std::ranges::equal(memory.view(0x41b774u,4),binding),
        "Cold factory tick import lost its exact source identity");

    // Authenticated19-byte29AF5C opcode table. The condition word at29AF6C
    // includes three table bytes and one separate original alignment byte.
    constexpr std::array<std::uint8_t,19> expected{
        0,0,0,0,0,1,0,1,1,1,1,0,0,0,0,0,0,0,0};
    const auto& typed=region(checkpoint,0x29af5cu);
    check(typed.access==SourceRuntimeMemory::Access::ReadOnly &&
        typed.owner=="TU1.5 final startup immutable constructor metadata" &&
        !typed.page_bytes && typed.committed.empty() &&
        std::ranges::equal(typed.bytes,std::as_bytes(std::span(expected))),
        "Cold factory changed the exact authored typed-table bytes/extent/owner");
    const auto& padding=region(checkpoint,0x29af6fu);
    check(padding.access==SourceRuntimeMemory::Access::ReadOnly &&
        padding.owner=="native TU1.5 authored AI condition dispatch data" &&
        !padding.page_bytes && padding.committed.empty() &&
        padding.bytes.size()==1 && padding.bytes[0]==std::byte{0x90},
        "Cold factory joined or relabelled the independently owned padding byte");
    const auto native=memory.native_address(0x29af5cu,expected.size());
    check(native.value>0xffffffffu && memory.source_address(native,expected.size())==0x29af5cu,
        "Typed table lost its complete native/source address ownership");
    rejects([&]{(void)memory.view(0x29af6cu,4);},
        "Contiguous scalar view crossed independent typed-table/padding owners");
    rejects([&]{(void)memory.view(0x29af5cu,expected.size()+1);},
        "Typed-table view borrowed its separately owned padding");
    constexpr std::array word{std::byte{},std::byte{},std::byte{},std::byte{0x90}};
    std::array<std::byte,4> read{};
    memory.read(0x29af6cu,read);
    check(read==word,"Actual source scalar read across owner boundaries differs");
    for(const auto address:{0x29af5cu,0x29af6fu}) {
        bool rejected=false;
        try{(void)memory.mutable_view(address,1);}catch(const std::runtime_error&){rejected=true;}
        check(rejected,"An immutable table or padding owner admitted a mutable borrower");
    }
    bool rejected=false;
    try{memory.write(0x29af6cu,word);}catch(const std::runtime_error&){rejected=true;}
    check(rejected && memory.checkpoint()==checkpoint,
        "Cross-owner write bypassed immutable access or partially changed the cold bank");
    SourceRuntimeMemory restored;
    restored.restore(checkpoint);
    check(restored.checkpoint()==checkpoint &&
        restored.native_address(0x29af5cu,expected.size()).value>0xffffffffu,
        "Cold checkpoint restore changed exact split-owner bytes/access/lifetimes");
    rejects([&]{(void)restored.view(0x29af6cu,4);},
        "Restored cold bank conflated separate scalar/table owners");
    restored.read(0x29af6cu,read);
    check(read==word,"Restored scalar read changed its original complete bits");
    std::cout<<"PASS actual cold-data factory: retained platform input, exact typed table and independent padding, "
        "immutable access, split views/scatter reads and complete checkpoint ownership; gameplay/bootstrap unqualified\n";
    return 0;
}catch(const std::exception& error){std::cerr<<error.what()<<'\n';return 1;}
