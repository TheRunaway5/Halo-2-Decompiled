// Research-only RenderDoc control. No guest or graphics call is replaced.
#define _GNU_SOURCE
#include <dlfcn.h>
#include <pthread.h>
#include <stdatomic.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#ifdef __linux__
#include <sys/prctl.h>
#endif
#include "renderdoc_app.h"
static RENDERDOC_API_1_6_0 *rdoc;
static pthread_t worker;
static bool started;
static atomic_bool stopping;
static void *captured_context;
// Called by the parent host debugger on the observed original rendering
// thread while the guest is paused. No guest or GL call is replaced.
void *halo2_gl_capture_current_context(void) {
 const char *library=getenv("HALO2_RENDERDOC_LIBRARY");
 void *lib=library?dlopen(library,RTLD_NOW|RTLD_LOCAL):NULL;
 void *(*get_context)(void)=lib?dlsym(lib,"glXGetCurrentContext"):NULL;
 return get_context?get_context():NULL;
}
unsigned halo2_gl_capture_start_current_context(void) {
 if(!rdoc)return 0;
 captured_context=halo2_gl_capture_current_context();
 if(!captured_context)return 0;
 rdoc->StartFrameCapture(captured_context,NULL);
 unsigned active=rdoc->IsFrameCapturing();
 fprintf(stderr,"research GL capture: debugger start context %p (%u)\n",captured_context,active);
 return active;
}
unsigned halo2_gl_capture_end_observed_context(void) {
 if(!rdoc || !captured_context)return 0;
 unsigned ok=rdoc->EndFrameCapture(captured_context,NULL);
 fprintf(stderr,"research GL capture: debugger end context %p (%u)\n",captured_context,ok);
 captured_context=NULL;
 return ok;
}
static void *requests(void *unused) {
 (void)unused;
 const char *path=getenv("HALO2_GL_CAPTURE_REQUEST");
 while(!atomic_load(&stopping)) {
  if(path && access(path,F_OK)==0) {
   unsigned count=1;FILE *f=fopen(path,"r");
   if(f){if(fscanf(f,"%u",&count)!=1)count=1;fclose(f);}unlink(path);
   if(count<1 || count>3)count=1;
   rdoc->TriggerMultiFrameCapture(count);
   fprintf(stderr,"research GL capture: requested %u presented frames\n",count);
  }
  usleep(20000);
 }
 return NULL;
}
__attribute__((constructor)) static void start(void) {
#ifdef __linux__
 // Opt-in permission for a debugger of this private research process only.
 // No system ptrace policy or guest instruction is changed.
 if(getenv("HALO2_CAPTURE_ALLOW_DEBUGGER"))
  prctl(PR_SET_PTRACER,PR_SET_PTRACER_ANY,0,0,0);
#endif
 const char *library=getenv("HALO2_RENDERDOC_LIBRARY");
 if(!library)return;
 void *lib=dlopen(library,RTLD_NOW|RTLD_LOCAL);
 if(!lib)return;
 pRENDERDOC_GetAPI get=(pRENDERDOC_GetAPI)dlsym(lib,"RENDERDOC_GetAPI");
 if(!get || !get(eRENDERDOC_API_Version_1_6_0,(void **)&rdoc))return;
 const char *prefix=getenv("HALO2_CAPTURE_PREFIX");
 if(prefix)rdoc->SetCaptureFilePathTemplate(prefix);
 rdoc->MaskOverlayBits(0,0);
 if(pthread_create(&worker,NULL,requests,NULL)==0)started=true;
 fprintf(stderr,"research GL capture: control initialized\n");
}
__attribute__((destructor)) static void stop(void) {
 atomic_store(&stopping,true);
 if(started)pthread_join(worker,NULL);
}
