/* Reference-only host observations and explicit raw mix/parameter inputs.
 * Never compiled into the game; no original GP instruction or result patch. */
static unsigned dry_record_entry(unsigned index) {
    const unsigned metadata=0x818+(ldl_le_p(active->title+0x804)+ldl_le_p(active->title+0x80c))*4;
    return 0x171+(ldl_le_p(active->title+metadata+8+index*32)-0x818)/4;
}
static unsigned dry_state(unsigned record){return 0x10d+(record-4)*72;}
static void dry_words(char space,unsigned at,unsigned n) {
    for(unsigned j=0;j<n;j++)printf("%s%u",j?",":"",frame_peek(active->dsp,space=='Y'?DSP_SPACE_Y:DSP_SPACE_X,at+j));
}
static void dry_node(unsigned record,const char* point) {
    const unsigned state=dry_state(record),history=frame_peek(active->dsp,DSP_SPACE_X,state+2);
    printf("{\"event\":\"combined_dry_node\",\"frame\":%u,\"record\":%u,\"point\":\"%s\",\"sr\":%u,\"modifiers\":[",active->frame_index,record,point,active->dsp->core.registers[DSP_REG_SR]);
    for(unsigned j=0;j<8;j++)printf("%s%u",j?",":"",active->dsp->core.registers[DSP_REG_M0+j]);
    printf("],\"state\":[");dry_words('X',state,72);
    printf("],\"audio\":[");dry_words('X',0xc00+(6+record-4)*32,32);
    printf("],\"modulation\":[");dry_words('X',0xc00+14*32,32);
    printf("],\"output\":[");dry_words('X',0x77a+(record-4)*32,32);
    printf("],\"history\":[");dry_words('Y',history,401);printf("]}\n");
}
static void combined_dry_checkpoint(unsigned pc) {
    struct Context *c=active;if(c->phase!=2||!c->entered_title||c->returned_title)return;
    if(pc==0x171){
        /* Four input bins and common modulation are declared external input
         * values. Gain fields are declared parameters; flags/cursors/history
         * and cached bindings are never overlaid. */
        c->phase=3;
        for(unsigned j=0;j<128;j++)dsp_write_memory(c->dsp,'X',0xc00+6*32+j,c->frames[c->frame_index][343+j]);
        for(unsigned j=0;j<32;j++)dsp_write_memory(c->dsp,'X',0xc00+14*32+j,c->frames[c->frame_index][471+j]);
        for(unsigned j=0;j<4;j++)dsp_write_memory(c->dsp,'X',dry_state(4+j)+8,c->frames[c->frame_index][503+j]);
        c->phase=2;
    }
    for(unsigned record=4;record<8;record++){
        if(pc==dry_record_entry(record))dry_node(record,"begin");
        if(pc==dry_record_entry(record+1))dry_node(record,"end");
    }
    if(pc==dry_record_entry(4)||pc==dry_record_entry(10)||pc==0xd60){
        printf("{\"event\":\"combined_dry_boundary\",\"frame\":%u,\"pc\":%u,\"base_outputs\":[",c->frame_index,pc);
        for(unsigned j=0;j<4;j++){if(j)printf(",");printf("[");dry_words('X',crosstalk_outputs[j],32);printf("]");}
        printf("],\"history\":[");dry_words('Y',8,1601);printf("]}\n");
    }
}
