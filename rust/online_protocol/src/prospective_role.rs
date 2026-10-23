//! Checked metadata for private prospective host and role-neutral client roles.
//!
//! This describes what the existing host agreement's exact successor context
//! would be. It contains no decision, ballot, vote, readiness, authorization or
//! native ownership. Only an independently learned real certificate can later
//! authorize a role change; the native owner must bind it to this exact value.

pub use crate::host_agreement::{Context, Cut};
use crate::migration_preparation::MAX_PREPARATION_BYTES;

#[derive(Clone, Copy, Debug, PartialEq, Eq)]
pub struct Descriptor {
    pub previous: Context,
    pub next: Context,
    pub cut: Cut,
}

/// The resident client alternative has no chosen successor endpoint until an
/// independently learned certificate supplies one. This owns no replica.
#[derive(Clone, Copy, Debug, PartialEq, Eq)]
pub struct ClientDescriptor {
    pub previous: Context,
    pub cut: Cut,
    pub self_peer: u8,
    pub next_epoch: u32,
    pub next_membership_revision: u64,
    pub survivors: u16,
}

#[derive(Clone, Copy, Debug, PartialEq, Eq)]
pub enum Error {
    InvalidContext,
    InvalidSelf,
    InvalidCut,
    EpochExhausted,
    MembershipRevisionExhausted,
    NoClientAlternative,
    InvalidClientRole,
    InvalidClientEndpoint,
}
impl std::fmt::Display for Error {
    fn fmt(&self, f: &mut std::fmt::Formatter<'_>) -> std::fmt::Result {
        f.write_str(match self {
            Self::InvalidContext => "Invalid prospective-role predecessor context",
            Self::InvalidSelf => "Prospective host is not a fixed physical survivor",
            Self::InvalidCut => "Invalid prospective-role selected cut",
            Self::EpochExhausted => "Prospective-role epoch is exhausted",
            Self::MembershipRevisionExhausted => {
                "Prospective-role membership revision is exhausted"
            }
            Self::NoClientAlternative => "Prospective client has no other fixed survivor host",
            Self::InvalidClientRole => {
                "Prospective client metadata differs from its exact derivation"
            }
            Self::InvalidClientEndpoint => {
                "Prospective client endpoint differs from the certificate metadata"
            }
        })
    }
}
impl std::error::Error for Error {}

pub fn derive(previous: Context, selected: Cut, self_peer: u8) -> Result<Descriptor, Error> {
    if previous.host >= 16
        || previous.peers.count_ones() < 2
        || previous.peers & (1u16 << previous.host) == 0
        || previous.session.iter().all(|&byte| byte == 0)
    {
        return Err(Error::InvalidContext);
    }
    let survivors = previous.peers & !(1u16 << previous.host);
    if self_peer >= 16 || survivors & (1u16 << self_peer) == 0 {
        return Err(Error::InvalidSelf);
    }
    if selected.id == 0
        || selected.bytes == 0
        || selected.bytes > MAX_PREPARATION_BYTES
        || selected.digest.iter().all(|&byte| byte == 0)
    {
        return Err(Error::InvalidCut);
    }
    let next = Context {
        session: previous.session,
        epoch: previous.epoch.checked_add(1).ok_or(Error::EpochExhausted)?,
        membership_revision: previous
            .membership_revision
            .checked_add(1)
            .ok_or(Error::MembershipRevisionExhausted)?,
        peers: survivors,
        host: self_peer,
    };
    Ok(Descriptor {
        previous,
        next,
        cut: selected,
    })
}

pub fn derive_client(
    previous: Context,
    selected: Cut,
    self_peer: u8,
) -> Result<ClientDescriptor, Error> {
    let derived = derive(previous, selected, self_peer)?;
    if derived.next.peers.count_ones() < 2 {
        return Err(Error::NoClientAlternative);
    }
    Ok(ClientDescriptor {
        previous,
        cut: selected,
        self_peer,
        next_epoch: derived.next.epoch,
        next_membership_revision: derived.next.membership_revision,
        survivors: derived.next.peers,
    })
}

/// Validate only the actual certificate's endpoint metadata. The caller must
/// separately compare its full independently learned certificate, including
/// votes/ballot, and retain the genuine native owners. No Decision is created.
pub fn bind_client(
    role: ClientDescriptor,
    previous: Context,
    next: Context,
    value: crate::host_agreement::Value,
) -> Result<Context, Error> {
    if derive_client(role.previous, role.cut, role.self_peer)? != role {
        return Err(Error::InvalidClientRole);
    }
    if previous != role.previous
        || value.cut != role.cut
        || value.host >= 16
        || value.host == role.self_peer
        || role.survivors & (1u16 << value.host) == 0
    {
        return Err(Error::InvalidClientEndpoint);
    }
    let expected = Context {
        session: role.previous.session,
        epoch: role.next_epoch,
        membership_revision: role.next_membership_revision,
        peers: role.survivors,
        host: value.host,
    };
    if next != expected {
        return Err(Error::InvalidClientEndpoint);
    }
    Ok(next)
}

#[cfg(test)]
mod tests {
    use super::*;

    fn context() -> Context {
        let mut session = [0; 32];
        session[31] = 0x80;
        Context {
            session,
            epoch: 0xfedc_ba98,
            membership_revision: 0xfedc_ba98_7654_3210,
            peers: 0x111,
            host: 0,
        }
    }
    fn cut() -> Cut {
        Cut {
            id: u64::MAX,
            tick: 0x9abc_def0_1234_5678,
            bytes: MAX_PREPARATION_BYTES,
            digest: [0xa5; 32],
        }
    }

    #[test]
    fn descriptor_preserves_exact_full_width_predecessor_and_cut() {
        let previous = context();
        let selected = cut();
        let descriptor = derive(previous, selected, 8).unwrap();
        assert_eq!(descriptor.previous, previous);
        assert_eq!(descriptor.cut, selected);
        assert_eq!(
            descriptor.next,
            Context {
                epoch: previous.epoch + 1,
                membership_revision: previous.membership_revision + 1,
                peers: 0x110,
                host: 8,
                ..previous
            }
        );
        assert_eq!(derive(previous, selected, 8), Ok(descriptor));
    }

    #[test]
    fn exact_physical_successor_never_counts_couch_seats_or_shrinks_survivors() {
        let selected = cut();
        for host in 0..16 {
            for self_peer in 0..16 {
                let previous = Context {
                    peers: u16::MAX,
                    host,
                    ..context()
                };
                if self_peer == host {
                    assert_eq!(
                        derive(previous, selected, self_peer),
                        Err(Error::InvalidSelf)
                    );
                    continue;
                }
                let value = derive(previous, selected, self_peer).unwrap();
                assert_eq!(value.next.peers, !(1u16 << host));
                assert_eq!(value.next.host, self_peer);
                assert_eq!(value.next.peers.count_ones(), 15);
            }
        }
        for self_peer in [1, 2, 3, 5, 6, 7, 9, 16, 255] {
            assert_eq!(
                derive(context(), selected, self_peer),
                Err(Error::InvalidSelf)
            );
        }
        let previous = Context {
            peers: 0x8001,
            host: 15,
            ..context()
        };
        assert_eq!(derive(previous, selected, 0).unwrap().next.peers, 1);
    }

    #[test]
    fn invalid_predecessor_and_indices_fail_before_any_shift_or_advance() {
        for previous in [
            Context {
                host: 16,
                ..context()
            },
            Context {
                host: 255,
                ..context()
            },
            Context {
                host: 1,
                ..context()
            },
            Context {
                peers: 1,
                ..context()
            },
            Context {
                peers: 0,
                ..context()
            },
            Context {
                session: [0; 32],
                ..context()
            },
        ] {
            assert_eq!(derive(previous, cut(), 8), Err(Error::InvalidContext));
        }
        for self_peer in [0, 16, 255] {
            assert_eq!(derive(context(), cut(), self_peer), Err(Error::InvalidSelf));
        }
    }

    #[test]
    fn checked_epoch_and_revision_never_wrap_or_change_other_fields() {
        assert_eq!(
            derive(
                Context {
                    epoch: u32::MAX,
                    ..context()
                },
                cut(),
                4
            ),
            Err(Error::EpochExhausted)
        );
        assert_eq!(
            derive(
                Context {
                    membership_revision: u64::MAX,
                    ..context()
                },
                cut(),
                4
            ),
            Err(Error::MembershipRevisionExhausted)
        );
        let previous = Context {
            epoch: u32::MAX - 1,
            membership_revision: u64::MAX - 1,
            ..context()
        };
        let descriptor = derive(previous, cut(), 4).unwrap();
        assert_eq!(descriptor.next.epoch, u32::MAX);
        assert_eq!(descriptor.next.membership_revision, u64::MAX);
        assert_eq!(descriptor.previous, previous);
    }

    #[test]
    fn complete_seal_bounds_and_zero_tick_are_checked_without_relabeling() {
        for selected in [
            Cut { id: 0, ..cut() },
            Cut { bytes: 0, ..cut() },
            Cut {
                bytes: MAX_PREPARATION_BYTES + 1,
                ..cut()
            },
            Cut {
                digest: [0; 32],
                ..cut()
            },
        ] {
            assert_eq!(derive(context(), selected, 4), Err(Error::InvalidCut));
        }
        let mut selected = Cut {
            id: 1,
            tick: 0,
            bytes: 1,
            digest: [0; 32],
        };
        selected.digest[31] = 0x80;
        assert_eq!(derive(context(), selected, 4).unwrap().cut, selected);
    }

    #[test]
    fn client_role_is_host_neutral_and_binds_exact_full_width_metadata_late() {
        let previous = Context {
            epoch: u32::MAX - 1,
            membership_revision: u64::MAX - 1,
            ..context()
        };
        let role = derive_client(previous, cut(), 4).unwrap();
        assert_eq!(
            role,
            ClientDescriptor {
                previous,
                cut: cut(),
                self_peer: 4,
                next_epoch: u32::MAX,
                next_membership_revision: u64::MAX,
                survivors: 0x110
            }
        );
        let next = derive(previous, cut(), 8).unwrap().next;
        assert_eq!(
            bind_client(
                role,
                previous,
                next,
                crate::host_agreement::Value {
                    host: 8,
                    cut: cut()
                }
            ),
            Ok(next)
        );
        assert_eq!(role.previous, previous);
        assert_eq!(role.cut, cut());
    }

    #[test]
    fn client_alternative_requires_another_physical_survivor_and_never_shrinks_membership() {
        for old in 0..16 {
            for self_peer in 0..16 {
                if old == self_peer {
                    continue;
                }
                let previous = Context {
                    peers: u16::MAX,
                    host: old,
                    ..context()
                };
                let role = derive_client(previous, cut(), self_peer).unwrap();
                assert_eq!(role.survivors, !(1u16 << old));
                let candidate = (0..16)
                    .find(|&peer| peer != old && peer != self_peer)
                    .unwrap();
                let next = derive(previous, cut(), candidate).unwrap().next;
                let value = crate::host_agreement::Value {
                    host: candidate,
                    cut: cut(),
                };
                assert_eq!(bind_client(role, previous, next, value), Ok(next));
                let shrunken = Context {
                    peers: 1u16 << self_peer,
                    ..next
                };
                assert!(bind_client(role, previous, shrunken, value).is_err());
            }
        }
        let previous = Context {
            peers: 0x8001,
            host: 15,
            ..context()
        };
        assert!(derive(previous, cut(), 0).is_ok()); // The host-only alternative stays valid.
        assert_eq!(
            derive_client(previous, cut(), 0),
            Err(Error::NoClientAlternative)
        );
        for peer in [0, 1, 16, 255] {
            assert!(derive_client(context(), cut(), peer).is_err());
        }
    }

    #[test]
    fn client_derivation_preserves_existing_cut_context_and_exhaustion_bounds() {
        for previous in [
            Context {
                epoch: u32::MAX,
                ..context()
            },
            Context {
                membership_revision: u64::MAX,
                ..context()
            },
            Context {
                session: [0; 32],
                ..context()
            },
            Context {
                peers: 0,
                ..context()
            },
            Context {
                host: 255,
                ..context()
            },
        ] {
            assert!(derive_client(previous, cut(), 4).is_err());
        }
        for selected in [
            Cut { id: 0, ..cut() },
            Cut { bytes: 0, ..cut() },
            Cut {
                bytes: MAX_PREPARATION_BYTES + 1,
                ..cut()
            },
            Cut {
                digest: [0; 32],
                ..cut()
            },
        ] {
            assert!(derive_client(context(), selected, 4).is_err());
        }
        assert_eq!(
            derive_client(context(), Cut { tick: 0, ..cut() }, 4)
                .unwrap()
                .cut
                .tick,
            0
        );
    }

    #[test]
    fn late_client_binding_rederives_every_role_field_without_trusting_mutable_metadata() {
        let role = derive_client(context(), cut(), 4).unwrap();
        let next = derive(context(), cut(), 8).unwrap().next;
        let value = crate::host_agreement::Value {
            host: 8,
            cut: cut(),
        };
        for field in 0..14 {
            let mut changed = role;
            match field {
                0 => changed.previous.session[31] ^= 1,
                1 => changed.previous.epoch -= 1,
                2 => changed.previous.membership_revision -= 1,
                3 => changed.previous.peers ^= 2,
                4 => changed.previous.host = 4,
                5 => changed.cut.id -= 1,
                6 => changed.cut.tick -= 1,
                7 => changed.cut.bytes -= 1,
                8 => changed.cut.digest[31] ^= 1,
                9 => changed.self_peer = 8,
                10 => changed.next_epoch -= 1,
                11 => changed.next_membership_revision -= 1,
                12 => changed.survivors ^= 2,
                _ => changed.self_peer = 255,
            }
            assert!(bind_client(changed, context(), next, value).is_err());
        }
    }

    #[test]
    fn late_client_binding_requires_every_actual_previous_next_and_value_field() {
        let role = derive_client(context(), cut(), 4).unwrap();
        let next = derive(context(), cut(), 8).unwrap().next;
        let value = crate::host_agreement::Value {
            host: 8,
            cut: cut(),
        };
        for field in 0..15 {
            let mut previous = context();
            let mut successor = next;
            let mut selected = value;
            match field {
                0 => previous.session[31] ^= 1,
                1 => previous.epoch -= 1,
                2 => previous.membership_revision -= 1,
                3 => previous.peers ^= 2,
                4 => previous.host = 4,
                5 => successor.session[31] ^= 1,
                6 => successor.epoch -= 1,
                7 => successor.membership_revision -= 1,
                8 => successor.peers ^= 2,
                9 => successor.host = 4,
                10 => selected.host = 4,
                11 => selected.cut.id -= 1,
                12 => selected.cut.tick -= 1,
                13 => selected.cut.bytes -= 1,
                _ => selected.cut.digest[31] ^= 1,
            }
            assert!(bind_client(role, previous, successor, selected).is_err());
        }
        for host in [0, 1, 4, 16, 255] {
            assert!(
                bind_client(
                    role,
                    context(),
                    next,
                    crate::host_agreement::Value { host, ..value }
                )
                .is_err()
            );
        }
    }
}
