#include "source_script_wake_fixture.hpp"
using namespace halo2::script_wake_fixture;
#include "halo2/source_authority_codec.hpp"
#include <algorithm>
#include <openssl/sha.h>
#include <zlib.h>
namespace {
std::vector<std::byte> previous_thread_extent(const std::vector<std::byte> &frame) {
    const auto get=[&](unsigned at){std::uint64_t v{};for(unsigned i=0;i<8;++i)v|=std::to_integer<std::uint64_t>(frame[at+i])<<(8*i);return v;};
    std::vector<std::byte> raw(std::size_t(get(16)));uLongf bytes=raw.size();
    if(uncompress(reinterpret_cast<Bytef *>(raw.data()),&bytes,reinterpret_cast<const Bytef *>(frame.data()+64),frame.size()-64)!=Z_OK)
        throw std::runtime_error("cannot inspect production codec fixture");
    const std::array<std::byte,13> marker{std::byte{13},std::byte{9},std::byte{},std::byte{},std::byte{},
        std::byte{6},std::byte{},std::byte{'s'},std::byte{'c'},std::byte{'r'},std::byte{'i'},std::byte{'p'},std::byte{'t'}};
    const auto at=std::search(raw.begin(),raw.end(),marker.begin(),marker.end());
    if(at==raw.end())throw std::runtime_error("thread record marker absent");
    at[1]=std::byte{8}; //Previous aggregate extent, with valid envelope checksum.
    std::vector<std::byte> compressed(compressBound(raw.size()));uLongf count=compressed.size();
    if(compress2(reinterpret_cast<Bytef *>(compressed.data()),&count,reinterpret_cast<const Bytef *>(raw.data()),raw.size(),Z_BEST_SPEED)!=Z_OK)
        throw std::runtime_error("cannot pack old-extent control");
    std::vector<std::byte> result(frame.begin(),frame.begin()+64);
    for(unsigned i=0;i<8;++i)result[24+i]=std::byte(std::uint64_t(count)>>(8*i));
    SHA256(reinterpret_cast<const unsigned char *>(raw.data()),raw.size(),reinterpret_cast<unsigned char *>(result.data()+32));
    result.insert(result.end(),compressed.begin(),compressed.begin()+count);return result;
}
}
int main(int argc,char **argv) {
    try {
        if(argc!=2)throw std::runtime_error("expected fixture directory");
        const std::filesystem::path root=argv[1];
        for(const std::string name:{"external_delayed_restore_wait","external_delayed_restore_dormant","external_expire_wait","external_expire_dormant"}) {
            auto p=load(root/(name+".hsw"));unsigned tick{};std::ostringstream a,b;
            const auto host=[&](auto &out,const auto &builtin,const auto &args)->ScriptValue {
                if(builtin!="print" || args.size()!=1)throw std::runtime_error("undeclared codec fixture boundary");
                out<<tick<<' '<<args[0].text<<'\n';return {};
            };
            ScriptVm source(p,[&](const auto &n,const auto &v){return host(a,n,v);});
            ScriptVm target(p,[&](const auto &n,const auto &v){return host(b,n,v);});
            source.start_level();for(tick=0;tick<=1;++tick)source.tick(tick);
            // This is declared inert owning envelope data solely to exercise
            // the production nested archive. No live source world or complete
            // SourceMatchAuthority import/replay is asserted by this fixture.
            SourceMatchAuthority::Checkpoint envelope;
            envelope.match.source_backed=true;envelope.match.simulation.source_backed=true;
            envelope.match.simulation.scripts=source.authority_checkpoint();
            const auto encoded=encode_source_authority(envelope);
            bool old_rejected=false;
            try{decode_source_authority(previous_thread_extent(encoded));}catch(const FormatError &){old_rejected=true;}
            if(!old_rejected)throw std::runtime_error("previous thread extent was accepted with defaulted saved wake");
            const auto decoded=decode_source_authority(encoded);
            if(decoded.match.simulation.scripts!=envelope.match.simulation.scripts)
                throw std::runtime_error(name+": codec dropped saved thread state");
            auto prepared=target.prepare_authority(decoded.match.simulation.scripts);
            if(!target.commit_authority(std::move(prepared)))throw std::runtime_error("decoded script authority import failed");
            a.str("");a.clear();
            for(tick=2;tick<105;++tick){source.tick(tick);target.tick(tick);}
            if(a.str()!=b.str() || source.authority_checkpoint()!=target.authority_checkpoint())
                throw std::runtime_error(name+": future dispatch differs after codec/import");
            std::cout<<"PASS production named aggregate codec and script authority import: "<<name<<'\n';
        }
    }catch(const std::exception &e){std::cerr<<e.what()<<'\n';return 1;}
}
