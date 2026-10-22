#include "halo2/native_tu15_profile_file.hpp"
namespace halo2 {
namespace {
std::uint32_t word(const SourceRuntimeMemory& m,std::uint32_t p) {
    auto b=m.view(p,4);std::uint32_t v=0;for(unsigned i=0;i<4;++i)v|=std::to_integer<std::uint32_t>(b[i])<<(8*i);return v;
}
std::string path_string(const SourceRuntimeMemory& m,std::uint32_t pointer) {
    std::string path;
    for(unsigned i=0;i<256;++i) {
        const auto c=std::to_integer<unsigned char>(m.view(pointer+i,1)[0]);
        if(!c)return path;path.push_back(char(c));
    }
    throw std::invalid_argument("Native private-cache path has no bounded terminator");
}
}
NativeTu15ProfileFile::NativeTu15ProfileFile(SourceRuntimeMemory& m,std::filesystem::path directory)
    :memory_(m),directory_(std::move(directory)),thread_(std::this_thread::get_id()) {
    if(directory_.empty() || !std::filesystem::is_directory(directory_))throw std::invalid_argument("Native profile probe requires its retained private directory");
}
bool NativeTu15ProfileFile::try_call(SourcePhysicsContactMemory& bank,
    const SourcePhysicsContactRuntimeCall& call,SourcePhysicsWorldRuntimeAbi& abi) {
    if(call.target!=0x2e0b1eu && call.target!=0x2dbc2eu && call.target!=0x33bb15u && call.target!=0x2dbc56u)return false;
    const auto sp=abi.gp[4];
    if(&bank!=&memory_ || std::this_thread::get_id()!=thread_ || call.tail || word(memory_,sp)!=call.instruction+5)
        throw std::invalid_argument("Native profile API bank, thread or return frame differs");
    if(call.target==0x2dbc2e) {
        if((call.instruction!=0x13941cu && call.instruction!=0x139320u) || !last_error_)
            throw std::invalid_argument("Native private-cache error query has no actual native result");
        abi.gp[0]=*last_error_;abi.gp[4]+=4;return true;
    }
    if(call.target==0x2dbc56) {
        if(call.instruction!=0x139327u || word(memory_,sp+4)!=0)
            throw std::invalid_argument("Native private-cache error reset requires its recovered caller");
        last_error_=0;abi.gp[4]+=8;return true;
    }
    if(call.target==0x33bb15) {
        if(call.instruction!=0x139317u || word(memory_,sp+8)!=0 || path_string(memory_,word(memory_,sp+4))!="n:\\sgf")
            throw std::invalid_argument("Native private-cache directory creation frame or path differs");
        const auto native=directory_/"sgf";std::error_code error;
        const bool created=std::filesystem::create_directory(native,error);
        if(error)throw std::system_error(error,"Native SGF-cache directory creation");
        const auto result=created?1u:0u,error_value=created?0u:183u;
        if(!created)last_error_=error_value;
        directory_creation_=NativeTu15DirectoryCreation{call.instruction,sp,sp+12,result,error_value,"n:\\sgf",native};
        ++directory_creations_;
        abi.gp[0]=result;abi.gp[4]+=12;return true;
    }
    if(call.instruction!=0x139407u)throw std::invalid_argument("Native profile attribute probe has an unowned caller");
    const auto path=path_string(memory_,word(memory_,sp+4));
    if(path=="n:\\sgf") {
        const auto native=directory_/"sgf";std::error_code error;
        const auto status=std::filesystem::status(native,error);
        if(error && error!=std::errc::no_such_file_or_directory)throw std::system_error(error,"Native SGF-cache directory probe");
        if(std::filesystem::exists(status) && !std::filesystem::is_directory(status))
            throw std::runtime_error("Native SGF-cache path is occupied by a non-directory");
        const auto attributes=std::filesystem::is_directory(status)?0x10u:0xffffffffu;
        const auto error_value=attributes==0xffffffffu?2u:0u;
        directory_probe_=NativeTu15ProfileFileProbe{call.instruction,sp,sp+8,attributes,error_value,path,native};
        if(error_value)last_error_=error_value;
        abi.gp[0]=attributes;abi.gp[4]+=8;return true;
    }
    if(path!="z:\\player_configuration_cache.dat")throw std::invalid_argument("Native profile probe path differs from its actual private cache caller");
    const auto native=directory_/"player_configuration_cache.dat";std::error_code error;
    const auto status=std::filesystem::status(native,error);
    if(error && error!=std::errc::no_such_file_or_directory)throw std::system_error(error,"Native player configuration probe");
    if(std::filesystem::exists(status))throw std::runtime_error("Existing player configuration requires its complete native file/profile owner");
    probe_=NativeTu15ProfileFileProbe{call.instruction,sp,sp+8,0xffffffffu,2,path,native};
    last_error_=2;
    abi.gp[0]=probe_->attributes;abi.gp[4]+=8;return true;
}
}
