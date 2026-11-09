#include "halo2/native_tu15_main_heap_allocate.hpp"
#include <bit>
#include <stdexcept>

namespace halo2 {
namespace {
using Slot=NativeTu15MainHeapAllocationSlot;
using Phase=NativeTu15MainHeapAllocationPhase;
using Data=NativeTu15MainHeapAllocationPhaseData;
std::int32_t signed_word(std::uint32_t value){return std::bit_cast<std::int32_t>(value);}
class Memory {
    SourcePhysicsContactMemory&memory_;
    const SourcePhysicsContactRecordObserver&observe_;
public:
    Memory(SourcePhysicsContactMemory&m,const SourcePhysicsContactRecordObserver&o):memory_(m),observe_(o){}
    std::uint32_t word(std::uint32_t at)const {
        std::array<std::byte,4> bytes{};memory_.read(at,bytes);std::uint32_t value{};
        for(unsigned n=0;n<4;++n)value|=std::to_integer<unsigned>(bytes[n])<<(8u*n);return value;
    }
    std::uint32_t byte(std::uint32_t at)const {
        std::array<std::byte,1> value{};memory_.read(at,value);return std::to_integer<unsigned>(value[0]);
    }
    void word(std::uint32_t site,std::uint32_t at,std::uint32_t value) {
        SourcePhysicsContactRecordStore event{site,at,{},{}};
        if(observe_){event.before.resize(4);memory_.read(at,event.before);}
        std::array<std::byte,4> bytes{};for(unsigned n=0;n<4;++n)bytes[n]=std::byte(value>>(8u*n));
        memory_.write(at,bytes);
        if(observe_){event.written.assign(bytes.begin(),bytes.end());observe_(event);}
    }
};
NativeTu15MainHeapAlignmentResidual residual(std::uint32_t site,std::uint32_t owner,
    NativeTu15MainHeapAllocationStats*stats) {
    if(stats){++stats->alignment_requests;++stats->unbound_alignment_owners[owner];}
    return {true,site,owner};
}
}

NativeTu15MainHeapGapResult native_tu15_probe_main_heap_gap(SourcePhysicsContactMemory&memory,
    const NativeTu15MainHeapGapRequest&request,const NativeTu15MainHeapGapArguments&arguments,
    NativeTu15MainHeapCandidateSink&sink,const NativeTu15MainHeapAllocationPhases&phases,
    const SourcePhysicsContactRecordObserver&observe,NativeTu15MainHeapAllocationStats*stats) {
    if(stats)++stats->gap_probes;Memory m(memory,observe);Data data;
    data.cursor=request.cursor;data.bytes=request.bytes;data.bound=request.limit;
    data.previous_offset=request.cursor;
    const auto notify=[&](Phase phase){if(phases.observe)phases.observe(phase,data);};
    NativeTu15MainHeapGapResult result{0,request.cursor,request.bytes,request.limit,0,0,{}};
    notify(Phase::gap_begin);
    if(!arguments.allocator)throw std::invalid_argument("Native main heap gap needs its actual allocator argument");
    data.allocator=arguments.allocator();notify(Phase::gap_allocator);
    data.capacity_exhausted=signed_word(request.limit)<signed_word(result.cursor);
    notify(Phase::gap_capacity);
    if(signed_word(request.limit)>=signed_word(result.cursor)) {
        if(!arguments.memory_class)throw std::invalid_argument("Native main heap gap needs its actual memory class argument");
        data.memory_class=arguments.memory_class();notify(Phase::gap_class);
        data.category=m.byte(0x232d7bu+data.memory_class);result.category=data.category;notify(Phase::gap_category);
        data.target=m.word(0x232d70u+data.category*4u);result.target=data.target;notify(Phase::gap_branch);
        if(data.target!=0x232d4eu&&data.target!=0x232d64u) {
            result.residual=residual(0x232d47u,data.target,stats);return result;
        }
        if(data.target==0x232d64u) {
            data.previous_offset=result.cursor;result.cursor=(result.cursor+15u)&0xfffffff0u;
            data.cursor=result.cursor;data.aligned=true;notify(Phase::gap_aligned);
        }
        data.end=result.cursor+request.bytes;data.capacity_checked=true;
        data.capacity_exhausted=signed_word(request.limit)<signed_word(data.end);notify(Phase::gap_candidate);
        if(signed_word(request.limit)>=signed_word(data.end)) {
            sink.candidate(0,result.cursor,0x232d55u);
            sink.candidate(1,request.bytes,0x232d57u);
            data.base=m.word(data.allocator+0x18u);notify(Phase::gap_base);
            result.allocation=data.base+result.cursor;data.result=result.allocation;notify(Phase::gap_result);
            if(stats)++stats->gap_successes;
        }
    }
    data.cursor=result.cursor;data.result=result.allocation;notify(Phase::gap_returning);return result;
}

NativeTu15MainHeapAllocationResult native_tu15_allocate_main_heap_with_phases(
    SourcePhysicsContactMemory&memory,std::uint32_t allocator,NativeTu15MainHeapAllocationFrame&frame,
    const NativeTu15MainHeapAllocationServices&services,const NativeTu15MainHeapAllocationPhases&phases,
    const SourcePhysicsContactRecordObserver&observe,NativeTu15MainHeapAllocationStats*stats) {
    if(stats)++stats->allocations;Memory m(memory,observe);Data data;data.allocator=allocator;
    const auto notify=[&](Phase phase){if(phases.observe)phases.observe(phase,data);};
    notify(Phase::begin);
    const auto initial_count=m.word(allocator+0x20u);data.count=initial_count;data.previous_count=initial_count;
    notify(Phase::count);
    std::uint32_t cursor{},initial_tail_end{},last_cursor{},bytes=initial_count;
    frame.store(Slot::result,0,0x232ae2u);
    if(signed_word(initial_count)>0) {
        data.end=m.word(allocator+0x1cu);data.field=0;notify(Phase::tail_header);
        data.base=m.word(allocator+0x18u);data.field=1;notify(Phase::tail_header);
        last_cursor=data.end-initial_count*8u+data.base;data.row=last_cursor;notify(Phase::tail_cursor);
        data.offset=m.word(last_cursor);data.field=0;notify(Phase::tail_record);
        data.length=m.word(last_cursor+4u);data.field=1;notify(Phase::tail_record);
        initial_tail_end=data.offset+data.length;
    }
    data.end=m.word(allocator+0x1cu);notify(Phase::capacity_end);
    last_cursor=data.end-(initial_count*8u+8u);
    data.cursor=initial_tail_end;data.bound=last_cursor;notify(Phase::capacity);
    if(signed_word(initial_tail_end)<signed_word(last_cursor)) {
        frame.store(Slot::index,0,0x232b1bu);
        if(signed_word(initial_count)>0) {
            data.base=m.word(allocator+0x18u);data.row=data.base+data.end-4u;
            auto row=data.row;last_cursor=row;notify(Phase::search_begin);
            frame.store(Slot::row_cursor,row,0x232b30u);
            for(;;) {
                if(stats)++stats->visited_rows;
                bytes=frame.load(Slot::bytes);const auto row_offset=m.word(row-4u);
                data.cursor=cursor;data.bytes=bytes;data.offset=row_offset;data.row=row;data.end=cursor+bytes;
                notify(Phase::gap_inputs);
                bool inserted=false;
                if(signed_word(row_offset)>=signed_word(cursor+bytes)) {
                    const auto memory_class=frame.load(Slot::memory_class);
                    data.memory_class=memory_class;notify(Phase::gap_call);
                    const NativeTu15MainHeapGapRequest request{cursor,bytes,row_offset};
                    const auto probe=services.probe_gap?services.probe_gap(request,allocator,memory_class,frame):
                        native_tu15_probe_main_heap_gap(memory,request,{[allocator]{return allocator;},
                            [memory_class]{return memory_class;}},frame,{},observe,stats);
                    if(probe.residual.required)return {probe.allocation,probe.cursor,probe.bytes,probe.residual};
                    last_cursor=probe.cursor;bytes=probe.bytes;data.cursor=probe.cursor;data.bytes=bytes;
                    data.result=probe.allocation;notify(Phase::gap_return);
                    frame.store(Slot::result,probe.allocation,0x232b55u);
                    if(probe.allocation) {
                        const auto count=m.word(allocator+0x20u);
                        const auto index=frame.load(Slot::index);
                        data.previous_count=count;data.count=count+1u;data.index=index;notify(Phase::insert_count);
                        m.word(0x232b88u,allocator+0x20u,count+1u);
                        if(signed_word(count)>signed_word(index)) {
                            auto shift_offset=count*8u,remaining=count-index;
                            data.shift_offset=shift_offset;data.remaining=remaining;notify(Phase::insert_move_begin);
                            for(;;) {
                                data.end=m.word(allocator+0x1cu);data.field=0;notify(Phase::insert_header);
                                data.base=m.word(allocator+0x18u);data.field=1;notify(Phase::insert_header);
                                const auto source=data.base+data.end-shift_offset;
                                const auto first=m.word(source);data.row=source;data.value=first;data.field=0;
                                notify(Phase::insert_word);m.word(0x232bacu,source-8u,first);
                                const auto second=m.word(source+4u);data.value=second;data.field=1;notify(Phase::insert_word);
                                shift_offset-=8u;--remaining;
                                data.shift_offset=shift_offset;data.remaining=remaining;notify(Phase::insert_advance);
                                m.word(0x232bb6u,source-4u,second);last_cursor=source;
                                if(stats)++stats->shifted_rows;
                                if(!remaining)break;
                            }
                        }
                        data.base=m.word(allocator+0x18u);data.field=0;notify(Phase::insert_publish_header);
                        data.end=m.word(allocator+0x1cu);data.field=1;notify(Phase::insert_publish_header);
                        const auto record=data.base+data.end-(index*8u+8u);
                        data.row=record;data.value=frame.load(Slot::candidate_offset);data.field=0;
                        notify(Phase::insert_candidate);m.word(0x232bceu,record,data.value);
                        data.value=frame.load(Slot::candidate_bytes);data.field=1;
                        notify(Phase::insert_candidate);m.word(0x232bd5u,record+4u,data.value);
                        bytes=frame.load(Slot::bytes);last_cursor=data.base;inserted=true;
                    } else {
                        bytes=frame.load(Slot::bytes);row=frame.load(Slot::row_cursor);last_cursor=row;
                    }
                }
                if(inserted)break;
                const auto row_length=m.word(row);
                const auto index=frame.load(Slot::index);
                const auto count=m.word(allocator+0x20u);
                data.previous_offset=cursor;cursor=row_offset+row_length;
                data.previous_row=row;row-=8u;last_cursor=row;
                data.previous_index=index;data.index=index+1u;data.count=count;
                data.offset=row_offset;data.length=row_length;data.cursor=cursor;data.row=row;data.bytes=bytes;
                notify(Phase::search_advance);
                frame.store(Slot::index,index+1u,0x232b74u);
                frame.store(Slot::row_cursor,row,0x232b78u);
                if(signed_word(index+1u)>=signed_word(count))break;
            }
            data.result=frame.load(Slot::result);data.cursor=last_cursor;data.bytes=bytes;notify(Phase::result_check);
            if(data.result) {
                data.result=frame.load(Slot::result);notify(Phase::returning);
                if(stats&&!data.result)++stats->exhausted;
                return {data.result,last_cursor,bytes,{}};
            }
        } else bytes=frame.load(Slot::bytes);
        // No admitted gap returned a retained result: try the current tail.
        data.count=m.word(allocator+0x20u);data.field=0;notify(Phase::tail_inputs);
        data.end=m.word(allocator+0x1cu);data.field=1;notify(Phase::tail_inputs);
        const auto end=data.end;
        last_cursor=end-(data.count*8u+8u);
        data.cursor=cursor;data.bound=last_cursor;data.bytes=bytes;notify(Phase::tail_capacity);
        std::uint32_t allocation{};
        bool fits=signed_word(last_cursor)>=signed_word(cursor);
        if(fits) {
            data.memory_class=frame.load(Slot::memory_class);notify(Phase::memory_class);
            data.category=m.byte(0x232c67u+data.memory_class);notify(Phase::category);
            data.target=m.word(0x232c5cu+data.category*4u);notify(Phase::branch);
            if(data.target!=0x232c1au&&data.target!=0x232c14u)
                return {0,last_cursor,bytes,residual(0x232c0du,data.target,stats)};
            if(data.target==0x232c14u) {
                data.previous_offset=cursor;cursor=(cursor+15u)&0xfffffff0u;
                data.cursor=cursor;data.aligned=true;notify(Phase::aligned_cursor);
            }
            data.offset=cursor+bytes;notify(Phase::tail_capacity);
            fits=signed_word(last_cursor)>=signed_word(cursor+bytes);
            if(fits) {
                data.base=m.word(allocator+0x18u);notify(Phase::tail_result_base);
                allocation=data.base+cursor;data.result=allocation;notify(Phase::tail_result);
            }
        }
        if(!fits) {
            bytes=frame.load(Slot::candidate_bytes);cursor=frame.load(Slot::candidate_offset);
        }
        data.result=allocation;data.cursor=cursor;data.bytes=bytes;
        frame.store(Slot::result,allocation,0x232c32u);
        if(allocation) {
            data.previous_count=m.word(allocator+0x20u);data.field=0;notify(Phase::tail_insert_inputs);
            data.base=m.word(allocator+0x18u);data.field=1;notify(Phase::tail_insert_inputs);
            data.count=data.previous_count+1u;data.end=end;notify(Phase::tail_insert_count);
            m.word(0x232c3fu,allocator+0x20u,data.count);
            last_cursor=data.count*8u;
            const auto record=data.base-last_cursor+end;
            data.row=record;data.value=cursor;data.field=0;notify(Phase::tail_insert_word);
            m.word(0x232c47u,record,cursor);
            data.value=bytes;data.field=1;notify(Phase::tail_insert_word);
            m.word(0x232c4au,record+4u,bytes);
            if(stats)++stats->tail_successes;
        }
    }
    data.result=frame.load(Slot::result);data.cursor=last_cursor;data.bytes=bytes;notify(Phase::returning);
    if(stats&&!data.result)++stats->exhausted;
    return {data.result,last_cursor,bytes,{}};
}

std::uint32_t native_tu15_allocate_main_heap(SourcePhysicsContactMemory&memory,std::uint32_t allocator,
    std::uint32_t bytes,std::uint32_t memory_class,const SourcePhysicsContactRecordObserver&observe,
    NativeTu15MainHeapAllocationStats*stats) {
    NativeTu15MainHeapAllocationValues frame(bytes,memory_class);
    const auto result=native_tu15_allocate_main_heap_with_phases(memory,allocator,frame,{}, {},observe,stats);
    if(result.residual.required)throw std::runtime_error("Native TU1.5 main heap allocation requires actual alignment owner="+
        std::to_string(result.residual.owner));
    return result.allocation;
}
} // namespace halo2
