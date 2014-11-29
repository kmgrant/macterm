# vim: set fileencoding=UTF-8 :

from __future__ import division
from __future__ import print_function

def run_module_tests(mod):
    """Every module must have a _test() function, which
    uses "doctest"; this function invokes that for the
    given module.
    
    Note that a _test() call is only expected to print
    output if there are problems.
    
    """
    (failures, test_count) = mod._test()
    if not failures: print("MacTerm: %s module: SUCCESSFUL unit test (total tests: %d)" % (mod.__name__, test_count))

def run_all_tests():
    """Run tests in every module that has tests.
    
    """
    import pymacterm.file.kvp
    import pymacterm.file.open
    import pymacterm.term.text
    import pymacterm.url.open
    import pymacterm.url.parse
    import pymacterm.utilities
    run_module_tests(pymacterm.file.kvp)
    run_module_tests(pymacterm.file.open)
    run_module_tests(pymacterm.term.text)
    run_module_tests(pymacterm.url.open)
    run_module_tests(pymacterm.url.parse)
    run_module_tests(pymacterm.utilities)
