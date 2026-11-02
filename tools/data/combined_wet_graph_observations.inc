/* Offline observations only. The authenticated GP program is unchanged. */
static unsigned wet_entry(unsigned record){
    unsigned metadata=0x818+(ldl_le_p(active->title+0x804)+ldl_le_p(active->title+0x80c))*4;
    return 0x171+(ldl_le_p(active->title+metadata+8+record*32)-0x818)/4;
}
static void wet_words(unsigned at,unsigned n){
    for(unsigned j=0;j<n;j++)printf("%s%u",j?",":"",frame_peek(active->dsp,DSP_SPACE_X,at+j));
}
/* A portable numeric comparison digest, over little endian logical words.
 * SHA256 separately authenticates the original program and reference sources. */
static uint64_t wet_hash_word(uint64_t h,uint32_t w){
    for(unsigned b=0;b<4;b++){h^=(w>>(b*8))&255;h*=UINT64_C(1099511628211);}return h;
}
static void wet_workspace_hash(void){
    uint64_t h=UINT64_C(14695981039346656037);
    for(unsigned j=0;j<544;j++)h=wet_hash_word(h,frame_peek(active->dsp,DSP_SPACE_X,0x7fa+j));
    printf("\"%016"PRIx64"\"",h);
}
static void wet_reverb(unsigned record,const char* point){
    const unsigned base=record==8?0x22d:0x457;
    printf("{\"event\":\"combined_wet_reverb\",\"frame\":%u,\"record\":%u,\"point\":\"%s\",\"sr\":%u,\"modifiers\":[",
        active->frame_index,record,point,active->dsp->core.registers[DSP_REG_SR]);
    for(unsigned j=0;j<8;j++)printf("%s%u",j?",":"",active->dsp->core.registers[DSP_REG_M0+j]);
    printf("],\"state\":[");wet_words(base,554);
    printf("],\"workspace_hash\":");wet_workspace_hash();
    printf(",\"input\":[");wet_words(record==8?0x1580:0x1560,32);
    printf("],\"outputs\":[");wet_words(record==8?0xe60:0xde0,128);
    printf("],\"ring_hashes\":[");
    static const unsigned sizes[14]={6000,6000,6000,6000,680,612,640,720,1392,1684,2044,2476,24000,24000};
    unsigned offset=frame_peek(active->dsp,DSP_SPACE_X,base);
    for(unsigned j=0;j<14;j++){
        uint64_t h=UINT64_C(14695981039346656037);
        for(unsigned k=0;k<sizes[j];k++,offset+=4)h=wet_hash_word(h,scratch_word(active,offset));
        printf("%s\"%016"PRIx64"\"",j?",":"",h);
    }printf("]}\n");
}
static void combined_wet_checkpoint(unsigned pc){
    struct Context* c=active;if(c->phase!=2||!c->entered_title||c->returned_title)return;
    if(pc==0x171){
        /* Explicit caller mix values only, after original installation validation.
         * No state, histories, status, return values or availability are supplied. */
        c->phase=3;
        for(unsigned j=0;j<128;j++)dsp_write_memory(c->dsp,'X',0xc00+10*32+j,c->frames[c->frame_index][343+j]);
        for(unsigned j=0;j<256;j++)dsp_write_memory(c->dsp,'X',0xc00+15*32+j,c->frames[c->frame_index][471+j]);
        c->phase=2;
    }
    if(pc==wet_entry(0)||pc==wet_entry(1)||pc==wet_entry(2)||pc==wet_entry(3)||pc==wet_entry(4)){
        printf("{\"event\":\"combined_wet_prepare\",\"frame\":%u,\"pc\":%u,\"sr\":%u,\"modifiers\":[",c->frame_index,pc,c->dsp->core.registers[DSP_REG_SR]);
        for(unsigned j=0;j<8;j++)printf("%s%u",j?",":"",c->dsp->core.registers[DSP_REG_M0+j]);
        printf("],\"fanout_state\":[");wet_words(0x80,46);
        printf("],\"distortion_history\":[");
        for(unsigned j=0;j<8;j++)printf("%s%u",j?",":"",frame_peek(c->dsp,DSP_SPACE_Y,j));
        printf("],\"distortion_state\":[");wet_words(0xae,39);
        printf("],\"accumulator_states\":[");wet_words(0xd5,56);
        printf("],\"mix\":[");wet_words(0xc00+10*32,128);
        printf("],\"workspace_hash\":");wet_workspace_hash();printf("}\n");
    }
    if(pc==wet_entry(8))wet_reverb(8,"begin");
    if(pc==wet_entry(9)){wet_reverb(8,"end");wet_reverb(9,"begin");}
    if(pc==wet_entry(10))wet_reverb(9,"end");
}
