"""Exercise the actual runtime disconnect-loop body with bounded owner callbacks.

Native certificate/pair eligibility is covered by source_common_cut_runtime_tests.
This focused compiler test covers the caller's cancellation/retention ordering,
without constructing a world or manufacturing a native proof. Automatic native
preparation admission also compiles the actual shared predicate and gameplay
worker-allocation block; explicit owner/transport cases remain native fixtures.
"""
import argparse
import os
from pathlib import Path
import subprocess
import tempfile

ROOT = Path(__file__).resolve().parents[1]
GUARD = '''        if(authority.recovery_eligibility()==MultiplayerRuntimeAuthority::RecoveryEligibility::Unavailable){
          authority.cancel_recovery();return;
        }
'''


def check(directory, legacy):
    source = (ROOT / 'src/gameplay/multiplayer/multiplayer_runtime.cpp').read_text()
    begin = source.index('  const auto retain_recovery=')
    end = source.index('  const auto retain_outcome', begin)
    body = source[begin:end]
    assert body.count(GUARD) == 1, 'Actual runtime recovery admission guard changed'
    if legacy:
        body = body.replace(GUARD, '', 1)
    harness = r'''
#include <algorithm>
#include <chrono>
#include <iostream>
#include <memory>
#include <stdexcept>
#include <thread>
#include <vector>
struct SourceAuthorityRetainedWorld {};
enum class MultiplayerExitReason { online_disconnected,escape };
struct MultiplayerRuntimeAuthority { enum class RecoveryEligibility { Unavailable,LegacyDrain,LearnedCommonCut }; };
using Eligibility=MultiplayerRuntimeAuthority::RecoveryEligibility;
constexpr int SDL_INIT_VIDEO=1,SDL_QUIT=2;
int SDL_WasInit(int){return 0;} void SDL_PumpEvents(){} bool SDL_HasEvent(int){return false;}
struct Owner {
  std::vector<Eligibility> stages;unsigned checked{},exports{},cancelled{};
  bool complete{};std::shared_ptr<const SourceAuthorityRetainedWorld> actual=std::make_shared<SourceAuthorityRetainedWorld>();
  bool has_receiver(){return true;}
  Eligibility recovery_eligibility(){const auto n=checked++;return stages.at(std::min<std::size_t>(n,stages.size()-1));}
  std::shared_ptr<const SourceAuthorityRetainedWorld> recover(){
    if(++exports>3)throw std::runtime_error("Bounded legacy infinite-loop control");
    return complete&&exports==2?actual:nullptr;
  }
  void cancel_recovery(){++cancelled;}
};
void require(bool value,const char *why){if(!value)throw std::runtime_error(why);}
void run(unsigned control){
  Owner authority;
  authority.stages=control==0?std::vector{Eligibility::Unavailable}:
      control==1?std::vector{Eligibility::LearnedCommonCut,Eligibility::Unavailable}:
      std::vector{Eligibility::LearnedCommonCut};
  authority.complete=control==2;
  std::shared_ptr<const SourceAuthorityRetainedWorld> output;
  auto *recovery=&output;
''' + body + r'''
  retain_recovery(control==3?MultiplayerExitReason::escape:MultiplayerExitReason::online_disconnected);
  if(control==0)require(authority.exports==0&&authority.cancelled==1&&!output,
      "Unavailable disconnect recovery entered the export wait");
  if(control==1)require(authority.exports==1&&authority.cancelled==1&&!output,
      "Lost eligibility did not cancel the actual pending export");
  if(control==2)require(authority.exports==2&&!authority.cancelled&&output==authority.actual,
      "Eligible pending recovery did not preserve the same returned owner");
  if(control==3)require(!authority.exports&&!authority.checked&&!authority.cancelled&&!output,
      "Ordinary non-disconnect exit entered recovery");
}
int main()try{for(unsigned control=0;control<4;++control)run(control);
  std::cout<<"PASS actual disconnect caller: unavailable refuses before export; revoked eligibility cancels pending work; genuine returned owner preserved; non-disconnect untouched\n";
}catch(const std::exception &error){std::cerr<<error.what()<<'\n';return 1;}
'''
    source_path = directory / 'disconnect.cpp'
    executable = directory / 'disconnect'
    source_path.write_text(harness)
    subprocess.run(['c++', '-std=c++20', '-Wall', '-Wextra', '-Werror', str(source_path), '-o', str(executable)], check=True)
    result = subprocess.run([str(executable)], capture_output=True, text=True, timeout=3)
    if legacy:
        assert result.returncode == 1 and 'Unavailable disconnect recovery entered the export wait' in result.stderr, result
        print('PASS RED control: removing only the admission guard reproduces the unsupported export wait', flush=True)
    else:
        assert result.returncode == 0, result.stderr
        print(result.stdout, end='', flush=True)


def check_preparation(directory, negative=False):
    authority = (ROOT / 'src/network/multiplayer_runtime_authority.cpp').read_text()
    begin = authority.index('bool MultiplayerRuntimeAuthority::recovery_preparation_enabled(')
    end = authority.index('MultiplayerRuntimeAuthority::MultiplayerRuntimeAuthority(', begin)
    admission = authority[begin:end]
    caller = (ROOT / 'src/gameplay/multiplayer/multiplayer_runtime.cpp').read_text()
    begin = caller.index('  std::unique_ptr<SourceAuthorityReplayWorker> source_receiver;')
    end = caller.index('  MultiplayerRuntimeAuthority authority(', begin)
    allocation = caller[begin:end]
    guard = 'MultiplayerRuntimeAuthority::recovery_preparation_enabled(options.online_match)'
    assert allocation.count(guard) == 1, 'Actual automatic worker admission changed'
    assert authority.count('if(!authority||!recovery_preparation)return;') == 1, 'Automatic publisher admission changed'
    assert authority.count('s.recovery_preparation=recovery_preparation_enabled(s.online,preparation);') == 1
    if negative:
        allocation = allocation.replace(guard, 'options.online_match', 1)
    harness = r'''
#include <iostream>
#include <memory>
#include <optional>
#include <stdexcept>
struct OnlineMatchSession {
  bool legacy{},peer{};
  std::optional<unsigned> continuity()const{return legacy||peer?std::optional<unsigned>(1):std::nullopt;}
};
struct MultiplayerRuntimeAuthority {
  enum class RecoveryPreparation { Automatic,PreparationOnly };
  static bool recovery_preparation_enabled(const std::shared_ptr<OnlineMatchSession>&,
      RecoveryPreparation=RecoveryPreparation::Automatic);
};
''' + admission + r'''
struct MultiplayerOptions {std::shared_ptr<OnlineMatchSession> online_match;};
struct OriginalData {enum class Image {TitleUpdate15,Other};Image image=Image::TitleUpdate15;};
struct Cache {bool present=true;OriginalData data;const OriginalData *original_data()const{return present?&data:nullptr;}};
enum class CustomSlayer {none,Other};
struct Rules {CustomSlayer custom=CustomSlayer::none;bool defusal=false;};
unsigned constructions;
struct SourceAuthorityReplayWorker {explicit SourceAuthorityReplayWorker(const MultiplayerOptions&){++constructions;}};
void require(bool value,const char *why){if(!value)throw std::runtime_error(why);}
void run(unsigned control){
  auto online=std::make_shared<OnlineMatchSession>();online->legacy=control==1||control>=4;online->peer=control==2;
  MultiplayerOptions options{control==3?nullptr:online};Cache cache;Rules rules;bool client=control!=8;
  if(control==4)cache.present=false;
  if(control==5)cache.data.image=OriginalData::Image::Other;
  if(control==6)rules.custom=CustomSlayer::Other;
  if(control==7)rules.defusal=true;
  constructions=0;
''' + allocation + r'''
  require(constructions==unsigned(control==1||control==2),
      "Ordinary or ineligible ONLINE allocated automatic shadow worker");
  require(bool(source_receiver)==bool(constructions),"Worker ownership changed");
  using Purpose=MultiplayerRuntimeAuthority::RecoveryPreparation;
  require(MultiplayerRuntimeAuthority::recovery_preparation_enabled(online,Purpose::PreparationOnly),
      "Explicit preparation-only owner was refused");
  require(!MultiplayerRuntimeAuthority::recovery_preparation_enabled({},Purpose::PreparationOnly),
      "Preparation without an online owner was admitted");
  require(!MultiplayerRuntimeAuthority::recovery_preparation_enabled(online,Purpose(99)),
      "Unknown preparation purpose was admitted");
  require(online->legacy==(control==1||control>=4)&&online->peer==(control==2),
      "Native preparation changed continuity or election admission");
}
int main()try{for(unsigned control=0;control<9;++control)run(control);
  std::cout<<"PASS actual automatic preparation caller: ordinary refuses; legacy/peer continuity retains workers; explicit preparation admits no election; native eligibility preserved\n";
}catch(const std::exception &error){std::cerr<<error.what()<<'\n';return 1;}
'''
    source_path = directory / ('preparation-red.cpp' if negative else 'preparation.cpp')
    executable = source_path.with_suffix('')
    source_path.write_text(harness)
    subprocess.run(['c++', '-std=c++20', '-Wall', '-Wextra', '-Werror', str(source_path), '-o', str(executable)], check=True)
    result = subprocess.run([str(executable)], capture_output=True, text=True, timeout=3)
    if negative:
        assert result.returncode == 1 and 'Ordinary or ineligible ONLINE allocated automatic shadow worker' in result.stderr, result
        print('PASS RED control: removing only actual caller preparation admission constructs an unconsumed ordinary worker', flush=True)
    else:
        assert result.returncode == 0, result.stderr
        print(result.stdout, end='', flush=True)


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--legacy-negative-control', action='store_true')
    args = parser.parse_args()
    scratch = os.environ.get('HALO2_TEST_SCRATCH')
    if scratch:
        print('OWNED disconnect recovery scratch=' + scratch, flush=True)
    with tempfile.TemporaryDirectory(prefix='disconnect-recovery-', dir=scratch) as temporary:
        check(Path(temporary), args.legacy_negative_control)
        if not args.legacy_negative_control:
            check_preparation(Path(temporary))
            check_preparation(Path(temporary), negative=True)
    assert not Path(temporary).exists(), 'Disposable compiler outputs survived cleanup'


if __name__ == '__main__':
    main()
