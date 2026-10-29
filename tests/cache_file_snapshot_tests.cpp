#include "halo2/cache_file_snapshot.hpp"
#include <algorithm>
#include <array>
#include <chrono>
#include <fstream>
#include <iostream>
#include <vector>
#include <cerrno>
#if defined(__linux__) || defined(__APPLE__)
#include <fcntl.h>
#include <unistd.h>
#elif defined(_WIN32)
#include <fcntl.h>
#include <io.h>
#endif

using namespace halo2;
void require(bool value, const char *message) { if (!value) throw std::runtime_error(message); }
#if defined(HALO2_SNAPSHOT_WRITEBACK_TEST)
namespace {
struct WritebackCall {off64_t offset,length;unsigned flags;};
struct WritebackProbe {
    bool active{};std::vector<WritebackCall> ranges;
    std::vector<int> range_errors,sync_errors;std::size_t range_step{},sync_step{};
} writeback;
int injected(std::vector<int>& errors,std::size_t& step) {
    return step<errors.size()?errors[step++]:0;
}
}
extern "C" int __real_sync_file_range(int,off64_t,off64_t,unsigned);
extern "C" int __real_fdatasync(int);
extern "C" int __wrap_sync_file_range(int fd,off64_t offset,off64_t length,unsigned flags) {
    if(writeback.active) {
        writeback.ranges.push_back({offset,length,flags});
        if(const auto error=injected(writeback.range_errors,writeback.range_step)){errno=error;return -1;}
    }
    return __real_sync_file_range(fd,offset,length,flags);
}
extern "C" int __wrap_fdatasync(int fd) {
    if(writeback.active) {
        if(const auto error=injected(writeback.sync_errors,writeback.sync_step)){errno=error;return -1;}
        ++writeback.sync_step;
    }
    return __real_fdatasync(fd);
}
void writeback_contracts() {
    constexpr std::size_t batch=1024*1024;
    std::vector<std::byte> bytes(3*batch+137);
    std::fill_n(bytes.begin(),batch,std::byte{0x31});
    std::fill(bytes.begin()+2*batch,bytes.end(),std::byte{0x75});
    writeback={};writeback.active=true;
    auto snapshot=CacheFileSnapshot::try_capture(bytes);
    require(snapshot && std::ranges::equal(snapshot->bytes(),bytes),"Bounded writeback changed complete sparse capture bytes");
    require(writeback.ranges.size()==3,"Capture did not write back exactly its nonzero ranges");
    for(unsigned n=0;n<3;++n) {
        const auto& call=writeback.ranges[n];
        require(call.offset==off64_t(n?batch*(n+1):0) && call.length==off64_t(n==2?137:batch) &&
            call.flags==(SYNC_FILE_RANGE_WAIT_BEFORE|SYNC_FILE_RANGE_WRITE|SYNC_FILE_RANGE_WAIT_AFTER),
            "Capture writeback escaped the actual written range or dropped its completion/error wait");
    }
    require(writeback.sync_step==0,"Supported range writeback still issued durable whole-file sync");
    const std::array payload{std::byte{0x93},std::byte{0x45}};
    writeback={};writeback.active=true;writeback.range_errors={EINTR};
    auto interrupted=CacheFileSnapshot::try_capture(payload);
    require(interrupted && writeback.ranges.size()==2 && writeback.sync_step==0 &&
        std::ranges::equal(interrupted->bytes(),payload),"Interrupted range writeback did not retry its actual capture");
    for(const auto unsupported:{ENOSYS,EINVAL,EOPNOTSUPP}) {
        writeback={};writeback.active=true;writeback.range_errors={unsupported};writeback.sync_errors={EINTR};
        auto fallback=CacheFileSnapshot::try_capture(payload);
        require(fallback && writeback.ranges.size()==1 && writeback.sync_step==2 &&
            std::ranges::equal(fallback->bytes(),payload),"Unsupported range writeback did not complete its interrupted durable fallback");
    }
    for(const auto failure:{EIO,ENOSPC}) {
        writeback={};writeback.active=true;writeback.range_errors={failure};
        const auto failed=CacheFileSnapshot::try_capture(payload);
        require(!failed && errno==failure && writeback.sync_step==1,
            "Fallback swallowed a range IO/storage error after clearing its kernel report");
        require(std::ranges::equal(snapshot->bytes(),bytes),"Failed writeback changed an earlier admitted snapshot");
    }
    writeback={};writeback.active=true;writeback.range_errors={EOPNOTSUPP};writeback.sync_errors={EIO};
    require(!CacheFileSnapshot::try_capture(payload) && errno==EIO,"Failed durable fallback admitted an unavailable capture");
    writeback={};
    std::cout<<"PASS exact sparse written ranges, completion waits, EINTR retry, unsupported fallback and retained IO/storage errors\n";
}
#endif
int main() try {
#if !defined(__linux__) && !defined(__APPLE__) && !defined(_WIN32)
    return 77;
#endif
#if defined(HALO2_SNAPSHOT_WRITEBACK_TEST)
    writeback_contracts();
#endif
    const auto path = std::filesystem::temp_directory_path() /
        ("halo2-file-snapshot-" + std::to_string(std::chrono::steady_clock::now().time_since_epoch().count()));
    struct Cleanup { std::filesystem::path path; ~Cleanup() { std::error_code error; std::filesystem::remove(path,error); } } cleanup{path};
    std::vector<std::byte> expected(128 * 1024);
    for (std::size_t i = 0; i < expected.size(); ++i) expected[i] = std::byte((i * 13 + i / 4096) & 255);
    { std::ofstream file(path, std::ios::binary); file.write(reinterpret_cast<const char *>(expected.data()), expected.size()); }
    auto first = CacheFileSnapshot::try_open(path), second = CacheFileSnapshot::try_open(path);
    require(first && second, "Snapshot backing unavailable");
    const auto before=CacheFileSnapshot::storage_stats();
    auto private_view=first->try_private_copy(expected.size());
    auto prefix=first->try_private_copy(65537);
    const auto after=CacheFileSnapshot::storage_stats();
    require(private_view && prefix && private_view->bytes().data()!=first->bytes().data() &&
        prefix->bytes().data()!=first->bytes().data() && prefix->bytes().size()==65537 &&
        std::ranges::equal(private_view->bytes(),expected) &&
        std::equal(prefix->bytes().begin(),prefix->bytes().end(),expected.begin()),
        "Immutable file copies did not own separate complete/prefix private mappings");
    require(after.capture_input_bytes==before.capture_input_bytes && after.capture_written_bytes==before.capture_written_bytes &&
        after.independent_mapping_bytes-before.independent_mapping_bytes==expected.size()+65537,
        "Private file copies recaptured/wrote unchanged bytes");
    require(!first->try_private_copy(0) && !first->try_private_copy(expected.size()+1),"Invalid private prefix was admitted");
    auto captured = CacheFileSnapshot::try_capture(expected, std::array{std::byte{0xa9}});
    require(captured && captured->bytes().size() == expected.size() + 1 &&
        std::equal(expected.begin(), expected.end(), captured->bytes().begin()) &&
        captured->bytes().back() == std::byte{0xa9}, "Captured bytes/suffix differ");
#if defined(__linux__) || defined(__APPLE__)
    const auto source_fd = ::open(path.c_str(), O_RDONLY | O_CLOEXEC);
    require(source_fd >= 0 && ::lseek(source_fd, 37, SEEK_SET) == 37, "Capture source descriptor setup failed");
    auto from_descriptor = CacheFileSnapshot::try_capture_file(source_fd);
    require(from_descriptor && ::lseek(source_fd, 0, SEEK_CUR) == 37 &&
        std::equal(expected.begin(), expected.end(), from_descriptor->bytes().begin()),
        "Descriptor capture changed position or complete bytes");
    ::close(source_fd);
#elif defined(_WIN32)
    const auto source_fd=::_wopen(path.c_str(),_O_RDONLY|_O_BINARY|_O_NOINHERIT);
    require(source_fd>=0 && ::_lseeki64(source_fd,37,SEEK_SET)==37,"Capture source descriptor setup failed");
    auto from_descriptor=CacheFileSnapshot::try_capture_file(source_fd);
    require(from_descriptor && ::_lseeki64(source_fd,0,SEEK_CUR)==37 &&
        std::equal(expected.begin(),expected.end(),from_descriptor->bytes().begin()),
        "Descriptor capture changed position or complete bytes");
    ::_close(source_fd);
#endif
    require(std::equal(first->bytes().begin(), first->bytes().end(), expected.begin(), expected.end()), "Snapshot bytes differ");
    const auto escaped=first->mutable_bytes();
    require(!first->try_private_copy(expected.size()),"Mutable exposure without a write did not permanently retire private copying");
    escaped[17] ^= std::byte{255};
    require(second->bytes()[17] == expected[17], "Private writes reached another snapshot");
    require(private_view->bytes()[17]==expected[17] && prefix->bytes()[17]==expected[17],
        "An escaped pointer changed earlier independent mappings");
    private_view->mutable_bytes()[18]^=std::byte{255};
    require(first->bytes()[18]==expected[18] && prefix->bytes()[18]==expected[18] &&
        !private_view->try_private_copy(expected.size()),"Copied view writes reached another view or retained a false pristine receipt");
    { std::ifstream file(path, std::ios::binary); file.seekg(17); require(std::byte(file.get()) == expected[17], "Private writes reached source"); }
    // An unread page must remain the original snapshot after source mutation.
    { std::ofstream file(path, std::ios::binary | std::ios::trunc); file.put('X'); }
    require(second->bytes().size() == expected.size() && second->bytes().back() == expected.back(),
            "Source truncation changed snapshot backing");
    std::filesystem::remove(path);
    require(second->bytes()[65537] == expected[65537], "Source unlink invalidated snapshot");
#if defined(__linux__) || defined(__APPLE__) || defined(_WIN32)
    require(from_descriptor->bytes()[65537] == expected[65537], "Descriptor capture lost independence after source unlink");
#endif
    require(captured->bytes()[65537] == expected[65537], "Captured bytes lost independence after source unlink");
    first.reset();
    require(prefix->bytes()[65536]==expected[65536] && private_view->bytes().back()==expected.back(),
        "Independent mappings lost their private file after the source mapping retired");
    auto retained = second; second.reset();
    require(retained->bytes()[65538] == expected[65538], "Retained snapshot lost its lifetime");
    require(!CacheFileSnapshot::try_open(path), "Missing source was accepted");
    std::cout << "PASS private cache file snapshot: bytes, private writes, source truncation/unlink, retained lifetime, missing input\n";
    return 0;
} catch (const std::exception &error) { std::cerr << error.what() << '\n'; return 1; }
