// Test-only retained reference for the three whole original-byte-pinned
// entries which have no generated run_* owner. The physical i386 executable
// independently owns every expected result, caller bank and ABI image.
namespace production {
static void run_232aa0(ObservedWorldState &s) {
    if(observe_entry)observe_entry(s,0x232aa0u);
    s.services.instruction(0x232aa0u);s.gp[2]=r(s.memory,s.gp[4]+8u,4);
    s.services.instruction(0x232aa4u);s.gp[0]=r(s.memory,s.gp[1],4);
    s.services.instruction(0x232aa6u);s.push(0x232aa6u,s.gp[2]);
    s.services.instruction(0x232aa7u);s.gp[2]=r(s.memory,s.gp[4]+8u,4);
    s.services.instruction(0x232aabu);s.push(0x232aabu,s.gp[2]);
    s.services.instruction(0x232aacu);s.push(0x232aacu,1u);
    s.services.instruction(0x232aaeu);const auto owner=r(s.memory,s.gp[0]+8u,4);
    s.push(0x232aaeu,0x232ab1u);dispatch(s,owner,0x232aaeu,false);
    s.services.instruction(0x232ab1u);s.pop();s.gp[4]+=8u;
}
static void run_232ac0(ObservedWorldState &s) {
    if(observe_entry)observe_entry(s,0x232ac0u);
    s.services.instruction(0x232ac0u);s.gp[0]=r(s.memory,s.gp[1],4);
    s.services.instruction(0x232ac2u);dispatch(s,r(s.memory,s.gp[0]+12u,4),0x232ac2u,true);
}
static void run_2e1030(ObservedWorldState &s) {
    if(observe_entry)observe_entry(s,0x2e1030u);
    s.services.instruction(0x2e1030u);dispatch(s,0x1cb560u,0x2e1030u,true);
}
static void run_149930(ObservedWorldState &s) {dispatch(s,0x149930u,0,false);}
}
