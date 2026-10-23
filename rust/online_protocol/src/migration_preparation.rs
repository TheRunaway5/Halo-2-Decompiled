//! Exact-cut preparation barrier for authenticated physical peers.
//!
//! This checks metadata supplied by the established session. It does not
//! authenticate senders, retain native owners, elect a host, or certify that
//! the caller's preparation includes every game, renderer, input and audio
//! owner. Those obligations remain with the enclosing native transaction.

pub const PHYSICAL_PEERS: usize = 16;
pub const MAX_PREPARATION_BYTES: u64 = 64 * 1024 * 1024;

#[derive(Clone, Copy, Debug, PartialEq, Eq)]
pub struct PreparationCut {
    pub id: u64,
    pub tick: u64,
    pub bytes: u64,
    pub digest: [u8; 32],
}

impl PreparationCut {
    fn valid(self) -> bool {
        self.id != 0
            && self.bytes != 0
            && self.bytes <= MAX_PREPARATION_BYTES
            && self.digest.iter().any(|&byte| byte != 0)
    }
}

#[derive(Clone, Copy, Debug, PartialEq, Eq)]
pub enum PreparationError {
    InvalidHost,
    InvalidMembership,
    InvalidSelectedCut,
    InvalidPeerCount,
    MissingSurvivor(u8),
    InvalidSurvivorCut(u8),
    ConflictingSurvivorCut(u8),
}

/// Require one exact prepared cut from every fixed physical survivor.
///
/// The successor set must contain every preceding physical peer except the
/// departed host. Neither silence nor missing preparation may shrink it.
/// `prepared` is indexed by physical peer, never by local player/couch seat.
/// Entries outside the successor membership are irrelevant to this barrier.
pub fn all_survivors_prepared(
    previous_peers: u16,
    successor_peers: u16,
    old_host: u8,
    new_host: u8,
    selected: &PreparationCut,
    prepared: &[Option<PreparationCut>],
) -> Result<(), PreparationError> {
    if old_host >= PHYSICAL_PEERS as u8 || new_host >= PHYSICAL_PEERS as u8 {
        return Err(PreparationError::InvalidHost);
    }
    let old_bit = 1u16 << old_host;
    let new_bit = 1u16 << new_host;
    let expected_survivors = previous_peers & !old_bit;
    if previous_peers.count_ones() < 2
        || previous_peers & old_bit == 0
        || successor_peers == 0
        || successor_peers != expected_survivors
        || successor_peers & new_bit == 0
    {
        return Err(PreparationError::InvalidMembership);
    }
    if !selected.valid() {
        return Err(PreparationError::InvalidSelectedCut);
    }
    if prepared.len() != PHYSICAL_PEERS {
        return Err(PreparationError::InvalidPeerCount);
    }
    for (peer, ready) in prepared.iter().enumerate() {
        if successor_peers & (1u16 << peer) == 0 {
            continue;
        }
        let cut = ready.ok_or(PreparationError::MissingSurvivor(peer as u8))?;
        if !cut.valid() {
            return Err(PreparationError::InvalidSurvivorCut(peer as u8));
        }
        if cut != *selected {
            return Err(PreparationError::ConflictingSurvivorCut(peer as u8));
        }
    }
    Ok(())
}

#[cfg(test)]
mod tests {
    use super::*;

    const CUT: PreparationCut = PreparationCut {
        id: 7,
        tick: 128,
        bytes: 4096,
        digest: [0xa5; 32],
    };

    fn ready(peers: u16, cut: PreparationCut) -> [Option<PreparationCut>; PHYSICAL_PEERS] {
        std::array::from_fn(|peer| (peers & (1u16 << peer) != 0).then_some(cut))
    }

    #[test]
    fn candidate_only_cannot_stand_in_for_every_physical_survivor() {
        let prepared = ready(0b0010, CUT);
        assert_eq!(
            all_survivors_prepared(0b1111, 0b1110, 0, 1, &CUT, &prepared),
            Err(PreparationError::MissingSurvivor(2))
        );
    }

    #[test]
    fn exact_preparation_from_every_fixed_survivor_succeeds() {
        let prepared = ready(0b1110, CUT);
        assert_eq!(
            all_survivors_prepared(0b1111, 0b1110, 0, 1, &CUT, &prepared),
            Ok(())
        );
        // A two-PC match has one genuine physical survivor.
        assert_eq!(
            all_survivors_prepared(0x8001, 0x8000, 0, 15, &CUT, &ready(0x8000, CUT)),
            Ok(())
        );
        assert_eq!(
            all_survivors_prepared(u16::MAX, 0x7fff, 15, 0, &CUT, &ready(0x7fff, CUT)),
            Ok(())
        );
    }

    #[test]
    fn every_seal_field_and_each_survivor_must_match() {
        for peer in 1..4 {
            let mut prepared = ready(0b1110, CUT);
            prepared[peer] = None;
            assert_eq!(
                all_survivors_prepared(0b1111, 0b1110, 0, 1, &CUT, &prepared),
                Err(PreparationError::MissingSurvivor(peer as u8))
            );
            for conflict in [
                PreparationCut {
                    id: CUT.id + 1,
                    ..CUT
                },
                PreparationCut {
                    tick: CUT.tick + 1,
                    ..CUT
                },
                PreparationCut {
                    bytes: CUT.bytes + 1,
                    ..CUT
                },
                PreparationCut {
                    digest: [0x5a; 32],
                    ..CUT
                },
            ] {
                prepared[peer] = Some(conflict);
                assert_eq!(
                    all_survivors_prepared(0b1111, 0b1110, 0, 1, &CUT, &prepared),
                    Err(PreparationError::ConflictingSurvivorCut(peer as u8))
                );
            }
        }
    }

    #[test]
    fn invalid_cut_bounds_fail_for_selection_and_survivors() {
        for invalid in [
            PreparationCut { id: 0, ..CUT },
            PreparationCut { bytes: 0, ..CUT },
            PreparationCut {
                bytes: MAX_PREPARATION_BYTES + 1,
                ..CUT
            },
            PreparationCut {
                bytes: u64::MAX,
                ..CUT
            },
            PreparationCut {
                digest: [0; 32],
                ..CUT
            },
        ] {
            assert_eq!(
                all_survivors_prepared(3, 2, 0, 1, &invalid, &ready(2, invalid)),
                Err(PreparationError::InvalidSelectedCut)
            );
            assert_eq!(
                all_survivors_prepared(3, 2, 0, 1, &CUT, &ready(2, invalid)),
                Err(PreparationError::InvalidSurvivorCut(1))
            );
        }
        // Tick zero is a valid completed boundary; maximum encoded size is inclusive.
        let boundary = PreparationCut {
            tick: 0,
            bytes: MAX_PREPARATION_BYTES,
            ..CUT
        };
        assert_eq!(
            all_survivors_prepared(3, 2, 0, 1, &boundary, &ready(2, boundary)),
            Ok(())
        );
    }

    #[test]
    fn membership_cannot_drop_silent_peers_keep_old_host_or_add_outsiders() {
        for (previous, successor, old, new) in [
            (0, 0, 0, 1),
            (1, 0, 0, 1),
            (0b1110, 0b1110, 0, 1),
            (0b1111, 0b0010, 0, 1),
            (0b1111, 0b1111, 0, 1),
            (0b0111, 0b1110, 0, 1),
            (0b1111, 0b1110, 0, 0),
            (0b1111, 0b1110, 0, 15),
        ] {
            assert_eq!(
                all_survivors_prepared(previous, successor, old, new, &CUT, &ready(u16::MAX, CUT)),
                Err(PreparationError::InvalidMembership)
            );
        }
        for (old, new) in [(16, 1), (0, 16), (u8::MAX, 1), (0, u8::MAX)] {
            assert_eq!(
                all_survivors_prepared(3, 2, old, new, &CUT, &ready(2, CUT)),
                Err(PreparationError::InvalidHost)
            );
        }
    }

    #[test]
    fn physical_membership_never_counts_local_couch_seats() {
        // Three PCs may each own four seats. Only physical peers 0, 5 and 15
        // belong to this session; the surviving PCs each contribute one cut.
        let prepared = ready((1 << 5) | (1 << 15), CUT);
        assert_eq!(
            all_survivors_prepared(0x8021, 0x8020, 0, 5, &CUT, &prepared),
            Ok(())
        );
        // Extra local-seat positions cannot substitute for missing PC 15.
        let prepared = ready(0x0ffe, CUT);
        assert_eq!(
            all_survivors_prepared(0x8021, 0x8020, 0, 5, &CUT, &prepared),
            Err(PreparationError::MissingSurvivor(15))
        );
    }

    #[test]
    fn preparation_bank_must_have_all_sixteen_physical_slots() {
        let prepared = ready(2, CUT);
        for length in 0..PHYSICAL_PEERS {
            assert_eq!(
                all_survivors_prepared(3, 2, 0, 1, &CUT, &prepared[..length]),
                Err(PreparationError::InvalidPeerCount)
            );
        }
        let prepared = [Some(CUT); PHYSICAL_PEERS + 1];
        assert_eq!(
            all_survivors_prepared(3, 2, 0, 1, &CUT, &prepared),
            Err(PreparationError::InvalidPeerCount)
        );
    }
}
