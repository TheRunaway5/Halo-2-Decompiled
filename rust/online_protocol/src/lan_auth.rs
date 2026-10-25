//! Established raw-LAN authentication. Endpoint keys are fixed by admission;
//! discovery supplies no external server identity or confidentiality claim.
//!
//! `LNA1` frames carry the sender's public key and a full HMAC-SHA256 tag.
//! Once a peer has proven the shared key with an authenticated frame, the
//! sender switches to `LNA2`: no public key (the session is already pinned)
//! and a 128-bit truncated tag, cutting per-datagram overhead from 77 to 29
//! bytes. Both share one sequence space and replay window.
use hmac::{Hmac, Mac};
use sha2::Sha256;
use x25519_dalek::{PublicKey, StaticSecret};

pub const OVERHEAD: usize = 77;
pub const COMPACT_OVERHEAD: usize = 29;
const COMPACT_TAG: usize = 16;
const WINDOW: usize = 1024;
#[derive(Debug)]
pub struct Error;
impl std::fmt::Display for Error {
    fn fmt(&self, f: &mut std::fmt::Formatter<'_>) -> std::fmt::Result { f.write_str("Invalid authenticated LAN packet") }
}
impl std::error::Error for Error {}
pub struct Endpoint { secret: StaticSecret, public: [u8; 32] }
#[derive(Clone, Default)]
pub struct Session {
    peer: Option<[u8; 32]>, shared: [u8; 32], sent: u64, received: u64, seen: [u64; 16], established: bool,
}
impl Endpoint {
    pub fn new() -> Result<Self, Error> {
        let mut seed = [0; 32]; getrandom::getrandom(&mut seed).map_err(|_| Error)?;
        let secret = StaticSecret::from(seed);
        Ok(Self { public: PublicKey::from(&secret).to_bytes(), secret })
    }
    pub fn public(&self) -> [u8; 32] { self.public }
    pub fn bind(&self, session: &mut Session, peer: [u8; 32]) -> Result<(), Error> {
        if let Some(old) = session.peer { return if old == peer { Ok(()) } else { Err(Error) }; }
        let shared = self.secret.diffie_hellman(&PublicKey::from(peer));
        if !shared.was_contributory() { return Err(Error); }
        session.shared = shared.to_bytes(); session.peer = Some(peer); Ok(())
    }
    pub fn seal(&self, s: &mut Session, host: bool, body: &[u8]) -> Result<Vec<u8>, Error> {
        let peer = s.peer.ok_or(Error)?;
        if body.len() > 1400 - OVERHEAD { return Err(Error); }
        let sequence = s.sent.checked_add(1).ok_or(Error)?;
        if s.established {
            let mut frame = Vec::with_capacity(COMPACT_OVERHEAD + body.len());
            frame.extend_from_slice(b"LNA2"); frame.push(u8::from(host));
            frame.extend_from_slice(&sequence.to_le_bytes()); frame.extend_from_slice(body);
            let tag = compact_mac(&s.shared, peer, &frame).finalize().into_bytes();
            frame.extend_from_slice(&tag[..COMPACT_TAG]);
            s.sent = sequence; return Ok(frame);
        }
        let mut frame = Vec::with_capacity(OVERHEAD + body.len());
        frame.extend_from_slice(b"LNA1"); frame.push(u8::from(host));
        frame.extend_from_slice(&sequence.to_le_bytes()); frame.extend_from_slice(&self.public);
        frame.extend_from_slice(body);
        frame.extend_from_slice(&mac(&s.shared, peer, &frame).finalize().into_bytes());
        s.sent = sequence; Ok(frame)
    }
    pub fn open(&self, session: &mut Session, from_host: bool, bytes: &[u8]) -> Result<Vec<u8>, Error> {
        if !is_packet(bytes) || bytes.len() > 1400 || bytes[4] != u8::from(from_host) { return Err(Error); }
        if bytes.starts_with(b"LNA2") {
            // Compact frames are valid only on an already pinned session.
            if session.peer.is_none() { return Err(Error); }
            let mut candidate = session.clone();
            let tag = bytes.len() - COMPACT_TAG;
            compact_mac(&candidate.shared, self.public, &bytes[..tag]).verify_truncated_left(&bytes[tag..]).map_err(|_| Error)?;
            let sequence = u64::from_le_bytes(bytes[5..13].try_into().map_err(|_| Error)?);
            accept_sequence(&mut candidate, sequence)?;
            candidate.established = true;
            *session = candidate; return Ok(bytes[13..tag].to_vec());
        }
        let peer = bytes[13..45].try_into().map_err(|_| Error)?;
        let mut candidate = session.clone(); self.bind(&mut candidate, peer)?;
        let tag = bytes.len() - 32;
        mac(&candidate.shared, self.public, &bytes[..tag]).verify_slice(&bytes[tag..]).map_err(|_| Error)?;
        let sequence = u64::from_le_bytes(bytes[5..13].try_into().map_err(|_| Error)?);
        accept_sequence(&mut candidate, sequence)?;
        candidate.established = true;
        *session = candidate; Ok(bytes[45..tag].to_vec())
    }
}
fn accept_sequence(candidate: &mut Session, sequence: u64) -> Result<(), Error> {
    if sequence == 0 { return Err(Error); }
    if sequence > candidate.received {
        let advance = sequence - candidate.received;
        let old = candidate.seen; candidate.seen = [0; 16];
        if advance < WINDOW as u64 {
            for age in 0..WINDOW - advance as usize {
                if old[age / 64] & (1 << (age % 64)) != 0 {
                    let next = age + advance as usize; candidate.seen[next / 64] |= 1 << (next % 64);
                }
            }
        }
        candidate.received = sequence;
    }
    let age = candidate.received - sequence;
    if age >= WINDOW as u64 { return Err(Error); }
    let age = age as usize; let mask = 1 << (age % 64);
    if candidate.seen[age / 64] & mask != 0 { return Err(Error); }
    candidate.seen[age / 64] |= mask;
    Ok(())
}
pub fn is_packet(bytes: &[u8]) -> bool {
    (bytes.len() >= OVERHEAD && bytes.starts_with(b"LNA1")) || (bytes.len() >= COMPACT_OVERHEAD && bytes.starts_with(b"LNA2"))
}
fn compact_mac(secret: &[u8; 32], recipient: [u8; 32], frame: &[u8]) -> Hmac<Sha256> {
    let mut mac = Hmac::<Sha256>::new_from_slice(secret).expect("fixed HMAC key");
    mac.update(b"H2LAN102"); mac.update(&recipient); mac.update(frame); mac
}
fn mac(secret: &[u8; 32], recipient: [u8; 32], frame: &[u8]) -> Hmac<Sha256> {
    // HMAC accepts every key width; this fixed-width constructor cannot fail.
    let mut mac = Hmac::<Sha256>::new_from_slice(secret).expect("fixed HMAC key");
    mac.update(b"H2LAN101"); mac.update(&recipient); mac.update(frame); mac
}
impl Session {
    pub fn peer(&self) -> Option<[u8; 32]> { self.peer }
}
#[cfg(test)]
mod tests {
    use super::*;
    #[test]
    fn observed_cookies_cannot_forge_input_leave_or_snapshots() {
        let host = Endpoint::new().unwrap(); let client = Endpoint::new().unwrap();
        let mut h = Session::default(); let mut c = Session::default();
        host.bind(&mut h, client.public()).unwrap(); client.bind(&mut c, host.public()).unwrap();
        for kind in [2, 4, 3] {
            let packet = host.seal(&mut h, true, &[kind, 5, 6, 7]).unwrap();
            for at in 0..packet.len() {
                let mut forged = packet.clone(); forged[at] ^= 1;
                assert!(client.open(&mut c, true, &forged).is_err());
            }
            assert!(client.open(&mut c, false, &packet).is_err());
            assert_eq!(client.open(&mut c, true, &packet).unwrap(), [kind, 5, 6, 7]);
            assert!(client.open(&mut c, true, &packet).is_err());
        }
    }
    #[test]
    fn compact_frames_follow_proof_and_resist_forgery_and_replay() {
        let host = Endpoint::new().unwrap(); let client = Endpoint::new().unwrap();
        let mut h = Session::default(); let mut c = Session::default();
        host.bind(&mut h, client.public()).unwrap();
        // Until the client proves the key, the host keeps the full frame.
        let first = host.seal(&mut h, true, &[1, 2]).unwrap();
        assert_eq!(first.len(), OVERHEAD + 2);
        assert_eq!(client.open(&mut c, true, &first).unwrap(), [1, 2]);
        let reply = client.seal(&mut c, false, &[3; 40]).unwrap();
        assert_eq!(reply.len(), COMPACT_OVERHEAD + 40);
        assert_eq!(host.open(&mut h, false, &reply).unwrap(), [3; 40]);
        assert!(host.open(&mut h, false, &reply).is_err());
        let compact = host.seal(&mut h, true, &[9, 8, 7]).unwrap();
        assert_eq!(compact.len(), COMPACT_OVERHEAD + 3);
        for at in 0..compact.len() {
            let mut forged = compact.clone(); forged[at] ^= 1;
            assert!(client.open(&mut c, true, &forged).is_err());
        }
        assert!(client.open(&mut c, false, &compact).is_err());
        let other = Endpoint::new().unwrap();
        assert!(other.open(&mut Session::default(), true, &compact).is_err());
        let mut unrelated = Session::default(); other.bind(&mut unrelated, host.public()).unwrap();
        assert!(other.open(&mut unrelated, true, &compact).is_err());
        assert_eq!(client.open(&mut c, true, &compact).unwrap(), [9, 8, 7]);
        assert!(client.open(&mut c, true, &compact).is_err());
        assert!(client.open(&mut c, true, &first).is_err());
    }
    #[test]
    fn bounded_reordering_pins_keys_and_rejects_old_connections() {
        let a = Endpoint::new().unwrap(); let b = Endpoint::new().unwrap(); let other = Endpoint::new().unwrap();
        let mut sender = Session::default(); let mut receiver = Session::default(); a.bind(&mut sender, b.public()).unwrap();
        let frames: Vec<_> = (0..1030).map(|_| a.seal(&mut sender, true, &[1]).unwrap()).collect();
        b.open(&mut receiver, true, &frames[1029]).unwrap();
        b.open(&mut receiver, true, &frames[6]).unwrap();
        assert!(b.open(&mut receiver, true, &frames[5]).is_err());
        assert!(b.bind(&mut receiver, other.public()).is_err());
        assert!(other.open(&mut Session::default(), true, &frames[1029]).is_err());
        assert!(a.bind(&mut Session::default(), [0; 32]).is_err());
    }
}
