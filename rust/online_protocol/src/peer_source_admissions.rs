//! Immutable native-player admission exchange over authenticated physical peers.
//! This is the replacement protocol, not recovered original Live/XNet wire.
//! Accepted metadata and delivery acknowledgements never certify a native world.
use sha2::{Digest, Sha256};

const HEADER: usize = 89;
const MAX_PACKET: usize = 522;
#[derive(Clone, Copy, Debug, PartialEq, Eq)]
pub struct Context {
    pub session: [u8; 32],
    pub epoch: u32,
    pub membership_revision: u64,
    pub peers: u16,
    // Preserve the full native unsigned operand, including invalid inputs.
    pub host: u32,
}
#[derive(Clone, Debug, PartialEq, Eq)]
pub struct Seat {
    pub slot: u32,
    pub peer: u32,
    pub guest: bool,
    pub account: [u8; 8],
}
impl Seat {
    fn owner(&self) -> u32 {
        if self.peer == u32::MAX {
            self.slot
        } else {
            self.peer
        }
    }
}
#[derive(Clone, Debug, PartialEq, Eq)]
pub struct Player {
    pub slot: u32,
    pub player: u32,
    pub identity: [u8; 12],
}
#[derive(Clone, Debug, PartialEq, Eq)]
pub struct Accepted {
    pub context: Context,
    pub digest: [u8; 32],
    pub players: Vec<Player>,
}
#[derive(Clone, Debug, PartialEq, Eq)]
pub struct Packet {
    pub peer: u32,
    pub bytes: Vec<u8>,
}
#[derive(Clone, Copy, Debug, PartialEq, Eq)]
pub struct Error;
impl std::fmt::Display for Error {
    fn fmt(&self, f: &mut std::fmt::Formatter<'_>) -> std::fmt::Result {
        f.write_str("Invalid native source admission exchange")
    }
}
impl std::error::Error for Error {}
pub type Result<T> = std::result::Result<T, Error>;
fn need(value: bool) -> Result<()> {
    if value { Ok(()) } else { Err(Error) }
}
fn member(c: Context, peer: u32) -> bool {
    peer < 16 && c.peers & (1u16 << peer) != 0
}
fn context_bytes(c: Context, out: &mut Vec<u8>) {
    out.extend_from_slice(&c.session);
    out.extend_from_slice(&c.epoch.to_le_bytes());
    out.extend_from_slice(&c.membership_revision.to_le_bytes());
    out.extend_from_slice(&c.peers.to_le_bytes());
    out.push(c.host as u8);
}
fn hash(c: Context, body: &[u8]) -> [u8; 32] {
    let mut bytes = b"H2 SOURCE ADMISSIONS 1".to_vec();
    context_bytes(c, &mut bytes);
    bytes.extend_from_slice(body);
    Sha256::digest(bytes).into()
}
fn validate(self_peer: u32, c: Context, roster: &[Seat]) -> Result<()> {
    need(
        (2..=16).contains(&roster.len())
            && member(c, self_peer)
            && member(c, c.host)
            && c.session.iter().any(|b| *b != 0),
    )?;
    let mut physical = 0u16;
    for (slot, seat) in roster.iter().enumerate() {
        let owner = seat.owner() as usize;
        need(seat.slot as usize == slot && owner < roster.len())?;
        need(roster[owner].owner() as usize == owner && !roster[owner].guest)?;
        need(
            roster
                .iter()
                .filter(|s| s.owner() as usize == owner)
                .count()
                <= 4,
        )?;
        if owner == slot {
            physical |= 1u16 << slot;
        }
    }
    need(c.peers & !physical == 0)
}
fn body(players: &[Player], roster: &[Seat]) -> Result<Vec<u8>> {
    need(players.len() == roster.len())?;
    let mut bytes = Vec::with_capacity(1 + 27 * players.len());
    bytes.push(players.len() as u8);
    for (slot, (player, seat)) in players.iter().zip(roster).enumerate() {
        need(
            player.slot as usize == slot
                && player.player != u32::MAX
                && player.player & 65535 == slot as u32,
        )?;
        need(
            players[..slot]
                .iter()
                .all(|p| p.identity != player.identity),
        )?;
        bytes.extend_from_slice(&[slot as u8, seat.owner() as u8, seat.guest as u8]);
        bytes.extend_from_slice(&seat.account);
        bytes.extend_from_slice(&player.player.to_le_bytes());
        bytes.extend_from_slice(&player.identity);
    }
    Ok(bytes)
}
struct Reader<'a> {
    bytes: &'a [u8],
    at: usize,
}
impl<'a> Reader<'a> {
    fn raw<const N: usize>(&mut self) -> Result<[u8; N]> {
        let end = self.at.checked_add(N).ok_or(Error)?;
        let value = self
            .bytes
            .get(self.at..end)
            .ok_or(Error)?
            .try_into()
            .map_err(|_| Error)?;
        self.at = end;
        Ok(value)
    }
    fn byte(&mut self) -> Result<u8> {
        Ok(self.raw::<1>()?[0])
    }
    fn context(&mut self) -> Result<Context> {
        Ok(Context {
            session: self.raw()?,
            epoch: u32::from_le_bytes(self.raw()?),
            membership_revision: u64::from_le_bytes(self.raw()?),
            peers: u16::from_le_bytes(self.raw()?),
            host: self.byte()? as u32,
        })
    }
    fn players(&mut self, roster: &[Seat]) -> Result<Vec<Player>> {
        need(self.byte()? as usize == roster.len())?;
        let mut players = Vec::with_capacity(roster.len());
        for (slot, seat) in roster.iter().enumerate() {
            need(
                self.byte()? as usize == slot
                    && self.byte()? as u32 == seat.owner()
                    && self.byte()? == seat.guest as u8,
            )?;
            need(self.raw::<8>()? == seat.account)?;
            players.push(Player {
                slot: slot as u32,
                player: u32::from_le_bytes(self.raw()?),
                identity: self.raw()?,
            });
        }
        need(self.at == self.bytes.len())?;
        body(&players, roster)?;
        Ok(players)
    }
}
pub fn is_packet(bytes: &[u8]) -> bool {
    bytes.len() >= 7 && &bytes[..4] == b"H2N2" && bytes[6] == 8
}

pub struct Exchange {
    self_peer: u32,
    protocol: u16,
    binding: Context,
    roster: Vec<Seat>,
    value: Option<Accepted>,
    payload: Vec<u8>,
    acknowledged: u16,
    sent: u16,
    sent_at: [u64; 16],
    cursor: u32,
    ack_pending: bool,
}
impl Exchange {
    pub fn new(self_peer: u32, protocol: u16, context: Context, roster: Vec<Seat>) -> Result<Self> {
        validate(self_peer, context, &roster)?;
        Ok(Self {
            self_peer,
            protocol,
            binding: context,
            roster,
            value: None,
            payload: Vec::new(),
            acknowledged: 0,
            sent: 0,
            sent_at: [0; 16],
            cursor: 0,
            ack_pending: false,
        })
    }
    pub fn context(&self) -> Context {
        self.binding
    }
    pub fn accepted(&self) -> Option<&Accepted> {
        self.value.as_ref()
    }
    pub fn replace_context(&mut self, expected: Context, next: Context) -> Result<bool> {
        validate(self.self_peer, next, &self.roster)?;
        if self.binding != expected
            || next.session != expected.session
            || next.epoch < expected.epoch
            || (next.epoch == expected.epoch
                && (next.host != expected.host
                    || next.membership_revision <= expected.membership_revision))
        {
            return Ok(false);
        }
        self.binding = next;
        self.value = None;
        self.payload.clear();
        self.acknowledged = 0;
        self.sent = 0;
        self.sent_at.fill(0);
        self.ack_pending = false;
        // The original poll cursor survives a committed reset.
        Ok(true)
    }
    pub fn resume(
        self_peer: u32,
        protocol: u16,
        next: Context,
        roster: Vec<Seat>,
        previous: Accepted,
    ) -> Result<Self> {
        let before = previous.context;
        validate(self_peer, before, &roster)?;
        need(
            next.session == before.session
                && before.epoch.checked_add(1) == Some(next.epoch)
                && before.membership_revision.checked_add(1) == Some(next.membership_revision)
                && next.host != before.host
                && next.peers == before.peers & !(1u16 << before.host),
        )?;
        let payload = body(&previous.players, &roster)?;
        need(hash(before, &payload) == previous.digest)?;
        let mut result = Self::new(self_peer, protocol, next, roster)?;
        result.value = Some(Accepted {
            context: next,
            digest: hash(next, &payload),
            players: previous.players,
        });
        if self_peer == next.host {
            result.payload = payload;
        }
        Ok(result)
    }
    pub fn publish(&mut self, context: Context, players: Vec<Player>) -> Result<bool> {
        let bytes = body(&players, &self.roster)?;
        let digest = hash(context, &bytes);
        if context != self.binding || self.self_peer != context.host {
            return Ok(false);
        }
        let value = Accepted {
            context,
            digest,
            players,
        };
        if let Some(current) = &self.value {
            return Ok(*current == value);
        }
        self.value = Some(value);
        self.payload = bytes;
        Ok(true)
    }
    pub fn confirmed(&self, peer: u32) -> bool {
        self.self_peer == self.binding.host
            && self.value.is_some()
            && peer != self.self_peer
            && member(self.binding, peer)
            && self.acknowledged & (1u16 << peer) != 0
    }
    fn header(&self, target: u32, kind: u8, digest: [u8; 32]) -> Vec<u8> {
        let mut bytes = Vec::with_capacity(MAX_PACKET);
        bytes.extend_from_slice(b"H2N2");
        bytes.extend_from_slice(&self.protocol.to_le_bytes());
        bytes.extend_from_slice(&[8, 1, kind]);
        context_bytes(self.binding, &mut bytes);
        bytes.push(target as u8);
        bytes.extend_from_slice(&digest);
        bytes
    }
    pub fn poll(&mut self, now: u64, budget: u32) -> Vec<Packet> {
        let mut packets = Vec::new();
        let budget = budget.min(16) as usize;
        let Some(value) = &self.value else {
            return packets;
        };
        let digest = value.digest;
        if budget == 0 {
            return packets;
        }
        if self.self_peer != self.binding.host {
            if self.ack_pending {
                packets.push(Packet {
                    peer: self.binding.host,
                    bytes: self.header(self.binding.host, 1, digest),
                });
                self.ack_pending = false;
            }
            return packets;
        }
        for _ in 0..16 {
            if packets.len() >= budget {
                break;
            }
            let peer = self.cursor % 16;
            self.cursor = self.cursor.wrapping_add(1);
            if peer == self.self_peer
                || !member(self.binding, peer)
                || self.acknowledged & (1u16 << peer) != 0
            {
                continue;
            }
            if self.sent & (1u16 << peer) != 0
                && now
                    .checked_sub(self.sent_at[peer as usize])
                    .is_none_or(|age| age < 250)
            {
                continue;
            }
            let mut bytes = self.header(peer, 0, digest);
            bytes.extend_from_slice(&self.payload);
            packets.push(Packet { peer, bytes });
            self.sent |= 1u16 << peer;
            self.sent_at[peer as usize] = now;
        }
        packets
    }
    pub fn receive(&mut self, peer: u32, bytes: &[u8]) -> bool {
        if !is_packet(bytes) {
            return false;
        }
        let _ = self.receive_checked(peer, bytes);
        true
    }
    fn receive_checked(&mut self, peer: u32, bytes: &[u8]) -> Result<()> {
        need((HEADER..=MAX_PACKET).contains(&bytes.len()))?;
        let mut r = Reader { bytes, at: 0 };
        need(
            r.raw::<4>()? == *b"H2N2"
                && u16::from_le_bytes(r.raw()?) == self.protocol
                && r.byte()? == 8
                && r.byte()? == 1,
        )?;
        let kind = r.byte()?;
        need(kind <= 1)?;
        let c = r.context()?;
        let target = r.byte()? as u32;
        let digest = r.raw::<32>()?;
        need(
            c == self.binding
                && target == self.self_peer
                && peer != self.self_peer
                && member(c, peer),
        )?;
        if kind == 1 {
            need(
                r.at == bytes.len()
                    && self.self_peer == c.host
                    && self.value.as_ref().is_some_and(|v| v.digest == digest)
                    && self.sent & (1u16 << peer) != 0,
            )?;
            self.acknowledged |= 1u16 << peer;
        } else {
            need(peer == c.host && self.self_peer != c.host)?;
            let start = r.at;
            let players = r.players(&self.roster)?;
            need(hash(c, &bytes[start..]) == digest)?;
            let value = Accepted {
                context: c,
                digest,
                players,
            };
            if let Some(current) = &self.value {
                need(*current == value)?;
            } else {
                self.value = Some(value);
            }
            self.ack_pending = true;
        }
        Ok(())
    }
}

#[cfg(test)]
mod tests {
    use super::*;
    fn context() -> Context {
        Context {
            session: [0x42; 32],
            epoch: 3,
            membership_revision: 5,
            peers: 0x1111,
            host: 0,
        }
    }
    fn roster() -> Vec<Seat> {
        (0..16)
            .map(|slot| Seat {
                slot,
                peer: slot / 4 * 4,
                guest: slot % 4 != 0,
                account: [slot as u8; 8],
            })
            .collect()
    }
    fn players() -> Vec<Player> {
        (0..16)
            .map(|slot| Player {
                slot,
                player: 0x10000 + slot,
                identity: [slot as u8; 12],
            })
            .collect()
    }
    fn exchange(peer: u32) -> Exchange {
        Exchange::new(peer, 123, context(), roster()).unwrap()
    }
    #[test]
    fn roster_validation_keeps_physical_couch_and_full_width_unsigned_boundaries() {
        for bad in [1, 16, 255, 256, u32::MAX] {
            assert!(Exchange::new(bad, 123, context(), roster()).is_err());
        }
        let mut c = context();
        c.host = 256;
        assert!(Exchange::new(0, 123, c, roster()).is_err());
        let mut r = roster();
        r[4].guest = true;
        assert!(Exchange::new(0, 123, context(), r).is_err());
        let mut r = roster();
        r[4].peer = 0;
        assert!(Exchange::new(0, 123, context(), r).is_err());
        let mut c = context();
        c.peers = 1;
        assert!(Exchange::new(0, 123, c, roster()[..4].to_vec()).is_ok());
        let mut r = roster();
        for s in &mut r {
            s.peer = u32::MAX;
            s.guest = false;
        }
        let mut c = context();
        c.peers = u16::MAX;
        assert!(Exchange::new(15, 123, c, r).is_ok());
    }
    #[test]
    fn exact_retry_boundaries_rewind_and_ack_delivery_preserve_immutable_identity() {
        let mut host = exchange(0);
        let mut client = exchange(4);
        assert!(host.publish(context(), players()).unwrap());
        assert!(host.poll(1000, 0).is_empty());
        let offer = host.poll(1000, 1).remove(0);
        assert_eq!(offer.peer, 4);
        assert_eq!(offer.bytes.len(), 522);
        assert!(client.receive(0, &offer.bytes));
        let accepted = client.accepted().unwrap().clone();
        let ack = client.poll(1001, 1).remove(0);
        assert_eq!(ack.bytes.len(), 89);
        assert!(!host.confirmed(4));
        assert!(host.poll(999, 16).iter().all(|p| p.peer != 4));
        assert!(host.poll(1249, 16).iter().all(|p| p.peer != 4));
        assert!(host.poll(1250, 16).iter().any(|p| p.peer == 4));
        assert!(client.receive(0, &offer.bytes));
        assert_eq!(client.accepted(), Some(&accepted));
        host.receive(4, &ack.bytes);
        assert!(host.confirmed(4));
        assert!(!host.confirmed(256));
        let mut changed = players();
        changed[0].player ^= 0x10000;
        assert!(!host.publish(context(), changed).unwrap());
        assert!(host.poll(2000, 16).iter().all(|p| p.peer != 4));
    }
    #[test]
    fn acknowledgements_cannot_precede_an_actual_offer_or_change_the_fixed_sender() {
        let mut host = exchange(0);
        let mut producer = exchange(0);
        let mut client = exchange(4);
        host.publish(context(), players()).unwrap();
        producer.publish(context(), players()).unwrap();
        let offer = producer.poll(0, 1).remove(0);
        client.receive(0, &offer.bytes);
        let ack = client.poll(0, 1).remove(0);
        host.receive(4, &ack.bytes);
        assert!(!host.confirmed(4));
        host.poll(0, 1);
        for peer in [0, 1, 16, 256, u32::MAX] {
            host.receive(peer, &ack.bytes);
            assert!(!host.confirmed(4));
        }
        host.receive(4, &ack.bytes);
        assert!(host.confirmed(4));
    }
    #[test]
    fn recognized_malformed_frames_are_atomic_and_cannot_evict_or_overwrite() {
        let mut host = exchange(0);
        host.publish(context(), players()).unwrap();
        let offer = host.poll(0, 1).remove(0);
        for len in 0..offer.bytes.len() {
            let mut client = exchange(4);
            assert_eq!(client.receive(0, &offer.bytes[..len]), len >= 7);
            assert!(client.accepted().is_none());
        }
        let mut client = exchange(4);
        client.receive(0, &offer.bytes);
        let accepted = client.accepted().unwrap().clone();
        client.poll(0, 1);
        for at in 0..offer.bytes.len() {
            let mut changed = offer.bytes.clone();
            changed[at] ^= 0x80;
            client.receive(0, &changed);
            assert_eq!(client.accepted(), Some(&accepted));
        }
        assert!(client.poll(1, 16).is_empty());
    }
    #[test]
    fn wrapping_cursor_and_committed_reset_preserve_original_scan_state() {
        let mut host = exchange(0);
        host.publish(context(), players()).unwrap();
        host.cursor = u32::MAX - 3;
        let p = host.poll(0, 1);
        assert_eq!(p[0].peer, 12);
        assert_eq!(host.cursor, u32::MAX - 2);
        let mut next = context();
        next.epoch += 1;
        assert!(host.replace_context(context(), next).unwrap());
        assert_eq!(host.cursor, u32::MAX - 2);
        assert!(host.accepted().is_none());
        host.publish(next, players()).unwrap();
        assert_eq!(host.poll(0, 1)[0].peer, 4);
        assert_eq!(host.cursor, 5);
        assert!(!host.replace_context(context(), next).unwrap());
    }
    #[test]
    fn independently_retained_rows_survive_successive_host_loss_without_fabricated_receipts() {
        let mut host = exchange(0);
        host.publish(context(), players()).unwrap();
        let previous = host.accepted().unwrap().clone();
        let mut next = context();
        next.epoch += 1;
        next.membership_revision += 1;
        next.host = 4;
        next.peers &= !1;
        let successor = Exchange::resume(4, 123, next, roster(), previous.clone()).unwrap();
        let mut delayed = Exchange::resume(12, 123, next, roster(), previous.clone()).unwrap();
        assert_eq!(successor.accepted().unwrap().players, players());
        assert!(delayed.poll(0, 16).is_empty());
        assert!(!successor.confirmed(12));
        let mut last = next;
        last.epoch += 1;
        last.membership_revision += 1;
        last.host = 8;
        last.peers &= !(1 << 4);
        let final_owner =
            Exchange::resume(12, 123, last, roster(), delayed.accepted().unwrap().clone()).unwrap();
        assert_eq!(final_owner.accepted().unwrap().players, players());
        let mut corrupt = previous.clone();
        corrupt.digest[0] ^= 1;
        assert!(Exchange::resume(4, 123, next, roster(), corrupt).is_err());
        let mut invalid = next;
        invalid.peers = context().peers;
        assert!(Exchange::resume(4, 123, invalid, roster(), previous).is_err());
    }
    #[test]
    fn exhausted_successor_generations_cannot_wrap_to_a_new_identity_namespace() {
        for epoch in [false, true] {
            let mut c = context();
            if epoch {
                c.epoch = u32::MAX;
            } else {
                c.membership_revision = u64::MAX;
            }
            let mut host = Exchange::new(0, 123, c, roster()).unwrap();
            host.publish(c, players()).unwrap();
            let mut next = c;
            next.host = 4;
            next.peers &= !1;
            next.epoch = next.epoch.wrapping_add(1);
            next.membership_revision = next.membership_revision.wrapping_add(1);
            assert!(
                Exchange::resume(4, 123, next, roster(), host.accepted().unwrap().clone()).is_err()
            );
        }
    }
}
