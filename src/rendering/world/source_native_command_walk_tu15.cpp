#include "halo2/native_tu15_command_walk.hpp"
#include "halo2/native_tu15_command_emit.hpp"
#include "halo2/native_tu15_command_part_prepare.hpp"
#include "halo2/native_tu15_material_selection.hpp"
#include "halo2/native_tu15_render_prepare.hpp"
#include "halo2/physics_live_world_tu15.hpp"
#include "halo2/source_live_shadow_storage_tu15.hpp"
#include "halo2/source_opaque_sort.hpp"
#include "halo2/source_runtime_undo_journal.hpp"
#include <algorithm>
#include <atomic>
#include <bit>
#include <cstdlib>
#include <iostream>
#include <optional>
#include <stdexcept>

namespace halo2 {
namespace {
struct QueueStats {
    std::atomic<std::uint64_t> native{},reference{},residual{},appends{},comparisons{},mismatches{};
    void report(const char* flag,const char* name) const {
        if(const auto* v=std::getenv(flag);v&&*v)
            std::cerr<<name<<" stats: native="<<native<<" reference="<<reference<<" residual="<<residual<<
                " appends="<<appends<<" comparisons="<<comparisons<<" mismatches="<<mismatches<<'\n';
    }
};
struct Stats {
    std::atomic<std::uint64_t> frames{},geometry{},parts{},emits{};
    std::atomic<std::uint64_t> native_parts{},reference_parts{},residual_parts{},model_parts{},plane_hits{},comparisons{},mismatches{};
    std::atomic<std::uint64_t> native_emits{},reference_emits{},residual_emits{},bank_appends{},transparent_appends{};
    std::atomic<std::uint64_t> group_native{},group_reference{},group_residual{},group_comparisons{},group_mismatches{},
        group_parts{},group_native_parts{},group_residual_parts{},group_emits{},group_native_emits{},group_residual_emits{},group_appends{};
    QueueStats sections,camouflage,selection;
    std::atomic<std::uint64_t> material_native{},material_reference{},material_residual{},
        material_accepted{},material_class_rejected{},material_post_rejected{};
    ~Stats(){
        if(const auto* v=std::getenv("HALO2_NATIVE_COMMAND_WALK_STATS");v&&*v)
            std::cerr<<"Native40E80 stats: frames="<<frames<<" geometry_calls="<<geometry<<
                " part_calls="<<parts<<" emit_calls="<<emits<<'\n';
        if(const auto* v=std::getenv("HALO2_NATIVE_PART_PREPARE_STATS");v&&*v)
            std::cerr<<"Part45CD0 stats: native="<<native_parts<<" reference="<<reference_parts<<
                " residual="<<residual_parts<<" models="<<model_parts<<" plane_hits="<<plane_hits<<
                " comparisons="<<comparisons<<" mismatches="<<mismatches<<'\n';
        if(const auto* v=std::getenv("HALO2_NATIVE_COMMAND_EMIT_STATS");v&&*v)
            std::cerr<<"Emit41030 stats: native="<<native_emits<<" reference="<<reference_emits<<
                " residual="<<residual_emits<<" banks="<<bank_appends<<" transparent="<<transparent_appends<<'\n';
        if(const auto* v=std::getenv("HALO2_NATIVE_TRANSPARENT_COLLECTION_STATS");v&&*v)
            std::cerr<<"Collect44C80 stats: native="<<group_native<<" reference="<<group_reference<<
                " residual="<<group_residual<<" parts="<<group_parts<<" part_native="<<group_native_parts<<
                " part_residual="<<group_residual_parts<<" emits="<<group_emits<<" emit_native="<<group_native_emits<<
                " emit_residual="<<group_residual_emits<<" bank_appends="<<group_appends<<
                " comparisons="<<group_comparisons<<" mismatches="<<group_mismatches<<'\n';
        sections.report("HALO2_NATIVE_SECTION_QUEUE_STATS","Queue42750");
        camouflage.report("HALO2_NATIVE_CAMOUFLAGE_QUEUE_STATS","Queue42090");
        selection.report("HALO2_NATIVE_COMMAND_SELECTION_STATS","Select3BAA0");
        if(const auto* v=std::getenv("HALO2_NATIVE_MATERIAL_SELECTION_STATS");v&&*v)
            std::cerr<<"Select4CBE0 stats: native="<<material_native<<" reference="<<material_reference<<
                " residual="<<material_residual<<" accepted="<<material_accepted<<
                " class_rejected="<<material_class_rejected<<" post_rejected="<<material_post_rejected<<'\n';
    }
};
Stats& stats(){static Stats s;return s;}
class SeparatedPacketMemory:public SourcePhysicsContactMemory {
    const SourcePhysicsContactMemory& memory_;std::uint32_t packet_;
public:
    SeparatedPacketMemory(const SourcePhysicsContactMemory& m,std::uint32_t p):memory_(m),packet_(p){}
    void read(std::uint32_t at,std::span<std::byte> out)const override {
        if(std::uint64_t(at)<std::uint64_t(packet_)+44u&&std::uint64_t(packet_)<std::uint64_t(at)+out.size())
            throw std::invalid_argument("Native45CD0 input aliases its caller packet");
        memory_.read(at,out);
    }
    void write(std::uint32_t,std::span<const std::byte>)override {
        throw std::invalid_argument("Native45CD0 has no non-packet store owner");
    }
};
void invoke(PhysicsLiveWorldTu15& world,std::uint32_t entry,SourcePhysicsContactRuntimeAbi& abi,NativeTu15PhysicsFrameFp& arithmetic) {
    abi.x87_status=arithmetic.status;
    abi.floating_point=SourcePhysicsFactoryFloatingPoint{arithmetic.control_word,arithmetic.mxcsr};
    world.run_actual(entry,abi);
    arithmetic={abi.floating_point->x87_control_word,abi.x87_status,abi.floating_point->mxcsr};
}
}

bool source_native_command_selection_3baa0(PhysicsLiveWorldTu15& world,SourcePhysicsContactRuntimeAbi& caller,
    const void* camera_receipts) {
    const auto enabled=[](const char* flag){const auto* p=std::getenv(flag);return p&&*p&&*p!='0';};
    static const bool reference=enabled("HALO2_REFERENCE_COMMAND_SELECTION");
    static const bool oracle=enabled("HALO2_COMMAND_SELECTION_ORACLE");
    static const bool material_reference=enabled("HALO2_REFERENCE_MATERIAL_SELECTION");
    auto& counters=stats().selection;
    if(reference) {++counters.reference;return false;}
    const auto residual=[&] {++counters.residual;return false;};
    constexpr auto width=sizeof(SourceOpaqueSortRecord);
    constexpr unsigned capacity=2560;
    auto& memory=world.memory();
    const auto word=[&](std::uint32_t at){return Bytes(memory.view(at,4)).u32(0);};
    const auto put=[&]<class T>(std::uint32_t at,T value){memory.write(at,std::as_bytes(std::span(&value,1)));};
    const auto stack=caller.gp[4];
    if(!caller.floating_point||(stack&3u)||stack<0x3cu||stack>0xffffffe7u||
        !memory.try_write_extent(stack-0x3cu,0x50u))return residual();
    const auto bank=word(stack+4u),category=word(stack+8u),layer=word(stack+12u),output=word(stack+16u);
    if(category>2u||bank!=0x4ce268u+24u*category||output!=stack+0x1cu||
        std::uint64_t(output)+width*capacity>0x100000000ull)return residual();
    const auto limit=word(bank+4u),first=word(bank+12u),base=word(bank+20u);
    if(first>limit||limit>word(bank)||limit>capacity)return residual();
    const auto bytes=width*std::size_t(limit-first);
    const auto layer_mask=1u<<(layer&31u);
    if(bytes) {
        if(!base||std::uint64_t(base)+width*limit>0x100000000ull||
            !memory.try_read_view(base,std::size_t(limit)*width)||!memory.try_write_extent(output,bytes))return residual();
        const auto end=std::uint64_t(base)+width*limit;
        if(std::uint64_t(base)<std::uint64_t(output)+bytes&&std::uint64_t(stack-0x3cu)<end)return residual();
        for(auto ordinal=first;ordinal<limit;++ordinal) {
            const auto record=base+24u*ordinal;
            if((word(record+16u)&layer_mask)&&word(record)!=0x4cbe0u)return residual();
        }
    }
    const std::array ranges{SourceRuntimeNativeWriteRange{stack-0x3cu,0x50u},
        SourceRuntimeNativeWriteRange{output,bytes}};
    const auto writes=std::span(ranges).first(bytes?2u:1u);

    std::vector<SourceOpaqueSortRecord> descriptors(limit-first);
    const auto select=[&](SourcePhysicsContactRuntimeAbi& result) {
        auto child=result;
        std::uint32_t accepted=0;
        put(stack-0x10u,result.gp[5]);
        put(stack-0x14u,result.gp[6]);
        if(first<limit) {
            put(stack-0x18u,result.gp[3]);
            put(stack-0x1cu,result.gp[7]);
            put(stack-4u,layer_mask);
            put(stack-8u,24u*first);
        }
        auto count=limit;
        for(auto ordinal=first;ordinal<count;++ordinal) {
            const auto record=word(bank+20u)+word(stack-8u);
            if(word(record+16u)&layer_mask) {
                const auto slot=output+24u*accepted;
                const auto shader=word(record+8u);
                const auto lod=std::to_integer<std::uint32_t>(memory.view(record+21u,1)[0]);
                put(stack-0x20u,slot);
                put(slot,std::uint16_t(ordinal));
                put(stack-0xcu,++accepted);
                const auto shader_low=Bytes(memory.view(record+8u,2)).u16(0);
                put(slot+2u,shader_low);
                const auto token=word(record+12u);
                put(stack-0x24u,token);
                const auto pass=std::to_integer<std::uint32_t>(memory.view(record+20u,1)[0]);
                put(stack-0x28u,pass);
                put(stack-0x2cu,layer);
                put(stack-0x30u,lod);
                put(stack-0x34u,category);
                put(stack-0x38u,shader);
                const auto target=word(record);
                if(target!=0x4cbe0u)throw FormatError("Native3BAA0 selector changed its admitted callback");
                put(stack-0x3cu,0x3bb1eu);
                // 4CBE0's entry SUB sets its arithmetic flags before use.
                child.gp={record,category,shader,lod,stack-0x3cu,(accepted&0xffff0000u)|shader_low,ordinal,slot+24u};
                std::optional<NativeTu15MaterialSelection> selected;
                if(!material_reference) {
                    try {
                        selected=native_tu15_material_selection_4cbe0(memory,{shader,lod,layer,pass,token,slot,child.gp[4]});
                    } catch(const std::invalid_argument&) {} catch(const std::out_of_range&) {}
                }
                if(selected) {
                    for(const auto& store:selected->rows()) {
                        if(store.width==1u)put(store.address,std::uint8_t(store.value));
                        else if(store.width==2u)put(store.address,std::uint16_t(store.value));
                        else put(store.address,store.value);
                    }
                    child.gp[0]=selected->accepted;
                    child.gp[4]+=0x20u;
                    ++stats().material_native;
                    if(selected->accepted)++stats().material_accepted;
                    else if(selected->class_rejected)++stats().material_class_rejected;
                    else ++stats().material_post_rejected;
                } else {
                    ++stats().material_reference;
                    if(!material_reference)++stats().material_residual;
                    world.run_actual(target,child);
                }
                if(child.gp[4]!=stack-0x1cu)throw FormatError("Native3BAA0 selector escaped its original call frame");
                accepted=word(stack-0xcu);
                // The next candidate reuses the rejected slot with its residue.
                if(!(child.gp[0]&255u))--accepted;
            }
            const auto offset=word(stack-8u);
            count=word(bank+4u);
            if(count>limit)throw FormatError("Native3BAA0 selector expanded its admitted bank");
            put(stack-8u,offset+24u);
        }
        put(stack-0x18u,bank);
        put(stack-0x1cu,layer==5u?0x3bbc0u:0x3bb70u);
        put(stack-0x20u,24u);
        put(stack-0x24u,0x3bb63u);
        if(accepted>1) {
            auto rows=std::span(descriptors).first(accepted);
            memory.read(output,std::as_writable_bytes(rows));
            source_opaque_sort(rows,layer==5u?SourceOpaqueSortOrder::layer5:SourceOpaqueSortOrder::material);
            memory.write(output,std::as_bytes(rows));
        }
        const auto nonvolatile=result.gp;
        result=child;
        for(const auto index:{3u,5u,6u,7u})result.gp[index]=nonvolatile[index];
        result.gp[0]=accepted;
        result.gp[4]=stack+20u;
        ++counters.native;
        counters.appends+=accepted;
    };
    // The retained reference dispatcher includes 3BB70. Layer 5 is checked
    // against its physical XBE result because 3BBC0 has no dispatch owner.
    {
        // Other observers keep the original sort's individual swap and stack
        // stores. Camera receipts only track whether their anchors changed.
        const std::array allowed{camera_receipts};
        const SourceRuntimeNativeWriteLease admission(memory,writes,allowed);
        if(!admission)return residual();
        if(!oracle||layer==5u) {select(caller);return true;}
    }

    auto native=caller;
    std::vector<std::byte> expected(bytes);
    {
        SourceRuntimeUndoJournal undo(memory,SourceRuntimeUndoJournal::Storage::packed,
            [](auto,auto){throw FormatError("Native3BAA0 oracle cannot change World topology");});
        const std::array allowed{camera_receipts,static_cast<const void*>(&undo)};
        const SourceRuntimeNativeWriteLease admission(memory,writes,allowed);
        if(!admission)return residual();
        select(native);
        if(bytes)memory.read(output,expected);
    }
    world.run_actual(0x3baa0u,caller);
    ++counters.comparisons;
    if((bytes&&!std::equal(expected.begin(),expected.end(),memory.view(output,bytes).begin()))||
        caller.gp[0]!=native.gp[0]||caller.gp[4]!=native.gp[4]||caller.x87_status!=native.x87_status||
        caller.floating_point->x87_control_word!=native.floating_point->x87_control_word||
        caller.floating_point->mxcsr!=native.floating_point->mxcsr||caller.xmm!=native.xmm) {
        ++counters.mismatches;
        throw FormatError("Native3BAA0 differs from original descriptors, count, stack or FP");
    }
    return true;
}
namespace {
enum class QueueProducer : std::uint32_t { camouflage=0x42090u, sections=0x42750u };
void queue_sections(PhysicsLiveWorldTu15& world,QueueProducer producer,std::uint32_t mask,NativeTu15PhysicsFrameFp& fp) {
    const auto enabled=[](const char* flag){const auto* p=std::getenv(flag);return p&&*p&&*p!='0';};
    static const bool section_reference=enabled("HALO2_REFERENCE_SECTION_QUEUE");
    static const bool camouflage_reference=enabled("HALO2_REFERENCE_CAMOUFLAGE_QUEUE");
    const bool camouflage=producer==QueueProducer::camouflage;
    auto& counters=camouflage?stats().camouflage:stats().sections;
    const auto actual=[&] {
        auto abi=camouflage?world.prepare_source_call():world.prepare_source_call(std::array{mask});
        invoke(world,std::uint32_t(producer),abi,fp);
        ++counters.reference;
    };
    if(camouflage?camouflage_reference:section_reference) {actual();return;}
    const auto residual=[&] {++counters.residual;actual();};
#if !(defined(__i386__) || defined(__x86_64__)) || defined(_MSC_VER)
    residual();
#else
    static const bool section_oracle=enabled("HALO2_SECTION_QUEUE_ORACLE");
    static const bool camouflage_oracle=enabled("HALO2_CAMOUFLAGE_QUEUE_ORACLE");
    const bool oracle=camouflage?camouflage_oracle:section_oracle;
    constexpr auto sections=0x4cdd4cu,highwater_address=0x4ce1ccu,record_size=0x70u,pool_capacity=0x400u;
    auto& memory=world.memory();
    const auto word=[&](std::uint32_t at) {return Bytes(memory.view(at,4)).u32(0);};
    const auto highwater=std::bit_cast<std::int32_t>(word(highwater_address));
    unsigned selected=0;
    std::uint32_t records=0,record_bytes=0;
    if(highwater>0) {
        if(highwater>32||(fp.control_word&0x3fu)!=0x3fu||(fp.status&0x8080u)||
           (fp.mxcsr&0x1f80u)!=0x1f80u||(fp.mxcsr&0xffff0000u)) {residual();return;}
        const auto count=word(0x4c2aa0u),pool=word(0x4c2aa8u);
        if(count>pool_capacity) {residual();return;}
        for(unsigned index=0;index<unsigned(highwater);++index)
            if(word(sections+36u*index+32u)==(camouflage?1u:3u))++selected;
        const auto appends=std::min(selected,pool_capacity-count);
        if(appends) {
            const auto first=std::uint64_t(pool)+record_size*count;
            const auto end=first+record_size*appends;
            const auto intersects=[&](std::uint32_t begin,std::uint32_t limit) {return first<limit&&begin<end;};
            if(!pool||end>0x100000000ull||intersects(0x4c2aa0u,0x4c2ab2u)||intersects(sections,highwater_address+4u)) {
                residual();return;
            }
            memory.validate_mutable_view(std::uint32_t(first),record_size*appends);
            memory.validate_mutable_view(0x4c2aa0u,4);
            records=std::uint32_t(first);
            record_bytes=record_size*appends;
        }
        if(selected>appends&&memory.view(0x4c2ab1u,1)[0]!=std::byte{})
            memory.validate_mutable_view(0x4c2ab1u,1);
    }
    const auto publish=[&](NativeTu15PhysicsFrameFp& arithmetic) {
        counters.appends+=camouflage?native_tu15_queue_camouflage_42090(memory,arithmetic):
            native_tu15_queue_sections_42750(memory,mask,arithmetic);
        ++counters.native;
    };
    if(!oracle||!selected) {publish(fp);return;}
    const auto outputs=[&] {
        const auto state=memory.view(0x4c2aa0u,0x12u);
        std::vector<std::byte> bytes(state.begin(),state.end());
        if(record_bytes) {
            const auto rows=memory.view(records,record_bytes);
            bytes.insert(bytes.end(),rows.begin(),rows.end());
        }
        return bytes;
    };
    auto expected_fp=fp;
    std::vector<std::byte> expected;
    {
        SourceRuntimeUndoJournal undo(memory,SourceRuntimeUndoJournal::Storage::packed,
            [](auto,auto){throw FormatError("Native transparent queue oracle cannot change World topology");});
        publish(expected_fp);
        expected=outputs();
    }
    actual();
    ++counters.comparisons;
    if(outputs()!=expected||fp.control_word!=expected_fp.control_word||fp.status!=expected_fp.status||fp.mxcsr!=expected_fp.mxcsr) {
        ++counters.mismatches;
        throw FormatError(camouflage?"Native42090 differs from original queue records or FP":
            "Native42750 differs from original queue records or FP");
    }
#endif
}
using LeafFrame=std::function<SourcePhysicsContactRuntimeAbi(std::uint32_t,std::span<const std::uint32_t>)>;
NativeTu15CommandPartServices part_services(PhysicsLiveWorldTu15& world,std::uint32_t packet,
    const LeafFrame& frame,bool group) {
    NativeTu15CommandPartServices s;s.packet=packet;
    // The bank reference is stable for the world's lifetime; the public
    // accessor only re-checks publication/staging state, which cannot change
    // while this frame's source call runs. Taking it once avoids two
    // call-gate mutex operations per access per part. Journaled source-call
    // input writes below are unchanged. HALO2_REFERENCE_WORLD_MEMORY_CALLS=1
    // keeps one accessor call per use.
    static const bool reference_memory_calls=[]{const auto* p=std::getenv("HALO2_REFERENCE_WORLD_MEMORY_CALLS");return p&&*p;}();
    SourceRuntimeMemory *const cached_memory=reference_memory_calls?nullptr:&world.memory();
    const auto bank=[&world,cached_memory]()->SourceRuntimeMemory& {return cached_memory?*cached_memory:world.memory();};
    static const bool reference_part=[]{const auto* p=std::getenv("HALO2_REFERENCE_PART_PREPARE");return p&&*p;}();
    static const bool oracle_part=[]{const auto* p=std::getenv("HALO2_PART_PREPARE_ORACLE");return p&&*p;}();
    static const bool reference_emit=[]{const auto* p=std::getenv("HALO2_REFERENCE_COMMAND_EMIT");return p&&*p&&*p!='0';}();
    s.prepare_45cd0=[&world,bank,packet,frame,group,planes=std::optional<SourceLiveShadowStorageTu15>{}]
        (auto unit,auto part,auto matrix,auto& arithmetic) mutable {
        ++stats().parts;
        if(group)++stats().group_parts;
        const auto actual=[&] {
            ++stats().reference_parts;auto abi=frame(0x45cd0u,std::array{part,matrix});
            abi.gp[0]=packet;abi.gp[1]=unit;invoke(world,0x45cd0u,abi,arithmetic);return (abi.gp[0]&255u)!=0;
        };
        const auto residual=[&] {++stats().residual_parts;if(group)++stats().group_residual_parts;return actual();};
        if(reference_part)return actual();
#if !(defined(__i386__) || defined(__x86_64__)) || defined(_MSC_VER)
        return residual();
#else
        const auto before_fp=arithmetic;std::array<std::byte,44> initial{};bank().read(packet,initial);
        std::optional<NativeTu15PartPacket45cd0> result;
        bool model{};
        try {
            const auto at=0x4c6958u+32u*std::uint32_t(std::bit_cast<std::int16_t>(std::uint16_t(unit)));
            std::uint32_t datum{};bank().read(at+4u,std::as_writable_bytes(std::span(&datum,1)));model=datum!=0xffffffffu;
            if(!model) {
                if(!planes)planes=source_live_shadow_storage_frame_tu15(world);
                else source_live_shadow_storage_validate_tu15(world,*planes);
            }
            const SeparatedPacketMemory memory(bank(),packet);
            result=native_tu15_command_part_prepare_45cd0(memory,unit,part,matrix,initial,arithmetic);
        }catch(const std::invalid_argument&) {arithmetic=before_fp;return residual();}
         catch(const std::out_of_range&) {arithmetic=before_fp;return residual();}
        ++stats().native_parts;if(group)++stats().group_native_parts;
        if(model)++stats().model_parts;
        else if(result->accepted&&result->bytes[0x14]!=std::byte{})++stats().plane_hits;
        if(oracle_part) {
            const auto native_fp=arithmetic;arithmetic=before_fp;const auto accepted=actual();
            std::array<std::byte,44> original{};bank().read(packet,original);++stats().comparisons;
            if(accepted!=result->accepted||original!=result->bytes||arithmetic.control_word!=native_fp.control_word||
               arithmetic.status!=native_fp.status||arithmetic.mxcsr!=native_fp.mxcsr) {
                ++stats().mismatches;throw FormatError("Native45CD0 caller packet, AL or full FP differs from original");
            }
            return accepted;
        }
        for(const auto& store:result->stores) {
            std::array<std::byte,4> bytes{};for(unsigned i=0;i<store.bytes;++i)bytes[i]=std::byte(store.value>>(8*i));
            world.write_source_call_input(packet+store.offset,std::span(bytes).first(store.bytes));
        }
        return result->accepted;
#endif
    };
    s.emit_41030=[&world,bank,frame,group](const NativeTu15CommandEmitArgs& args,NativeTu15PhysicsFrameFp& arithmetic) {
        ++stats().emits;
        if(group)++stats().group_emits;
        const auto actual=[&] {
            ++stats().reference_emits;
            auto abi=frame(0x41030u,std::array{args.mode,args.coverage,args.packet,args.shader,args.grouped,args.mask,args.add});
            abi.gp[1]=args.part;abi.gp[2]=args.unit;invoke(world,0x41030u,abi,arithmetic);
        };
        const auto residual=[&] {++stats().residual_emits;if(group)++stats().group_residual_emits;actual();};
        if(reference_emit){actual();return;}
        auto pending_fp=arithmetic;
        std::optional<NativeTu15CommandEmitStores> pending;
        try {
            pending=native_tu15_command_emit_41030(bank(),args,pending_fp);
        } catch(const std::invalid_argument&) {residual();return;}
          catch(const std::out_of_range&) {residual();return;}
        // Check access for the complete append before any of it is visible.
        // Do not bypass the World write hooks or replace the retained banks.
        for(const auto& store:pending->rows())bank().validate_mutable_view(store.address,store.width);
        for(const auto& store:pending->rows()) {
            std::array<std::byte,4> bytes{};
            for(unsigned i=0;i<store.width;++i)bytes[i]=std::byte(store.value>>(8*i));
            world.write_source_call_input(store.address,std::span(bytes).first(store.width));
            if(store.instruction==0x3ba38u) {++stats().bank_appends;if(group)++stats().group_appends;}
            if(store.instruction==0x1e2d0u)++stats().transparent_appends;
        }
        arithmetic=pending_fp;++stats().native_emits;if(group)++stats().group_native_emits;
    };
    return s;
}
}
NativeTu15PhysicsFrameFp source_native_command_frame_tu15(PhysicsLiveWorldTu15& world,std::uint32_t selector) {
    const auto initial=world.prepare_source_call();const auto controls=initial.floating_point.value();
    NativeTu15PhysicsFrameFp fp{controls.x87_control_word,initial.x87_status,controls.mxcsr};
    // Keep the packet above the scratch arguments of this frame's children.
    const auto packet=initial.gp[4]+0x80u;
    std::array<std::uint32_t,42> retained{};
    world.memory().read(initial.gp[4]+4u,std::as_writable_bytes(std::span(retained)));
    world.prepare_source_call(retained);
    NativeTu15CommandFrameServices s;
    s.parts=part_services(world,packet,[&world](auto,std::span<const std::uint32_t> args) {
        return world.prepare_source_call(args);
    },false);
    static const bool reference=[]{const auto* p=std::getenv("HALO2_REFERENCE_RENDER_PREPARE");return p&&*p;}();
    s.parts.geometry_449d0=[&](auto unit,auto owner,auto& arithmetic) {
        ++stats().geometry;
        if(!reference)return source_native_render_geometry_tu15(world,unit,0,std::uint8_t(owner),arithmetic);
        auto abi=world.prepare_source_call(std::array{owner});
        abi.gp[0]=unit;abi.gp[2]=0;invoke(world,0x449d0u,abi,arithmetic);return abi.gp[0];
    };
    s.prepare_41c70=[&](auto context,auto mode,auto& arithmetic) {
        if(!reference){source_native_render_prepare_tu15(world,context,mode,arithmetic);return;}
        auto abi=world.prepare_source_call();abi.gp[7]=context;abi.gp[3]=mode;invoke(world,0x41c70u,abi,arithmetic);
    };
    const auto no_args=[&](auto entry,auto& arithmetic) {auto abi=world.prepare_source_call();invoke(world,entry,abi,arithmetic);};
    s.producer_179a40=[&](auto& arithmetic){no_args(0x179a40u,arithmetic);};
    s.producer_179b40=[&](auto& arithmetic){no_args(0x179b40u,arithmetic);};
    s.producer_1770c0=[&](auto& arithmetic){auto abi=world.prepare_source_call(std::array{0u});invoke(world,0x1770c0u,abi,arithmetic);};
    s.producer_42090=[&](auto& arithmetic){queue_sections(world,QueueProducer::camouflage,0,arithmetic);};
    s.producer_42750=[&](auto mask,auto& arithmetic){queue_sections(world,QueueProducer::sections,mask,arithmetic);};
    s.producer_41630=[&](auto& arithmetic){no_args(0x41630u,arithmetic);};
    native_tu15_command_frame_40e80(world.memory(),selector,fp,s);++stats().frames;return fp;
}

void source_native_transparent_collection_tu15(PhysicsLiveWorldTu15& world,SourcePhysicsContactRuntimeAbi& caller) {
    const auto actual=[&] {++stats().group_reference;world.run_actual(0x44c80u,caller);};
    static const bool reference=[]{const auto* p=std::getenv("HALO2_REFERENCE_TRANSPARENT_COLLECTION");return p&&*p&&*p!='0';}();
    static const bool oracle=[]{const auto* p=std::getenv("HALO2_TRANSPARENT_COLLECTION_ORACLE");return p&&*p&&*p!='0';}();
    if(reference) {actual();return;}
    const auto residual=[&] {++stats().group_residual;actual();};
#if !(defined(__i386__) || defined(__x86_64__)) || defined(_MSC_VER)
    (void)oracle;residual();
#else
    auto& memory=world.memory();
    const auto stack=caller.gp[4];
    if(!caller.floating_point||(stack&3u)||stack<0x70u||stack>0xffffffdbu) {residual();return;}
    std::array<std::uint32_t,8> words{};
    memory.read(stack+4u,std::as_writable_bytes(std::span(words)));
    const NativeTu15CommandGroup44c80 args{words[0],words[1],words[2],words[3],words[4],words[5],words[6],words[7]};
    const auto word=[&](std::uint32_t at) {return Bytes(memory.view(at,4)).u32(0);};
    // The live 423B0 corridor owns a completed mode-zero kind-3 group.
    // Other callers may use the original until their stack and banks are bound.
    const auto prefix=word(0x4cd39cu),draw_count=word(0x4cd398u);
    if(args.mode!=0||args.section>=31u||prefix!=draw_count||prefix>850u) {residual();return;}
    const auto packet=stack-0x2cu;
    memory.validate_mutable_view(stack-0x70u,0x94u);
    const auto run_native=[&](SourcePhysicsContactRuntimeAbi& result) {
        auto fp=NativeTu15PhysicsFrameFp{result.floating_point->x87_control_word,result.x87_status,result.floating_point->mxcsr};
        const auto services=part_services(world,packet,[&](auto entry,std::span<const std::uint32_t> arguments) {
            // The source parent has already reserved E-0x50..E at the call
            // sites. Put genuine residual children below its packet/locals.
            auto child=result;
            const auto child_stack=stack-0x50u-4u*std::uint32_t(arguments.size()+1);
            std::array<std::uint32_t,8> frame{};
            frame[0]=entry==0x45cd0u?0x44e11u:0x44e55u;
            std::copy(arguments.begin(),arguments.end(),frame.begin()+1);
            world.write_source_call_input(child_stack,std::as_bytes(std::span(frame).first(arguments.size()+1)));
            child.gp[4]=child_stack;
            return child;
        },true);
        result.gp[0]=native_tu15_command_group_44c80(memory,args,stack,fp,services);
        result.gp[4]=stack+36u;
        result.x87_status=fp.status;
        result.floating_point=SourcePhysicsFactoryFloatingPoint{fp.control_word,fp.mxcsr};
        ++stats().group_native;
    };
    if(!oracle) {run_native(caller);return;}
    const auto capacity=word(0x4ce268u),records=word(0x4ce27cu);
    if(!records||!capacity||capacity>2560u)throw FormatError("Native44C80 oracle lost the constructed bank0 capacity");
    const auto outputs=[&] {
        std::vector<std::byte> bytes;
        const auto append=[&](std::uint32_t at,std::size_t size) {
            const auto source=memory.view(at,size);
            bytes.insert(bytes.end(),source.begin(),source.end());
        };
        append(0x4ce268u,72u);
        append(records,24u*capacity);
        append(packet,44u);
        append(stack+0x10u,8u);
        return bytes;
    };
    auto expected=caller;
    std::vector<std::byte> expected_outputs;
    {
        SourceRuntimeUndoJournal undo(memory,SourceRuntimeUndoJournal::Storage::packed,
            [](auto,auto){throw FormatError("Native44C80 oracle cannot change World topology");});
        run_native(expected);
        expected_outputs=outputs();
    }
    actual();
    ++stats().group_comparisons;
    const bool abi_equal=caller.gp[0]==expected.gp[0]&&caller.gp[3]==expected.gp[3]&&caller.gp[4]==expected.gp[4]&&
        caller.gp[5]==expected.gp[5]&&caller.gp[6]==expected.gp[6]&&caller.gp[7]==expected.gp[7];
    const bool fp_equal=caller.x87_status==expected.x87_status&&
        caller.floating_point->x87_control_word==expected.floating_point->x87_control_word&&
        caller.floating_point->mxcsr==expected.floating_point->mxcsr;
    if(!abi_equal||!fp_equal||outputs()!=expected_outputs) {
        ++stats().group_mismatches;
        throw FormatError("Native44C80 differs from original caller, full command storage or FP");
    }
#endif
}
}
