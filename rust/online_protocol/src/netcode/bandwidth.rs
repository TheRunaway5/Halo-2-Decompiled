//! Per-client channel budget modeled on the retail Halo 2 network observer.
//!
//! Defaults come from the original configuration initializer `0x66330`
//! (observer block `0x4CF4E0`, see `tools/reference/halo2-decompiled/docs/
//! unknown_078900.md`): a new channel starts at 30720 bits/s, never drops
//! below 4096, probes upward by min(3072, 20%) per 2000 ms round only while
//! demand exceeds the budget, and backs off by 0.8 when more than 10% of a
//! 32-outcome window is lost, holding for four round-trip delays. This is a
//! behavioral model of those constants, not the retail controller's code.

#[derive(Clone, Copy, Debug, PartialEq)]
pub struct Profile {
    pub start_bps: u32,
    pub floor_bps: u32,
    pub ceiling_bps: u32,
    pub unlimited: bool,
    /// Carry WORLD poses losslessly instead of quantized and prioritized.
    pub exact: bool,
}

impl Profile {
    /// Retail new-channel budget, minimum budget and probe ceiling.
    pub const ORIGINAL: Self =
        Self { start_bps: 30_720, floor_bps: 4_096, ceiling_bps: 71_680, unlimited: false, exact: false };
    /// Retail observer's operational cap (1 Mbit/s) for broadband peers.
    pub const BROADBAND: Self =
        Self { start_bps: 262_144, floor_bps: 30_720, ceiling_bps: 1_048_576, unlimited: false, exact: false };
    /// Every tick, every byte exact: client state matches the host's.
    pub const UNLIMITED: Self =
        Self { start_bps: u32::MAX, floor_bps: u32::MAX, ceiling_bps: u32::MAX, unlimited: true, exact: true };

    /// `original`, `broadband`, `unlimited`, or `start[,floor[,ceiling]]` in bits/s.
    pub fn parse(text: &str) -> Option<Self> {
        match text.trim() {
            "" | "original" | "og" | "xbox" => return Some(Self::ORIGINAL),
            "broadband" => return Some(Self::BROADBAND),
            "unlimited" => return Some(Self::UNLIMITED),
            _ => {}
        }
        let values: Vec<u32> = text.split(',').map(|v| v.trim().parse().ok()).collect::<Option<_>>()?;
        let start = *values.first()?;
        let floor = values.get(1).copied().unwrap_or(start.min(Self::ORIGINAL.floor_bps));
        let ceiling = values.get(2).copied().unwrap_or(start);
        (values.len() <= 3 && floor >= 1_024 && floor <= start && start <= ceiling)
            .then_some(Self { start_bps: start, floor_bps: floor, ceiling_bps: ceiling, unlimited: false, exact: false })
    }
}

const ROUND_MS: u64 = 2_000;
const WINDOW: usize = 32;
const LOSS_LIMIT: f64 = 0.1;
const BACKOFF: f64 = 0.8;
const BACKOFF_HOLD_DELAYS: u64 = 4;
const PROBE_STEP_MAX: u32 = 3_072;
const PROBE_STEP_FRACTION: f64 = 0.2;
const DEFAULT_RTT_MS: u64 = 250;
const DEBT_SECONDS: f64 = 0.25;

pub struct Bandwidth {
    profile: Profile,
    budget_bps: u32,
    tokens: f64,
    last_ms: Option<u64>,
    round_start_ms: u64,
    demand_limited: bool,
    outcomes: [bool; WINDOW],
    outcome_count: usize,
    outcome_next: usize,
    hold_until_ms: u64,
    rtt_ms: Option<u64>,
}

impl Bandwidth {
    pub fn new(profile: Profile) -> Self {
        let mut value = Self {
            profile,
            budget_bps: profile.start_bps,
            tokens: 0.0,
            last_ms: None,
            round_start_ms: 0,
            demand_limited: false,
            outcomes: [true; WINDOW],
            outcome_count: 0,
            outcome_next: 0,
            hold_until_ms: 0,
            rtt_ms: None,
        };
        value.tokens = value.burst();
        value
    }

    pub fn budget_bps(&self) -> u32 {
        self.budget_bps
    }

    pub fn unlimited(&self) -> bool {
        self.profile.unlimited
    }

    pub fn rtt_ms(&self) -> Option<u64> {
        self.rtt_ms
    }

    fn bytes_per_second(&self) -> f64 {
        f64::from(self.budget_bps) / 8.0
    }

    fn burst(&self) -> f64 {
        (self.bytes_per_second() * 0.2).max(1_500.0)
    }

    pub fn refill(&mut self, now_ms: u64) {
        if self.profile.unlimited {
            return;
        }
        let elapsed = self.last_ms.map_or(0, |last| now_ms.saturating_sub(last));
        self.last_ms = Some(now_ms);
        self.tokens = (self.tokens + elapsed as f64 * self.bytes_per_second() / 1_000.0).min(self.burst());
        if self.round_start_ms == 0 {
            self.round_start_ms = now_ms.max(1);
        } else if now_ms.saturating_sub(self.round_start_ms) >= ROUND_MS {
            self.adapt(now_ms);
            self.round_start_ms = now_ms.max(1);
        }
    }

    /// Bytes this message may use; `None` skips the tick entirely.
    pub fn allowance(&self) -> Option<usize> {
        if self.profile.unlimited {
            return Some(usize::MAX);
        }
        (self.tokens > 0.0).then_some(self.tokens as usize)
    }

    /// Allowance plus half of the permitted debt, for one overdue refresh.
    pub fn overdraft(&self, allowance: usize) -> usize {
        if self.profile.unlimited {
            return usize::MAX;
        }
        allowance.saturating_add((self.bytes_per_second() * DEBT_SECONDS / 2.0) as usize)
    }

    /// Records demand the budget could not serve this round.
    pub fn limited(&mut self) {
        self.demand_limited = true;
    }

    /// Debits actual wire bytes. Debt is capped so one large keyframe delays
    /// later messages by at most a quarter second of budget.
    pub fn charge(&mut self, bytes: usize) {
        if self.profile.unlimited {
            return;
        }
        self.tokens = (self.tokens - bytes as f64).max(-self.bytes_per_second() * DEBT_SECONDS);
    }

    pub fn outcome(&mut self, delivered: bool) {
        self.outcomes[self.outcome_next] = delivered;
        self.outcome_next = (self.outcome_next + 1) % WINDOW;
        self.outcome_count = (self.outcome_count + 1).min(WINDOW);
    }

    pub fn rtt_sample(&mut self, sample_ms: u64) {
        // Original smoothing shift 3 for delay.
        self.rtt_ms = Some(match self.rtt_ms {
            None => sample_ms,
            Some(old) => old - (old >> 3) + (sample_ms >> 3),
        });
    }

    pub fn loss(&self) -> f64 {
        if self.outcome_count == 0 {
            return 0.0;
        }
        let lost = self.outcomes.iter().take(self.outcome_count).filter(|v| !**v).count();
        lost as f64 / self.outcome_count as f64
    }

    fn adapt(&mut self, now_ms: u64) {
        let budget = f64::from(self.budget_bps);
        if self.outcome_count >= WINDOW / 4 && self.loss() > LOSS_LIMIT {
            self.budget_bps = ((budget * BACKOFF) as u32).max(self.profile.floor_bps);
            let delay = self.rtt_ms.unwrap_or(DEFAULT_RTT_MS).max(ROUND_MS / 8);
            self.hold_until_ms = now_ms + BACKOFF_HOLD_DELAYS * delay;
            self.outcome_count = 0;
        } else if self.demand_limited && now_ms >= self.hold_until_ms {
            let step = ((budget * PROBE_STEP_FRACTION) as u32).clamp(1, PROBE_STEP_MAX);
            self.budget_bps = self.budget_bps.saturating_add(step).min(self.profile.ceiling_bps);
        }
        self.demand_limited = false;
    }
}

#[cfg(test)]
mod tests {
    use super::*;

    #[test]
    fn profiles_parse_named_and_explicit_values() {
        assert_eq!(Profile::parse("original"), Some(Profile::ORIGINAL));
        assert_eq!(Profile::parse(""), Some(Profile::ORIGINAL));
        assert_eq!(Profile::parse("unlimited"), Some(Profile::UNLIMITED));
        assert_eq!(
            Profile::parse("64000,8000,128000"),
            Some(Profile { start_bps: 64_000, floor_bps: 8_000, ceiling_bps: 128_000, unlimited: false, exact: false })
        );
        assert_eq!(Profile::parse("64000").unwrap().ceiling_bps, 64_000);
        assert!(Profile::parse("100").is_none());
        assert!(Profile::parse("9000,10000").is_none());
        assert!(Profile::parse("fast").is_none());
    }

    #[test]
    fn token_bucket_enforces_the_average_rate() {
        let mut b = Bandwidth::new(Profile::ORIGINAL);
        let mut sent = 0usize;
        for tick in 0..300u64 {
            let now = tick * 1000 / 30;
            b.refill(now);
            if let Some(allowance) = b.allowance() {
                let size = allowance.max(200);
                b.charge(size);
                sent += size;
            }
        }
        // Ten seconds at 30720 bits/s plus the initial burst.
        let ceiling = 10.0 * 30_720.0 / 8.0 + 1_500.0 + 400.0;
        assert!((sent as f64) < ceiling, "sent {sent}");
        assert!((sent as f64) > 10.0 * 30_720.0 / 8.0 * 0.8, "sent {sent}");
    }

    #[test]
    fn loss_backs_off_and_demand_probes_within_limits() {
        let mut b = Bandwidth::new(Profile::ORIGINAL);
        b.refill(1);
        for n in 0..32 {
            b.outcome(n % 4 != 0);
        }
        b.refill(2_100);
        assert_eq!(b.budget_bps(), (30_720.0 * 0.8) as u32);
        let mut now = 2_100;
        for _ in 0..200 {
            now += 2_000;
            for _ in 0..32 {
                b.outcome(true);
            }
            b.limited();
            b.refill(now);
        }
        assert_eq!(b.budget_bps(), Profile::ORIGINAL.ceiling_bps);
        for _ in 0..200 {
            now += 2_000;
            for _ in 0..32 {
                b.outcome(false);
            }
            b.refill(now);
        }
        assert_eq!(b.budget_bps(), Profile::ORIGINAL.floor_bps);
    }

    #[test]
    fn unlimited_never_skips() {
        let mut b = Bandwidth::new(Profile::UNLIMITED);
        b.refill(0);
        b.charge(1 << 30);
        assert_eq!(b.allowance(), Some(usize::MAX));
    }
}
