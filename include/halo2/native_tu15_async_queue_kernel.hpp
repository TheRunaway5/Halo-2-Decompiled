#pragma once
#include <bit>
#include <cstdint>

namespace halo2 {
enum class NativeAsyncOrderRead {
    other_callback,history,incoming_callback,valid,history_file,incoming_file,
    other_file,history_offset,incoming_offset,other_offset,record_file,record_offset
};
enum class NativeAsyncOrderWrite {file,offset,valid};
enum class NativeAsyncOrderLevel {priority,callback,history_valid,file,offset};
struct NativeAsyncOrder {
    std::int32_t incoming_priority{},other_priority{};
    std::uint32_t incoming{},other{},history{},read_callback{},other_callback{},incoming_callback{},valid{};
    std::uint32_t origin_file{},incoming_file{},other_file{},incoming_file_distance{},other_file_distance{};
    std::uint32_t origin_offset{},incoming_offset{},other_offset{},incoming_offset_distance{},other_offset_distance{};
    std::uint32_t record_file{},record_offset{};
    NativeAsyncOrderLevel level{NativeAsyncOrderLevel::priority};bool before{};
};

// Priority comes first, then read-callback affinity and unsigned locality
// relative to the live insertion history. History publication is deliberately
// interleaved: writing the file can alias the subsequent other-offset read.
// Access owns storage and a separate optional ABI view; this algorithm has no
// register bank, processor flags, instruction cursor, or source-PC decisions.
template<class Access>bool native_async_before(Access&access,NativeAsyncOrder&d){
    using R=NativeAsyncOrderRead;using W=NativeAsyncOrderWrite;using L=NativeAsyncOrderLevel;
    d.before=false;d.level=L::priority;
    d.other_callback=access.read(R::other_callback,d);
    d.history=access.read(R::history,d);
    if(d.incoming_priority>d.other_priority)d.before=true;
    else if(d.incoming_priority==d.other_priority){
        d.level=L::callback;d.incoming_callback=access.read(R::incoming_callback,d);
        if(d.incoming_callback==d.read_callback){
            if(d.other_callback!=d.read_callback)d.before=true;
            else{
                d.level=L::history_valid;d.valid=access.read(R::valid,d);
                if(d.valid&255u){
                    d.level=L::file;d.origin_file=access.read(R::history_file,d);
                    d.incoming_file=access.read(R::incoming_file,d);d.other_file=access.read(R::other_file,d);
                    d.incoming_file_distance=d.incoming_file-d.origin_file;d.other_file_distance=d.other_file-d.origin_file;
                    if(d.incoming_file_distance<d.other_file_distance)d.before=true;
                    else if(d.incoming_file_distance==d.other_file_distance){
                        d.level=L::offset;d.origin_offset=access.read(R::history_offset,d);
                        d.incoming_offset=access.read(R::incoming_offset,d);d.other_offset=access.read(R::other_offset,d);
                        d.incoming_offset_distance=d.incoming_offset-d.origin_offset;d.other_offset_distance=d.other_offset-d.origin_offset;
                        d.before=d.incoming_offset_distance<d.other_offset_distance;
                    }
                }
            }
        }
    }
    access.decision(d);
    if(d.other_callback==d.read_callback){
        d.record_file=access.read(R::record_file,d);access.write(W::file,d,d.record_file);
        d.record_offset=access.read(R::record_offset,d);access.write(W::offset,d,d.record_offset);
        access.write(W::valid,d,1u);
    }
    access.finish(d);return d.before;
}

enum class NativeAsyncInsertRead {head,incoming,incoming_priority,head_priority,head_next,
    incoming_callback,successor,successor_callback,saved_callback,successor_priority,
    after_compare_next,saved_incoming,saved_priority,insertion_next};
enum class NativeAsyncInsertWrite {head,new_next,previous_next};
enum class NativeAsyncInsertExit {empty,head,before_successor,append};
struct NativeAsyncInsertion {
    std::uint32_t head{},incoming{},previous{},successor{},next{},incoming_priority{},head_priority{};
    std::uint32_t incoming_callback{},successor_callback{},successor_priority{};
    NativeAsyncInsertExit exit{NativeAsyncInsertExit::append};bool scanning{};
};
struct NativeAsyncCompareResume {bool before{};std::uint32_t previous{},successor{};};

// Insert into the real priority/locality list. Null tokens and node addressing
// belong to Access, allowing both the existing value queue and actual backed
// pointer queue to share this policy. Compare is a genuine continuation: its
// resumed predecessor/successor are authoritative, including saved-frame aliases.
template<class Access>void native_async_insert(Access&access,NativeAsyncInsertion&d){
    using R=NativeAsyncInsertRead;using W=NativeAsyncInsertWrite;using E=NativeAsyncInsertExit;
    const auto signed_word=[](std::uint32_t v){return std::bit_cast<std::int32_t>(v);};
    access.begin(d);d.head=access.read(R::head,d);access.history(d);d.incoming=access.read(R::incoming,d);
    if(access.is_null(d.head)){
        d.exit=E::empty;access.write(W::head,d,d.incoming);access.write(W::new_next,d,d.head);access.finish(d);return;
    }
    d.incoming_priority=access.read(R::incoming_priority,d);d.head_priority=access.read(R::head_priority,d);access.priority(d);
    if(signed_word(d.incoming_priority)>signed_word(d.head_priority)){
        d.exit=E::head;access.write(W::new_next,d,d.head);access.write(W::head,d,d.incoming);access.finish(d);return;
    }
    d.previous=d.head;d.next=access.read(R::head_next,d);d.scanning=!access.is_null(d.next);access.candidates(d);
    if(d.scanning){
        d.incoming_callback=access.read(R::incoming_callback,d);access.callback(d);
        for(;;){
            d.successor=access.read(R::successor,d);d.successor_callback=access.read(R::successor_callback,d);
            d.incoming_callback=access.read(R::saved_callback,d);d.successor_priority=access.read(R::successor_priority,d);
            const auto resumed=access.compare(d);d.previous=resumed.previous;d.successor=resumed.successor;
            if(resumed.before){
                d.exit=E::before_successor;d.next=access.read(R::insertion_next,d);d.incoming=access.read(R::saved_incoming,d);
                access.restore_candidates(d);access.write(W::new_next,d,d.next);access.restore_previous(d);
                access.write(W::previous_next,d,d.incoming);access.finish(d);return;
            }
            d.previous=d.successor;d.next=access.read(R::after_compare_next,d);
            if(access.is_null(d.next)){d.incoming=access.read(R::saved_incoming,d);break;}
            d.incoming_priority=access.read(R::saved_priority,d);
        }
    }
    d.exit=E::append;access.restore_candidates(d);access.write(W::previous_next,d,d.incoming);access.restore_previous(d);
    access.write(W::new_next,d,access.null());access.finish(d);
}
} // namespace halo2
