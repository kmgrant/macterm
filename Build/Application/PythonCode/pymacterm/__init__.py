# vim: set fileencoding=UTF-8 :

"""Routines for testing this package:

run_module_tests -- invoke _test() method of given module
run_all_tests -- run tests of all modules in the package

"""
from __future__ import absolute_import
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
    if not failures:
        print("MacTerm: %s module: SUCCESSFUL unit test (total tests: %d)" %
              (mod.__name__, test_count))

def run_all_tests():
    """Run tests in every module that has tests.
    """
    from . import file_kvp
    from . import file_open
    from . import term_text
    from . import url_open
    from . import url_parse
    from . import utilities
    run_module_tests(file_kvp)
    run_module_tests(file_open)
    run_module_tests(term_text)
    run_module_tests(url_open)
    run_module_tests(url_parse)
    run_module_tests(utilities)
