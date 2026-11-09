"""Exact observed-PC attribution for generated runtime instruction censuses.

The default address space is the selected TU1.5 live runtime. Function names
can belong to the older image (notably ObservedWorldState and VehicleState),
so instruction callback expressions, rather than neighboring run_ names,
define routine ownership. A PC with conflicting owners stays unassigned.

Call edges are static, including literal dispatch/required calls. They do not
establish which caller executed a shared routine; indirect calls are unresolved.
"""
from collections import Counter, defaultdict
from dataclasses import dataclass, field
from pathlib import Path
import re
import sys
sys.path.insert(0,str(Path(__file__).resolve().parents[1]))
from generated_routine_ownership import function_bodies,literal_source_paths


FUNCTION = re.compile(r'^(?:\[\[[^\]]+\]\]\s*)?(?:static\s+)?void run_([0-9a-f]+)\(\w+\s*&s\)\s*\{')
HEX = r'0x[0-9a-f]+u?'
EXPRESSION = (rf'(?:(?:world|damage)_observation_pc\(\s*{HEX}\s*,\s*{HEX}\s*,\s*s\s*\)'
              rf'|selected\(\s*{HEX}\s*,\s*s\.image\s*\)|{HEX})')
INSTRUCTION = re.compile(rf'\b(?:[A-Za-z_]\w*(?:\(\))?\.)*instruction(?:_literal)?\(\s*({EXPRESSION})\s*\)')
DIRECT = re.compile(r'\brun_([0-9a-f]+)\(s\)')
DISPATCH = re.compile(rf'\bdispatch\(s,\s*({EXPRESSION})\s*,')
TARGET = re.compile(rf'\b(?:const\s+)?auto\s+target\s*=\s*({EXPRESSION})\s*;')
REQUIRED = re.compile(rf'\bs\.required\(\s*{EXPRESSION}\s*,\s*({EXPRESSION})\s*,')
MAPPING = re.compile(r'case (0x[0-9a-f]+)u:return (0x[0-9a-f]+)u;')
CPP_LINE_TOKENS = re.compile(r'//.*|/\*|\*/|"(?:\\.|[^"\\])*"|\'(?:\\.|[^\'\\])*\'|[{}]')
BINDING = re.compile(r'\{(0x[0-9a-f]+)u,(0x[0-9a-f]+)u\},')
REMAPPED_VEHICLE_UNITS = {
    'physics_vehicle_runtime_generated.cpp', 'physics_vehicle_update_generated.cpp',
    'physics_vehicle_motion_generated.cpp', 'physics_vehicle_motion_math_generated.cpp',
}
FIXED_STATE_IMAGE = re.compile(r'\b\w*State\s*(?:\w+\s*)?\{[^;\n]*SourcePhysicsContactImage::(MapPack11|TitleUpdate15)')
DYNAMIC_STATE_IMAGE = re.compile(r'\b\w*State\s*(?:\w+\s*)?\{[^;\n]*\bimage\s*\}')


def callback_image_scope(text):
    """Conservative image evidence from explicit state construction.

    Unknown/mixed scopes stay visible. Names such as State, source_1_1, and
    native are never sufficient evidence that a compiled owner is offline.
    """
    images=set(FIXED_STATE_IMAGE.findall(text))
    if DYNAMIC_STATE_IMAGE.search(text):return 'image-selected-or-unresolved'
    if images=={'MapPack11'}:return 'map-pack11'
    if images=={'TitleUpdate15'}:return 'tu15'
    return 'image-selected-or-unresolved'


def vehicle_observations(root):
    """Read the live composition's checked canonical-PC adapter, not offsets."""
    path = Path(root) / 'physics/physics_phantom_world_tu15_generated.cpp'
    if not path.exists():
        return {}
    result, active = {}, False
    with path.open() as source:
        for line in source:
            if re.match(r'constexpr std::array<Binding,\d+> vehicle_sites=\{\{', line):
                active = True
                continue
            if active and line.startswith('}};'):
                break
            if active:
                match = BINDING.fullmatch(line.strip())
                if match:
                    result[int(match[1], 16)] = int(match[2], 16)
    return result


def address(expression, mapping, image):
    values = [int(value.removesuffix('u'), 16) for value in re.findall(HEX, expression)]
    if expression.startswith(('world_observation_pc','damage_observation_pc')):
        return values[1] if image == 'tu15' else values[0]
    if expression.startswith('selected') and image == 'tu15':
        return mapping.get(values[0])
    return values[0]


def brace_change(line, block_comment=False):
    depth, cursor = 0, 0
    while cursor < len(line):
        if block_comment:
            end = line.find('*/', cursor)
            if end < 0:
                break
            block_comment, cursor = False, end + 2
            continue
        token = CPP_LINE_TOKENS.search(line, cursor)
        if token is None:
            break
        value, cursor = token[0], token.end()
        if value == '/*':
            block_comment = True
        elif value.startswith('//'):
            break
        elif value == '{':
            depth += 1
        elif value == '}':
            depth -= 1
    return depth, block_comment


@dataclass
class Routine:
    symbol: int | str
    unit: str
    pcs: set = field(default_factory=set)
    entry: int | None = None
    direct: set = field(default_factory=set)
    targets: set = field(default_factory=set)


class CensusIndex:
    def __init__(self):
        self.owners = defaultdict(set)
        self.calls = defaultdict(set)
        self.units = defaultdict(set)
        self.excluded_image_units = {}

    @classmethod
    def scan(cls, root=Path('src'), image='tu15', paths=None):
        index = cls()
        remap = vehicle_observations(root) if image == 'tu15' else {}
        for path in sorted(paths if paths is not None else literal_source_paths(Path(root).parent)):
            path = Path(path)
            index.add_unit(path, image, remap if path.name in REMAPPED_VEHICLE_UNITS else None)
        return index

    def add_unit(self, path, image='tu15', observation_remap=None):
        mapping, routines = {}, []
        text=path.read_text(errors='replace')
        for line in text.splitlines():
            if line.startswith('std::uint32_t selected('):
                mapping.update((int(a, 16), int(b, 16)) for a, b in MAPPING.findall(line))
        for match,body in function_bodies(text):
            symbol=int(match[1],16) if re.fullmatch('[0-9a-f]+',match[1]) else match[1]
            current=Routine(symbol,path.name);routines.append(current)
            for line in body.splitlines():
                for observed in INSTRUCTION.finditer(line):
                    pc = address(observed[1], mapping, image)
                    if pc is not None and observation_remap:
                        pc = observation_remap.get(pc)
                    if pc is not None:
                        if current.entry is None:
                            current.entry = pc
                        current.pcs.add(pc)
                current.direct.update(int(value, 16) for value in DIRECT.findall(line))
                if 'dispatch(s,' in line:
                    expressions = DISPATCH.findall(line)
                    if 'dispatch(s,target,' in line:
                        expressions.extend(TARGET.findall(line))
                    for expression in expressions:
                        target = address(expression, mapping, image)
                        if target is not None:
                            current.targets.add(target)
                for expression in REQUIRED.findall(line):
                    target = address(expression, mapping, image)
                    if target is not None:
                        current.targets.add(target)
        scope=callback_image_scope(text)
        if not observation_remap and scope in {'map-pack11','tu15'} and scope!=image:
            self.excluded_image_units[str(path)]=scope
            return
        local = {routine.symbol: routine.entry for routine in routines}
        for routine in routines:
            if routine.entry is None:
                continue
            self.units[routine.entry].add(routine.unit)
            for pc in routine.pcs:
                self.owners[pc].add(routine.entry)
            self.calls[routine.entry].update(routine.targets)
            for symbol in routine.direct:
                target = local.get(symbol)
                if target is not None:
                    self.calls[routine.entry].add(target)

    def owner(self, pc):
        owners = self.owners.get(pc, ())
        return next(iter(owners)) if len(owners) == 1 else None

    def attribute(self, census):
        per = Counter()
        for pc, count in census.items():
            per[self.owner(pc)] += count
        return per

    def tree(self, root, executed=None):
        seen, pending = set(), [root]
        while pending:
            entry = pending.pop()
            if entry in seen or (executed is not None and entry not in executed):
                continue
            seen.add(entry)
            pending.extend(self.calls.get(entry, ()))
        return seen


def load_census(path):
    census = Counter()
    with Path(path).open() as source:
        for line in source:
            if line.strip():
                pc, count = line.split()
                census[int(pc, 16)] += int(count)
    return census


def describe(entry):
    return 'unassigned' if entry is None else f'{entry:x}'


def attribution_note(index, census):
    unknown = sum(count for pc, count in census.items() if pc not in index.owners)
    ambiguous = sum(count for pc, count in census.items() if len(index.owners.get(pc, ())) > 1)
    return (f'exact callback-PC attribution; unknown instructions {unknown:,.0f}; ambiguous instructions {ambiguous:,.0f}; '
            f'excluded other-image units {len(index.excluded_image_units)}')
