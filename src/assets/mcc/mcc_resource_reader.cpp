#include "halo2/mcc_resource_reader.hpp"
#include "halo2/mcc_cache.hpp"
#include "halo2/cache.hpp"
#include "halo2/confined_file.hpp"
#include <memory>
#if defined(__linux__)
#include <cerrno>
#include <fcntl.h>
#include <sys/stat.h>
#include <unistd.h>
#endif

namespace halo2 {
namespace {
constexpr std::uint32_t maximum_resource=128u*1024*1024;
std::vector<std::byte> owned_range(std::span<const std::byte> source,
    std::uint64_t offset,std::uint32_t bytes) {
    if(offset>source.size() || bytes>source.size()-offset)
        throw FormatError("MCC resource range escapes decoded container");
    const auto part=source.subspan(std::size_t(offset),bytes);
    return {part.begin(),part.end()};
}
#if defined(__linux__)
constexpr std::uint64_t maximum_container=1024ull*1024*1024;
struct File {
    int fd;
    explicit File(const std::filesystem::path& path)
        // identity() admits the regular-file type and extent on this same FD.
        :fd(open_confined_file(path,false)) {
        if(fd<0)throw FormatError("Cannot open MCC resource container: "+path.string());
    }
    ~File(){::close(fd);}
    File(const File&)=delete;
    File& operator=(const File&)=delete;
};
struct Identity {
    dev_t device;ino_t inode;off_t size;
    std::int64_t modified_seconds,modified_ns,changed_seconds,changed_ns;
    bool operator==(const Identity&)const=default;
};
Identity identity(const File& file) {
    struct stat s{};
    if(::fstat(file.fd,&s)!=0 || !S_ISREG(s.st_mode) || s.st_size<896 ||
       std::uint64_t(s.st_size)>maximum_container)
        throw FormatError("MCC resource container has an invalid file identity/extent");
    return {s.st_dev,s.st_ino,s.st_size,s.st_mtim.tv_sec,s.st_mtim.tv_nsec,
        s.st_ctim.tv_sec,s.st_ctim.tv_nsec};
}
struct DecodedOwner {Identity file;std::vector<std::byte> bytes;};
thread_local std::unique_ptr<const DecodedOwner> last_container;
void unchanged(const File& file,const Identity& before) {
    try {if(identity(file)==before)return;}
    catch(...) {last_container.reset();throw;}
    last_container.reset();
    throw FormatError("MCC resource container changed during its admitted read");
}
std::vector<std::byte> read_opened(const File& file,const Identity& source) {
    std::vector<std::byte> bytes(static_cast<std::size_t>(source.size));
    std::size_t at=0;
    while(at<bytes.size()) {
        const auto count=::pread(file.fd,bytes.data()+at,bytes.size()-at,off_t(at));
        if(count<0 && errno==EINTR)continue;
        if(count<=0)throw FormatError("Short MCC resource container read");
        at+=std::size_t(count);
    }
    return bytes;
}
#endif
}
std::vector<std::byte> read_mcc_resource_range(const std::filesystem::path& path,
    std::uint64_t offset,std::uint32_t bytes) {
    if(!bytes || bytes>maximum_resource)
        throw FormatError("Invalid MCC resource size");
#if defined(__linux__)
    const File opened(path);
    const auto admitted=identity(opened);
    if(last_container && last_container->file==admitted) {
        auto result=owned_range(last_container->bytes,offset,bytes);
        unchanged(opened,admitted);
        return result;
    }
    // No Cache-address or path-name key: only the actual opened file identity
    // can own this immutable snapshot. Drop the prior container before decode
    // so a thread retains at most one previous decoded allocation.
    last_container.reset();
    const auto encoded=read_opened(opened,admitted);
    auto decoded=decode_mcc_cache(encoded);
    unchanged(opened,admitted);
    auto result=owned_range(decoded,offset,bytes);
    unchanged(opened,admitted);
    last_container=std::make_unique<const DecodedOwner>(DecodedOwner{admitted,std::move(decoded)});
    return result;
#else
    return owned_range(decode_mcc_cache(read_confined_file(path)),offset,bytes);
#endif
}
}
