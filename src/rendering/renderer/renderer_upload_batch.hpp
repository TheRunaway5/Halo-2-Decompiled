#pragma once
#include <cstddef>
#include <stdexcept>
namespace halo2::detail {
class TextureUploadBatch {
    std::size_t bytes_{},count_{};
public:
    static constexpr std::size_t capacity=32u*1024u*1024u,alignment=256,max_images=128;
    bool empty()const noexcept{return !count_;}
    std::size_t offset()const noexcept{return bytes_>capacity?bytes_:(bytes_+alignment-1)&~(alignment-1);}
    bool fits(std::size_t bytes)const noexcept {
        if(empty())return true; // One oversized image uses its own allocation.
        if(count_>=max_images || bytes_>capacity)return false;
        const auto at=offset();return at<=capacity && bytes<=capacity-at;
    }
    std::size_t append(std::size_t bytes) {
        if(!fits(bytes))throw std::logic_error("Texture staging batch must retire before reuse");
        const auto at=offset();bytes_=at+bytes;++count_;return at;
    }
    void clear()noexcept{bytes_=count_=0;}
};
}
