#pragma once
#include "halo2/render_program.hpp"
#include <compare>
#include <initializer_list>
#include <iterator>
#include <memory>
#include <stdexcept>
#include <type_traits>
#include <utility>
#include <vector>

namespace halo2 {
// Indexed value bank, deliberately noncontiguous. Material fan-out shares
// unchanged authored records; mutable access copies only the selected record.
// Previously exposed mutable references keep snapshot independence on copy.
class MaterialPassBank {
    struct Entry {
        std::shared_ptr<MaterialPass> value=std::make_shared<MaterialPass>();
        bool exposed{};
        Entry()=default;
        explicit Entry(const MaterialPass& pass):value(std::make_shared<MaterialPass>(pass)){}
        explicit Entry(MaterialPass&& pass):value(std::make_shared<MaterialPass>(std::move(pass))),exposed(true){}
        Entry(const Entry& other):value(other.exposed?
            std::make_shared<MaterialPass>(*other.value):other.value){}
        Entry(Entry&&)noexcept=default;
        Entry& operator=(Entry&&)noexcept=default;
        Entry& operator=(const Entry& other){if(this!=&other){Entry copy(other);*this=std::move(copy);}return *this;}
    };
    std::vector<Entry> entries_;
    MaterialPass& writable(std::size_t n){auto& e=entries_[n];
        if(!e.value.unique())e.value=std::make_shared<MaterialPass>(*e.value);
        e.exposed=true;return *e.value;}
    friend class Cache;
    // Cache calls this only after its local decoder/binder views expire.
    void publish()noexcept{for(auto& entry:entries_)entry.exposed=false;}
    template<bool Const>class Iter {
        using Owner=std::conditional_t<Const,const MaterialPassBank,MaterialPassBank>;
        Owner* owner_{};std::ptrdiff_t index_{};
        friend class MaterialPassBank;
        template<bool>friend class Iter;
        Iter(Owner* owner,std::ptrdiff_t index):owner_(owner),index_(index){}
    public:
        using value_type=MaterialPass;
        using difference_type=std::ptrdiff_t;
        using reference=std::conditional_t<Const,const MaterialPass&,MaterialPass&>;
        using pointer=std::conditional_t<Const,const MaterialPass*,MaterialPass*>;
        using iterator_category=std::random_access_iterator_tag;
        using iterator_concept=std::random_access_iterator_tag;
        Iter()=default;
        template<bool C=Const>requires C
        Iter(const Iter<false>& other):owner_(other.owner_),index_(other.index_){}
        reference operator*()const{return (*owner_)[std::size_t(index_)];}
        pointer operator->()const{return std::addressof(**this);}
        reference operator[](difference_type n)const{return *(*this+n);}
        Iter& operator++(){++index_;return *this;}
        Iter operator++(int){auto before=*this;++*this;return before;}
        Iter& operator--(){--index_;return *this;}
        Iter operator--(int){auto before=*this;--*this;return before;}
        Iter& operator+=(difference_type n){index_+=n;return *this;}
        Iter& operator-=(difference_type n){index_-=n;return *this;}
        friend Iter operator+(Iter i,difference_type n){return i+=n;}
        friend Iter operator+(difference_type n,Iter i){return i+=n;}
        friend Iter operator-(Iter i,difference_type n){return i-=n;}
        friend difference_type operator-(const Iter& a,const Iter& b){return a.index_-b.index_;}
        bool operator==(const Iter& b)const{return owner_==b.owner_&&index_==b.index_;}
        auto operator<=>(const Iter& b)const{return index_<=>b.index_;}
    };
public:
    using value_type=MaterialPass;
    using size_type=std::size_t;
    using iterator=Iter<false>;
    using const_iterator=Iter<true>;
    MaterialPassBank()=default;
    MaterialPassBank(std::initializer_list<MaterialPass> values){reserve(values.size());for(const auto& value:values)push_back(value);}
    MaterialPassBank(const MaterialPassBank&)=default;
    MaterialPassBank(MaterialPassBank&&)noexcept=default;
    MaterialPassBank& operator=(const MaterialPassBank& other){if(this!=&other){MaterialPassBank copy(other);swap(copy);}return *this;}
    MaterialPassBank& operator=(MaterialPassBank&&)noexcept=default;
    MaterialPassBank& operator=(std::initializer_list<MaterialPass> values){MaterialPassBank copy(values);swap(copy);return *this;}
    void swap(MaterialPassBank& other)noexcept{entries_.swap(other.entries_);}
    size_type size()const noexcept{return entries_.size();}
    bool empty()const noexcept{return entries_.empty();}
    size_type capacity()const noexcept{return entries_.capacity();}
    void reserve(size_type n){entries_.reserve(n);}
    void resize(size_type n){entries_.resize(n);}
    void clear()noexcept{entries_.clear();}
    const MaterialPass& operator[](size_type n)const{return *entries_[n].value;}
    MaterialPass& operator[](size_type n){return writable(n);}
    const MaterialPass& at(size_type n)const{return *entries_.at(n).value;}
    MaterialPass& at(size_type n){if(n>=size())throw std::out_of_range("Material pass outside bank");return writable(n);}
    const MaterialPass& front()const{return (*this)[0];}
    MaterialPass& front(){return writable(0);}
    const MaterialPass& back()const{return (*this)[size()-1];}
    MaterialPass& back(){return writable(size()-1);}
    void push_back(const MaterialPass& value){entries_.emplace_back(value);}
    void push_back(MaterialPass&& value){entries_.emplace_back(std::move(value));}
    template<class... Args>MaterialPass& emplace_back(Args&&... args){entries_.emplace_back(MaterialPass(std::forward<Args>(args)...));return writable(size()-1);}
    iterator begin()noexcept{return {this,0};}
    iterator end()noexcept{return {this,std::ptrdiff_t(size())};}
    const_iterator begin()const noexcept{return {this,0};}
    const_iterator end()const noexcept{return {this,std::ptrdiff_t(size())};}
    const_iterator cbegin()const noexcept{return begin();}
    const_iterator cend()const noexcept{return end();}
};
}
