#include "halo2/source_authority_binding.hpp"
#include <openssl/sha.h>
#include <stdexcept>
#include <string_view>

namespace halo2 {
SourceMatchAuthority::AuthorityDigest source_authority_binding(const AuthorityTransferContext &c){
    if(c.host>=16||!(c.peers&(1u<<c.host)))throw std::invalid_argument("Invalid source authority context");
    std::vector<std::byte> input;for(char value:std::string_view("H2 SOURCE AUTHORITY 1"))input.push_back(std::byte(value));
    input.insert(input.end(),c.session.begin(),c.session.end());
    const auto put=[&](std::uint64_t value,unsigned n){for(unsigned i=0;i<n;++i)input.push_back(std::byte(value>>(8*i)));};
    put(c.epoch,4);put(c.membership_revision,8);put(c.peers,2);put(c.host,1);SourceMatchAuthority::AuthorityDigest result{};
    if(!SHA256(reinterpret_cast<const unsigned char *>(input.data()),input.size(),reinterpret_cast<unsigned char *>(result.data())))
        throw std::runtime_error("Cannot bind native source authority");
    return result;
}
}
