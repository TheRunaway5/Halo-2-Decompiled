/* Host i386 replay with actual native weapon-resource and interface-locale input.
 * Original code bytes remain unchanged; the lower async queue is excluded.
 * Strict helper SHA256: b88808670072395d0bd855a1503f69f11b6d0ddae35f74e6ff2f252b97e3f7b8 */
#define _GNU_SOURCE
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <unistd.h>
#include <sys/mman.h>
#include <sys/syscall.h>
#include <asm/ldt.h>
#include <signal.h>
#include <ucontext.h>
uint32_t guest_input[42],guest_entry,guest_host_stack;
uint16_t guest_selector,host_selector,guest_cw=0x27f,guest_status;
uint32_t guest_mxcsr=0x1f80;
unsigned char guest_exit[180],guest_environment[28];
_Alignas(16) unsigned char guest_host_fp[512];
extern void guest_run(void),guest_return(void);
void (*guest_return_target)(void)=guest_return;
struct region {uint32_t address,size;};
static struct region rows[4096];
static unsigned region_count;
static int resource_request_fd,resource_reply_fd;
static unsigned resource_count;
static uint32_t resource_receipts[64][7];
static unsigned language_count;
static uint32_t language_receipts[64][3];
static unsigned char fault_stack[65536];
static void transfer(int fd,void *data,size_t bytes,int writing){
 unsigned char *p=data;
 while(bytes){ssize_t n=writing?write(fd,p,bytes):read(fd,p,bytes);if(n<0 && errno==EINTR)continue;if(n<=0)_exit(10);p+=n;bytes-=n;}
}
static void source_range(uint32_t address,uint32_t size){
 if(!size || address+size<address)_exit(11);
 for(unsigned n=0;n<region_count;++n)if(rows[n].address<=address &&
     (uint64_t)address+size<=(uint64_t)rows[n].address+rows[n].size)return;
 _exit(11);
}
static uint32_t source_word(uint32_t address){source_range(address,4);return *(uint32_t*)(uintptr_t)address;}
static void fail(const char *text){perror(text);exit(1);}
static void fault(int signal,siginfo_t *info,void *context){
 ucontext_t *u=context;
 if(signal==SIGSEGV && info->si_code==SEGV_ACCERR &&
    (uint32_t)(uintptr_t)info->si_addr==0x1a3b70u &&
    (uint32_t)u->uc_mcontext.gregs[REG_EIP]==0x1a3b70u && resource_count<64){
  const uint32_t sp=(uint32_t)u->uc_mcontext.gregs[REG_ESP];
  const uint32_t tracking=(uint32_t)u->uc_mcontext.gregs[REG_EDI];
  source_range(sp,28);
  const uint32_t returned=source_word(sp);
  const uint32_t size=source_word(sp+4),destination=source_word(sp+8);
  const uint32_t encoded=(uint32_t)u->uc_mcontext.gregs[REG_ECX];
  uint32_t descriptor,done,kind;
  if(source_word(sp+12))_exit(12);
  if(returned==0x12613eu){
   source_range(tracking,16);
   descriptor=source_word(tracking+12);source_range(descriptor,20);done=tracking+4;kind=1;
   if(source_word(sp+16)!=done || source_word(sp+20)!=6 || encoded!=source_word(descriptor+8))_exit(12);
  }else if(returned==0x130f6du){
   const uint32_t tr=(uint32_t)u->uc_mcontext.gregs[REG_ESI];descriptor=(uint32_t)u->uc_mcontext.gregs[REG_EBP];
   const uint32_t pool=source_word(0x4f2d5cu),first=source_word(pool+0x44u);
   const uint32_t cache=source_word(0x4f2d6cu),pages=source_word(cache+0x64u);
   const uint32_t token=source_word(descriptor+0x20u),index=token&65535u,page=source_word(pages+0x44u)+index*0x18u;
   const uint32_t expected=source_word(0x4f2d64u)+(source_word(page+8u)<<(source_word(cache+0x34u)&31u));
   done=tr+2;kind=4;
   if(tr<first || (tr-first)%12 || (tr-first)/12>=source_word(pool+0x38u) || source_word(tr+8u)!=descriptor ||
      token==0xffffffffu || index>=source_word(pages+0x38u) || destination!=expected || encoded!=source_word(descriptor) ||
      size!=((source_word(descriptor+4u)+511u)&~511u) || source_word(sp+16)!=done || source_word(sp+20)!=4u ||
      source_word(sp+24)!=(uint32_t)u->uc_mcontext.gregs[REG_EBX])_exit(12);
  }else if(returned==0x1301b5u){
   const uint32_t tr=(uint32_t)u->uc_mcontext.gregs[REG_ESI],level=source_word(tr+4u);
   descriptor=source_word(tr+0x10u);const uint32_t pool=source_word(0x4f2d24u),first=source_word(pool+0x44u);done=tr+3;kind=5;
   if(level>=3 || tr<first || (tr-first)%0x28u || (tr-first)/0x28u>=source_word(pool+0x38u) ||
      encoded!=source_word(descriptor+0x1cu+level*4u) || size!=((source_word(descriptor+0x34u+level*4u)+511u)&~511u) ||
      destination!=source_word(descriptor+0x40u+level*4u) || source_word(sp+16)!=done || source_word(sp+20)!=3u)_exit(12);
  }else _exit(12);
  source_range(destination,size);source_range(done,1);source_range(returned-5,5);
  if(*(unsigned char*)(uintptr_t)(returned-5)!=0xe8u || returned+source_word(returned-4)!=0x1a3b70u)_exit(12);
  uint32_t request[5]={kind,descriptor,destination,size,encoded};
  transfer(resource_request_fd,request,sizeof(request),1);
  uint32_t response[2];unsigned char authored[20];uint32_t payload_size;
  transfer(resource_reply_fd,response,sizeof(response),0);
  if(response[0])_exit(13);
  transfer(resource_reply_fd,authored,sizeof(authored),0);
  transfer(resource_reply_fd,&payload_size,4,0);
  if((kind==1 && (memcmp(authored,(void*)(uintptr_t)descriptor,12) || size!=((response[1]+511u)&~511u))) || payload_size!=size)_exit(14);
  transfer(resource_reply_fd,(void*)(uintptr_t)destination,size,0);
  request[0]=2;transfer(resource_request_fd,request,sizeof(request),1);
  transfer(resource_reply_fd,response,sizeof(response),0);
  if(response[0])_exit(15);
  const uint32_t job=response[1];
  *(unsigned char*)(uintptr_t)done=1;
  const uint32_t receipt[7]={descriptor,destination,size,encoded,done,job,returned};
  memcpy(resource_receipts[resource_count++],receipt,sizeof(receipt));
  // Linux sigreturn restores the interrupted FP/flags and all untouched GP.
  // This is the explicit native I/O ABI, not an Xbox kernel-clobber claim.
  u->uc_mcontext.gregs[REG_EAX]=job;
  u->uc_mcontext.gregs[REG_ESP]=sp+28;u->uc_mcontext.gregs[REG_EIP]=returned;return;
 }
 if(signal==SIGSEGV && info->si_code==SEGV_ACCERR &&
    (uint32_t)(uintptr_t)info->si_addr==0x33a4a6u &&
    (uint32_t)u->uc_mcontext.gregs[REG_EIP]==0x33a4a6u && language_count<64){
  const uint32_t sp=(uint32_t)u->uc_mcontext.gregs[REG_ESP],returned=source_word(sp),caller=returned-5;
  if(caller!=0x1a9da2u && caller!=0x1a82abu)_exit(16);
  source_range(caller,5);
  if(*(unsigned char*)(uintptr_t)caller!=0xe8u || returned+source_word(caller+1)!=0x33a4a6u)_exit(16);
  uint32_t request[5]={3,caller,returned,0,0},response[2];
  transfer(resource_request_fd,request,sizeof(request),1);transfer(resource_reply_fd,response,sizeof(response),0);
  if(response[0])_exit(17);
  language_receipts[language_count][0]=caller;language_receipts[language_count][1]=returned;
  language_receipts[language_count++][2]=response[1];
  u->uc_mcontext.gregs[REG_EAX]=response[1];u->uc_mcontext.gregs[REG_ESP]=sp+4;u->uc_mcontext.gregs[REG_EIP]=returned;return;
 }
 char text[200];int n=snprintf(text,sizeof(text),"Original hardware fault signal=%d address=%p eip=%08x esp=%08x eax=%08x ecx=%08x\n",signal,info->si_addr,u->uc_mcontext.gregs[REG_EIP],u->uc_mcontext.gregs[REG_ESP],u->uc_mcontext.gregs[REG_EAX],u->uc_mcontext.gregs[REG_ECX]);write(2,text,n);_exit(1);
}
int main(int argc,char **argv){
 if(argc!=7){fprintf(stderr,"Supply CASE ENTRY_HEX OUTPUT REQUEST_FD REPLY_FD BANK_FD\n");return 1;}
 resource_request_fd=atoi(argv[4]);resource_reply_fd=atoi(argv[5]);
 stack_t alternate={.ss_sp=fault_stack,.ss_size=sizeof(fault_stack)};
 if(sigaltstack(&alternate,0))fail("native I/O alternate signal stack");
 struct sigaction action={0};action.sa_sigaction=fault;action.sa_flags=SA_SIGINFO|SA_ONSTACK;sigaction(SIGSEGV,&action,0);sigaction(SIGILL,&action,0);sigaction(SIGBUS,&action,0);
 char path[4096];FILE *input=fdopen(dup(atoi(argv[6])),"rb");if(!input)fail("authenticated before bank fd");
 unsigned char *mapped=calloc(1,1u<<20);unsigned count=0;
 while(fread(&rows[count],8,1,input)==1){struct region r=rows[count++];if(count>=4096 || !r.size || r.address+r.size<r.address)fail("invalid source region");
  uint32_t page=r.address&~4095u,last=(r.address+r.size-1)&~4095u;
  for(;;){if(!mapped[page>>12]){void *p=mmap((void*)(uintptr_t)page,4096,PROT_READ|PROT_WRITE|PROT_EXEC,MAP_PRIVATE|MAP_ANONYMOUS|MAP_FIXED_NOREPLACE,-1,0);if(p==MAP_FAILED){fprintf(stderr,"mapping %08x failed\n",page);fail("source page");}mapped[page>>12]=1;}if(page==last)break;page+=4096;}
  if(fread((void*)(uintptr_t)r.address,1,r.size,input)!=r.size)fail("short source region");
 }
 fclose(input);free(mapped);region_count=count;
 snprintf(path,sizeof(path),"%s/native-input.bin",argv[1]);input=fopen(path,"rb");if(!input || fread(guest_input,1,168,input)!=168)fail("native caller ABI");fclose(input);
 snprintf(path,sizeof(path),"%s/native-fp-input.bin",argv[1]);input=fopen(path,"rb");int captured_fp=input!=0;
 if(input){unsigned char fp[12];if(fread(fp,1,12,input)!=12 || fgetc(input)!=EOF)fail("captured FP caller");fclose(input);uint32_t depth;memcpy(&depth,fp,4);memcpy(&guest_status,fp+4,2);memcpy(&guest_cw,fp+6,2);memcpy(&guest_mxcsr,fp+8,4);if(depth || (guest_cw&0x3f)!=0x3f || (guest_mxcsr&0x1f80)!=0x1f80){fprintf(stderr,"Unsupported captured FP caller\n");return 1;}}
 if((!captured_fp && guest_input[4]!=0x600f0000u) || guest_input[4]<0x60000000u || guest_input[4]>0x601ffffcu || guest_input[41]!=0x60200000u || *(uint32_t*)(uintptr_t)guest_input[4]!=0x60300000u){fprintf(stderr,"Unexpected declared native caller boundary\n");return 1;}
 if(mmap((void*)0x60300000u,4096,PROT_READ|PROT_WRITE|PROT_EXEC,MAP_PRIVATE|MAP_ANONYMOUS|MAP_FIXED_NOREPLACE,-1,0)==MAP_FAILED)fail("caller return page");
 unsigned char *stop=(unsigned char*)0x60300000u;stop[0]=0xff;stop[1]=0x25;*(uint32_t*)(stop+2)=(uint32_t)(uintptr_t)&guest_return_target;
 struct user_desc descriptor={0};descriptor.entry_number=(uint32_t)-1;descriptor.base_addr=guest_input[41];descriptor.limit=0xfffff;descriptor.seg_32bit=1;descriptor.limit_in_pages=1;descriptor.useable=1;
 if(syscall(SYS_set_thread_area,&descriptor))fail("actual FS descriptor");guest_selector=(descriptor.entry_number<<3)|3;
 guest_entry=strtoul(argv[2],0,16);
 if(guest_entry!=0xfdeb0u){fprintf(stderr,"Only the captured Needler update is supported\n");return 1;}
 // The unchanged lower asynchronous submission body is outside the declared
 // native contract. Deny execute only; reads and every text byte stay intact.
 if(mprotect((void*)0x1a3000u,4096,PROT_READ|PROT_WRITE))fail("native resource entry execute boundary");
 if(mprotect((void*)0x33a000u,4096,PROT_READ|PROT_WRITE))fail("native locale entry execute boundary");
 guest_run();close(resource_request_fd);close(resource_reply_fd);
 snprintf(path,sizeof(path),"%s/host-animation-reads.bin",argv[3]);FILE *resources=fopen(path,"wb");
 if(!resources || fwrite(resource_receipts,28,resource_count,resources)!=resource_count)fail("host animation read receipt");fclose(resources);
 snprintf(path,sizeof(path),"%s/host-interface-language.bin",argv[3]);resources=fopen(path,"wb");
 if(!resources || fwrite(language_receipts,12,language_count,resources)!=language_count)fail("host interface locale receipt");fclose(resources);
 uint16_t tags=*(uint16_t*)(guest_environment+8);uint32_t depth=0;for(unsigned n=0;n<8;++n)depth+=((tags>>(n*2))&3)!=3;memcpy(guest_exit+168,&depth,4);
 snprintf(path,sizeof(path),"%s/hardware-exit.bin",argv[3]);FILE *out=fopen(path,"wb");if(!out || fwrite(guest_exit,1,180,out)!=180)fail("hardware exit");fclose(out);
 snprintf(path,sizeof(path),"%s/hardware-after.bin",argv[3]);out=fopen(path,"wb");if(!out)fail("hardware bank");for(unsigned n=0;n<count;++n)if(fwrite(&rows[n],8,1,out)!=1 || fwrite((void*)(uintptr_t)rows[n].address,1,rows[n].size,out)!=rows[n].size)fail("hardware source bank");fclose(out);
 printf("Original hardware return=%08x x87_status=%04x control=%04x mxcsr=%08x depth=%u regions=%u\n",*(uint32_t*)guest_exit,*(uint16_t*)(guest_exit+172),*(uint16_t*)(guest_exit+174),*(uint32_t*)(guest_exit+176),depth,count);return 0;
}
