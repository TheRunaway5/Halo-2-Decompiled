import collections, re, sys, runpy, io, contextlib
prefix, binary = sys.argv[1], sys.argv[2]
sys.argv = [sys.argv[0], prefix, binary, '0', sys.argv[3] if len(sys.argv) > 3 else '0']
with contextlib.redirect_stdout(io.StringIO()):
    g = runpy.run_path(__file__.replace('buckets.py', 'analyze.py'), run_name='analyze')
samples, sym = g['samples'], g['sym']
rules = [
 ('teardown', r'~Impl|~Mesh|~Material|_M_release'),
 ('mem.write.lock', r'pthread_rwlock'),
 ('mem.write.observers', r'_Function_handler<void \(unsigned int, unsigned long\)|native_write_guards'),
 ('mem.write', r'SourceRuntimeMemory::write|validate_write|SourceRuntimeMemory::validate\(|physics_step_detail::w\(|physics_step_detail::vw|State::push'),
 ('mem.read', r'SourceRuntimeMemory::read|SourceRuntimeMemory::locate|validate_committed|physics_step_detail::r\(|physics_step_detail::v\(|SourceRuntimeMemory::view|Bytes::'),
 ('fpenv/call', r'State::load|State::save|State::required|fegetenv|fesetenv|required_entry|services\(\)|call_actual|InstructionRemap'),
 ('translated.body', r'::run_[0-9a-f]+\(|native_single_spill|Flags::|ExtendedStack'),
 ('skinning', r'skin_|finish_skin|place_vertices|ModelWorldPoseCache'),
 ('motion_sensor', r'motion_sensor'),
 ('vulkan/driver', r'ioctl|libnvidia|libvulkan|\[\?\]'),
 ('malloc', r'malloc|free|operator new|morecore|_int_'),
 ('memcpy', r'memcpy|memmove|memset'),
]
self_b, incl_hint = collections.Counter(), collections.Counter()
for t, pcs in samples:
    leaf = sym(pcs[0])
    for name, pat in rules:
        if re.search(pat, leaf):
            self_b[name] += 1; break
    else:
        self_b['other'] += 1
n = len(samples)
for k, c in self_b.most_common(): print(f'{100*c/n:6.2f}% {k}')
if len(sys.argv) > 0:
    other = collections.Counter()
    for t, pcs in samples:
        leaf = sym(pcs[0])
        if not any(re.search(p, leaf) for _, p in rules): other[leaf[:140]] += 1
    print('--- other')
    for k, c in other.most_common(40): print(f'{100*c/n:6.2f}% {k}')
