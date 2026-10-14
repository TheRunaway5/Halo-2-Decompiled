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
uint16_t guest_selector,host_selector,guest_cw=0x27f;
uint32_t guest_mxcsr=0x1f80;
unsigned char guest_exit[180],guest_environment[28];
_Alignas(16) unsigned char guest_host_fp[512];
extern void guest_run(void),guest_return(void);
void (*guest_return_target)(void)=guest_return;
struct region {uint32_t address,size;};
static void fail(const char *text){perror(text);exit(1);}
static void fault(int signal,siginfo_t *info,void *context){ucontext_t *u=context;char text[200];int n=snprintf(text,sizeof(text),"Original hardware fault signal=%d address=%p eip=%08x esp=%08x eax=%08x ecx=%08x\n",signal,info->si_addr,u->uc_mcontext.gregs[REG_EIP],u->uc_mcontext.gregs[REG_ESP],u->uc_mcontext.gregs[REG_EAX],u->uc_mcontext.gregs[REG_ECX]);write(2,text,n);_exit(1);}
int main(int argc,char **argv){
 if(argc!=4){fprintf(stderr,"Supply CASE ENTRY_HEX OUTPUT\n");return 1;}
 struct sigaction action={0};action.sa_sigaction=fault;action.sa_flags=SA_SIGINFO;sigaction(SIGSEGV,&action,0);sigaction(SIGILL,&action,0);sigaction(SIGBUS,&action,0);
 char path[4096];snprintf(path,sizeof(path),"%s/before.bin",argv[1]);FILE *input=fopen(path,"rb");if(!input)fail("before bank");
 unsigned char *mapped=calloc(1,1u<<20);struct region rows[4096];unsigned count=0;
 while(fread(&rows[count],8,1,input)==1){struct region r=rows[count++];if(count>=4096 || !r.size || r.address+r.size<r.address)fail("invalid source region");
  uint32_t page=r.address&~4095u,last=(r.address+r.size-1)&~4095u;
  for(;;){if(!mapped[page>>12]){void *p=mmap((void*)(uintptr_t)page,4096,PROT_READ|PROT_WRITE|PROT_EXEC,MAP_PRIVATE|MAP_ANONYMOUS|MAP_FIXED_NOREPLACE,-1,0);if(p==MAP_FAILED){fprintf(stderr,"mapping %08x failed\n",page);fail("source page");}mapped[page>>12]=1;}if(page==last)break;page+=4096;}
  if(fread((void*)(uintptr_t)r.address,1,r.size,input)!=r.size)fail("short source region");
 }
 fclose(input);free(mapped);
 snprintf(path,sizeof(path),"%s/native-input.bin",argv[1]);input=fopen(path,"rb");if(!input || fread(guest_input,1,168,input)!=168)fail("native caller ABI");fclose(input);
 if(guest_input[4]!=0x600f0000u || guest_input[41]!=0x60200000u || *(uint32_t*)(uintptr_t)guest_input[4]!=0x60300000u){fprintf(stderr,"Unexpected declared native caller boundary\n");return 1;}
 if(mmap((void*)0x60300000u,4096,PROT_READ|PROT_WRITE|PROT_EXEC,MAP_PRIVATE|MAP_ANONYMOUS|MAP_FIXED_NOREPLACE,-1,0)==MAP_FAILED)fail("caller return page");
 unsigned char *stop=(unsigned char*)0x60300000u;stop[0]=0xff;stop[1]=0x25;*(uint32_t*)(stop+2)=(uint32_t)(uintptr_t)&guest_return_target;
 struct user_desc descriptor={0};descriptor.entry_number=(uint32_t)-1;descriptor.base_addr=guest_input[41];descriptor.limit=0xfffff;descriptor.seg_32bit=1;descriptor.limit_in_pages=1;descriptor.useable=1;
 if(syscall(SYS_set_thread_area,&descriptor))fail("actual FS descriptor");guest_selector=(descriptor.entry_number<<3)|3;
 guest_entry=strtoul(argv[2],0,16);guest_run();
 uint16_t tags=*(uint16_t*)(guest_environment+8);uint32_t depth=0;for(unsigned n=0;n<8;++n)depth+=((tags>>(n*2))&3)!=3;memcpy(guest_exit+168,&depth,4);
 snprintf(path,sizeof(path),"%s/hardware-exit.bin",argv[3]);FILE *out=fopen(path,"wb");if(!out || fwrite(guest_exit,1,180,out)!=180)fail("hardware exit");fclose(out);
 snprintf(path,sizeof(path),"%s/hardware-after.bin",argv[3]);out=fopen(path,"wb");if(!out)fail("hardware bank");for(unsigned n=0;n<count;++n)if(fwrite(&rows[n],8,1,out)!=1 || fwrite((void*)(uintptr_t)rows[n].address,1,rows[n].size,out)!=rows[n].size)fail("hardware source bank");fclose(out);
 printf("Original hardware return=%08x x87_status=%04x control=%04x mxcsr=%08x depth=%u regions=%u\n",*(uint32_t*)guest_exit,*(uint16_t*)(guest_exit+172),*(uint16_t*)(guest_exit+174),*(uint32_t*)(guest_exit+176),depth,count);return 0;
}
