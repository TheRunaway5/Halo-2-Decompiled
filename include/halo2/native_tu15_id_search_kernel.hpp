#pragma once
#include <cstddef>
#include <cstdint>

namespace halo2::native_id_search_detail {
// First equal ID in an ordered queue. Count is captured once; each row stays
// lazy. The raw view publishes cursor/flags at its observable ABI boundaries.
template<class View>auto find(View&view){
    auto count=view.count();
    if(count){
        view.open_rows();
        for(decltype(count)index{};index<count;++index){
            const auto value=view.item(index);
            if(value==view.key())return view.found(index,value);
            view.advance(index,value);
        }
    }
    return view.absent();
}
template<class Range,class Key>struct Records {
    using Record=typename Range::value_type;
    const Range&rows;Key requested;
    std::size_t count()const{return rows.size();}
    void open_rows()const{}
    std::uint32_t item(std::size_t index)const{return rows[index].id;}
    Key key()const{return requested;}
    const Record*found(std::size_t index,std::uint32_t)const{return &rows[index];}
    void advance(std::size_t,std::uint32_t)const{}
    const Record*absent()const{return nullptr;}
};
template<class Range,class Key>auto record(const Range&rows,Key key){Records view{rows,key};return find(view);}
} // namespace halo2::native_id_search_detail
