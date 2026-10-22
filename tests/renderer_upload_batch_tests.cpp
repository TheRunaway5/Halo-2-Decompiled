#include "../src/rendering/renderer/renderer_upload_batch.hpp"
#include <algorithm>
#include <iostream>
#include <limits>
#include <random>
#include <vector>
using halo2::detail::TextureUploadBatch;
int main() {
    const auto require=[](bool ok){if(!ok)throw std::runtime_error("Texture upload batch changed bytes, alignment, retirement or capacity");};
    TextureUploadBatch batch;
    std::vector<std::byte> buffer(TextureUploadBatch::capacity);
    struct Copy {std::size_t offset;std::vector<std::byte> source;};std::vector<Copy> copies;
    const auto retire=[&] {
        for(const auto &copy:copies)require(std::equal(copy.source.begin(),copy.source.end(),buffer.begin()+copy.offset));
        copies.clear();batch.clear();
    };
    std::mt19937 random(0x20261020);unsigned batches{};
    for(unsigned i=0;i<1024;++i) {
        std::vector<std::byte> source(i%13?1+random()%100000:TextureUploadBatch::capacity-128);
        for(auto &byte:source)byte=std::byte(random());
        if(!batch.fits(source.size())){retire();++batches;}
        const auto offset=batch.append(source.size());
        require(offset%TextureUploadBatch::alignment==0 && source.size()<=buffer.size()-offset);
        for(const auto &copy:copies)require(copy.offset+copy.source.size()<=offset);
        std::copy(source.begin(),source.end(),buffer.begin()+offset);copies.push_back({offset,std::move(source)});
    }
    retire();require(batches>1);
    for(unsigned i=0;i<TextureUploadBatch::max_images;++i)batch.append(1);
    require(!batch.fits(1));batch.clear();
    batch.append(TextureUploadBatch::capacity);require(!batch.fits(1));batch.clear();
    batch.append(std::numeric_limits<std::size_t>::max());require(!batch.fits(1));batch.clear();
    require(batch.append(7)==0 && batch.append(7)==TextureUploadBatch::alignment);
    std::cout<<"1024 upload payloads remain exact across bounded aligned batches, count limits and oversized retirement\n";
}
