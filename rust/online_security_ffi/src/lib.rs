//! Scalar C ABI for the Python service. Policy is in the unsafe-forbidden
//! protocol crate; the sole pointer conversion copies a bounded PCR buffer.
use halo2_online_protocol::security_policy as policy;
use std::collections::HashMap;
use std::sync::{LazyLock, Mutex};
struct Queues { next: u64, values: HashMap<u64, policy::EosAdmissions> }
static QUEUES: LazyLock<Mutex<Queues>> = LazyLock::new(|| Mutex::new(Queues {next: 0, values: HashMap::new()}));
#[unsafe(no_mangle)]
pub extern "C" fn h2_security_abi() -> u32 { 1 }
#[unsafe(no_mangle)]
pub extern "C" fn h2_player_capacity() -> u64 { policy::PLAYER_CAPACITY }
#[unsafe(no_mangle)]
pub extern "C" fn h2_listing_admission(origin:u64, foreign:u64, total:u64) -> u16 { policy::listing_admission(origin,foreign,total) }
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
