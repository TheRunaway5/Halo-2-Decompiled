"""Reuse the existing deterministic input recipes, never their outcomes."""
import ast
import hashlib
import importlib
from pathlib import Path
RECIPE_PINS={'order':'22f648b894b661d3782493dc60decd16c2d8cb802a010bba37be5c8575601b4c',
    'insert':'4780a1b14945e7ff1c95a5a8bcac4feee21db8fe104bf49f9100e55cce0779d4'}

def corpus(kind,scratch):
    if kind not in('order','insert'):raise ValueError('Unknown async input owner')
    module=importlib.import_module('validate_native_async_'+kind+'_tu15');path=Path(module.__file__);text=path.read_text();tree=ast.parse(text)
    main=next(node for node in tree.body if isinstance(node,ast.FunctionDef)and node.name=='main')
    start=next(index for index,node in enumerate(main.body)if isinstance(node,ast.Assign)and any(isinstance(target,ast.Name)and target.id=='rng'for target in node.targets))
    end=next(index for index,node in enumerate(main.body[start:],start)if isinstance(node,ast.Expr)and ast.unparse(node.value).startswith('sys.path.insert('))
    selected=main.body[start:end]
    if len(selected)!=(7 if kind=='order'else 6)or not isinstance(selected[-1],ast.With):raise ValueError('Protected async input-recipe structure changed')
    namespace=dict(vars(module));namespace['scratch']=Path(scratch)
    # Only the seed/bank/caller-input construction block is evaluated. It has
    # no compilation, original CPU execution, native function or outcome path.
    block=ast.fix_missing_locations(ast.Module(body=selected,type_ignores=[]));digest=hashlib.sha256(ast.dump(block,include_attributes=False).encode()).hexdigest()
    if digest!=RECIPE_PINS[kind]:raise ValueError('Complete async input recipe changed')
    exec(compile(block,str(path),'exec'),namespace)
    rows=namespace['rows'];values=('0000000000000080ff3f','00000000000000000080','35c26821a2da0fc90040','45230100000000c0ff7f')*2
    for ordinal,row in enumerate(rows):
        depth=(0,1,4,8)[(ordinal//128)%4];row['live_x87']=list(values[:depth]);row['before_fp'][1]=(row['before_fp'][1]&~0x3800)|((ordinal%8)<<11)
    return rows,dict(recipe=str(path),recipe_sha256=hashlib.sha256(text.encode()).hexdigest(),
        construction_ast_sha256=digest,
        cases=len(rows),scope='Existing deterministic declared bank/alias/caller inputs only; live80/TOP are additional caller operands. No function or service result is supplied.')
