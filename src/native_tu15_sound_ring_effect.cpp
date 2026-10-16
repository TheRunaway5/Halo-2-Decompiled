#include "halo2/native_tu15_sound_ring_effect.hpp"
#include <algorithm>
#include <limits>
#include <stdexcept>
namespace halo2::tu15_sound_ring_effect {
namespace {
constexpr std::int64_t unit=std::int64_t{1}<<24;
constexpr std::uint64_t mask56=(std::uint64_t{1}<<56)-1;
constexpr std::array<unsigned,14> ring_words{6000,6000,6000,6000,680,612,640,720,1392,1684,2044,2476,24000,24000};
constexpr std::array<unsigned,31> read_ring{0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,1,1,2,2,3,3,4,5,6,7,8,9,10,11,12,13};
constexpr std::array<unsigned,14> write_block{2,16,18,20,3,4,5,6,7,8,9,10,11,12};
std::int64_t signed_word(Word v){return v&0x800000u?std::int64_t(v)-unit:v;}
// SM0 accumulators retain the full56 bits; stores apply the normal24-bit limiter.
// MACR rounds the actual sum to even, and ASL wraps the56-bit accumulator.
std::int64_t wrap56(std::uint64_t v){v&=mask56;return v&(std::uint64_t{1}<<55)?std::int64_t(v)-std::int64_t(std::uint64_t{1}<<56):std::int64_t(v);}
std::int64_t add(std::int64_t a,std::int64_t b){return wrap56(std::uint64_t(a)+std::uint64_t(b));}
std::int64_t shifted(std::int64_t a,unsigned n){return wrap56(std::uint64_t(a)<<n);}
std::int64_t floor_units(std::int64_t a){return a>=0?a/unit:-((-a+unit-1)/unit);}
Word transfer(std::int64_t a){return Word(std::clamp(floor_units(a),std::int64_t{-0x800000},std::int64_t{0x7fffff}))&0xffffffu;}
std::int64_t product(Word a,Word b){return 2*signed_word(a)*signed_word(b);}
std::int64_t rounded(std::int64_t a){a=add(a,unit/2);auto bits=std::uint64_t(a)&mask56;if((bits&0xffffffu)==0)bits&=~std::uint64_t(unit);return wrap56(bits&~std::uint64_t(0xffffffu));}
std::int64_t macr(std::int64_t a,Word b,Word c){return rounded(add(a,product(b,c)));}
void word(Word w){if(w>0xffffffu)throw std::invalid_argument("reverb word outside 24-bit domain");}
void emit(ArithmeticTrace* trace,Word value){if(trace){if(trace->count>=trace->values.size())throw std::logic_error("reverb arithmetic trace capacity");trace->values[trace->count++]=value;}}
void one_pole(OnePole& f,Word* samples,ArithmeticTrace* trace){for(unsigned i=0;i<16;i++){auto a=add(product(f.feedback,f.previous),product(f.feedforward,samples[i]));f.previous=transfer(a);samples[i]=transfer(a);emit(trace,samples[i]);}}
void scale(Word* samples,Word gain,ArithmeticTrace* trace){for(unsigned i=0;i<16;i++){samples[i]=transfer(product(samples[i],gain));emit(trace,samples[i]);}}
void add_block(const Word* from,Word* to,ArithmeticTrace* trace){for(unsigned i=0;i<16;i++){to[i]=transfer(add(signed_word(to[i])*unit,signed_word(from[i])*unit));emit(trace,to[i]);}}
void sum_block(const Word* a,const Word* b,Word* out,ArithmeticTrace* trace){for(unsigned i=0;i<16;i++){out[i]=transfer(add(signed_word(a[i])*unit,signed_word(b[i])*unit));emit(trace,out[i]);}}
void mix_block(const Word* from,Word* to,Word gain,ArithmeticTrace* trace){for(unsigned i=0;i<16;i++){to[i]=transfer(macr(signed_word(to[i])*unit,from[i],gain));emit(trace,to[i]);}}
std::uint32_t delayed_cursor(const State& s,unsigned read,unsigned write,Word delay){
    const auto length=std::int64_t(s.rings[write].words.size()*4);
    auto cursor=signed_word(Word((s.rings[write].write_byte_cursor-delay)&0xfffff0u));
    while(cursor<0)cursor+=length;
    if(cursor>=std::int64_t(s.rings[read_ring[read]].words.size()*4))throw std::invalid_argument("reverb delay exceeds selected ring");
    return std::uint32_t(cursor);
}
void set_delay(State& s,unsigned read,unsigned write,Word delay){s.taps[read].read_byte_cursor=delayed_cursor(s,read,write,delay);}
void initialize(State& s){
    s.flags|=1;
    for(auto& r:s.rings)r.write_byte_cursor=0;
    for(auto& t:s.taps)t.read_byte_cursor=0;
    s.current_gains=s.target_gains;s.fade.pending_mask=0;s.fade.sample_count=0;s.smoothing_count=0xffffffu;
    s.dormant_biquad_coefficients={0x424778,0x4b40e6,0x424778,0x23eb20,0xdcbab6,0x424778,0x045b75,0x424778,0x117de1,0x8f1750};
    for(unsigned j=0;j<5;j++)set_delay(s,j,0,s.delays[j]);
    for(unsigned j=0;j<8;j++)set_delay(s,5+j,0,transfer(add(signed_word(s.cluster_taps[j])*unit,signed_word(s.delays[5])*unit)));
    for(unsigned j=0;j<4;j++)for(unsigned k=0;k<2;k++)set_delay(s,13+j*2+k,j,s.delays[6+j*2+k]);
    for(unsigned j=0;j<4;j++)set_delay(s,21+j,4+j,s.delays[14+j]);
    for(unsigned j=0;j<4;j++)set_delay(s,25+j,8+j,s.cluster_taps[8+j]);
    for(unsigned j=0;j<2;j++)set_delay(s,29+j,12+j,s.delays[18]);
}
Word current_fade_tap(const State& s){const auto j=s.fade.tap_cursor;if(s.fade.tap_owner==TapOwner::targets&&j<19)return s.delays[j];if(s.fade.tap_owner==TapOwner::adjusted&&j<8)return s.fade.adjusted_taps[j];throw std::invalid_argument("unsupported reverb fade tap cursor");}
void begin_fade_group(State& s){auto& f=s.fade;
    if(f.pending_mask&1u){f.selected_read=0;f.selected_write=0;f.tap_cursor=0;f.tap_owner=TapOwner::targets;f.write_stride=0;}
    else if(f.pending_mask&(1u<<5)){f.selected_write=0;f.selected_read=5;f.tap_cursor=0;f.tap_owner=TapOwner::adjusted;f.write_stride=0;for(unsigned j=0;j<8;j++)f.adjusted_taps[j]=transfer(add(signed_word(s.cluster_taps[j])*unit,signed_word(s.delays[5])*unit));}
    else if(f.pending_mask&(1u<<13)){f.tap_cursor=14;f.tap_owner=TapOwner::targets;f.selected_write=4;f.selected_read=21;f.write_stride=1;}
    else if(f.pending_mask&(1u<<17)){f.selected_write=12;f.selected_read=29;f.tap_cursor=0;f.tap_owner=TapOwner::adjusted;f.adjusted_taps[0]=s.delays[18];f.adjusted_taps[1]=s.delays[18];f.write_stride=1;}
    else if(f.pending_mask&(1u<<19)){f.pending_mask=0;f.sample_count=0;s.smoothing_count=0xffffffu;return;}
    else {++f.selected_read;f.selected_write+=f.write_stride;}
    f.bindings_initialized=true;f.step=0xff0000;f.amplitude=0x7f0000;
}
void parameters(State& s,Workspace& workspace,ArithmeticTrace* trace){
    if(signed_word(s.smoothing_count)>=0){if(s.smoothing_count==0x150){s.current_gains=s.target_gains;s.smoothing_count=0xffffff;}else {s.smoothing_count=(s.smoothing_count+1)&0xffffffu;for(unsigned j=0;j<6;j++){const Word difference=transfer(add(signed_word(s.current_gains[j])*unit,-signed_word(s.target_gains[j])*unit));s.current_gains[j]=transfer(add(signed_word(s.target_gains[j])*unit,product(0x7c49ba,difference)));}}}
    auto& f=s.fade;if(!f.pending_mask)return;
    if(f.sample_count==0){begin_fade_group(s);if(!f.pending_mask)return;}
    else if(f.sample_count==0x80){const auto delay=current_fade_tap(s);++f.tap_cursor;const auto shortened=std::max<std::int64_t>(signed_word(delay)-0x40,0);set_delay(s,f.selected_read,f.selected_write,Word(shortened));f.step=0x10000;}
    else if(f.sample_count==0x100){f.sample_count=0;f.pending_mask=(f.pending_mask<<1)&0xffffffu;return;}
    auto amplitude=signed_word(f.amplitude)*unit;auto* samples=workspace[2+f.selected_read].data();
    for(unsigned j=0;j<16;j++){samples[j]=transfer(product(transfer(amplitude),samples[j]));emit(trace,samples[j]);amplitude=add(amplitude,signed_word(f.step)*unit);}
    f.amplitude=transfer(amplitude);f.sample_count=(f.sample_count+16)&0xffffffu;
}
void combine(State& s,Workspace& workspace,unsigned first,unsigned second,unsigned dry,unsigned destination,unsigned gain,bool dual,ArithmeticTrace* trace){
    const Word a_gain=s.current_gains[gain],b_gain=s.current_gains[gain+1],wet=s.current_gains[4];Word dry_gain=s.current_gains[5];
    if(dual)dry_gain=transfer(signed_word(dry_gain)*unit/2); // arithmetic ASR before transfer
    for(unsigned j=0;j<16;j++){
        auto b=product(workspace[first+1][j],b_gain);
        auto a=dual?product(workspace[first][j],a_gain):macr(b,workspace[first][j],a_gain);
        if(dual)b=macr(b,workspace[second+1][j],wet);else a=macr(a,workspace[second+1][j],wet);
        a=macr(a,workspace[second][j],wet);
        if(dual){b=macr(b,dry_gain,workspace[dry][j]);a=macr(a,dry_gain,workspace[dry][j]);workspace[destination+1][j]=transfer(shifted(b,7));emit(trace,workspace[destination+1][j]);}
        else a=macr(a,dry_gain,workspace[dry][j]);
        workspace[destination][j]=transfer(shifted(a,7));emit(trace,workspace[destination][j]);
    }
}
struct Extent{std::uintptr_t begin,end;};
Extent extent(const void* p,std::size_t n){const auto b=reinterpret_cast<std::uintptr_t>(p);if(!p||n>std::numeric_limits<std::uintptr_t>::max()-b)throw std::invalid_argument("invalid reverb span");return{b,b+n};}
bool overlaps(Extent a,Extent b){return a.begin<b.end&&b.begin<a.end;}
void validate(const State& s,const Workspace& workspace,std::span<Word,32> input,const std::array<std::span<Word,32>,4>& outputs,const Controls& c,ArithmeticTrace* trace){
    constexpr Word forbidden=(3u<<10)|(1u<<12)|(1u<<13)|(1u<<14)|(1u<<15)|(1u<<16)|(1u<<17)|(1u<<18)|(1u<<20)|(1u<<21);
    word(c.entry_status);if(c.entry_status&forbidden)throw std::invalid_argument("unsupported reverb entry status");
    for(auto m:c.modifiers)if(m!=0xffffffu)throw std::invalid_argument("unsupported reverb addressing mode");
    std::array<Extent,22> extents{};extents[20]=extent(&workspace,sizeof workspace);extents[0]=extent(&s,sizeof s);extents[1]=extent(input.data(),input.size_bytes());for(unsigned j=0;j<4;j++)extents[2+j]=extent(outputs[j].data(),outputs[j].size_bytes());
    for(unsigned j=0;j<14;j++){const auto& ring=s.rings[j];if(ring.words.size()!=ring_words[j]||ring.write_byte_cursor>=ring.words.size()*4||ring.write_byte_cursor%16)throw std::invalid_argument("unsupported reverb history geometry");extents[6+j]=extent(ring.words.data(),ring.words.size()*sizeof(Word));for(auto x:ring.words)word(x);}
    if(trace)extents[21]=extent(trace,sizeof *trace);
    for(unsigned j=0;j<(trace?22u:21u);j++)for(unsigned k=0;k<j;k++)if(overlaps(extents[j],extents[k]))throw std::invalid_argument("aliased reverb owner or channels");
    for(unsigned j=0;j<31;j++)if(s.taps[j].read_byte_cursor>=s.rings[read_ring[j]].words.size()*4||s.taps[j].read_byte_cursor%16)throw std::invalid_argument("unsupported reverb read cursor");
    word(s.flags);word(s.cross_gain);word(s.mono_gain);word(s.output_mode);word(s.smoothing_count);
    for(auto x:s.delays){word(x);if(x&0x800000)throw std::invalid_argument("unsupported negative reverb delay");}
    for(auto x:s.cluster_taps){word(x);if(x&0x800000)throw std::invalid_argument("unsupported negative cluster tap");}
    for(auto x:s.reflection_gains)word(x);
    for(auto x:s.target_gains)word(x);
    for(auto x:s.current_gains)word(x);
    for(auto x:s.diffusion_gains)word(x);
    for(auto x:s.wet_gains)word(x);
    for(auto x:s.dormant_biquad_coefficients)word(x);
    for(const auto& f:s.filters){word(f.previous);word(f.feedback);word(f.feedforward);}for(const auto& b:workspace)for(auto x:b)word(x);for(auto x:input)word(x);for(auto out:outputs)for(auto x:out)word(x);
    const auto& f=s.fade;word(f.pending_mask);word(f.sample_count);word(f.step);word(f.amplitude);for(auto x:f.adjusted_taps)word(x);
    if(f.pending_mask&&(f.pending_mask>(1u<<19)||(f.pending_mask&(f.pending_mask-1))))throw std::invalid_argument("unsupported reverb fade mask");
    if(f.sample_count>0x100||f.sample_count%16||f.selected_read>30||f.selected_write>13||f.write_stride>1||(f.tap_owner!=TapOwner::targets&&f.tap_owner!=TapOwner::adjusted)||f.tap_cursor>(f.tap_owner==TapOwner::targets?19u:8u))throw std::invalid_argument("unsupported reverb transition state");
    // Probe all cursor arithmetic on a separate typed owner before effect mutation.
    State check=s;if(!(check.flags&1u))initialize(check);if(check.flags&4u){check.flags&=~4u;check.fade.sample_count=0;check.fade.pending_mask=1;check.smoothing_count=0;for(unsigned j=0;j<4;j++)for(unsigned k=0;k<2;k++)set_delay(check,13+j*2+k,j,check.delays[6+j*2+k]);for(unsigned j=0;j<4;j++)set_delay(check,25+j,8+j,check.cluster_taps[8+j]);}
    const auto pending=check.fade.pending_mask;
    const bool binding_group=pending==1u||pending==(1u<<5)||pending==(1u<<13)||pending==(1u<<17)||pending==(1u<<19);
    if(pending&&!check.fade.bindings_initialized&&!(check.fade.sample_count==0x100||(check.fade.sample_count==0&&binding_group)))throw std::invalid_argument("uninitialized reverb transition bindings");
    if(check.fade.pending_mask&&check.fade.sample_count==0){begin_fade_group(check);if(check.fade.selected_read>30||check.fade.selected_write>13)throw std::invalid_argument("reverb fade advance exceeds typed descriptor owner");}
    else if(check.fade.pending_mask&&check.fade.sample_count==0x80){const auto delay=current_fade_tap(check);(void)delayed_cursor(check,check.fade.selected_read,check.fade.selected_write,Word(std::max<std::int64_t>(signed_word(delay)-0x40,0)));}
}
}
void process(State& s,Workspace& workspace,std::span<Word,32> input,std::array<std::span<Word,32>,4> outputs,const Controls& controls,ArithmeticTrace* trace){
    validate(s,workspace,input,outputs,controls,trace);
    if(trace)trace->count=0;
    if(!(s.flags&1u))initialize(s);
    if(s.flags&4u){s.flags&=~4u;s.fade.sample_count=0;s.fade.pending_mask=1;s.smoothing_count=0;for(unsigned j=0;j<4;j++)for(unsigned k=0;k<2;k++)set_delay(s,13+j*2+k,j,s.delays[6+j*2+k]);for(unsigned j=0;j<4;j++)set_delay(s,25+j,8+j,s.cluster_taps[8+j]);}
    // All31 reads precede arithmetic; all14 writes follow it. Rings and shared
    // workspace preserve actual source aliases without an interpreted DSP bank.
    for(unsigned j=0;j<31;j++){auto& t=s.taps[j];const auto& ring=s.rings[read_ring[j]];for(unsigned k=0;k<16;k++)workspace[2+j][k]=ring.words[((t.read_byte_cursor/4)+k)%ring.words.size()];t.read_byte_cursor=std::uint32_t((t.read_byte_cursor+64)%(ring.words.size()*4));}
    scale(input.data(),0x400000,trace);scale(input.data()+16,0x400000,trace);
    one_pole(s.filters[0],input.data(),trace);one_pole(s.filters[0],input.data()+16,trace);
    for(unsigned j=0;j<16;j++)input[j]=input[j*2];
    parameters(s,workspace,trace);
    auto block=[&](unsigned j){return workspace[j].data();};
    for(unsigned j=0;j<4;j++)add_block(block(2),block(3+j),trace);
    for(unsigned j=0;j<4;j++)mix_block(block(23+j),block(3+j),s.wet_gains[j],trace);
    for(unsigned j=0;j<4;j++)for(unsigned k=0;k<2;k++)mix_block(block(7+j*2+k),block(27+j),s.reflection_gains[j*2+k],trace);
    sum_block(block(16),block(17),block(11),trace);sum_block(block(15),block(18),block(12),trace);add_block(block(19),block(11),trace);add_block(block(20),block(12),trace);add_block(block(22),block(11),trace);add_block(block(21),block(12),trace);sum_block(block(11),block(12),block(13),trace);
    scale(block(13),s.mono_gain,trace);scale(block(11),s.cross_gain,trace);scale(block(12),s.cross_gain,trace);
    add_block(block(31),block(11),trace);add_block(block(32),block(12),trace);workspace[31]=workspace[11];workspace[32]=workspace[12];one_pole(s.filters[8],block(11),trace);one_pole(s.filters[9],block(12),trace);
    for(unsigned j=0;j<3;j++)one_pole(s.filters[1+j],block(16+j*2),trace);
    for(unsigned row=0;row<4;row++)for(unsigned sample=0;sample<16;sample++){std::int64_t a=0;for(unsigned column=0;column<4;column++)a=macr(a,workspace[27+column][sample],s.diffusion_gains[3-row+column]);workspace[7+row][sample]=transfer(a);emit(trace,workspace[7+row][sample]);}
    for(unsigned j=0;j<4;j++)add_block(block(13),block(7+j),trace);
    for(unsigned j=0;j<4;j++)one_pole(s.filters[4+j],block(7+j),trace);
    std::copy_n(input.data(),16,block(2));
    const bool dual=s.output_mode==4;combine(s,workspace,3,27,31,23,0,dual,trace);combine(s,workspace,5,29,32,dual?25:24,2,dual,trace);
    auto output=[&](unsigned from,unsigned destination){for(unsigned j=0;j<16;j++){workspace[29+j/8][(j*2)%16]=workspace[from][j];workspace[29+j/8][(j*2+1)%16]=workspace[from][j];}add_block(block(29),outputs[destination].data(),trace);add_block(block(30),outputs[destination].data()+16,trace);};
    if(dual){output(25,2);output(26,3);}output(23,0);output(24,1);
    for(unsigned j=0;j<14;j++){auto& ring=s.rings[j];for(unsigned k=0;k<16;k++)ring.words[((ring.write_byte_cursor/4)+k)%ring.words.size()]=workspace[write_block[j]][k];ring.write_byte_cursor=std::uint32_t((ring.write_byte_cursor+64)%(ring.words.size()*4));}
}
}
