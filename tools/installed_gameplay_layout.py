"""Compile debugger layouts only from the installed build's exact header closure."""
import hashlib,os,shlex,subprocess
from pathlib import Path
ROOT=Path(__file__).resolve().parents[1]
def sha(path):return hashlib.sha256(path.read_bytes()).hexdigest()
def compile_installed_layout(source,pins,required,cpp,work):
    source=Path(source).resolve(strict=True)
    for name in required:assert sha(source/name)==pins[name],('Installed layout header changed',name)
    includes=shlex.split(subprocess.check_output(['pkg-config','--cflags','sdl2'],text=True))
    executable=work/'layout';dependencies=work/'layout.d'
    subprocess.run(['c++','-std=c++20',*includes,'-I'+str(source/'include'),'-MMD','-MF',str(dependencies),
        str(cpp),'-o',str(executable)],check=True)
    inputs={}
    for name in dependencies.read_text().replace(chr(92)+chr(10),'').split(':',1)[1].split():
        header=Path(name).resolve()
        if not header.is_relative_to(source/'include'):continue
        relative=header.relative_to(source).as_posix();digest=sha(header)
        assert digest==pins[relative],('Installed transitive layout header changed',relative)
        inputs[relative]=digest
    assert inputs
    return subprocess.check_output([str(executable)],text=True),dict(source=str(source),inputs=inputs)
def installed_layout_source():
    return Path(os.environ.get('HALO2_INSTALLED_LAYOUT_SOURCE',str(ROOT)))
