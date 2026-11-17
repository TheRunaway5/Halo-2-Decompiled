"""Compile defects in the private actual3136A0 retained frame and admission."""
import subprocess


def run_mutants(root, scratch, command, inputs, env):
    cases = (
        ('raw_callback_admission', 'owner', 'if constexpr(!Literal)if(s.services.instruction)return false;',
         'if constexpr(false)if(s.services.instruction)return false;', 'baseline',
         'Hull-line complete source route differs'),
        ('unknown_provider_admission', 'owner',
         'if(!native_required_owner_policy_detail::preserves_instruction_policy(s.services))return false;',
         'if(false&&!native_required_owner_policy_detail::preserves_instruction_policy(s.services))return false;',
         'provider-controls', 'Uncertified provider did not take whole-entry fallback'),
        ('omitted_policy_retirement', 'owner',
         'if(!continuation_admitted<Literal>(v,next))throw Continuation{next,&token};',
         'if(false&&!continuation_admitted<Literal>(v,next))throw Continuation{next,&token};',
         'frontiers', 'Hull-line complete source route differs'),
        ('omitted_actual_child_retirement', 'frame', 'call_(s_,target,pc);retired(returned);',
         'call_(s_,target,pc);', 'child-return', 'Genuine child-return policy did not resume at the actual return'),
        ('stale_direction_after_push', 'frame', 'd.direction=s_.xmm[1];d.sign=s_.xmm[0];',
         '/* incorrectly keep pre-PUSH operands */', 'feedback',
         'Hull-line complete source route differs'),
        ('wrong_source_FLD_C1', 'frame', 'if constexpr(ClearLoadC1)s_.fp.status&=~0x200u;',
         's_.fp.status|=0x200u;', 'feedback',
         'Wrong retained FLD C1 profile'),
        ('physical_PUSH_publication', 'frame', 'at(pc);s_.push(pc,value);retired(next);',
         'at(pc);const auto address=s_.gp[4]-4u;physics_step_detail::w(s_.memory,s_.observe,pc,address,4u,value);s_.gp[4]=address;retired(next);',
         'source-publication', 'Hull-line complete source route differs'),
        ('swallowed_foreign_continuation', 'owner', 'if(resume.owner!=&token)throw;',
         'if(false&&resume.owner!=&token)throw;', 'foreign-exceptions',
         'Unsupported native hull-line continuation'),
    )
    result = {}
    for name, kind, old, new, mode, expected in cases:
        relative = 'physics/runtime/native_tu15_solver_hull_line_source_' + kind + '.hpp'
        source = (root / 'src' / relative).read_text()
        if source.count(old) != 1:
            raise ValueError('Hull-line source defect anchor drift ' + name)
        overlay = scratch / name
        header = overlay / relative
        header.parent.mkdir(parents=True)
        header.write_text(source.replace(old, new, 1))
        candidate = command.copy()
        candidate.insert(candidate.index('-I' + str(root / 'include')), '-I' + str(overlay))
        candidate[-1] = str(overlay / 'routes')
        built = subprocess.run(candidate, capture_output=True, text=True, timeout=600)
        if built.returncode:
            raise ValueError('Hull-line source defect failed compilation ' + name + ': ' + built.stderr[-5000:])
        with inputs.open('rb') as stream:
            run = subprocess.run([candidate[-1], mode], stdin=stream, capture_output=True,
                                 text=True, env=env, timeout=1200)
        if not run.returncode or expected not in run.stderr:
            raise ValueError('Hull-line source defect lacks its expected witness ' + name + ': ' + run.stderr[-5000:])
        result[name] = dict(rejected=True, mode=mode, expected_failure=expected, diagnostic=run.stderr[-5000:])
    return result
