#pragma once
#include <cstddef>
#include <cstdint>
#include <initializer_list>
#include <memory>
#include <span>
#include <utility>
#include <vector>
#include <filesystem>
#include <atomic>

namespace halo2 {
struct Texture;
struct RetailFont;
// Value-owned image bytes. Completed images share residency across material
// copies; mutable access detaches first. Borrowed const views expire at the
// next mutable access. A previously exposed mutable view forces an independent
// copy, including after moves, so existing writers cannot change a snapshot.
class TexturePixels {
    using Buffer=std::vector<std::byte>;
    struct Storage {
        Buffer bytes;
        mutable std::atomic<std::uint64_t> fingerprint{};
        mutable std::atomic<bool> fingerprint_ready{};
        Storage()=default;
        explicit Storage(Buffer value):bytes(std::move(value)){}
        Storage(std::initializer_list<std::byte> value):bytes(value){}
        template<class It>Storage(It first,It last):bytes(first,last){}
    };
    std::shared_ptr<Storage> storage_;
    bool exposed_{};
    const Buffer& read()const {static const Buffer empty;return storage_?storage_->bytes:empty;}
    Buffer& write() {
        if(!storage_)storage_=std::make_shared<Storage>();
        else if(storage_.use_count()!=1)storage_=std::make_shared<Storage>(storage_->bytes);
        storage_->fingerprint_ready.store(false,std::memory_order_relaxed);
        exposed_=true;return storage_->bytes;
    }
    friend class Cache;
    friend Texture prepare_signed_texture(Texture,std::uint32_t);
    friend RetailFont load_retail_font(const std::filesystem::path&);
    friend RetailFont decode_retail_font(std::span<const std::byte>);
    friend std::size_t apply_retail_font_sampling(RetailFont&,const RetailFont&);
    friend std::size_t apply_retail_font_sampling(RetailFont&,const RetailFont&,const RetailFont&);
    friend std::size_t apply_mcc_classic_font_sampling(RetailFont&,const RetailFont&);
    friend std::size_t apply_vista_prompt_sampling(RetailFont&,const std::filesystem::path&,unsigned);
    // Cache publishes only after decoder-local writable views have expired.
    void publish()noexcept {exposed_=false;}
public:
    using value_type=std::byte;
    using size_type=Buffer::size_type;
    using difference_type=Buffer::difference_type;
    using iterator=Buffer::iterator;
    using const_iterator=Buffer::const_iterator;
    TexturePixels()=default;
    TexturePixels(Buffer bytes):storage_(std::make_shared<Storage>(std::move(bytes))),exposed_(true){}
    TexturePixels(std::initializer_list<std::byte> bytes):storage_(std::make_shared<Storage>(bytes)){}
    explicit TexturePixels(std::span<const std::byte> bytes):storage_(std::make_shared<Storage>(bytes.begin(),bytes.end())){}
    TexturePixels(const TexturePixels& other):storage_(other.exposed_&&other.storage_?
        std::make_shared<Storage>(other.storage_->bytes):other.storage_){}
    TexturePixels(TexturePixels&&)noexcept=default;
    TexturePixels& operator=(const TexturePixels& other) {
        if(this!=&other){TexturePixels copy(other);swap(copy);}return *this;
    }
    TexturePixels& operator=(TexturePixels&&)noexcept=default;
    TexturePixels& operator=(std::initializer_list<std::byte> bytes){write()=bytes;return *this;}
    operator const Buffer&()const{return read();}
    void swap(TexturePixels& other)noexcept {storage_.swap(other.storage_);std::swap(exposed_,other.exposed_);}
    size_type size()const noexcept{return storage_?storage_->bytes.size():0;}
    bool empty()const noexcept{return !size();}
    size_type capacity()const noexcept{return storage_?storage_->bytes.capacity():0;}
    // Complete original FNV1a identity. Mutable access invalidates before
    // returning a writer; once exposed, every call hashes the actual bytes.
    // Published/shared snapshots have no retained writers and can share one
    // result. Concurrent immutable readers may compute the same result, with
    // atomic publication preserving the container's existing read contract.
    std::uint64_t fingerprint()const noexcept {
        if(!exposed_ && storage_ && storage_->fingerprint_ready.load(std::memory_order_acquire))
            return storage_->fingerprint.load(std::memory_order_relaxed);
        std::uint64_t hash=14695981039346656037ull;
        for(const auto byte:read()){hash^=std::to_integer<unsigned char>(byte);hash*=1099511628211ull;}
        if(!exposed_ && storage_) {
            storage_->fingerprint.store(hash,std::memory_order_relaxed);
            storage_->fingerprint_ready.store(true,std::memory_order_release);
        }
        return hash;
    }
    const std::byte* data()const noexcept{return read().data();}
    std::byte* data(){return write().data();}
    const_iterator begin()const noexcept{return read().begin();}
    const_iterator end()const noexcept{return read().end();}
    const_iterator cbegin()const noexcept{return read().cbegin();}
    const_iterator cend()const noexcept{return read().cend();}
    iterator begin(){return write().begin();}
    iterator end(){return write().end();}
    const std::byte& operator[](size_type n)const{return read()[n];}
    std::byte& operator[](size_type n){return write()[n];}
    const std::byte& at(size_type n)const{return read().at(n);}
    std::byte& at(size_type n){return write().at(n);}
    const std::byte& front()const{return read().front();}
    std::byte& front(){return write().front();}
    const std::byte& back()const{return read().back();}
    std::byte& back(){return write().back();}
    void resize(size_type n){write().resize(n);}
    void resize(size_type n,std::byte value){write().resize(n,value);}
    void reserve(size_type n){write().reserve(n);}
    void clear(){write().clear();}
    void push_back(std::byte value){write().push_back(value);}
    void pop_back(){write().pop_back();}
    void assign(size_type n,std::byte value){write().assign(n,value);}
    template<class It>void assign(It first,It last){write().assign(first,last);}
    template<class It>iterator insert(const_iterator at,It first,It last) {
        const auto offset=at-read().begin();auto& bytes=write();return bytes.insert(bytes.begin()+offset,first,last);
    }
    iterator insert(const_iterator at,size_type count,std::byte value) {
        const auto offset=at-read().begin();auto& bytes=write();return bytes.insert(bytes.begin()+offset,count,value);
    }
    friend bool operator==(const TexturePixels& a,const TexturePixels& b){return a.storage_==b.storage_||a.read()==b.read();}
    friend bool operator==(const TexturePixels& a,const Buffer& b){return a.read()==b;}
    friend bool operator==(const Buffer& a,const TexturePixels& b){return a==b.read();}
};
}
