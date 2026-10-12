#include "halo2/vista_local_user.hpp"
#include "halo2/cache.hpp"
#include <algorithm>
#include <fstream>
#include <iostream>
#include <sstream>
#include <stdexcept>
#include <string>
#include <vector>

using namespace halo2;
namespace {
using UserBank=std::array<std::byte,vista_local_user_bank_size>;
using UserRecord=std::array<std::byte,vista_local_user_stride>;
void check(bool ok,const std::string& message) {if(!ok)throw std::runtime_error(message);}
std::uint32_t word(const std::string& text,int base=16) {
  std::size_t used{};const auto value=std::stoull(text,&used,base);
  check(used==text.size() && value<=0xffffffffu,"Invalid original fixture word");
  return std::uint32_t(value);
}
template<std::size_t N> std::array<std::byte,N> bytes(const std::string& text) {
  check(text.size()==N*2,"Invalid original fixture byte count");
  std::array<std::byte,N> result;
  for(std::size_t i=0;i<N;++i)result[i]=std::byte(word(text.substr(i*2,2)));
  return result;
}
std::vector<std::string> split(const std::string& text,char separator) {
  std::istringstream input(text);std::vector<std::string> result;std::string token;
  while(std::getline(input,token,separator))result.push_back(token);
  return result;
}
std::uint32_t get(std::span<const std::byte> bank,std::size_t offset) {
  std::uint32_t result{};
  for(unsigned i=0;i<4;++i)result|=std::uint32_t(std::to_integer<unsigned char>(bank[offset+i]))<<(i*8);
  return result;
}
void put(std::span<std::byte> bank,std::size_t offset,std::uint32_t value) {
  for(unsigned i=0;i<4;++i)bank[offset+i]=std::byte(value>>(i*8));
}
void compare(std::span<const std::byte> actual,std::span<const std::byte> expected,
    const std::string& label) {
  check(actual.size()==expected.size(),label+": compared byte sizes differ");
  for(std::size_t i=0;i<actual.size();++i)if(actual[i]!=expected[i]) {
    std::ostringstream message;message<<label<<": source bank differs at +0x"<<std::hex<<i;
    throw std::runtime_error(message.str());
  }
}
template<class F> void rejects(F function,const std::string& label) {
  try {function();}catch(const FormatError&){return;}
  throw std::runtime_error(label+": unsupported source context was admitted");
}
std::span<std::byte> selected(UserBank& bank,std::uint32_t slot) {
  check(slot<4,"Invalid retained source slot");
  return std::span(bank).subspan(slot*vista_local_user_stride,vista_local_user_stride);
}

// These are explicit field projections from original retained raw records,
// not additional unchanged-PE payload-read executions. Its pinned 5B10E0
// instructions establish nonzero-active and independently nullable outputs.
unsigned projected_payload_reads(const UserBank& bank,std::uint32_t slot,
    const std::string& label) {
  const auto original=std::span(bank).subspan(slot*vista_local_user_stride,vista_local_user_stride);
  const bool active=original[0]!=std::byte{};
  for(unsigned presence=0;presence<16;++presence) {
    std::array<std::uint32_t,3> scalars{0xa5a5a5a5,0x6d6d6d6d,0x7c7c7c7c};
    const auto scalars_before=scalars;
    VistaLocalUserPayload payload;payload.fill(std::byte{0x3a});const auto payload_before=payload;
    VistaLocalUserPayloadOutputs outputs{
      presence&1 ? &scalars[0] : nullptr,presence&2 ? &payload : nullptr,
      presence&4 ? &scalars[1] : nullptr,presence&8 ? &scalars[2] : nullptr};
    check(vista_local_user_read_payload(bank,slot,outputs)==active,label+": payload AL differs");
    check(scalars[0]==(active && presence&1 ? get(original,0xc) : scalars_before[0]),
        label+": controller output/preservation differs");
    check(scalars[1]==(active && presence&4 ? get(original,0x94) : scalars_before[1]),
        label+": first trailer output/preservation differs");
    check(scalars[2]==(active && presence&8 ? get(original,0x98) : scalars_before[2]),
        label+": second trailer output/preservation differs");
    compare(payload,active && presence&2 ? original.subspan(0x10,0x84) : std::span<const std::byte>(payload_before),
        label+": payload output/preservation");
  }
  return 16;
}

void bounded_controls() {
  UserBank bank;bank.fill(std::byte{0x6d});bank[0]=std::byte{};
  VistaLocalUserIdentity identity;identity.fill(std::byte{0x32});
  VistaLocalUserPayload payload;payload.fill(std::byte{0x57});
  for(const auto size:{std::size_t(0),vista_local_user_bank_size-1}) {
    const auto before=bank;auto output=identity;const auto output_before=output;
    std::uint32_t scalar=0x12345678;
    const auto truncated=std::span(bank).first(size);
    rejects([&]{(void)vista_local_user_allocate(truncated,identity);},"truncated allocation");
    rejects([&]{vista_local_user_remove(truncated,0);},"truncated removal");
    rejects([&]{(void)vista_local_user_read_identity(truncated,0,output);},"truncated identity read");
    rejects([&]{(void)vista_local_user_read_payload(truncated,0,{&scalar});},"truncated payload read");
    rejects([&]{vista_local_user_write_identity(truncated,0,identity);},"truncated identity write");
    rejects([&]{vista_local_user_write_payload(truncated,0,0,payload,1,2);},"truncated payload write");
    check(bank==before && output==output_before && scalar==0x12345678,
        "Truncated owner rejection changed storage/output");
  }
  for(const auto slot:{4u,0xffffffffu}) {
    const auto before=bank;auto output=identity;
    std::uint32_t scalar=0x12345678;
    rejects([&]{vista_local_user_remove(bank,slot);},"unbounded removal");
    rejects([&]{(void)vista_local_user_read_identity(bank,slot,output);},"unbounded identity read");
    rejects([&]{(void)vista_local_user_read_payload(bank,slot,{&scalar});},"unbounded payload read");
    rejects([&]{vista_local_user_write_identity(bank,slot,identity);},"unbounded identity write");
    rejects([&]{vista_local_user_write_payload(bank,slot,0,payload,1,2);},"unbounded payload write");
    check(bank==before && output==identity && scalar==0x12345678,
        "Invalid user-slot rejection changed storage/output");
  }
  bank.fill(std::byte{0x80});const auto full=bank;
  rejects([&]{(void)vista_local_user_allocate(bank,identity);},"unsafe full-bank underflow");
  check(bank==full,"Full-bank native rejection changed the source owner");
  bank[2*vista_local_user_stride]=std::byte{};
  check(vista_local_user_allocate(bank,identity)==2,"Nonzero active byte was treated as an empty slot");
  auto expected=full;auto expected_record=selected(expected,2);
  std::fill(expected_record.begin(),expected_record.end(),std::byte{});
  expected_record[0]=std::byte{1};std::copy(identity.begin(),identity.end(),expected_record.begin()+1);
  compare(bank,expected,"allocation whole-record clear and other-bank preservation");

  // Active is any nonzero byte; output reads never normalize or mutate it.
  bank[2*vista_local_user_stride]=std::byte{0xff};auto output=identity;output.fill(std::byte{});
  const auto read_before=bank;
  check(vista_local_user_read_identity(bank,2,output) && output==identity,
      "Noncanonical active byte did not retain source identity-read semantics");
  projected_payload_reads(bank,2,"nonzero active byte");check(bank==read_before,"Read changed source bank");

  // A real typed output object is inside the byte view, so rejection itself
  // has defined C++ object lifetime/alignment and performs no aliased write.
  std::array<VistaLocalUserIdentity,vista_local_user_bank_size/8> identities{};
  auto identity_bank=std::as_writable_bytes(std::span(identities));identity_bank[0]=std::byte{1};
  const auto identities_before=identities;
  rejects([&]{(void)vista_local_user_read_identity(identity_bank,0,identities[1]);},"aliased identity output");
  check(identities==identities_before,"Aliased identity output rejection changed the bank");
  identity_bank[0]=std::byte{};const auto inactive_alias=identities;
  check(!vista_local_user_read_identity(identity_bank,0,identities[1]) && identities==inactive_alias,
      "Inactive original guard consumed an unneeded aliased output");
  std::array<VistaLocalUserPayload,6> payloads{};
  auto payload_bank=std::as_writable_bytes(std::span(payloads));payload_bank[0]=std::byte{1};
  const auto payloads_before=payloads;std::uint32_t scalar=0x12345678;
  rejects([&]{(void)vista_local_user_read_payload(payload_bank,0,{&scalar,&payloads[1]});},"aliased payload output");
  check(payloads==payloads_before && scalar==0x12345678,
      "Aliased payload output rejection partially changed storage/output");
  rejects([&]{(void)vista_local_user_read_payload(bank,2,{&scalar,nullptr,&scalar});},"overlapping scalar outputs");
  check(bank==read_before && scalar==0x12345678,"Overlapping output rejection partially wrote");

  bank.fill(std::byte{0x6d});bank[0]=std::byte{};const auto inactive_before=bank;
  vista_local_user_write_payload(bank,0,0x87654321,payload,0xdeadbeef,0xfedcba98);
  for(std::size_t i=0;i<bank.size();++i) {
    if(i>=0xc && i<0x9c)continue;
    check(bank[i]==inactive_before[i],"Payload write changed inactive/identity/opaque bytes");
  }
  check(bank[0]==std::byte{} && get(bank,0xc)==0x87654321 && get(bank,0x94)==0xdeadbeef &&
      get(bank,0x98)==0xfedcba98,"Inactive payload write activated a user or lost scalar bytes");
  compare(std::span(bank).subspan(0x10,0x84),payload,"complete inactive payload write");
  projected_payload_reads(bank,0,"inactive payload write remains inactive");
  vista_local_user_remove(bank,0);
  check(std::all_of(bank.begin(),bank.begin()+vista_local_user_stride,[](auto b){return b==std::byte{};}),
      "Removal did not clear an inactive record's opaque bytes");

  std::array<std::byte,vista_local_user_bank_size+16> extended{};
  std::fill(extended.begin()+vista_local_user_bank_size,extended.end(),std::byte{0x7c});
  check(vista_local_user_allocate(extended,identity)==0,"Extended source bank was rejected");
  vista_local_user_write_payload(extended,0,2,payload,3,4);vista_local_user_remove(extended,0);
  check(std::all_of(extended.begin()+vista_local_user_bank_size,extended.end(),
      [](auto b){return b==std::byte{0x7c};}),"Local-user leaf wrote beyond its owner extent");

  std::array<std::byte,vista_controller_user_bank_size+16> controllers{};
  const auto controllers_before=controllers;
  rejects([&]{(void)vista_controller_local_user(std::span(controllers).first(vista_controller_user_bank_size-1),0);},
      "truncated controller owner");
  for(const auto slot:{4u,0xffffffffu})rejects([&]{(void)vista_controller_local_user(controllers,slot);},
      "unbounded controller slot");
  check(controllers==controllers_before,"Controller query rejection mutated the bank");
  for(const auto mapping:{0xffffffffu,5u,0x12345678u}) {
    put(controllers,3*vista_controller_user_stride+4,mapping);
    check(vista_controller_local_user(controllers,3)==mapping,"Raw mapping was coerced into a user slot");
  }
}
}

int main(int argc,char** argv) {
  try {
    check(argc==2,"Supply retained original Vista local-user fixture");
    std::ifstream input(argv[1]);check(bool(input),"Missing original Vista local-user fixture");
    unsigned getters{},allocations{},identities{},streams{},removals{},underflows{},projected{},direct_reads{};
    unsigned profile_streams{},update_streams{},source_allocations{},source_identity_writes{},source_payload_writes{};
    std::array<bool,256> direct_coverage{};
    bool version{};std::string line;
    while(std::getline(input,line)) {
      if(line=="# H2V_LOCAL_USER_ORIGINAL_V2")version=true;
      if(line.empty() || line.front()=='#')continue;
      std::istringstream row(line);std::vector<std::string> fields;std::string token;
      while(row>>token)fields.push_back(token);
      check(fields.size()>=2,"Truncated original local-user row");const auto& label=fields[1];
      const auto kind=fields[0];
      if(kind=="G") {
        check(fields.size()==6,label+": malformed controller getter row");
        const auto controller=word(fields[2],10);check(controller<4,"Invalid original controller input");
        std::array<std::byte,vista_controller_user_bank_size> bank;bank.fill(std::byte{0x6d});
        put(bank,controller*vista_controller_user_stride,word(fields[3]));
        put(bank,controller*vista_controller_user_stride+4,word(fields[4]));const auto before=bank;
        check(vista_controller_local_user(bank,controller)==word(fields[5]),label+": raw original EAX differs");
        check(bank==before,label+": controller query mutated bytes");++getters;
      } else if(kind=="A") {
        check(fields.size()==8,label+": malformed allocation row");
        const auto mask=word(fields[2],10);check(mask<16,"Invalid original occupancy mask");
        const auto identity=bytes<8>(fields[3]);
        auto storage=bytes<5*vista_local_user_stride>(fields[4]);const auto before=storage;
        const auto expected=bytes<5*vista_local_user_stride>(fields[5]);const auto slot=word(fields[6]);
        auto bank=std::span(storage).subspan(vista_local_user_stride);
        if(word(fields[7],10)) {
          check(slot==0xffffffff && mask==15,label+": missing original unsafe underflow");
          check(expected!=before && std::equal(expected.begin()+vista_local_user_stride,expected.end(),
              before.begin()+vista_local_user_stride),label+": original did not retain the recorded preceding-bank write");
          rejects([&]{(void)vista_local_user_allocate(bank,identity);},label+": original unsafe full bank");
          compare(storage,before,label+": bounded native rejection must be atomic");++underflows;
        } else {
          check(vista_local_user_allocate(bank,identity)==slot,label+": original allocated index differs");
          compare(storage,expected,label+": complete original allocation before/after");
        }
        ++allocations;
      } else if(kind=="I") {
        check(fields.size()==9,label+": malformed identity row");const auto slot=word(fields[2],10);
        check(word(fields[3],10)<2,"Invalid retained active input");
        UserBank bank;bank.fill(std::byte{0x6d});const auto before_record=bytes<vista_local_user_stride>(fields[4]);
        std::copy(before_record.begin(),before_record.end(),selected(bank,slot).begin());
        const auto before=bank;VistaLocalUserIdentity output;output.fill(std::byte{0xa5});
        check(vista_local_user_read_identity(bank,slot,output)==(word(fields[8],10)!=0),label+": original AL differs");
        compare(output,bytes<8>(fields[5]),label+": original identity output/preservation");
        check(bank==before,label+": identity read mutated bytes");
        vista_local_user_write_identity(bank,slot,bytes<8>(fields[6]));auto expected=before;
        const auto after_record=bytes<vista_local_user_stride>(fields[7]);
        std::copy(after_record.begin(),after_record.end(),selected(expected,slot).begin());
        compare(bank,expected,label+": complete original identity write before/after");
        projected+=projected_payload_reads(bank,slot,label);++identities;
      } else if(kind=="S") {
        check(fields.size()>=8,label+": malformed retained leaf stream");
        check(word(fields[2],10)<4,"Invalid retained controller input");
        const auto slot=word(fields[3],10);auto bank=bytes<vista_local_user_bank_size>(fields[4]);const auto before=bank;
        const auto calls=word(fields[5],10);check(fields.size()==7+calls,label+": source leaf count differs");
        for(unsigned i=0;i<calls;++i) {
          const auto call=split(fields[6+i],',');check(!call.empty(),"Empty retained source call");
          if(call[0]=="a") {
            check(call.size()==2,"Malformed retained allocation call");
            check(vista_local_user_allocate(bank,bytes<8>(call[1]))==slot,label+": allocated slot differs");++source_allocations;
          } else if(call[0]=="i") {
            check(call.size()==3,"Malformed retained identity-write call");
            vista_local_user_write_identity(bank,word(call[1],10),bytes<8>(call[2]));++source_identity_writes;
          } else if(call[0]=="p") {
            check(call.size()==6,"Malformed retained payload-write call");
            vista_local_user_write_payload(bank,word(call[1],10),word(call[2]),bytes<0x84>(call[3]),
                word(call[4]),word(call[5]));++source_payload_writes;
          } else throw std::runtime_error("Unknown retained source leaf call");
        }
        auto expected=before;const auto after_record=bytes<vista_local_user_stride>(fields.back());
        std::copy(after_record.begin(),after_record.end(),selected(expected,slot).begin());
        compare(bank,expected,label+": retained original selected bank/other-bank preservation");
        projected+=projected_payload_reads(bank,slot,label);++streams;
        profile_streams+=label.starts_with("profile_leaf_stream_");
        update_streams+=label.starts_with("identity_update_leaf_stream_");
      } else if(kind=="R") {
        check(fields.size()==5,label+": malformed retained removal row");const auto slot=word(fields[2],10);
        UserBank bank;bank.fill(std::byte{0x6d});const auto before_record=bytes<vista_local_user_stride>(fields[3]);
        std::copy(before_record.begin(),before_record.end(),selected(bank,slot).begin());const auto before=bank;
        vista_local_user_remove(bank,slot);auto expected=before;
        const auto after_record=bytes<vista_local_user_stride>(fields[4]);
        std::copy(after_record.begin(),after_record.end(),selected(expected,slot).begin());
        compare(bank,expected,label+": original complete B8 removal assertion");++removals;
      } else if(kind=="D") {
        check(fields.size()==11,label+": malformed direct original payload-read row");
        const auto slot=word(fields[2],10),active=word(fields[3]),presence=word(fields[4]);
        check(slot<4 && presence<16,"Invalid direct original read input");
        const std::array<std::uint32_t,4> active_values{0,1,0x80,0xff};
        const auto position=std::find(active_values.begin(),active_values.end(),active);
        check(position!=active_values.end(),"Unknown direct original active-byte control");
        const auto coverage=slot*64+unsigned(position-active_values.begin())*16+presence;
        check(!direct_coverage[coverage],"Duplicated direct original payload-read control");direct_coverage[coverage]=true;
        auto bank=bytes<vista_local_user_bank_size>(fields[5]);const auto original_after=bytes<vista_local_user_bank_size>(fields[6]);
        check(bank[slot*vista_local_user_stride]==std::byte(active),"Direct original active input differs from raw source bank");
        auto output=bytes<0xd0>(fields[7]);const auto expected_output=bytes<0xd0>(fields[8]);
        std::uint32_t controller=get(output,0),first=get(output,0xb0),second=get(output,0xc0);
        VistaLocalUserPayload payload;std::copy_n(output.begin()+0x20,payload.size(),payload.begin());
        const VistaLocalUserPayloadOutputs outputs{presence&1 ? &controller : nullptr,presence&2 ? &payload : nullptr,
          presence&4 ? &first : nullptr,presence&8 ? &second : nullptr};
        const auto original_al=word(fields[9],10),original_eax=word(fields[10]);
        check(original_al<2 && (original_eax&0xff)==original_al,"Original AL/EAX fixture mismatch");
        check(vista_local_user_read_payload(bank,slot,outputs)==(original_al!=0),label+": direct original AL differs");
        put(output,0,controller);std::copy(payload.begin(),payload.end(),output.begin()+0x20);
        put(output,0xb0,first);put(output,0xc0,second);
        compare(output,expected_output,label+": direct original complete output bank");
        compare(bank,original_after,label+": direct original complete source bank");++direct_reads;
      } else throw std::runtime_error("Unknown retained Vista local-user row");
    }
    check(input.eof() && version && getters==16 && allocations==16 && identities==8 && streams==32 &&
        removals==4 && underflows==1 && profile_streams==24 && update_streams==8 &&
        source_allocations==16 && source_identity_writes==8 && source_payload_writes==28 && projected==640 && direct_reads==256 &&
        std::all_of(direct_coverage.begin(),direct_coverage.end(),[](bool present){return present;}),
        "Retained original Vista local-user fixture lacks required coverage");
    bounded_controls();
    std::cout<<"75 admitted retained original control projections and "<<direct_reads
        <<" direct original payload-read controls matched; "<<projected<<" separate field projections passed. "
        <<"Unsafe original full-bank underflow rejected atomically; no controller/profile/session activation.\n";
    return 0;
  }catch(const std::exception& error){std::cerr<<error.what()<<'\n';return 1;}
}
