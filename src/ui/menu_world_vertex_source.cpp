// Independently implemented within this MIT project. The instruction fields
// and arithmetic originate in src/rendering/vertex_program.cpp and shaders/vertex_program.glsl.
// The separately retained original-source experiments verify behavior; their
// GPL/LGPL generator tables, initializer and implementation are not imported.
#include "halo2/menu_world_vertex_source.hpp"
#include <algorithm>
#include <array>
#include <sstream>

namespace halo2 {
namespace {
struct Operand {
  unsigned file{}, index{};
  std::array<unsigned,4> swizzle{};
  bool negative{};
  bool relative{};
};
Operand operand(const VertexInstruction &t,unsigned which,bool skinned=false,bool material_attributes=false) {
  Operand result;
  if(which==0) {
    result.file=t[2]>>26&3;result.index=t[2]>>28;
    result.negative=(t[1]>>8&1)!=0;
    result.swizzle={t[1]>>6&3,t[1]>>4&3,t[1]>>2&3,t[1]&3};
  } else if(which==1) {
    result.file=t[2]>>11&3;result.index=t[2]>>13&15;
    result.negative=(t[2]>>25&1)!=0;
    result.swizzle={t[2]>>23&3,t[2]>>21&3,t[2]>>19&3,t[2]>>17&3};
  } else {
    result.file=t[3]>>28&3;result.index=(t[2]&3)*4+(t[3]>>30);
    result.negative=(t[2]>>10&1)!=0;
    result.swizzle={t[2]>>8&3,t[2]>>6&3,t[2]>>4&3,t[2]>>2&3};
  }
  if(result.file==1) {
    if(result.index>12)throw UnsupportedVertexProgram("menu source temporary input exceeds register file");
  } else if(result.file==2) {
    result.index=t[1]>>9&15;
    if(result.index>(skinned?6u:4u) &&
       !(material_attributes && (result.index==10 || result.index==11)))
      throw UnsupportedVertexProgram("menu source vertex input is outside proved corridor");
  } else if(result.file==3) {
    result.index=t[1]>>13&255;
    if(result.index>=192)throw UnsupportedVertexProgram("menu source constant input exceeds c192");
    result.relative=(t[3]&2)!=0;
    if(result.relative && (!skinned || result.index<50 || result.index>52))
      throw UnsupportedVertexProgram("menu source relative input escapes its model palette corridor");
  } else throw UnsupportedVertexProgram("menu source operand register type is unsupported");
  return result;
}
std::string temporary(unsigned index) {
  return index==12?"vpOut[0]":"h2MenuTemp"+std::to_string(index);
}
std::string components(unsigned mask) {
  std::string result;
  for(unsigned i=0;i<4;++i)if(mask&(8u>>i))result+="xyzw"[i];
  return result;
}
void add(std::vector<unsigned> &values,unsigned value) {
  if(std::find(values.begin(),values.end(),value)==values.end())values.push_back(value);
}
std::string expression(const Operand &value,MenuWorldVertexSource &result) {
  std::string text;
  if(value.file==1)text=temporary(value.index);
  else if(value.file==2) {
    text="vpInput["+std::to_string(value.index)+"]";add(result.consumed_inputs,value.index);
  } else {
    if(value.relative) {
      text="vpConstant(uint("+std::to_string(value.index)+"+h2MenuAddress))";
      for(unsigned index=50;index<170;++index)add(result.consumed_constants,index);
    }else {text="vpConstant("+std::to_string(value.index)+"u)";add(result.consumed_constants,value.index);}
  }
  text+='.';
  for(const auto component:value.swizzle)text+="xyzw"[component];
  return value.negative?"-"+text:text;
}
const char *helper(unsigned operation) {
  switch(operation) {
  case 1:return "h2MenuMove";
  case 2:return "h2MenuMultiply";
  case 3:return "h2MenuAdd";
  case 4:return "h2MenuMultiplyAdd";
  case 5:return "h2MenuDot3";
  case 6:return "h2MenuHomogeneousDot";
  case 7:return "h2MenuDot4";
  case 17:return "h2MenuMove";
  case 19:return "h2MenuReciprocalClamp";
  case 20:return "h2MenuReciprocalSqrt";
  default:throw UnsupportedVertexProgram("menu source arithmetic operation is unsupported");
  }
}
const char helpers[]=R"GLSL(vec4 h2MenuMultiply(vec4 left,vec4 right) {
    vec4 product=left*right;
    return vec4((left.x==0.0 || right.x==0.0)?0.0:product.x,
                (left.y==0.0 || right.y==0.0)?0.0:product.y,
                (left.z==0.0 || right.z==0.0)?0.0:product.z,
                (left.w==0.0 || right.w==0.0)?0.0:product.w);
}
vec4 h2MenuAdd(vec4 left,vec4 right) {return left+right;}
vec4 h2MenuMultiplyAdd(vec4 left,vec4 right,vec4 addend) {
    return h2MenuMultiply(left,right)+addend;
}
vec4 h2MenuDot3(vec4 left,vec4 right) {return vec4(dot(left.xyz,right.xyz));}
vec4 h2MenuHomogeneousDot(vec4 left,vec4 right) {
    return vec4(dot(vec4(left.xyz,1.0),right));
}
vec4 h2MenuDot4(vec4 left,vec4 right) {return vec4(dot(left,right));}
vec4 h2MenuMove(vec4 value) {return value;}
vec4 h2MenuReciprocalClamp(vec4 value) {
    float reciprocal=1.0/value.x;
    float magnitude=clamp(abs(reciprocal),5.421010862427522e-20,1.8446744073709552e19);
    uint signedMagnitude=(floatBitsToUint(reciprocal)&0x80000000u)|(floatBitsToUint(magnitude)&0x7fffffffu);
    return vec4(uintBitsToFloat(signedMagnitude));
}
vec4 h2MenuReciprocalSqrt(vec4 value) {return vec4(inversesqrt(abs(value.x)));}
)GLSL";
void call(MenuWorldVertexSource &result,std::ostringstream &text,unsigned operation,
          const std::string &destination,unsigned mask,const std::vector<Operand> &inputs) {
  if(!mask)return;
  const auto axes=components(mask);
  text<<"    "<<destination<<'.'<<axes<<'='<<helper(operation)<<'(';
  for(unsigned i=0;i<inputs.size();++i) {
    if(i)text<<',';
    text<<expression(inputs[i],result);
  }
  text<<")."<<axes<<";\n";
  result.helper_operations.push_back(operation);
}
// Components affecting the masked result, before the operand's swizzle.
// Dot results broadcast; DPH substitutes 1 for the first operand's w.
unsigned read_lanes(unsigned operation,unsigned argument,unsigned mask) {
  if(!mask)return 0;
  if(operation==5)return 14;
  if(operation==6)return argument==0?14u:15u;
  if(operation==7)return 15;
  if(operation==19 || operation==20)return 8;
  return mask;
}
unsigned position_reads(unsigned operation,unsigned mask,const std::vector<Operand> &inputs) {
  unsigned result=0;
  for(unsigned argument=0;argument<inputs.size();++argument) {
    const auto &input=inputs[argument];
    if(input.file!=1 || input.index!=12)continue;
    const auto lanes=read_lanes(operation,argument,mask);
    for(unsigned axis=0;axis<4;++axis)
      if(lanes&(8u>>axis))result|=8u>>input.swizzle[axis];
  }
  return result;
}
}
namespace {
MenuWorldVertexSource emit_vertex_source(const VertexProgram &program,bool skinned,bool material_attributes=false) {
  if(program.count==0 || program.count>program.instructions.size())
    throw FormatError("menu source instruction count is invalid");
  if(program.relative_constants && !skinned)
    throw UnsupportedVertexProgram("menu source relative constants are unsupported");
  MenuWorldVertexSource result;result.helpers=helpers;
  std::ostringstream text;
  if(skinned)text<<"    int h2MenuAddress=0;\n";
  bool address_written=false;
  for(unsigned i=0;i<12;++i)text<<"    vec4 h2MenuTemp"<<i<<"=vec4(0.0);\n";
  for(unsigned i=0;i<16;++i)text<<"    vpOut["<<i<<"]=vec4(0.0,0.0,0.0,1.0);\n";
  for(unsigned i=0;i<program.count;++i) {
    const auto &t=program.instructions[i];
    if(bool(t[3]&1)!=(i+1==program.count))throw FormatError("menu source final instruction marker is invalid");
    if((t[3]&2) && (!skinned || !address_written))
      throw UnsupportedVertexProgram("menu source relative token has no original address producer");
    const auto mac=t[1]>>21&15,ilu=t[1]>>25&7;
    if(mac>7 && !(skinned && mac==13))throw UnsupportedVertexProgram("menu source MAC opcode is outside proved corridor");
    if(mac==13 && ilu && !(material_attributes && ilu==1))
      throw UnsupportedVertexProgram("menu source paired address load is unproved");
    if(ilu!=0 && ilu!=1 && ilu!=3 && ilu!=4)throw UnsupportedVertexProgram("menu source ILU opcode is outside proved corridor");
    if(mac && ilu && !((mac==1 && (ilu==1 || ilu==3)) ||
                        (mac==2 && (ilu==3 || ilu==4)) || (mac==5 && (ilu==1 || ilu==3 || ilu==4)) ||
                        (mac==6 && (ilu==3 || ilu==4)) ||
                        (mac==7 && ilu==1) ||
                        (material_attributes && ((mac==6 && ilu==1) || (mac==13 && ilu==1)))))
      throw UnsupportedVertexProgram("menu source paired opcode is outside proved corridor");
    const auto reg=t[3]>>20&15,mm=t[3]>>24&15,im=t[3]>>16&15,om=t[3]>>12&15;
    if((mm && mac==0) || (im && ilu==0))throw UnsupportedVertexProgram("menu source masked absent operation is unsupported");
    if((mm || im) && reg>12)throw UnsupportedVertexProgram("menu source destination exceeds register file");
    const auto output=t[3]>>3&255;
    if(om && (!(t[3]&2048) || !(output==0 || output==3 || output==4 ||
                              (output==5 && om==9) || (output>=9 && output<=12))))
      throw UnsupportedVertexProgram("menu source output destination is outside proved corridor");
    // Original oFog writes use the leading result components rather than
    // ordinary vector destinations: e.g. authored shadow mask xw becomes xy.
    // The epilogue consumes x. Keep the remaining lanes consistent with the
    // retained original emitter instead of treating this scalar as oD/oT.
    // Only the authored Apply xw corridor is admitted here. Other oFog masks
    // remain unproved rather than expanding unrelated program families.
    const unsigned output_mask=output==5?12u:om;
    const bool output_ilu=(t[3]&4)!=0;
    if(om && (output_ilu?ilu==0:mac==0))throw UnsupportedVertexProgram("menu source output selects absent operation");
    if(!mac && !ilu)continue;
    text<<"    // instruction "<<i<<"\n";
    if(mac==13) {
      if(mm || (om && !output_ilu))throw UnsupportedVertexProgram("menu source address load has an unproved ordinary write");
      const auto input=operand(t,0,true,material_attributes);
      if(!ilu) {
        text<<"    h2MenuAddress=int(floor(("<<expression(input,result)<<").x));\n";
        result.helper_operations.push_back(13);address_written=true;continue;
      }
      text<<"    int h2MenuPendingAddress"<<i<<"=int(floor(("<<expression(input,result)<<").x));\n";
      result.helper_operations.push_back(13);
      // Paired ILU reads the old address/temporaries. Commit A0 only after
      // its output, as the original deferred-address submission does.
      if(ilu) {
        const std::vector<Operand> scalar{operand(t,2,skinned,material_attributes)};
        if(om)call(result,text,16+ilu,"vpOut["+std::to_string(output)+"]",output_mask,scalar);
        if(im)call(result,text,16+ilu,temporary(1),im,scalar);
      }
      text<<"    h2MenuAddress=h2MenuPendingAddress"<<i<<";\n";
      address_written=true;continue;
    }
    std::vector<Operand> main_inputs;
    if(mac) {
      main_inputs.push_back(operand(t,0,skinned,material_attributes));
      if(mac==2 || mac==4 || mac==5 || mac==6 || mac==7)main_inputs.push_back(operand(t,1,skinned,material_attributes));
      if(mac==3 || mac==4)main_inputs.push_back(operand(t,2,skinned,material_attributes));
    }
    const auto mac_mask=(ilu && reg==1)?0u:mm;
    std::vector<Operand> scalar_inputs;
    if(ilu) {
      auto scalar=operand(t,2,skinned,material_attributes);
      if(ilu!=1)scalar.swizzle.fill(scalar.swizzle[0]);
      scalar_inputs.push_back(scalar);
    }
    // Only oPos aliases an operand register (R12). Repeated static calls may
    // observe its preceding write. This same-token corridor is not covered by
    // the retained title programs; reject it rather than choosing semantics.
    if(om && output==0) {
      if((!output_ilu && (om&position_reads(mac,mac_mask,main_inputs))) ||
         (ilu && (om&position_reads(16+ilu,im,scalar_inputs))) ||
         (mac && ilu && reg==12 && output_ilu && (om&mac_mask)))
        throw UnsupportedVertexProgram("menu source same-token output/input alias corridor is unproved");
    }
    if(om && !output_ilu)call(result,text,mac,"vpOut["+std::to_string(output)+"]",output_mask,main_inputs);
    if(mac && mac_mask) {
      if(ilu) {
        text<<"    vec4 h2MenuPending"<<i<<"=vec4(0.0);\n";
        call(result,text,mac,"h2MenuPending"+std::to_string(i),mac_mask,main_inputs);
      } else call(result,text,mac,temporary(reg),mac_mask,main_inputs);
    }
    if(ilu) {
      if(om && output_ilu)call(result,text,16+ilu,"vpOut["+std::to_string(output)+"]",output_mask,scalar_inputs);
      if(im)call(result,text,16+ilu,temporary(mac?1u:reg),im,scalar_inputs);
    }
    if(mac && ilu && mac_mask) {
      const auto axes=components(mac_mask);
      text<<"    "<<temporary(reg)<<'.'<<axes<<"=h2MenuPending"<<i<<'.'<<axes<<";\n";
    }
  }
  std::sort(result.consumed_inputs.begin(),result.consumed_inputs.end());
  std::sort(result.consumed_constants.begin(),result.consumed_constants.end());
  result.statements=text.str();return result;
}
}
MenuWorldVertexSource menu_world_vertex_source(const VertexProgram &program) {
  return emit_vertex_source(program,false);
}
MenuWorldVertexSource menu_world_skinned_vertex_source(const VertexProgram &program) {
  return emit_vertex_source(program,true);
}
MenuWorldVertexSource menu_world_material_vertex_source(const VertexProgram &program,bool skinned) {
  return emit_vertex_source(program,skinned,true);
}
std::array<std::uint8_t,16> menu_world_vertex_input_components(const VertexProgram &program,bool skinned) {
  (void)emit_vertex_source(program,skinned,true);
  std::array<std::uint8_t,16> result{};
  const auto read=[&](unsigned operation,unsigned mask,const std::vector<Operand>& inputs) {
    for(unsigned argument=0;argument<inputs.size();++argument) {
      const auto& input=inputs[argument];
      if(input.file!=2)continue;
      const auto lanes=read_lanes(operation,argument,mask);
      for(unsigned axis=0;axis<4;++axis)if(lanes&(8u>>axis))
        result[input.index]|=std::uint8_t(1u<<input.swizzle[axis]);
    }
  };
  for(unsigned i=0;i<program.count;++i) {
    const auto&t=program.instructions[i];
    const auto mac=t[1]>>21&15,ilu=t[1]>>25&7,reg=t[3]>>20&15;
    const auto mm=t[3]>>24&15,im=t[3]>>16&15,om=t[3]>>12&15;
    const auto output=t[3]>>3&255u;
    const auto output_mask=output==5?12u:om;
    const bool output_ilu=t[3]&4u;
    if(mac==13) {
      read(1,8,{operand(t,0,skinned,true)});
      if(ilu)read(16+ilu,im | (om?output_mask:0u),{operand(t,2,skinned,true)});
      continue;
    }
    if(mac) {
      std::vector<Operand> inputs{operand(t,0,skinned,true)};
      if(mac==2 || mac==4 || mac==5 || mac==6 || mac==7)inputs.push_back(operand(t,1,skinned,true));
      if(mac==3 || mac==4)inputs.push_back(operand(t,2,skinned,true));
      read(mac,((ilu && reg==1)?0u:mm) | ((om && !output_ilu)?output_mask:0u),inputs);
    }
    if(ilu) {
      auto input=operand(t,2,skinned,true);
      if(ilu!=1)input.swizzle.fill(input.swizzle[0]);
      read(16+ilu,im | ((om && output_ilu)?output_mask:0u),{input});
    }
  }
  return result;
}
std::uint32_t menu_world_vertex_default_texture_w_mask(const VertexProgram &program) {
  if(program.count==0 || program.count>program.instructions.size())
    throw FormatError("default texture W analysis requires a valid admitted program");
  std::uint32_t result=15;
  for(unsigned i=0;i<program.count;++i) {
    const auto token=program.instructions[i][3];
    const auto output=(token>>3)&255u;
    if((token&2048u) && (token&(1u<<12)) && output>=9 && output<=12)
      result&=~(1u<<(output-9));
  }
  return result;
}
} // namespace halo2
