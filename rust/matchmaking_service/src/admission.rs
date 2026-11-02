//! Bounded independent source, prefix and principal budgets for Rust HTTP.
use halo2_online_protocol::matchmaking_model::{Error, Result};
#[cfg(test)]
use std::net::Ipv4Addr;
use std::{
    collections::{BTreeMap, HashMap},
    net::{IpAddr, Ipv6Addr},
};
#[derive(Default)]
struct Table {
    rows: HashMap<String, (f64, f64, u64)>,
    order: BTreeMap<u64, String>,
    next: u64,
}
impl Table {
    fn charge(
        &mut self,
        key: &str,
        refill: f64,
        burst: f64,
        now: f64,
        capacity: usize,
    ) -> Result<()> {
        let (tokens, previous, old) = self.rows.get(key).copied().unwrap_or((burst, now, 0));
        let now = now.max(previous);
        let tokens = (tokens + (now - previous) * refill / 60.).min(burst);
        if tokens < 1. {
            return Err(Error(429, "Rate limit exceeded"));
        }
        let serial = self
            .next
            .checked_add(1)
            .ok_or(Error(503, "Request budget generation exhausted"))?;
        if old != 0 {
            self.order.remove(&old);
        } else if self.rows.len() == capacity {
            let (seq, name) = self
                .order
                .pop_first()
                .ok_or(Error(503, "Request budget unavailable"))?;
            let _ = seq;
            self.rows.remove(&name);
        }
        self.next = serial;
        self.order.insert(serial, key.to_owned());
        self.rows.insert(key.to_owned(), (tokens - 1., now, serial));
        Ok(())
    }
}
#[derive(Default)]
pub struct Admission {
    source: Table,
    prefix: Table,
    principal: Table,
    global: Table,
    eos: Table,
    steam: Table,
    steam_devices: Table,
}
pub fn prefix(ip: IpAddr, bits: u32) -> String {
    match ip {
        IpAddr::V4(v) => format!("{v}/32"),
        IpAddr::V6(v) => {
            if let Some(v) = v.to_ipv4_mapped() {
                format!("{v}/32")
            } else {
                let mask = if bits == 128 {
                    u128::MAX
                } else {
                    u128::MAX << (128 - bits)
                };
                format!("{}/{}", Ipv6Addr::from(u128::from(v) & mask), bits)
            }
        }
    }
}
impl Admission {
    pub fn steam(&mut self, kind: &str, key: &str, now: f64) -> Result<()> {
        if kind == "global" {
            return self.global.charge("steam-start", 60., 16., now, 32);
        }
        let (rate, burst) = match kind {
            "start" => (20., 20.),
            "global" => (60., 16.),
            "callback" => (40., 40.),
            "poll" => (600., 600.),
            "device" => (120., 120.),
            "resume" | "forget" => (30., 30.),
            _ => return Err(Error(503, "Steam request budget unavailable")),
        };
        let table = if kind == "device" {
            &mut self.steam_devices
        } else {
            &mut self.steam
        };
        table.charge(&format!("{kind}:{key}"), rate, burst, now, 4096)
    }
    pub fn eos_room(&mut self, player: &str, now: f64) -> Result<()> {
        self.eos.charge(player, 120., 120., now, 4096)
    }
    pub fn request(&mut self, ip: IpAddr, now: f64) -> Result<()> {
        self.prefix
            .charge(&prefix(ip, 64), 1200., 512., now, 1024)?;
        self.global.charge("requests", 6000., 2048., now, 32)?;
        self.source
            .charge(&format!("ip:{ip}"), 600., 600., now, 4096)
    }
    pub fn owner_source(&mut self, ip: IpAddr, now: f64) -> Result<()> {
        self.source
            .charge(&format!("owner:{ip}"), 20., 20., now, 4096)
    }
    pub fn owner(&mut self, provider: &str, subject: &str, now: f64) -> Result<()> {
        self.principal.charge(
            &format!("owner-id:{provider}:{subject}"),
            20.,
            20.,
            now,
            4096,
        )
    }
}
#[cfg(test)]
mod tests {
    use super::*;
    #[test]
    fn churn_does_not_refill_principals_and_ipv6_aggregation_precedes_exact_sources() {
        let mut a = Admission::default();
        for _ in 0..20 {
            a.owner("steam", "one", 0.).unwrap();
        }
        for n in 0..5000 {
            a.source
                .charge(&n.to_string(), 600., 600., 0., 4096)
                .unwrap();
        }
        assert_eq!(a.owner("steam", "one", 0.).unwrap_err().0, 429);
        let mut a = Admission::default();
        for n in 0..512 {
            a.request(
                IpAddr::V6(Ipv6Addr::from((0x20010db800000000u128 << 64) | n)),
                0.,
            )
            .unwrap();
        }
        assert_eq!(
            a.request("2001:db8::ffff".parse().unwrap(), 0.)
                .unwrap_err()
                .0,
            429
        );
        assert_eq!(
            prefix("::ffff:192.0.2.1".parse().unwrap(), 64),
            prefix(IpAddr::V4(Ipv4Addr::new(192, 0, 2, 1)), 64)
        );
    }
}
