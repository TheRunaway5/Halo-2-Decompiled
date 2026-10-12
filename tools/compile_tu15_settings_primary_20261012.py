#!/usr/bin/env python3
"""Compile pinned complete EEPROM query expressions for an independent oracle.

The immutable single-thread oracle resource uses an actual i386 atomic lock and
declared freestanding byte memcpy/memset primitives. This is primary expression
evidence, not original Xbox kernel instructions, clobbers or synchronization.
"""
import argparse,hashlib,json,re,subprocess
from pathlib import Path
from oracle_tu15_startup_records_20261011 import ROOT
from oracle_source_process_shutdown import body
def sha(p):return hashlib.sha256(Path(p).read_bytes()).hexdigest()
def run(argv):
 result=subprocess.run(argv,capture_output=True,text=True)
 if result.returncode:raise RuntimeError(result.stdout+result.stderr)
 return result.stdout
def compile_primary(out):
 out.mkdir(parents=True,exist_ok=False)
 base=ROOT/'analysis/multiplayer/map-pack/smg-audio-host-contract/reference'
 types=ROOT/'analysis/multiplayer/integration/spawn-source/kernel-sync-world/Cxbx-Reloaded-types.h'
 pins={base/'EmuKrnlEx.cpp':'27840c9fcdc936012ce851010e407e00743fe7d5a0d9a017bfcf9a7f18c61597',
  base/'EmuEEPROM.cpp':'d1d316d124627f28e099fc228dee0c8a106cddc510c722c08dfc7d23f2c5b2cb',
  base/'EmuEEPROM.h':'a26d28ac250875c8b9e39352f160624b0ed90f41e2c55fa305db6388221d81e3',
  types:'7223ea0a9fb7e31e804008d5b0b1b3167908ab9c16d43fc2c8a67375c0f58f88'}
 for path,pin in pins.items():assert sha(path)==pin
 ex,cpp,header,t=[p.read_text() for p in pins]
 declarations=[]
 for name in ('XC_VALUE_INDEX','XBOX_ENCRYPTED_SETTINGS','XBOX_FACTORY_SETTINGS','XBOX_TIMEZONE_DATE','XBOX_USER_SETTINGS','XBOX_EEPROM'):
  declarations.append(re.search(r'typedef (?:struct|enum) _'+name+r'\s*\{.*?\}\s*'+name+r'[^;]*;',t,re.S).group())
 info=header[header.index('typedef struct EEPROMInfo'):header.index('// ******************************************************************',header.index('static const EEPROMInfo EEPROMInfos'))]
 info=info.replace('xbox::','').replace('offsetof(','__builtin_offsetof(')
 functions=[]
 for name,signature in (('section_does_not_require_checksum','static bool section_does_not_require_checksum(XC_VALUE_INDEX index)'),
  ('eeprom_section_checksum','static unsigned int eeprom_section_checksum(const unsigned int *section_data,unsigned int section_data_length)'),
  ('eeprom_data_is_valid','static bool eeprom_data_is_valid(XC_VALUE_INDEX index)')):
  functions.append(signature+body(ex,name+'(').replace('xbox::',''))
 query=body(ex,'xbox::ExQueryNonVolatileSetting\n').replace('xbox::','')
 query,n=re.subn(r'LOG_FUNC_BEGIN.*?LOG_FUNC_END;','',query,flags=re.S);assert n==1
 ntstatus=Path('/usr/i686-w64-mingw32/include/ntstatus.h')
 device=re.search(r'#define STATUS_DEVICE_DATA_ERROR \(\(NTSTATUS\)(0x[0-9A-F]+)\)',ntstatus.read_text()).group(1)
 constants='\n'.join(next(s for s in t.splitlines() if s.startswith('#define '+name+' ')) for name in ('X_STATUS_SUCCESS','X_STATUS_BUFFER_TOO_SMALL','X_STATUS_OBJECT_NAME_NOT_FOUND'))
 source='''typedef unsigned char uchar_xt;typedef char char_xt;typedef int long_xt,int_xt;
typedef unsigned int ulong_xt,dword_xt,ULONG,NTSTATUS,size_xt,uint32_t;typedef unsigned long long uint64_t;
typedef void *PVOID;typedef unsigned char *PBYTE;
#define XBOX_KEY_LENGTH 16
#define TIME_ZONE_NAME_LENGTH 4
#define EmuLog(...)
#define RETURN(x) return (x)
#define NULL 0
'''+constants+'\n#define STATUS_DEVICE_DATA_ERROR '+device+'u\n'+'\n'.join(declarations)+'''
inline constexpr int reg_binary=3,reg_dword=4;
'''+info+'''
extern "C" {
XBOX_EEPROM EEPROMBacking;
XBOX_EEPROM *EEPROM=&EEPROMBacking;
unsigned int XboxFactoryGameRegion;
volatile unsigned int query_lock;
void LockEeprom(){while(__sync_lock_test_and_set(&query_lock,1)){};}
void UnlockEeprom(){__sync_lock_release(&query_lock);}
void *memset(void *p,int value,unsigned int n){auto q=(unsigned char*)p;for(unsigned i=0;i<n;++i)q[i]=(unsigned char)value;return p;}
void *memcpy(void *p,const void *s,unsigned int n){auto q=(unsigned char*)p;auto r=(const unsigned char*)s;for(unsigned i=0;i<n;++i)q[i]=r[i];return p;}
}
const EEPROMInfo *EmuFindEEPROMInfo(XC_VALUE_INDEX index)
'''+body(cpp,'EmuFindEEPROMInfo(').replace('xbox::','')+'\n'+'\n'.join(functions)+'''
static_assert(sizeof(XBOX_EEPROM)==256);
static_assert(__builtin_offsetof(XBOX_EEPROM,UserSettings)==96 && sizeof(XBOX_USER_SETTINGS)==96);
static_assert(__builtin_offsetof(XBOX_USER_SETTINGS,MiscFlags)==88);
extern "C" __attribute__((stdcall,noinline,used)) unsigned int primary_query(
    dword_xt ValueIndex,dword_xt *Type,PVOID Value,size_xt ValueLength,size_xt *ResultLength)
'''+query+'\n'
 (out/'query.cpp').write_text(source)
 run(['c++','-m32','-std=c++20','-O0','-mno-sse','-mno-mmx','-msoft-float','-ffreestanding','-fno-builtin','-fno-exceptions',
  '-fno-rtti','-fno-pie','-fno-pic','-fno-stack-protector','-fno-asynchronous-unwind-tables','-c',str(out/'query.cpp'),'-o',str(out/'query.o')])
 bases={'.text':0x9400000,'.rodata':0x9410000,'.data':0x9420000,'.bss':0x9430000}
 run(['ld','-m','elf_i386',*[f'--section-start={k}={hex(v)}' for k,v in bases.items()],'-e','primary_query',str(out/'query.o'),'-o',str(out/'query.elf')])
 sections=run(['objdump','-h',str(out/'query.elf')]);banks=[]
 for name,address in bases.items():
  line=next(s for s in sections.splitlines() if re.match(r'\s*\d+ '+re.escape(name)+r'\s',s))
  size=int(line.split()[2],16);assert size and int(line.split()[3],16)==address
  path=out/(name[1:]+'.bin')
  if name=='.bss':path.write_bytes(bytes(size))
  else:run(['objcopy','-O','binary','--only-section='+name,str(out/'query.elf'),str(path)])
  assert path.stat().st_size==size;banks.append(dict(address=address,size=size,file=path.name,sha256=sha(path)))
 symbols=run(['nm','-n',str(out/'query.elf')]).splitlines()
 symbols={name:int(next(s.split()[0] for s in symbols if s.endswith(' '+name)),16) for name in ('primary_query','EEPROMBacking','query_lock')}
 report=dict(schema=1,primary_sources={str(p.relative_to(ROOT)):pin for p,pin in pins.items()},
  compiler_tool_sha256=sha(__file__),generated_source_sha256=sha(out/'query.cpp'),elf_sha256=sha(out/'query.elf'),
  status_header_sha256=sha(ntstatus),device_data_error=int(device,16),entries=symbols,banks=banks,
  eeprom_bytes=256,user_offset=96,user_bytes=96,misc_offset=88,query_body=query,
  original_kernel_clobber_proof=False,lock_contract='actual atomic i386 lock over one immutable oracle snapshot',
  byte_primitives='declared freestanding byte loops, independently compiled; no Xbox store-width claim')
 (out/'manifest.json').write_text(json.dumps(report,sort_keys=True,indent=2)+'\n');return report
if __name__=='__main__':
 ap=argparse.ArgumentParser(description=__doc__);ap.add_argument('--output',type=Path,required=True);a=ap.parse_args()
 print(json.dumps(compile_primary(a.output)))
