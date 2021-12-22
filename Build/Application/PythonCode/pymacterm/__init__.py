# vim: set fileencoding=UTF-8 :

"""Routines for testing this package:

run_module_tests -- invoke _test() method of given module
run_all_tests -- run tests of all modules in the package

"""

def run_module_tests(mod):
    """Every module must support doctest; this runs doctest.testmod()
    on each module and (if there are errors) prints results.

    """
    import doctest
    (failures, test_count) = doctest.testmod(mod)
    if not failures:
        print("MacTerm: %s module: SUCCESSFUL unit test (total tests: %d)" %
              (mod.__name__, test_count))
    return failures

def run_all_tests():
    """Run tests in every module that has tests.
    """
    failures = 0
    from . import file_kvp
    from . import file_open
    from . import term_text
    from . import url_open
    from . import url_parse
    from . import utilities
    failures += run_module_tests(file_kvp)
    failures += run_module_tests(file_open)
    failures += run_module_tests(term_text)
    failures += run_module_tests(url_open)
    failures += run_module_tests(url_parse)
    failures += run_module_tests(utilities)
    if failures > 0:
        raise RuntimeError("one or more tests failed; see above")
