"""Bind actual native camera inputs at seven authenticated listener loads."""
SITES=((0x18f947,'0x2u+s.gp[7]',1),(0x18f94e,'0x0u+s.gp[7]',2),
       (0x18f960,'0x0u+s.gp[7]',2),(0x190475,'0xffffffceu+s.gp[1]',1),
       (0x19047c,'0xfffffff8u+s.gp[1]',4),(0x190485,'0xfffffffcu+s.gp[1]',4),
       (0x19048f,'0x0u+s.gp[1]',4))

def bind_native_camera_inputs(source):
    inherited='    using physics_world_runtime_detail::WorldState::WorldState;'
    assert source.count(inherited)==1
    source=source.replace(inherited,inherited+'\n    const SourceObjectAudioViewsTu15* native_audio_views{};')
    alias='using WorldState=LiveState;'
    helper='''
std::uint32_t audio_view_read(WorldState &s,std::uint32_t instruction,
    std::uint32_t address,unsigned size) {
    if(s.native_audio_views)if(const auto value=source_object_audio_view_operand_tu15(
        s.native_audio_views,r(s.memory,0x4f2c50u,4),instruction,address,size))return *value;
    return r(s.memory,address,size);
}'''
    assert source.count(alias)==1;source=source.replace(alias,alias+helper)
    signature='    const SourcePhysicsContactRuntimeServices &services,const O &observe) {'
    assert source.count(signature)==1
    source=source.replace(signature,'    const SourcePhysicsContactRuntimeServices &services,const O &observe,\n    const SourceObjectAudioViewsTu15* native_views) {')
    state='    FpScope scope(fp);LiveState s{m,observe,services,image};s.load(abi);'
    assert source.count(state)==1;source=source.replace(state,state+'\n    s.native_audio_views=native_views;')
    lines=source.splitlines(keepends=True)
    for pc,operand,size in SITES:
        matching=[i for i,line in enumerate(lines) if f's.services.instruction(0x{pc:x}u)' in line]
        assert len(matching)==1,(hex(pc),matching)
        index=matching[0];old=f'r(s.memory,({operand}),{size})'
        assert lines[index].count(old)==1
        lines[index]=lines[index].replace(old,f'audio_view_read(s,0x{pc:x}u,({operand}),{size})')
    return ''.join(lines)
