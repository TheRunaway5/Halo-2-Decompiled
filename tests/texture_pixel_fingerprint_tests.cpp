#include "halo2/texture_pixels.hpp"
#include "platform/startup_texture_fingerprints.hpp"
#include <algorithm>
#include <atomic>
#include <chrono>
#include <iostream>
#include <stdexcept>
#include <string_view>
#include <thread>

using namespace halo2;
namespace {
void need(bool value,const char *message){if(!value)throw std::runtime_error(message);}
std::uint64_t actual(const TexturePixels &pixels) {
    const auto value=detail::texture_bytes_fnv1a({pixels.data(),pixels.size()});
    need(pixels.fingerprint()==value,"pixel fingerprint differs from complete current bytes");
    return value;
}
void check() {
    TexturePixels empty;need(actual(empty)==0xcbf29ce484222325ull,"empty FNV1a changed");
    const std::string_view hello="hello";
    TexturePixels text(std::as_bytes(std::span(hello.data(),hello.size())));
    need(actual(text)==0xa430d84680aabd0bull,"known FNV1a changed");
    auto snapshot=text;const auto original=actual(snapshot);
    auto *writer=text.data();writer[0]=std::byte{'j'};const auto changed=actual(text);
    need(changed!=original && actual(snapshot)==original,"writer changed cached shared snapshot");
    auto isolated=text;writer[0]=std::byte{'k'};
    need(actual(isolated)==changed && actual(text)!=changed,"retained raw writer changed an exposed-buffer copy");
    auto moved=std::move(text);const auto before_move_write=actual(moved);writer[1]=std::byte{'x'};
    need(actual(moved)!=before_move_write,"move lost exposed writable alias tracking");
    auto assigned=snapshot;assigned=std::move(moved);writer[2]=std::byte{'y'};actual(assigned);
    auto mutable_iterator=assigned.begin();actual(assigned);*mutable_iterator=std::byte{0};actual(assigned);
    assigned.resize(4096,std::byte{1});actual(assigned);assigned.clear();actual(assigned);
    snapshot.swap(isolated);need(actual(isolated)==original && actual(snapshot)==changed,"swap moved fingerprint separately from its bytes");
    std::vector<std::byte> bytes(1024*1024);
    for(unsigned i=0;i<bytes.size();++i)bytes[i]=std::byte((i*73u+i/257u)&255u);
    TexturePixels large{std::span<const std::byte>(bytes)};
    std::atomic<bool> equal{true};std::vector<std::thread> readers;
    const auto expected=detail::texture_bytes_fnv1a(bytes);
    for(unsigned i=0;i<8;++i)readers.emplace_back([&]{for(unsigned n=0;n<100;++n)if(large.fingerprint()!=expected)equal=false;});
    for(auto &reader:readers)reader.join();need(equal,"concurrent immutable fingerprint publication changed bytes");
    auto changed_large=large;changed_large[512]^=std::byte{1};
    need(actual(changed_large)!=expected && actual(large)==expected,"single-byte COW mutation reused stale fingerprint");
}
void benchmark() {
    std::vector<std::byte> bytes(1024*1024,std::byte{73});
    TexturePixels pixels{std::span<const std::byte>(bytes)};const auto expected=actual(pixels);
    const auto &input=std::as_const(pixels);std::uint64_t result{};
    for(bool reference:{true,false}) {
        const auto start=std::chrono::steady_clock::now();
        for(unsigned i=0;i<200;++i) {
            const auto value=reference?detail::texture_bytes_fnv1a({input.data(),input.size()}):input.fingerprint();
            need(value==expected,"fingerprint benchmark changed full-byte identity");result^=value;
        }
        std::cout<<(reference?"reference":"cached")<<"_ns="
            <<std::chrono::duration<double,std::nano>(std::chrono::steady_clock::now()-start).count()/200<<'\n';
    }
    need(result==0,"benchmark identity accumulation differs");
}
}
int main(int argc,char **argv)try {
    check();if(argc==2 && std::string_view(argv[1])=="--benchmark")benchmark();
    std::cout<<"PASS complete FNV1a, retained pointer/iterator writes, COW, moves, swaps and concurrent immutable readers\n";
}catch(const std::exception &error){std::cerr<<error.what()<<'\n';return 1;}
