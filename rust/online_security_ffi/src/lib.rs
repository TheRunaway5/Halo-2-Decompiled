//! Scalar C ABI for the Python service. Policy is in the unsafe-forbidden
//! protocol crate; pointer conversions copy bounded owned PCR/roster buffers.
mod matchmaking_candidates;
mod matchmaking_lifecycle;
mod matchmaking_coordinator;
use halo2_online_protocol::security_policy as policy;
use std::collections::HashMap;
use std::sync::{LazyLock, Mutex};
struct Queues { next: u64, values: HashMap<u64, policy::EosAdmissions> }
static QUEUES: LazyLock<Mutex<Queues>> = LazyLock::new(|| Mutex::new(Queues {next: 0, values: HashMap::new()}));
#[unsafe(no_mangle)]
pub extern "C" fn h2_security_abi() -> u32 { 2 }

/// # Safety
/// An optional admitted header contains exactly header_bytes bytes. Output is
/// disjoint live storage for exactly 128 rank bytes. No network pointer enters
/// this interface; failure leaves output untouched.
#[unsafe(no_mangle)]
pub unsafe extern "C" fn h2_matchmaking_rank_limits(header:*const u8,header_bytes:usize,
    output:*mut u8,output_bytes:usize) -> bool {
    use halo2_online_protocol::matchmaking_ratings::RankMap;
    if output.is_null() || output_bytes!=128 {return false;}
    let map=if header_bytes==0 && header.is_null() {RankMap::retail_launch()}
    else {
        if header.is_null() || header_bytes!=0x614 {return false;}
        let Some(map)=RankMap::from_header(unsafe {std::slice::from_raw_parts(header,header_bytes)})
            else {return false;};
        map
    };
    unsafe {std::ptr::copy_nonoverlapping(map.values().as_ptr(),output,128)};true
}

/// # Safety
/// The raw/identity-flag arrays contain exactly count owned elements, at most
/// 16. Output is four disjoint i32 words: average, maximum, minimum, rated count.
#[unsafe(no_mangle)]
pub unsafe extern "C" fn h2_matchmaking_raw_ratings(count:usize,raw:*const i32,
    flags:*const u32,output:*mut i32,output_words:usize) -> bool {
    if count>16 || raw.is_null() || flags.is_null() || output.is_null() || output_words!=4 {return false;}
    let raw=unsafe {std::slice::from_raw_parts(raw,count)}.to_vec();
    let flags=unsafe {std::slice::from_raw_parts(flags,count)}.to_vec();
    let Some(stats)=halo2_online_protocol::matchmaking_ratings::raw_statistics(&raw,&flags) else {return false;};
    let result=[stats.average,stats.maximum,stats.minimum,stats.rated_count as i32];
    unsafe {std::ptr::copy_nonoverlapping(result.as_ptr(),output,4)};true
}

unsafe fn matchmaking_rank_map(table:*const u8,table_bytes:usize)
    -> Option<halo2_online_protocol::matchmaking_ratings::RankMap> {
    if table.is_null() || table_bytes!=128 {return None;}
    halo2_online_protocol::matchmaking_ratings::RankMap::from_slice(
        unsafe {std::slice::from_raw_parts(table,table_bytes)})
}

/// # Safety
/// Raw/groups/output each contain count elements, at most 16. The admitted
/// table has exactly 128 bytes. Inputs are copied before any output is written.
#[unsafe(no_mangle)]
pub unsafe extern "C" fn h2_matchmaking_adjusted_ratings(count:usize,raw:*const i32,
    groups:*const i32,catalog_index:u32,table:*const u8,table_bytes:usize,output:*mut i32) -> bool {
    if count>16 || raw.is_null() || groups.is_null() || output.is_null() {return false;}
    let Some(map)=(unsafe {matchmaking_rank_map(table,table_bytes)}) else {return false;};
    let raw=unsafe {std::slice::from_raw_parts(raw,count)}.to_vec();
    let groups=unsafe {std::slice::from_raw_parts(groups,count)}.to_vec();
    let Some(result)=halo2_online_protocol::matchmaking_ratings::adjusted_ranks(&raw,&groups,catalog_index as usize,&map)
        else {return false;};
    unsafe {std::ptr::copy_nonoverlapping(result.as_ptr(),output,count)};true
}

/// # Safety
/// Table is 128 admitted rank bytes; output is one disjoint live i32.
#[unsafe(no_mangle)]
pub unsafe extern "C" fn h2_matchmaking_advertised_minimum(minimum:i32,maximum:i32,
    table:*const u8,table_bytes:usize,output:*mut i32) -> bool {
    if output.is_null() {return false;}
    let Some(map)=(unsafe {matchmaking_rank_map(table,table_bytes)}) else {return false;};
    let Some(result)=map.advertised_minimum(minimum,maximum) else {return false;};
    unsafe {output.write(result)};true
}

/// # Safety
/// Table is 128 admitted bytes; output is three disjoint i32 words. Radius -2
/// is this adapter's explicit full-window request, not an original search state.
#[unsafe(no_mangle)]
pub unsafe extern "C" fn h2_matchmaking_rank_window(minimum:i32,maximum:i32,radius:i32,
    table:*const u8,table_bytes:usize,output:*mut i32,output_words:usize) -> bool {
    if output.is_null() || output_words!=3 {return false;}
    let Some(map)=(unsafe {matchmaking_rank_map(table,table_bytes)}) else {return false;};
    let Some(envelope)=halo2_online_protocol::matchmaking_ratings::SearchEnvelope::new(minimum,maximum,&map)
        else {return false;};
    let radius=if radius==-2 {envelope.maximum_radius} else {radius};
    let Some((lower,upper))=envelope.bounds(radius) else {return false;};
    let result=[envelope.maximum_radius,lower,upper];
    unsafe {std::ptr::copy_nonoverlapping(result.as_ptr(),output,3)};true
}
/// # Safety
/// Service-owned weights/availability contain count entries (at most100).
/// Outputs are live, disjoint scalar storage. Failure leaves them unchanged.
#[unsafe(no_mangle)]
pub unsafe extern "C" fn h2_hopper_draw(seed:u32,count:usize,weights:*const i32,
    available:*const u8,next_seed:*mut u32,index:*mut u32,total:*mut u32,choice:*mut u32) -> u32 {
    use halo2_online_protocol::hopper_selection::{self,Row};
    if count>hopper_selection::MAX_ROWS || weights.is_null() || available.is_null() ||
        next_seed.is_null() || index.is_null() || total.is_null() || choice.is_null() {return 2;}
    let weights=unsafe {std::slice::from_raw_parts(weights,count)};
    let available=unsafe {std::slice::from_raw_parts(available,count)};
    if available.iter().any(|value|*value>1) {return 2;}
    let rows:Vec<_>=weights.iter().zip(available).map(|(&weight,&available)|Row{weight,available:available==1}).collect();
    match hopper_selection::choose(seed,&rows) {
        Ok(Some(draw))=>{
            unsafe {*next_seed=draw.seed;*index=draw.index as u32;*total=draw.total;*choice=draw.choice;}
            0
        },
        Ok(None)=>1,
        Err(_)=>2,
    }
}
#[unsafe(no_mangle)]
pub extern "C" fn h2_player_capacity() -> u64 { policy::PLAYER_CAPACITY }
#[unsafe(no_mangle)]
pub extern "C" fn h2_listing_admission(origin:u64, foreign:u64, total:u64) -> u16 { policy::listing_admission(origin,foreign,total) }
#[unsafe(no_mangle)]
pub extern "C" fn h2_local_listing_admission(owner:u64, local:u64, total:u64) -> u16 { policy::local_listing_admission(owner,local,total) }
#[unsafe(no_mangle)]
pub extern "C" fn h2_eos_queue_new() -> u64 {
    let Ok(mut queues) = QUEUES.lock() else { return 0; };
    let Some(id) = queues.next.checked_add(1) else { return 0; };
    queues.next=id; queues.values.insert(id,policy::EosAdmissions::default()); id
}
#[unsafe(no_mangle)]
pub extern "C" fn h2_eos_queue_drop(id:u64) { if let Ok(mut q)=QUEUES.lock() { q.values.remove(&id); } }
#[unsafe(no_mangle)]
pub extern "C" fn h2_eos_admit(id:u64, a:u64,b:u64,c:u64,d:u64) -> u16 {
    let Ok(mut queues) = QUEUES.lock() else { return 503; };
    queues.values.get_mut(&id).map_or(503,|q|q.admit([a,b,c,d]))
}
#[unsafe(no_mangle)]
pub extern "C" fn h2_eos_release(id:u64, a:u64,b:u64,c:u64,d:u64) {
    if let Ok(mut queues)=QUEUES.lock() && let Some(q)=queues.values.get_mut(&id) { q.release([a,b,c,d]); }
}
#[unsafe(no_mangle)]
pub extern "C" fn h2_eos_pending(id:u64) -> u64 {
    QUEUES.lock().ok().and_then(|q|q.values.get(&id).map(|v|v.pending() as u64)).unwrap_or(0)
}
/// # Safety
/// The trusted Python adapter must provide disjoint live buffers of exactly n
/// bytes. No pointer or length from a network packet is forwarded to this ABI.
#[unsafe(no_mangle)]
pub unsafe extern "C" fn h2_result_projection(input:*const u8, output:*mut u8, n:usize) -> bool {
    if input.is_null() || output.is_null() || n!=policy::PCR_SIZE { return false; }
    // Copy into an owning Rust result before writing through the output pointer.
    let Some(result)=policy::persistable_result(unsafe { std::slice::from_raw_parts(input,n) }) else {return false;};
    unsafe { std::ptr::copy_nonoverlapping(result.as_ptr(),output,n); } true
}

/// # Safety
/// Input/output are live non-overlapping PCR_SIZE buffers. The caller owns
/// the versioned client-verification capability and unanimous consent.
#[unsafe(no_mangle)]
pub unsafe extern "C" fn h2_result_accuracy_projection(input:*const u8, output:*mut u8, n:usize) -> bool {
    if input.is_null() || output.is_null() || n!=policy::PCR_SIZE {return false;}
    let raw=unsafe {std::slice::from_raw_parts(input,n)};
    let Some(result)=policy::persistable_accuracy_result(raw) else {return false;};
    unsafe {std::ptr::copy_nonoverlapping(result.as_ptr(),output,n)};
    true
}

fn balance_options(teams:u32, maximum:u32, flagged:bool, minimum:u32, limit:u32,
    parties:bool, optimize:bool) -> halo2_online_protocol::team_balance::Options {
    halo2_online_protocol::team_balance::Options {teams:teams as usize,maximum_team_size:maximum as usize,
        flag_parties:flagged,flag_minimum:minimum as usize,flag_maximum:limit as usize,use_parties:parties,optimize}
}
/// # Safety
/// Trusted service-owned ids, weights and disjoint output each have count
/// elements (at most 16). Inputs are copied before Rust evaluates the complete
/// assignment caller; failure leaves output untouched.
#[unsafe(no_mangle)]
pub unsafe extern "C" fn h2_matchmaking_assignment(ranked:bool,teams:u32,maximum:u32,
    clan_minimum:u32,clan_imbalance:u32,significant_minimum:u32,significant_maximum:u32,
    significant_spread:u32,count:usize,ids:*const i32,weights:*const i32,output:*mut i32) -> bool {
    if count>16 || ids.is_null() || weights.is_null() || output.is_null() {return false;}
    let ids=unsafe {std::slice::from_raw_parts(ids,count)}.to_vec();
    let weights=unsafe {std::slice::from_raw_parts(weights,count)}.to_vec();
    let policy=halo2_online_protocol::team_assignment::NativePolicy {ranked,team_count:teams,
        team_size:maximum,clan_minimum,clan_imbalance,significant_minimum,significant_maximum,significant_spread};
    let Some(result)=policy.assign(&ids,&weights) else {return false;};
    unsafe {std::ptr::copy_nonoverlapping(result.teams.as_ptr(),output,count)};true
}
/// # Safety
/// The header is exactly 0x614 live bytes from an admitted versioned profile;
/// the three arrays obey the same bounded ownership contract as above. This
/// entry performs no native-catalog projection or significant-party policy.
#[unsafe(no_mangle)]
pub unsafe extern "C" fn h2_original_hopper_assignment(header:*const u8,header_bytes:usize,
    count:usize,ids:*const i32,weights:*const i32,output:*mut i32) -> bool {
    if header_bytes!=halo2_online_protocol::team_assignment::HOPPER_BYTES || count>16 ||
        header.is_null() || ids.is_null() || weights.is_null() || output.is_null() {return false;}
    let Some(hopper)=halo2_online_protocol::team_assignment::Hopper::from_header(
        unsafe {std::slice::from_raw_parts(header,header_bytes)}) else {return false;};
    let ids=unsafe {std::slice::from_raw_parts(ids,count)}.to_vec();
    let weights=unsafe {std::slice::from_raw_parts(weights,count)}.to_vec();
    let Some(result)=halo2_online_protocol::team_assignment::assign(hopper,&ids,&weights) else {return false;};
    unsafe {std::ptr::copy_nonoverlapping(result.teams.as_ptr(),output,count)};true
}
/// # Safety
/// Trusted service-owned arrays contain exactly count elements (at most16).
/// Inputs are copied before the disjoint output is written; no packet pointer
/// or client-selected memory address is forwarded to this ABI.
#[unsafe(no_mangle)]
pub unsafe extern "C" fn h2_team_balance(teams:u32,maximum:u32,flagged:bool,minimum:u32,limit:u32,
    parties:bool,count:usize,ids:*const i32,weights:*const i32,optimize:bool,output:*mut i32) -> bool {
    if count>16 || ids.is_null() || weights.is_null() || output.is_null() {return false;}
    let ids=unsafe {std::slice::from_raw_parts(ids,count)}.to_vec();
    let weights=unsafe {std::slice::from_raw_parts(weights,count)}.to_vec();
    let options=balance_options(teams,maximum,flagged,minimum,limit,parties,optimize);
    let Some(result)=halo2_online_protocol::team_balance::balance(options,&ids,&weights) else {return false;};
    unsafe {std::ptr::copy_nonoverlapping(result.as_ptr(),output,count)};true
}
/// # Safety
/// Trusted service-owned ids contains count elements, and output is one live
/// disjoint i32. The same bounded roster ownership contract as h2_team_balance.
#[unsafe(no_mangle)]
pub unsafe extern "C" fn h2_team_balance_count(teams:u32,maximum:u32,flagged:bool,minimum:u32,limit:u32,
    parties:bool,count:usize,ids:*const i32,output:*mut i32) -> bool {
    if count>16 || ids.is_null() || output.is_null() {return false;}
    let ids=unsafe {std::slice::from_raw_parts(ids,count)}.to_vec();
    let options=balance_options(teams,maximum,flagged,minimum,limit,parties,false);
    let Some(result)=halo2_online_protocol::team_balance::by_count(options,&ids) else {return false;};
    unsafe {output.write(result)};true
}
/// # Safety
/// Trusted service-owned ids contains count elements, at most16 including
/// extra seats. Inputs are copied before calling the safe native kernel.
#[unsafe(no_mangle)]
pub unsafe extern "C" fn h2_team_balance_can_add(teams:u32,maximum:u32,flagged:bool,minimum:u32,limit:u32,
    parties:bool,count:usize,ids:*const i32,extra:usize,imbalance:i32) -> bool {
    if count>16 || extra>16-count || ids.is_null() {return false;}
    let ids=unsafe {std::slice::from_raw_parts(ids,count)}.to_vec();
    halo2_online_protocol::team_balance::can_add(balance_options(teams,maximum,flagged,minimum,limit,parties,false),&ids,extra,imbalance)
}

// Trusted service-owned scalars only. -1 represents the already-proven ready
// state9; the transport adapter owns authorization and parameter publication.
#[unsafe(no_mangle)]
pub extern "C" fn h2_party_countdown_press(seconds:i32) -> i32 {
    halo2_online_protocol::party_countdown::press(seconds)
}
#[unsafe(no_mangle)]
pub extern "C" fn h2_party_countdown_tick(seconds:i32,elapsed_ms:u32) -> i32 {
    halo2_online_protocol::party_countdown::tick(seconds,elapsed_ms)
}

#[unsafe(no_mangle)]
pub extern "C" fn h2_party_privacy_mode(value:i32) -> i32 {
    halo2_online_protocol::party_privacy::mode(value)
}
#[unsafe(no_mangle)]
pub extern "C" fn h2_party_privacy_can_change(established:bool,leader:bool) -> bool {
    halo2_online_protocol::party_privacy::can_change(established,leader)
}
#[unsafe(no_mangle)]
pub extern "C" fn h2_party_privacy_admission(mode:i32,invited:bool,count:i32,
    public_open:i32,private_open:i32) -> i32 {
    halo2_online_protocol::party_privacy::admission(mode,invited,count,public_open,private_open)
}

/// # Safety
/// Service-owned arrays hold peer_count masks and seat_count owners/teams,
/// bounded by16. No untrusted pointer or downloaded policy is accepted.
#[unsafe(no_mangle)]
pub unsafe extern "C" fn h2_matchmaking_connected_teams(peer_count:usize,masks:*const u32,current:u32,
    seat_count:usize,occupied:u32,owners:*const u32,teams:*const i32) -> bool {
    if peer_count==0 || peer_count>16 || seat_count>16 || masks.is_null() || owners.is_null() || teams.is_null() {return false;}
    let peers=unsafe {std::slice::from_raw_parts(masks,peer_count)}.to_vec();
    let owners=unsafe {std::slice::from_raw_parts(owners,seat_count)}.to_vec();
    let teams=unsafe {std::slice::from_raw_parts(teams,seat_count)}.to_vec();
    let seats=owners.into_iter().zip(teams).enumerate().map(|(i,(peer,team))|
        halo2_online_protocol::matchmaking_readiness::Seat{occupied:occupied&(1_u32<<i)!=0,peer,team}).collect::<Vec<_>>();
    halo2_online_protocol::matchmaking_readiness::connected_teams(&peers,current as usize,&seats)
}
#[unsafe(no_mangle)]
pub extern "C" fn h2_matchmaking_eligibility(teams:bool,connected:bool,check_policy:bool,
    live_logon:bool,bit31:bool,disable_teams:bool,disable_policy:bool) -> bool {
    halo2_online_protocol::matchmaking_readiness::eligible(teams,connected,check_policy,
        live_logon,bit31,disable_teams,disable_policy)
}
