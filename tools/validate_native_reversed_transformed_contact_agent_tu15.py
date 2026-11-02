#!/usr/bin/env python3
"""Compare complete2F75F0 with unchanged physical TU1.5 execution.

Declare the wrapper on the second input. The transformed child remains the
first selected-factory operand, and the original first input becomes second.
No original instruction, service outcome or expected postimage is supplied.
"""
import re
from pathlib import Path
from validate_native_transformed_contact_agent_tu15 import DRIVER as TRANSFORMED_DRIVER
from validate_native_wrapped_contact_agent_tu15 import main as validate


def driver():
    text=TRANSFORMED_DRIVER
    # The complete original companion's common instruction sites differ by
    #460h. Keep geometry2E44D0 and every genuine dependency untouched.
    text=re.sub(r'0x([0-9a-f]+)',lambda match:
        hex(int(match[1],16)+0x460)if 0x2f7190<=int(match[1],16)<=0x2f729f else match[0],text)
    changes=[
        ('original.word(a)', 'original.word(b)'),
        ('original.word(a+4u)', 'original.word(b+4u)'),
        ('original.word(a+8u)', 'original.word(b+8u)'),
        ('a=wrapper_payload;', 'b=wrapper_payload;'),
        ('native_tu15_create_transformed_contact_agent(', 'native_tu15_create_reversed_transformed_contact_agent('),
        ('case 0x2f76ff:address=r.rsi+12u;write=true;break;',
         'case 0x2f76ff:address=r.rsi+12u;write=true;break;case 0x2f7703:address=r.rsi;write=true;break;')]
    for before,after in changes:
        assert text.count(before)==1,(before,text.count(before))
        text=text.replace(before,after)
    return text


if __name__=='__main__':
    raise SystemExit(validate(driver(),'reversed_transformed',(
        ('wrong_child','m.word(shape+12u)','m.word(shape+8u)'),
        ('wrong_stride','(first_class<<5u)+second_class','(first_class<<4u)+second_class'),
        ('wrong_size','agent.identity+4u,16u,2','agent.identity+4u,20u,2'),
        ('missing_composition','native_tu15_compose_affine_geometry(geometry,fp);','(void)geometry;'),
        ('wrong_derived_vtable','0x41cc84u','0x41cc64u'),
    ),(Path(__file__),Path(__file__).with_name('validate_native_transformed_contact_agent_tu15.py')),
        implementation='src/physics/native_tu15_reversed_transformed_contact_agent.cpp'))
