#include "halo2/source_opaque_sort.hpp"
#include <limits>
#include <stdexcept>
#include <utility>
namespace halo2 {
bool source_opaque_compare(const SourceOpaqueSortRecord& a,const SourceOpaqueSortRecord& b) {
    if(a.words[4]!=b.words[4])return a.words[4]>b.words[4];
    const auto as=std::uint16_t(a.words[0]>>16),bs=std::uint16_t(b.words[0]>>16);
    if(as!=bs)return as>bs;
    const auto ac=std::uint8_t(a.words[1]>>8),bc=std::uint8_t(b.words[1]>>8);
    if(ac!=bc)return ac>bc;
    return std::uint16_t(a.words[2])>std::uint16_t(b.words[2]);
}
namespace {
bool layer5_compare(const SourceOpaqueSortRecord& a,const SourceOpaqueSortRecord& b) {
    // TU1.5 3BBC0 uses unsigned byte, halfword and dword comparisons.
    const auto key=[](const SourceOpaqueSortRecord& record) {
        const auto& w=record.words;
        return std::array<std::uint32_t,6>{(w[1]>>8)&255u,w[1]>>16,w[4],w[0]>>16,w[3],w[0]&65535u};
    };
    return key(a)>key(b);
}
}
void source_opaque_sort(std::span<SourceOpaqueSortRecord> records) {
    source_opaque_sort(records,SourceOpaqueSortOrder::material);
}
void source_opaque_sort(std::span<SourceOpaqueSortRecord> records,SourceOpaqueSortOrder order) {
    if(records.size()>unsigned(std::numeric_limits<int>::max()/24))
        throw std::invalid_argument("source opaque sort exceeds32-bit byte extent");
    if(records.size()<2)return;
    const auto compare=order==SourceOpaqueSortOrder::layer5?layer5_compare:source_opaque_compare;
    const auto greater=[&](int a,int b){return compare(records[unsigned(a)],records[unsigned(b)]);};
    const auto swap=[&](int a,int b){std::swap(records[unsigned(a)],records[unsigned(b)]);};
    struct Range{int left,right;};std::array<Range,30> pending{};unsigned count=0;
    int left=0,right=int(records.size())-1;
    for(;;) {
        if(right-left+1<=8) {
            for(int end=right;left<end;--end) {
                int maximum=left;
                for(int i=left+1;i<=end;++i)if(greater(i,maximum))maximum=i;
                swap(maximum,end);
            }
        } else {
            swap(left+(right-left+1)/2,left);int i=left,j=right+1;
            for(;;) {
                do{++i;}while(i<=right && !greater(i,left));
                do{--j;}while(j>left && !greater(left,j));
                if(i>j)break;
                swap(i,j);
            }
            swap(left,j);
            const auto push=[&](int a,int b) {
                if(count==pending.size())throw std::runtime_error("source opaque partition stack exceeded");
                pending[count++]={a,b};
            };
            //13DC44 compares byte distances: (j-left)*24-1 <(right-i)*24.
            if(j-left<=right-i) {
                if(i<right)push(i,right);
                if(left+1<j){right=j-1;continue;}
            } else {
                if(left+1<j)push(left,j-1);
                if(i<right){left=i;continue;}
            }
        }
        if(!count)break;
        const auto next=pending[--count];left=next.left;right=next.right;
    }
}
}
