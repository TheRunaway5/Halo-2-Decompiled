//! Native ports of the original team-balancing kernels (retail 91FC0/91E30/
//! 91F00; authenticated TU1.5 correspondence and whole-function controls are
//! retained by tools/oracle_tu15_team_balance.py). No matchmaking identities,
//! policy defaults, downloaded rotations or network measurements originate here.

#[derive(Clone, Debug)]
struct Player { weight: i32, index: usize }

#[derive(Clone, Debug, Default)]
struct Party { flagged: bool, id: usize, players: Vec<Player>, total: i32 }
impl Party {
    fn add(&mut self, player: Player) {
        let index=self.players.iter().position(|p|p.weight<player.weight).unwrap_or(self.players.len());
        self.total=self.total.wrapping_add(player.weight);
        self.players.insert(index,player);
    }
    fn remove(&mut self, player: &Player) {
        let index=self.players.iter().position(|p|p.index==player.index).expect("retained balance player");
        self.players.remove(index);
        self.total=self.total.wrapping_sub(player.weight);
    }
}

#[derive(Clone, Debug, Default)]
struct Team { parties: Vec<Party>, players: Party }
impl Team {
    fn add(&mut self, party: Party) {
        for player in &party.players {self.players.add(player.clone());}
        self.parties.push(party);
    }
    fn remove(&mut self, party: &Party) {
        let index=self.parties.iter().position(|p|p.id==party.id).expect("retained balance party");
        self.parties.remove(index);
        for player in &party.players {self.players.remove(player);}
    }
    fn remove_largest(&mut self) -> Party {
        let mut largest=0;
        for i in 1..self.parties.len() {
            let a=&self.parties[i];let b=&self.parties[largest];
            if a.players.len()>b.players.len() || (a.players.len()==b.players.len() && a.total>b.total) {largest=i;}
        }
        let party=self.parties[largest].clone();self.remove(&party);party
    }
}

#[derive(Clone, Debug)]
struct Solution { teams: Vec<Team> }
impl Solution {
    fn count_spread(&self) -> usize {
        let counts=self.teams.iter().map(|t|t.players.players.len());
        counts.clone().max().unwrap()-counts.min().unwrap()
    }
    fn cost(&self) -> i32 {
        let totals=self.teams.iter().map(|t|t.players.total);
        let spread=totals.clone().max().unwrap().wrapping_sub(totals.min().unwrap());
        let largest=self.teams.iter().map(|t|t.players.players.len()).max().unwrap();
        let mut ranks=0i32;
        for rank in 0..largest {
            let weights=self.teams.iter().map(|t|t.players.players.get(rank).map_or(0,|p|p.weight));
            ranks=ranks.wrapping_add(weights.clone().max().unwrap().wrapping_sub(weights.min().unwrap()));
        }
        spread.wrapping_mul(7).wrapping_add(ranks.wrapping_mul(3))
    }
    fn smallest(&self) -> usize {
        let mut smallest=0;
        for i in 1..self.teams.len() {
            let a=&self.teams[i].players;let b=&self.teams[smallest].players;
            if a.players.len()<b.players.len() || (a.players.len()==b.players.len() && a.total<b.total) {smallest=i;}
        }
        smallest
    }
    fn improve(&mut self) -> bool {
        let mut best: Option<Self>=None;
        for a in 0..self.teams.len()-1 {
            for b in a+1..self.teams.len() {
                let left=&self.teams[a];let right=&self.teams[b];
                let difference=(left.players.players.len() as i32-right.players.players.len() as i32).abs();
                for party_a in &left.parties {
                    for party_b in &right.parties {
                        if party_a.flagged!=party_b.flagged {continue;}
                        let new_difference=(left.players.players.len() as i32-
                            (party_a.players.len() as i32-party_b.players.len() as i32)*2-
                            right.players.players.len() as i32).abs();
                        if new_difference>difference {continue;}
                        let mut candidate=self.clone();
                        candidate.teams[a].remove(party_a);candidate.teams[b].remove(party_b);
                        candidate.teams[a].add(party_b.clone());candidate.teams[b].add(party_a.clone());
                        let reference=best.as_ref().unwrap_or(self);
                        if candidate.count_spread()<reference.count_spread() ||
                            (candidate.count_spread()==reference.count_spread() && reference.cost()>candidate.cost()) {
                            best=Some(candidate);
                        }
                    }
                }
            }
        }
        if let Some(best)=best {*self=best;true} else {false}
    }
}

#[derive(Clone, Copy, Debug)]
pub struct Options {
    pub teams: usize, pub maximum_team_size: usize,
    pub flag_parties: bool, pub flag_minimum: usize, pub flag_maximum: usize,
    pub use_parties: bool, pub optimize: bool,
}

/// The original complete greedy assignment plus optional pair-swap optimizer.
/// Invalid bounded owners are rejected before evaluating the original kernel.
pub fn balance(options: Options, party_indices: &[i32], weights: &[i32]) -> Option<Vec<i32>> {
    if !(1..=4).contains(&options.teams) || !(1..=16).contains(&options.maximum_team_size) ||
        party_indices.len()!=weights.len() || weights.len()>16 ||
        party_indices.iter().any(|&p|!(0..16).contains(&p)) ||
        (options.flag_parties && (options.flag_minimum>options.flag_maximum || options.flag_maximum>16)) {return None;}
    let mut parties: [Party;16]=std::array::from_fn(|id|Party {id,..Party::default()});
    for (index,&weight) in weights.iter().enumerate() {
        let party=if options.use_parties {party_indices[index] as usize} else {index};
        parties[party].add(Player {weight,index});
    }
    let mut pool=Team::default();
    for mut party in parties {
        if party.players.is_empty() {continue;}
        party.flagged=options.flag_parties && (options.flag_minimum..=options.flag_maximum).contains(&party.players.len());
        pool.add(party);
    }
    let mut solution=Solution {teams:vec![Team::default();options.teams]};
    while !pool.parties.is_empty() {
        let party=pool.remove_largest();let target=solution.smallest();solution.teams[target].add(party);
    }
    if solution.teams.iter().any(|t|t.players.players.len()>options.maximum_team_size) {return None;}
    if options.optimize {while solution.improve() {}}
    let mut result=vec![-1;weights.len()];
    for (team,players) in solution.teams.iter().enumerate() {
        for player in &players.players.players {result[player.index]=team as i32;}
    }
    Some(result)
}

pub fn by_count(mut options: Options, parties: &[i32]) -> Option<i32> {
    options.optimize=false;
    let result=balance(options,parties,&vec![0;parties.len()])?;
    let mut counts=vec![0i32;options.teams];
    for team in result {counts[team as usize]+=1;}
    Some(counts.iter().max()? - counts.iter().min()?)
}

/// Original91F00's partial-gathering capacity/imbalance gate. The repeated
/// next-party value for extra seats is retained exactly, including its original
/// behaviour; the unchanged-byte controls distinguish it from incrementing IDs.
pub fn can_add(options: Options, parties: &[i32], extra: usize, maximum_imbalance: i32) -> bool {
    if parties.len()>16 || extra>16-parties.len() || maximum_imbalance<0 ||
        parties.iter().any(|&p|!(0..16).contains(&p)) {return false;}
    let next_party=parties.iter().copied().max().map_or(0,|p|p+1);
    let mut all=parties.to_vec();all.extend(std::iter::repeat_n(next_party,extra));
    let Some(imbalance)=by_count(options,&all) else {return false;};
    let open_slots=(options.teams*options.maximum_team_size) as i32-all.len() as i32;
    (imbalance-open_slots).max(0)<=maximum_imbalance
}
