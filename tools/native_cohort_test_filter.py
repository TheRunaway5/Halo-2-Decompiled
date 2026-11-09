"""Expand finite test-name patterns without CTest's capture-group limit."""

def expand_test_names(pattern):
    if not pattern.startswith('^') or not pattern.endswith('$'):
        raise ValueError('Cohort test pattern must be anchored')
    expression=pattern[1:-1];position=0
    def alternatives():
        nonlocal position
        choices=sequence()
        while position<len(expression) and expression[position]=='|':
            position+=1;choices+=sequence()
        return choices
    def sequence():
        nonlocal position
        values=['']
        while position<len(expression) and expression[position]not in '|)':
            char=expression[position];position+=1
            if char=='(':
                parts=alternatives()
                if position==len(expression) or expression[position]!=')':raise ValueError('Unclosed cohort group')
                position+=1
            elif char.isascii() and (char.isalnum() or char in '_-'):
                parts=[char]
            else:raise ValueError('Unsupported non-finite cohort syntax: '+char)
            if position<len(expression) and expression[position]=='?':
                position+=1;parts+=['']
            values=[left+right for left in values for right in parts]
            if len(values)>10000:raise ValueError('Cohort expansion exceeds its bounded test set')
        return values
    names=alternatives()
    if position!=len(expression):raise ValueError('Unexpected closing cohort group')
    if not names or any(not name for name in names):raise ValueError('Cohort pattern contains an empty test name')
    return tuple(dict.fromkeys(names))


def flatten_test_filter(pattern):
    return '^('+'|'.join(expand_test_names(pattern))+')$'
