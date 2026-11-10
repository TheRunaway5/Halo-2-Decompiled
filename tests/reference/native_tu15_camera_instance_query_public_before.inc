bool native_tu15_query_camera_instance(SourcePhysicsContactMemory&memory,
 const NativeTu15CameraInstanceQuery&q,NativeTu15CameraInstanceQueryScratch scratch,
 NativeTu15PhysicsFrameFp&fp,unsigned depth,const SourcePhysicsContactRecordObserver&observe,
 NativeTu15CameraInstanceQueryStats*stats){
 if(stats)++stats->calls;
#if (defined(__i386__) || defined(__x86_64__)) && !defined(_MSC_VER)
 Memory m(memory,observe);const auto scenario=m.word(0x4ecc18u),instances=m.word(scenario+0x144u),definitions=m.word(scenario+0x13cu);
 const auto instance=instances+q.instance*0x58u,definition=definitions+sign16(m.word(instance+0x34u,2))*0xc8u;
 if(stats)++stats->admissions;
 if(!native_tu15_admit_camera_query_instance(memory,q.instance,definition,q.mask)){if(stats)++stats->completed;return false;}
 const auto radius=m.word(instance+0x48u);if(stats)++stats->sphere_calls;
 if(!native_tu15_segment_near_sphere(memory,instance+0x3cu,q.origin,q.displacement,radius,fp)){if(stats)++stats->completed;return false;}
 if(stats)++stats->inverse_calls;native_tu15_camera_inverse_frame(memory,instance,scratch.inverse,fp,observe);
 Environment environment(fp,depth);const auto inverse=scratch.inverse;
 const auto scale=scalar(m.word(inverse));const bool scale_origin=different_or_unordered(scale,scalar(m.word(0x46a0d0u)));
 auto ox=scalar(m.word(q.origin)),oy=scalar(m.word(q.origin+4u)),oz=scalar(m.word(q.origin+8u));
 if(scale_origin){ox=mul(scale,ox);oy=mul(scale,oy);oz=mul(scale,oz);}
 auto a28=scalar(m.word(inverse+28u)),a1c=scalar(m.word(inverse+16u)),a20=scalar(m.word(inverse+20u));
 a1c=mul(a1c,oy);const bool scale_displacement=different_or_unordered(scale,scalar(m.word(0x46a0d0u)));
 auto x=mul(a28,oz);x=add(x,a1c);auto term=scalar(m.word(inverse+4u));term=mul(term,ox);x=add(x,term);x=add(x,scalar(m.word(inverse+40u)));a20=mul(a20,oy);m.word(0x16c101u,scratch.local_origin,bits(x));
 const auto a2c=scalar(m.word(inverse+32u));auto y=mul(a2c,oz);y=add(y,a20);term=scalar(m.word(inverse+8u));term=mul(term,ox);y=add(y,term);y=add(y,scalar(m.word(inverse+44u)));m.word(0x16c12cu,scratch.local_origin+4u,bits(y));
 const auto a30=scalar(m.word(inverse+36u));auto z=mul(a30,oz);term=scalar(m.word(inverse+24u));term=mul(term,oy);auto first=scalar(m.word(inverse+12u));z=add(z,term);
 auto dz=scalar(m.word(q.displacement+8u));first=mul(first,ox);auto dx=scalar(m.word(q.displacement));z=add(z,first);auto dy=scalar(m.word(q.displacement+4u));z=add(z,scalar(m.word(inverse+48u)));m.word(0x16c173u,scratch.local_origin+8u,bits(z));
 if(scale_displacement){dx=mul(dx,scale);dy=mul(dy,scale);dz=mul(dz,scale);}
 auto delta_middle=scalar(m.word(inverse+16u));a28=mul(a28,dz);delta_middle=mul(delta_middle,dy);a28=add(a28,delta_middle);first=scalar(m.word(inverse+4u));first=mul(first,dx);a28=add(a28,first);m.word(0x16c1a7u,scratch.local_displacement,bits(a28));
 auto delta_y=scalar(m.word(inverse+20u));delta_y=mul(delta_y,dy);auto last_y=mul(a2c,dz);last_y=add(last_y,delta_y);first=scalar(m.word(inverse+8u));first=mul(first,dx);last_y=add(last_y,first);
 auto delta_z=scalar(m.word(inverse+24u));delta_z=mul(delta_z,dy);auto last_z=mul(a30,dz);last_z=add(last_z,delta_z);first=scalar(m.word(inverse+12u));first=mul(first,dx);last_z=add(last_z,first);
 m.word(0x16c1f9u,scratch.local_displacement+4u,bits(last_y));m.word(0x16c1ffu,scratch.local_displacement+8u,bits(last_z));
 const auto bitmap=native_tu15_camera_instance_collision_bitmap(memory,q.instance),fraction=m.word(q.result+4u);if(stats)++stats->bsp_calls;
 const auto found=environment.dependency([&]{return native_tu15_query_bsp_segment(memory,{q.bsp_filter,definition+0x70u,scratch.collision,fraction,8,bitmap,scratch.local_origin,scratch.local_displacement},scratch.bsp_context,fp,depth,observe);});
 if(!found){if(stats)++stats->completed;return false;}
 const auto material=m.word(scratch.collision+22u,2);m.word(0x16c234u,q.result,3u);
 std::uint32_t material_pointer;
 if(material!=0xffffu){const auto current=m.word(0x4ecc18u),materials=m.word(current+0x10u);material_pointer=materials+sign16(material)*20u+8u;}
 else material_pointer=0x48a0b0u;
 const auto selected=m.word(material_pointer,2);m.word(0x16c258u,q.result+0x24u,selected,2);
 m.word(0x16c25cu,q.result+0x38u,0xffffffffu);m.word(0x16c25fu,q.result+0x40u,0xffffffffu);m.word(0x16c26au,q.result+0x3cu,q.instance);
 environment.dependency([&]{native_tu15_publish_camera_collision(memory,q.result,instance,scratch.collision,fp,observe);return false;});
 if(stats){++stats->hits;++stats->completed;}return true;
#else
 (void)memory;(void)q;(void)scratch;(void)fp;(void)depth;(void)observe;throw std::runtime_error("Native camera instance query backend unavailable");
#endif
}
