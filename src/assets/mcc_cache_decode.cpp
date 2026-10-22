#include "halo2/mcc_cache.hpp"
#include "halo2/cache.hpp"
#include <algorithm>
#include <array>
#include <atomic>
#include <bit>
#include <exception>
#include <condition_variable>
#include <functional>
#include <mutex>
#include <memory>
#include <system_error>
#include <thread>
#include <zlib.h>
#ifdef HALO2_HAVE_LIBDEFLATE
#include <libdeflate.h>
#endif

namespace halo2 {
namespace {
constexpr std::size_t header_size=896,maximum_cache=1024ull*1024*1024;
std::vector<std::byte> inflate_reference(std::span<const std::byte> input,std::size_t maximum) {
    if(!maximum || maximum>maximum_cache || input.size()>maximum_cache)
        throw FormatError("Invalid MCC zlib bound");
    std::vector<std::byte> result(std::min<std::size_t>(maximum,65536));
    z_stream stream{};
    stream.next_in=reinterpret_cast<Bytef*>(const_cast<std::byte*>(input.data()));
    stream.avail_in=uInt(input.size());
    if(inflateInit(&stream)!=Z_OK)throw FormatError("MCC zlib initialization failed");
    struct End {z_stream* stream;~End(){inflateEnd(stream);}} end{&stream};
    for(;;) {
        if(stream.total_out==result.size()) {
            if(result.size()==maximum) {
                // A stream can consume its trailer after filling the exact
                // declared output extent; one extra byte detects a bomb.
                result.resize(maximum+1);
            } else result.resize(std::min(maximum,result.size()*2));
        }
        stream.next_out=reinterpret_cast<Bytef*>(result.data()+stream.total_out);
        stream.avail_out=uInt(result.size()-stream.total_out);
        const auto status=::inflate(&stream,Z_NO_FLUSH);
        if(stream.total_out>maximum)throw FormatError("MCC zlib output exceeds declared extent");
        if(status==Z_STREAM_END) {
            if(stream.avail_in || !stream.total_out)throw FormatError("Trailing or empty MCC zlib stream");
            result.resize(stream.total_out);return result;
        }
        if(status!=Z_OK || (!stream.avail_in && stream.avail_out))
            throw FormatError("Invalid or truncated MCC zlib stream");
    }
}
// Reuse workers for every batch in this file. Short-lived chunk threads
// discard their decoder state and can cost more than the decompression work.
class ChunkWorkers {
    std::mutex mutex_;
    std::condition_variable_any ready_;
    std::condition_variable done_;
    std::function<void()> job_;
    std::size_t generation_{},remaining_{},count_{};
    // Declared last so workers stop and join before their shared state dies.
    std::array<std::jthread,3> threads_;
public:
    explicit ChunkWorkers(unsigned workers) {
        for(unsigned i=1;i<workers;++i) {
            try {
                threads_[i-1]=std::jthread([this](std::stop_token stop) {
                    std::size_t seen{};
                    for(;;) {
                        std::unique_lock lock(mutex_);
                        if(!ready_.wait(lock,stop,[&]{return generation_!=seen;}))return;
                        seen=generation_;lock.unlock();job_();lock.lock();
                        if(!--remaining_)done_.notify_one();
                    }
                });++count_;
            } catch(const std::system_error&) {break;}
        }
    }
    void run(std::function<void()> job) {
        {
            std::lock_guard lock(mutex_);job_=std::move(job);
            remaining_=count_;++generation_;
        }
        ready_.notify_all();job_();
        std::unique_lock lock(mutex_);done_.wait(lock,[&]{return !remaining_;});
    }
};
std::vector<std::byte> inflate_chunk(std::span<const std::byte> input,std::size_t maximum) {
#ifdef HALO2_HAVE_LIBDEFLATE
    // libdeflate selects CPU instructions at runtime. Keep the generic zlib
    // decoder for unavailable accelerators, large bounds and unusual streams.
    if(maximum && maximum<=8*1024*1024 && input.size()<=maximum_cache) {
        struct Delete {void operator()(libdeflate_decompressor* p) const {libdeflate_free_decompressor(p);}};
        thread_local std::unique_ptr<libdeflate_decompressor,Delete> decoder{libdeflate_alloc_decompressor()};
        if(decoder) {
            std::vector<std::byte> result(maximum);
            std::size_t consumed{},written{};
            if(libdeflate_zlib_decompress_ex(decoder.get(),input.data(),input.size(),
                result.data(),result.size(),&consumed,&written)==LIBDEFLATE_SUCCESS &&
                consumed==input.size() && written && written<=maximum) {
                result.resize(written);return result;
            }
        }
    }
#endif
    return inflate_reference(input,maximum);
}
}
std::vector<std::byte> decode_mcc_cache(std::span<const std::byte> raw) {
    const Bytes input(raw);input.require(0,header_size);
    if(input.u32(0)!=0x68656164 || input.u32(4)!=13)throw FormatError("Expected MCC Halo 2 v13 cache");
    const auto extent=input.u32(8);
    if(extent<=header_size || extent>maximum_cache)throw FormatError("Invalid MCC decoded extent");
    if(!(input.u16(28)&1)) {
        if(extent!=raw.size())throw FormatError("Truncated uncompressed MCC cache");
        return {raw.begin(),raw.end()};
    }
    const auto chunk_size=input.u32(776),table=input.u32(784),count=input.u32(788);
    if(!chunk_size || chunk_size>maximum_cache || !count || count>65536)throw FormatError("Invalid MCC chunk count/size");
    input.require(table,std::size_t(count)*8);
    std::vector<std::byte> decoded(raw.begin(),raw.begin()+header_size);
    decoded.reserve(extent);
    // Each table entry owns an independent stream. Limit pending output to
    // four chunks and 32 MiB; small caches and large chunk bounds stay serial.
    const auto bound=std::min<std::size_t>(chunk_size,extent-header_size);
    const auto workers=count>=8 && extent-header_size>=1024*1024 && bound<=8*1024*1024?
        std::clamp(std::thread::hardware_concurrency(),1u,4u):1u;
    ChunkWorkers pool(workers);
    struct Chunk {std::span<const std::byte> raw;std::vector<std::byte> bytes;bool compressed{};std::exception_ptr error;};
    for(std::size_t base=0;base<count;) {
        const auto n=std::min<std::size_t>(workers,count-base);
        std::array<Chunk,4> chunks;
        for(std::size_t j=0;j<n;++j) {
            const auto at=table+(base+j)*8;
            const auto stored=std::bit_cast<std::int32_t>(input.u32(at));
            const auto address=input.u32(at+4);
            if(!stored || address<header_size || address>0x7fffffffu)
                throw FormatError("Invalid MCC chunk location");
            const auto size=std::size_t(stored<0?-std::int64_t(stored):stored);
            if(stored<0 && size>bound)throw FormatError("Excessive raw MCC chunk");
            chunks[j].raw=input.slice(address,size);chunks[j].compressed=stored>0;
        }
        std::atomic_size_t next{};
        const auto work=[&] {
            for(auto j=next.fetch_add(1,std::memory_order_relaxed);j<n;j=next.fetch_add(1,std::memory_order_relaxed)) {
                try {if(chunks[j].compressed)chunks[j].bytes=inflate_chunk(chunks[j].raw,bound);}
                catch(...) {chunks[j].error=std::current_exception();}
            }
        };
        pool.run(work); // join this batch before inspecting or publishing decoded bytes
        for(std::size_t j=0;j<n;++j) {
            if(decoded.size()>=extent)throw FormatError("Invalid MCC chunk location");
            if(chunks[j].error)std::rethrow_exception(chunks[j].error);
            const auto part=chunks[j].compressed?std::span<const std::byte>(chunks[j].bytes):chunks[j].raw;
            const auto limit=std::min<std::size_t>(chunk_size,extent-decoded.size());
            if(part.size()>limit)throw FormatError(chunks[j].compressed?
                "MCC zlib output exceeds declared extent":"Excessive raw MCC chunk");
            decoded.insert(decoded.end(),part.begin(),part.end());
        }
        base+=n;
    }
    if(decoded.size()!=extent)throw FormatError("MCC decoded size differs from header");
    return decoded;
}
}
