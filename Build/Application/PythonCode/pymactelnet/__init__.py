def run_module_tests( mod ):
    """Every module must have a _test() function, which
    uses "doctest"; this function invokes that for the
    given module.
    
    Note that a _test() call is only expected to print
    output if there are problems.
    
    """
    (failures, test_count) = mod._test()
    if not failures: print "MacTelnet: %s module: SUCCESSFUL unit test (total tests: %d)" % (mod.__name__, test_count)

def run_all_tests():
    """Run tests in every module that has tests.
    
    """
    import pymactelnet.file.kvp
    import pymactelnet.file.open
    import pymactelnet.term.text
    import pymactelnet.url.open
    import pymactelnet.url.parse
    import pymactelnet.utilities
    run_module_tests(pymactelnet.file.kvp)
    run_module_tests(pymactelnet.file.open)
    run_module_tests(pymactelnet.term.text)
    run_module_tests(pymactelnet.url.open)
    run_module_tests(pymactelnet.url.parse)
    run_module_tests(pymactelnet.utilities)
