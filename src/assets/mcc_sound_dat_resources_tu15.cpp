#include "halo2/mcc_sound_dat_resources_tu15.hpp"
#include "halo2/cache.hpp"
#include "halo2/cache_file_snapshot.hpp"
#include "halo2/confined_file.hpp"
#include "halo2/mcc_original_sound_runtime_delivery_tu15.hpp"
#include "halo2/source_runtime_mcc_preparation.hpp"
#include <algorithm>
#include <map>
#include <openssl/evp.h>
#include <set>
#include <tuple>
#if defined(_WIN32)
#include <fcntl.h>
#include <io.h>
#else
#include <sys/mman.h>
#endif

namespace halo2 {namespace {
namespace fs=std::filesystem;
void need(bool value,const char* why){if(!value)throw FormatError(why);}
class Hash {
    std::unique_ptr<EVP_MD_CTX,decltype(&EVP_MD_CTX_free)> context{EVP_MD_CTX_new(),EVP_MD_CTX_free};
public:
    Hash(){need(context&&EVP_DigestInit_ex(context.get(),EVP_sha256(),nullptr)==1,"MCC sound DAT authentication failed");}
    void bytes(std::span<const std::byte> value){need(value.empty()||EVP_DigestUpdate(context.get(),value.data(),value.size())==1,"MCC sound DAT authentication failed");}
    void word(std::uint32_t value){std::array<std::byte,4> data;for(unsigned n=0;n<4;++n)data[n]=std::byte(value>>(8*n));bytes(data);}
    void wide(std::uint64_t value){word(std::uint32_t(value));word(std::uint32_t(value>>32));}
    void text(std::string_view value){wide(value.size());bytes(std::as_bytes(std::span(value.data(),value.size())));}
    MccSoundDatResourcesTu15::Digest finish(){MccSoundDatResourcesTu15::Digest result;unsigned count{};
        need(EVP_DigestFinal_ex(context.get(),reinterpret_cast<unsigned char*>(result.data()),&count)==1&&count==32,"MCC sound DAT authentication failed");return result;}
};
auto seal(const MccSoundRuntimePlan& plan){Hash hash;hash.word(plan.address);hash.wide(plan.bytes.size());hash.bytes(plan.bytes);
    for(const auto& block:plan.blocks){hash.word(std::uint32_t(block.count));hash.word(block.address);}
    hash.wide(plan.chunks.size());for(const auto& c:plan.chunks){hash.word(c.gestalt.value);hash.word(c.source_address);hash.word(c.native_address);hash.word(c.source_index);
        hash.word(c.file_offset);hash.word(c.encoded_bytes);hash.word(c.decoded_bytes);hash.word(c.reset_after);}
    hash.wide(plan.sounds.size());for(const auto& s:plan.sounds){hash.word(s.sound.value);hash.word(s.gestalt.value);hash.word(unsigned(s.file));hash.word(s.source_root);hash.word(s.native_root);
        hash.word(s.source_flags);hash.word(s.native_flags);hash.word(s.codec.sample_rate);hash.word(s.codec.encoding);hash.word(s.codec.compression);}
    hash.wide(plan.patches.size());for(const auto& p:plan.patches){hash.word(p.owner.value);hash.word(p.source_address);hash.wide(p.before.size());hash.bytes(p.before);hash.wide(p.after.size());hash.bytes(p.after);}
    return hash.finish();
}
std::shared_ptr<CacheFileSnapshot> capture(const fs::path& path) {
#if defined(_WIN32)
    std::vector<HANDLE> handles;
    const auto close=[&]{for(auto handle:handles)CloseHandle(handle);handles.clear();};
    try {
        const auto full=fs::absolute(path);auto current=full.root_path();
        for(const auto& part:full.relative_path()) {
            need(part!=L"..","MCC sound DAT path traverses its owner");if(part==L".")continue;current/=part;
            const auto handle=CreateFileW(current.c_str(),GENERIC_READ,FILE_SHARE_READ,nullptr,OPEN_EXISTING,
                FILE_FLAG_OPEN_REPARSE_POINT|FILE_FLAG_BACKUP_SEMANTICS,nullptr);
            need(handle!=INVALID_HANDLE_VALUE,"Cannot open MCC sound DAT owner");handles.push_back(handle);BY_HANDLE_FILE_INFORMATION info{};
            need(GetFileInformationByHandle(handle,&info)&&(!(info.dwFileAttributes&FILE_ATTRIBUTE_REPARSE_POINT)||confined_file_detail::wine_host_mount(handle)),"Linked MCC sound DAT owner");
        }
        need(!handles.empty()&&GetFileType(handles.back())==FILE_TYPE_DISK,"MCC sound DAT owner is not a regular disk file");
        HANDLE duplicate{};need(DuplicateHandle(GetCurrentProcess(),handles.back(),GetCurrentProcess(),&duplicate,0,FALSE,DUPLICATE_SAME_ACCESS),"Cannot retain MCC sound DAT descriptor");
        const int fd=::_open_osfhandle(reinterpret_cast<std::intptr_t>(duplicate),_O_RDONLY|_O_BINARY);
        if(fd<0){CloseHandle(duplicate);throw FormatError("Cannot capture MCC sound DAT descriptor");}
        struct Descriptor {int value;~Descriptor(){::_close(value);}} descriptor{fd};
        const auto result=CacheFileSnapshot::try_capture_file(fd);close();
        need(bool(result),"MCC sound DAT requires a private disk-backed snapshot");return result;
    }catch(...){close();throw;}
#else
    const int fd=open_confined_file(path);
    try {
        struct stat before{},after{};need(::fstat(fd,&before)==0&&before.st_size>=0,"Cannot measure MCC sound DAT owner");
        const auto result=CacheFileSnapshot::try_capture_file(fd);
        need(::fstat(fd,&after)==0&&before.st_dev==after.st_dev&&before.st_ino==after.st_ino&&before.st_size==after.st_size,
            "MCC sound DAT owner changed during capture");
#if defined(__APPLE__)
        need(before.st_mtimespec.tv_sec==after.st_mtimespec.tv_sec&&before.st_mtimespec.tv_nsec==after.st_mtimespec.tv_nsec&&
            before.st_ctimespec.tv_sec==after.st_ctimespec.tv_sec&&before.st_ctimespec.tv_nsec==after.st_ctimespec.tv_nsec,"MCC sound DAT bytes changed during capture");
#else
        need(before.st_mtim.tv_sec==after.st_mtim.tv_sec&&before.st_mtim.tv_nsec==after.st_mtim.tv_nsec&&
            before.st_ctim.tv_sec==after.st_ctim.tv_sec&&before.st_ctim.tv_nsec==after.st_ctim.tv_nsec,"MCC sound DAT bytes changed during capture");
#endif
        need(result&&result->bytes().size()==std::uint64_t(before.st_size),"MCC sound DAT requires a complete private disk-backed snapshot");
        ::close(fd);return result;
    }catch(...){::close(fd);throw;}
#endif
}
auto file_digest(std::span<const std::byte> bytes){Hash hash;
    while(!bytes.empty()) {const auto window=bytes.first(std::min<std::size_t>(bytes.size(),4u*1024u*1024u));hash.bytes(window);
#if defined(__linux__) && defined(MADV_COLD)
        const auto saved=errno;const auto page=::sysconf(_SC_PAGESIZE);
        if(page>0){const auto address=reinterpret_cast<std::uintptr_t>(window.data());const auto skip=(std::size_t(page)-address%std::size_t(page))%std::size_t(page);
            const auto length=skip<window.size()?(window.size()-skip)/std::size_t(page)*std::size_t(page):0u;
            if(length)(void)::madvise(const_cast<std::byte*>(window.data()+skip),length,MADV_COLD);}
        errno=saved;
#endif
        bytes=bytes.subspan(window.size());
    }return hash.finish();
}
}
struct MccSoundDatResourcesTu15::Impl {
    struct File {std::shared_ptr<const CacheFileSnapshot> snapshot;Digest digest;};
    struct Route {std::uint32_t offset{},flags{},bytes{};std::size_t file{};};
    std::vector<File> files;
    std::map<std::uint32_t,Route> routes;
    struct EncodedRoute {std::uint32_t descriptor{};MccResourceBank bank{};};
    std::map<std::tuple<std::uint32_t,std::uint32_t,std::uint32_t>,EncodedRoute> encoded;
    const Cache* cache{}; // Borrowed equality only; no memory/voice eligibility.
    Digest plan_seal{},parent_seal{},resources{};
    std::shared_ptr<const MccOriginalSoundRuntimeDeliveryTu15> original;
};
MccSoundDatResourcesTu15::MccSoundDatResourcesTu15(std::shared_ptr<const Impl> impl):impl_(std::move(impl)){}
std::shared_ptr<const MccSoundDatResourcesTu15> MccSoundDatResourcesTu15::prepare(const Cache& cache,const SourceRuntimeMccPreparation& metadata) {
    need(cache.is_mcc(),"MCC sound DAT preparation requires an actual MCC Cache");const auto& actual=metadata.sound;
    need(metadata.delivery.original_sound_owner()==actual.original,"MCC sound DAT metadata/resource owners differ");
    // Reuse complete Cache-aware source/flags/class/timing admission, rather
    // than trusting caller-supplied file enums or a few matching descriptors.
    (void)build_mcc_sound_timing_reference(cache,actual,[&](auto at,auto bytes){return metadata.delivery.runtime_address(at,bytes);});
    if(actual.original)actual.original->require_plan(actual);
    const auto& parent=actual.original?actual.original->parent():actual;
    auto result=std::make_shared<Impl>();result->cache=&cache;result->original=actual.original;result->plan_seal=seal(actual);result->parent_seal=seal(parent);
    need(parent.blocks[8].count>=0&&parent.chunks.size()==std::size_t(parent.blocks[8].count)&&actual.chunks.size()==parent.chunks.size(),"MCC sound DAT prefix descriptor closure is incomplete");
    const auto row=[&](unsigned table,unsigned index,unsigned stride){const auto block=parent.blocks[table];need(block.count>=0&&index<std::uint32_t(block.count),"MCC sound DAT selector escapes its native table");
        const auto at=std::uint64_t(block.address)+std::uint64_t(index)*stride;need(at>=parent.address&&at-parent.address+stride<=parent.bytes.size(),"MCC sound DAT selector escapes its native bank");
        return Bytes(std::span(parent.bytes).subspan(at-parent.address,stride));};
    std::map<std::uint32_t,fs::path> selected;
    struct User {TagHandle sound,gestalt;MccResourceBank bank;};
    std::map<std::uint32_t,std::vector<User>> users;
    for(const auto& sound:parent.sounds) {
        MccResourceBank bank;
        switch(sound.file){case MccSoundDataFile::Neutral:bank=MccResourceBank::NeutralSounds;break;
            case MccSoundDataFile::English:bank=MccResourceBank::EnglishSounds;break;
            case MccSoundDataFile::Remastered:bank=MccResourceBank::RemasteredSounds;break;
            default:throw FormatError("Original sound appeared in retained MCC DAT prefix");}
        const auto path=cache.mcc_resource_bank_path(sound.sound,bank);
        const auto patch=std::ranges::find(parent.patches,sound.sound,&MccSoundRuntimePatch::owner);
        need(patch!=parent.patches.end()&&patch->after.size()==20,"MCC sound DAT has no exact native sound root");const Bytes root(patch->after);
        for(unsigned pitch=0;pitch<std::to_integer<unsigned>(root.slice(10,1)[0]);++pitch) {
            const auto range=row(4,root.u16(8)+pitch,12);
            for(unsigned p=0;p<range.u16(10);++p){const auto permutation=row(5,range.u16(8)+p,16);
                for(unsigned c=0;c<permutation.u16(14);++c){const auto index=permutation.u16(12)+c;(void)row(8,index,12);
                    const auto [at,added]=selected.emplace(index,path);need(added||at->second==path,"MCC sound descriptor has ambiguous DAT owners");
                    users[index].push_back({sound.sound,sound.gestalt,bank});}
            }
        }
    }
    std::map<fs::path,std::size_t> files;std::set<std::uint32_t> ordinals;Hash resources;resources.text("Halo 2 retained MCC sound DAT resources v1");resources.wide(actual.chunks.size());
    for(unsigned n=0;n<actual.chunks.size();++n){const auto& chunk=actual.chunks[n];const auto& before=parent.chunks[n];
        need(before.native_address>=parent.blocks[8].address&&(before.native_address-parent.blocks[8].address)%12u==0,"MCC sound DAT parent descriptor is foreign");
        const auto index=(before.native_address-parent.blocks[8].address)/12u;const auto literal=row(8,index,12);
        need(ordinals.insert(index).second&&selected.contains(index)&&chunk.native_address==actual.blocks[8].address+index*12u&&
            chunk.gestalt==before.gestalt&&chunk.source_address==before.source_address&&chunk.source_index==before.source_index&&
            chunk.file_offset==before.file_offset&&chunk.encoded_bytes==before.encoded_bytes&&chunk.decoded_bytes==before.decoded_bytes&&chunk.reset_after==before.reset_after&&
            literal.u32(0)==before.file_offset&&(literal.u32(4)&0x3fffffffu)==before.encoded_bytes,"MCC sound DAT prefix route changed");
        const auto path=selected.at(index);auto file=files.find(path);
        if(file==files.end()){auto snapshot=capture(path);const auto digest=file_digest(snapshot->bytes());
            file=files.emplace(path,result->files.size()).first;result->files.push_back({std::move(snapshot),digest});}
        const auto& owned=result->files[file->second];const auto raw=literal.u32(4)&0x3fffffffu;const auto bytes=(std::uint64_t(raw)+511u)&~511ull;
        need(raw&&bytes<=128u*1024u*1024u&&std::uint64_t(chunk.file_offset)+bytes<=owned.snapshot->bytes().size(),"MCC sound DAT sector escapes its actual file owner");
        need(result->routes.emplace(chunk.native_address,Impl::Route{chunk.file_offset,literal.u32(4),std::uint32_t(bytes),file->second}).second,"MCC sound DAT repeats a physical descriptor");
        for(const auto& user:users.at(index)) {
            const auto [at,added]=result->encoded.emplace(std::tuple{user.sound.value,user.gestalt.value,before.source_address},Impl::EncodedRoute{chunk.native_address,user.bank});
            need(added||(at->second.descriptor==chunk.native_address&&at->second.bank==user.bank),"MCC encoded sound aliases a different retained route");
        }
        // Native relocation and host paths do not alter asset identity. The
        // actual read still requires the exact current canonical descriptor.
        resources.word(index);resources.word(chunk.gestalt.value);resources.word(chunk.source_address);resources.word(chunk.source_index);
        resources.word(chunk.file_offset);resources.word(literal.u32(4));resources.word(chunk.decoded_bytes);resources.word(chunk.reset_after);
        resources.wide(owned.snapshot->bytes().size());resources.bytes(owned.digest);
    }
    need(ordinals.size()==selected.size(),"MCC sound DAT has unowned prefix descriptors");result->resources=resources.finish();
    return std::shared_ptr<const MccSoundDatResourcesTu15>(new MccSoundDatResourcesTu15(std::move(result)));
}
void MccSoundDatResourcesTu15::require_routes(const MccSoundRuntimePlan& plan)const {
    need(plan.original==impl_->original&&seal(plan)==impl_->plan_seal&&seal(plan.original?plan.original->parent():plan)==impl_->parent_seal,"MCC sound DAT route publication changed");
    if(plan.original)plan.original->require_plan(plan);
}
std::vector<std::byte> MccSoundDatResourcesTu15::read(const SourceSoundCacheReadTu15& request)const {
    const auto at=impl_->routes.find(request.descriptor);need(at!=impl_->routes.end(),"MCC sound read has no admitted DAT descriptor");const auto& route=at->second;
    need(request.encoded_offset==route.offset&&request.size_flags==route.flags&&request.bytes==route.bytes,"MCC sound read differs from its exact DAT route");
    const auto bytes=impl_->files[route.file].snapshot->bytes().subspan(route.offset,route.bytes);return {bytes.begin(),bytes.end()};
}
std::vector<std::byte> MccSoundDatResourcesTu15::read_encoded(const Cache& expected,const MccSoundResourceRead& request)const {
    need(&expected==impl_->cache,"Encoded MCC sound belongs to another Cache");
    const auto selected=impl_->encoded.find({request.sound.value,request.gestalt.value,request.descriptor});
    need(selected!=impl_->encoded.end()&&selected->second.bank==request.bank,"Encoded MCC sound has no exact retained source route");
    const auto& route=impl_->routes.at(selected->second.descriptor);
    need(request.file_offset==route.offset&&request.bytes==(route.flags&0x3fffffffu),"Encoded MCC sound changed its immutable DAT extent");
    const auto bytes=impl_->files[route.file].snapshot->bytes().subspan(route.offset,request.bytes);return {bytes.begin(),bytes.end()};
}
MccSoundDatResourcesTu15::Digest MccSoundDatResourcesTu15::bind_resource_identity(const Digest& resident)const {
    if(impl_->routes.empty())return resident;
    Hash hash;hash.text("Halo 2 MCC sound DAT resident binding v1");hash.bytes(resident);hash.bytes(impl_->resources);return hash.finish();
}
std::size_t MccSoundDatResourcesTu15::routes()const{return impl_->routes.size();}
}
