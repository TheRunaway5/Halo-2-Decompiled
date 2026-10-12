// Research-only frame capture: forwards every intercepted Vulkan call unchanged.
// The request file is consumed at a submit; capture ends after the requested ms.
#define _GNU_SOURCE
#include <dlfcn.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <unistd.h>
#include <vulkan/vulkan.h>
#include "renderdoc_app.h"
static void *(*real_dlsym)(void *,const char *);
static PFN_vkGetInstanceProcAddr real_gipa;
static PFN_vkGetDeviceProcAddr real_gdpa;
static PFN_vkCreateInstance real_create;
static PFN_vkQueueSubmit real_submit;
static VkInstance captured_instance;
static RENDERDOC_API_1_6_0 *rdoc;
static double until;
static double now(void) {struct timespec t;clock_gettime(CLOCK_MONOTONIC,&t);return t.tv_sec+t.tv_nsec/1e9;}
static void setup(void) {
 if(!real_dlsym)real_dlsym=dlvsym(RTLD_NEXT,"dlsym","GLIBC_2.2.5");
 if(!rdoc) {
  void *lib=dlopen(getenv("HALO2_RENDERDOC_LIBRARY"),RTLD_NOW|RTLD_LOCAL);
  if(lib) {
   pRENDERDOC_GetAPI get=real_dlsym(lib,"RENDERDOC_GetAPI");
   if(get && get(eRENDERDOC_API_Version_1_6_0,(void **)&rdoc)) {
    rdoc->SetCaptureFilePathTemplate(getenv("HALO2_CAPTURE_PREFIX"));
    fprintf(stderr,"research capture: RenderDoc initialized\n");
   }
  }
 }
}
static VkResult VKAPI_CALL create_instance(const VkInstanceCreateInfo *i,const VkAllocationCallbacks *a,VkInstance *v) {
 VkResult r=real_create(i,a,v);if(r==VK_SUCCESS){captured_instance=*v;fprintf(stderr,"research capture: Vulkan instance %p\n",(void*)*v);}return r;
}
static VkResult VKAPI_CALL queue_submit(VkQueue q,uint32_t count,const VkSubmitInfo *s,VkFence f) {
 const char *request=getenv("HALO2_CAPTURE_REQUEST");
 if(rdoc && captured_instance) {
  if(until>0 && now()>=until) {
   uint32_t ok=rdoc->EndFrameCapture(RENDERDOC_DEVICEPOINTER_FROM_VKINSTANCE(captured_instance),0);
   fprintf(stderr,"research capture: ended (%u)\n",ok);until=0;
  }
  if(until==0 && request && access(request,F_OK)==0) {
   FILE *in=fopen(request,"r");unsigned ms=100;if(in){fscanf(in,"%u",&ms);fclose(in);}unlink(request);
   rdoc->StartFrameCapture(RENDERDOC_DEVICEPOINTER_FROM_VKINSTANCE(captured_instance),0);
   if(rdoc->IsFrameCapturing()){until=now()+ms/1000.0;fprintf(stderr,"research capture: started %u ms\n",ms);}
   else fprintf(stderr,"research capture: start failed\n");
  }
 }
 return real_submit(q,count,s,f);
}
static PFN_vkVoidFunction VKAPI_CALL get_device(VkDevice d,const char *n) {
 PFN_vkVoidFunction f=real_gdpa(d,n);
 if(strcmp(n,"vkQueueSubmit")==0){real_submit=(PFN_vkQueueSubmit)f;return (PFN_vkVoidFunction)queue_submit;}return f;
}
static PFN_vkVoidFunction VKAPI_CALL get_instance(VkInstance i,const char *n) {
 PFN_vkVoidFunction f=real_gipa(i,n);
 if(strcmp(n,"vkQueueSubmit")==0){real_submit=(PFN_vkQueueSubmit)f;fprintf(stderr,"research capture: hooked submit\n");return (PFN_vkVoidFunction)queue_submit;}
 if(strcmp(n,"vkCreateInstance")==0){real_create=(PFN_vkCreateInstance)f;return (PFN_vkVoidFunction)create_instance;}
 if(strcmp(n,"vkGetDeviceProcAddr")==0){real_gdpa=(PFN_vkGetDeviceProcAddr)f;return (PFN_vkVoidFunction)get_device;}return f;
}
void *dlsym(void *h,const char *n) {
 if(!real_dlsym)real_dlsym=dlvsym(RTLD_NEXT,"dlsym","GLIBC_2.2.5");
 void *r=real_dlsym(h,n);
 if(strcmp(n,"vkGetInstanceProcAddr")==0){real_gipa=(PFN_vkGetInstanceProcAddr)r;setup();fprintf(stderr,"research capture: hooked Vulkan loader\n");return get_instance;}return r;
}
