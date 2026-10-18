#include "source_model_inline_cpu_bitmap_tu15.hpp"
#include "halo2/cache.hpp"
#include "halo2/source_model_render_tu15.hpp"
#include <algorithm>
#include <array>
#include <bit>
#include <limits>
#include <stdexcept>

namespace halo2::private_cpu_bitmap {
namespace {
void require(bool ok,const char* message){if(!ok)throw FormatError(message);}
unsigned word(const SourceRuntimeMemory&bank,unsigned at){return Bytes(bank.view(at,4)).u32(0);}
unsigned plus(unsigned at,unsigned offset){
    require(std::uint64_t(at)+offset+4<=0x100000000ull,"Private inline bitmap field wraps source address");
    return at+offset;
}
class Adapter final:public SourcePhysicsContactMemory {
    enum class Phase { idle,await_handle,await_edi,await_call,await_return_store,await_entry,prefix,done };
    SourceRuntimeMemory& bank_;
    const InlineBitmapFirstRow& provider_;
    Phase phase_=Phase::idle;
    unsigned pc_{},request_{},stack_{},table_{},entry_{},root_{},count_{};
    bool table_read_{},root_read_{},count_read_{};
    unsigned prefix_{};
    static constexpr std::array prefix{
        0xd5610u,0xd5613u,0xd5614u,0xd5615u,0xd5617u,0xd561au,0xd561cu,
        0xd561eu,0xd5624u,0xd5629u,0xd562cu,0xd5630u,0xd5632u,
        0xd5634u,0xd5637u,0xd5639u,0xd563bu};
    void current_directory()const {
        require(table_read_&&root_read_&&count_read_&&root_&&std::bit_cast<int>(count_)>0,
            "Private inline bitmap lacks its actual positive directory/root/count flow");
        require(word(bank_,0x4f0414u)==table_&&word(bank_,entry_)==0x6269746du&&
            word(bank_,plus(entry_,4))==request_&&word(bank_,plus(entry_,8))==root_&&
            word(bank_,plus(root_,0x44))==count_&&word(bank_,stack_-4u)==0xd45aau,
            "Private inline bitmap requested full salt, caller or current directory differs");
    }
public:
    Adapter(SourceRuntimeMemory&bank,const InlineBitmapFirstRow&provider):bank_(bank),provider_(provider){}
    void instruction(unsigned pc){
        pc_=pc;
        if(pc==0xd459du){
            require(phase_==Phase::idle||phase_==Phase::done,
                "Private inline bitmap caller replaced an unfinished qualified request");
            phase_=Phase::await_handle;table_read_=root_read_=count_read_=false;prefix_=0;return;
        }
        if(phase_==Phase::await_handle)throw FormatError("Private inline bitmap caller did not read its full handle");
        if(phase_==Phase::await_edi){require(pc==0xd45a1u,"Private inline bitmap changed EAX before original call");phase_=Phase::await_call;return;}
        if(phase_==Phase::await_call){require(pc==0xd45a5u,"Private inline bitmap lost its exact D45A5 call");phase_=Phase::await_return_store;return;}
        if(phase_==Phase::await_return_store)throw FormatError("Private inline bitmap call did not publish original return address");
        if(phase_==Phase::await_entry){
            require(pc==prefix[0]&&word(bank_,stack_-4u)==0xd45aau,
                "Private inline bitmap lost original D45A5 to D5610 call stack");
            phase_=Phase::prefix;prefix_=1;return;
        }
        if(phase_==Phase::prefix){
            if(pc==0xd563eu){
                const bool absent_handle=prefix_==7&&request_==0xffffffffu;
                const bool absent_root=prefix_==13&&root_read_&&!root_;
                const bool absent_count=prefix_==16&&count_read_&&std::bit_cast<int>(count_)<=0;
                require(absent_handle||absent_root||absent_count,
                    "Private inline bitmap skipped an unproved original prefix edge");
                phase_=Phase::done;return;
            }
            require(prefix_<prefix.size()&&pc==prefix[prefix_],
                "Private inline bitmap differs from original first-row lookup sequence");
            ++prefix_;return;
        }
        if(pc==0xd5610u||pc==0xd563bu)
            throw FormatError("Private inline bitmap D5610 route has no authenticated full-handle caller");
    }
    void read(unsigned at,std::span<std::byte> out)const override {
        auto&self=const_cast<Adapter&>(*this);
        if(pc_==0xd459du){
            require(phase_==Phase::await_handle&&out.size()==4&&at>=0x64u,
                "Private inline bitmap full-handle operand is not its complete original stack word");
            bank_.read(at,out);self.request_=Bytes(out).u32(0);self.stack_=at-0x60u;
            require(self.stack_>=4u,"Private inline bitmap caller stack underflows");
            self.phase_=Phase::await_edi;return;
        }
        if(pc_==0xd561eu){
            require(phase_==Phase::prefix&&out.size()==4&&at==0x4f0414u&&!table_read_,
                "Private inline bitmap directory operand differs");
            bank_.read(at,out);self.table_=Bytes(out).u32(0);
            const auto entry=std::uint64_t(table_)+std::uint64_t(request_&0xffffu)*16u;
            require(table_&&entry+16<=0x100000000ull,"Private inline bitmap directory index wraps");
            self.entry_=unsigned(entry);self.table_read_=true;return;
        }
        if(pc_==0xd562cu){
            require(phase_==Phase::prefix&&table_read_&&!root_read_&&out.size()==4&&at==plus(entry_,8),
                "Private inline bitmap root lookup is not indexed by its captured full request");
            bank_.read(at,out);self.root_=Bytes(out).u32(0);self.root_read_=true;return;
        }
        if(pc_==0xd5634u){
            require(phase_==Phase::prefix&&root_read_&&root_&&!count_read_&&out.size()==4&&at==plus(root_,0x44),
                "Private inline bitmap count operand differs from its actual root");
            bank_.read(at,out);self.count_=Bytes(out).u32(0);self.count_read_=true;return;
        }
        if(pc_==0xd563bu){
            require(phase_==Phase::prefix&&prefix_==prefix.size()&&out.size()==4&&at==plus(root_,0x48),
                "Private inline bitmap first-row operand differs from its original root+48 read");
            current_directory();bank_.read(at,out);const auto raw=Bytes(out).u32(0);
            const auto projected=provider_(TagHandle{request_},root_);
            current_directory();require(word(bank_,at)==raw,
                "Private inline bitmap provider changed raw canonical descriptor");
            require(projected&&projected!=raw&&
                bank_.source_address(bank_.native_address(projected,24),24)==projected&&
                std::ranges::equal(bank_.view(projected,24),bank_.view(raw,24)),
                "Private inline bitmap provider did not retain an actual matching common-prefix row");
            for(unsigned n=0;n<4;++n)out[n]=std::byte(projected>>(n*8));
            self.phase_=Phase::done;return;
        }
        bank_.read(at,out);
    }
    void write(unsigned at,std::span<const std::byte> in)override {
        if(pc_==0xd45a5u){
            require(phase_==Phase::await_return_store&&in.size()==4&&at==stack_-4u&&Bytes(in).u32(0)==0xd45aau,
                "Private inline bitmap call return-address store differs");
            bank_.write(at,in);phase_=Phase::await_entry;return;
        }
        if(phase_==Phase::prefix&&(pc_==0xd5613u||pc_==0xd5614u)){
            const auto offset=pc_==0xd5613u?0x84u:0x88u;
            require(stack_>=offset&&in.size()==4&&at==stack_-offset,
                "Private inline bitmap original callee stack does not match captured caller");
        }
        bank_.write(at,in);
    }
};
}
void run_model_3daf0_with_inline_cpu_bitmaps(SourceRuntimeMemory&bank,
    SourcePhysicsWorldRuntimeAbi&abi,SourcePhysicsFactoryFloatingPoint fp,
    const SourcePhysicsWorldRuntimeServices&input_services,
    const SourcePhysicsContactRecordObserver&observe,const InlineBitmapFirstRow&input_provider){
    // Input wrappers must remain valid during ordinary callable copying. After
    // the copies, callbacks may release/reset their caller-side wrappers.
    const auto parent=input_services;
    const auto provider=input_provider;
    require(bool(provider),"Private inline bitmap needs its actual typed owner provider");
    Adapter memory(bank,provider);
    auto services=parent;
    services.instruction=[&](unsigned pc){memory.instruction(pc);if(parent.instruction)parent.instruction(pc);};
    services.required_owner=[&](auto&source,const auto&call,auto&state,const auto&observer){
        require(&source==&memory,"Private inline bitmap descendant used a foreign memory adapter");
        require(bool(parent.required_owner),"Private inline bitmap descendant has no actual bank owner");
        parent.required_owner(bank,call,state,observer);
    };
    source_run_model_render_tu15(memory,0x3daf0u,abi,fp,services,observe);
}
}
