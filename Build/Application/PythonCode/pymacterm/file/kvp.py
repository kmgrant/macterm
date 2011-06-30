#!/usr/bin/python
# vim: set fileencoding=UTF-8 :
"""Parse key-value-pair file formats such as ".session".

A key-value-pair file uses a single, all-lowercase, alphanumeric key name on
each line, followed by an "=" sign, and the value.  The value may be in optional
quotation marks.  If it is within braces {}, it is considered a list, and list
items are each followed by a comma ",".

Extremely old incarnations of this file format, specifically the saved files of
NCSA Telnet 2.6, required an exact set of keys in a very specific order.  This
parser does not care what the keys are, or what order they are in.

Parser -- class to translate key-value-pair syntax into Python data

"""
__author__ = 'Kevin Grant <kmg@mac.com>'
__date__ = '13 April 2009'
__version__ = '4.0.0'

class Parser (file):
    """Read key-value-pair syntax and translate it into Python data.
    
    results -- retrieve key-value pairs as a dictionary
    
    """
    
    def _parse_any_value(value):
        """_parse_any_value(string) -> string
        
        Strip end whitespace and quotes from a string, returning
        a copy of the string or (for strings representing
        numbers) an actual numerical value.
        
        (Below are REAL testcases run by doctest!)
        
        >>> print Parser._parse_any_value('3')
        3
        >>> print Parser._parse_any_value('some words')
        some words
        >>> print '|%s|' % str(Parser._parse_any_value('   "  six seven  "\\n'))
        |  six seven  |
        
        """
        result = value.strip(" \t\n")
        result = result.strip("\"\'")
        try:
            result = int(result)
        except ValueError, e:
            pass
        return result
    _parse_any_value = staticmethod(_parse_any_value)

    def _parse_list_value(value):
        """_parse_list_value(string) -> tuple
        
        Return a tuple with the actual values (strings or
        numbers) from an expression of comma-separated values
        that may be enclosed in braces.
        
        (Below are REAL testcases run by doctest!)
        
        >>> print Parser._parse_list_value('3')
        (3,)
        >>> print Parser._parse_list_value('some words')
        ('some words',)
        >>> print Parser._parse_list_value('this,  is a list  , of stuff')
        ('this', 'is a list', 'of stuff')
        >>> print Parser._parse_list_value('"this","  is a list  "," of stuff"')
        ('this', '  is a list  ', ' of stuff')
        >>> print Parser._parse_list_value('{1, 2, 3, 4}')
        (1, 2, 3, 4)
        
        """
        value = value.lstrip("{")
        value = value.rstrip("}")
        result = tuple([Parser._parse_any_value(x) for x in value.split(",")])
        return result
    _parse_list_value = staticmethod(_parse_list_value)

    def __init__(self, lines=None, file_object=None):
        """Parser(lines) -> Parser
        Parser(file_object) -> Parser
        
        Parse lines from a key-value-pair file, and store any
        definitions in a dictionary that is returned by the
        results() method.
        
        The lines can either be given directly, or from a file
        (on which readlines() is then invoked).  If both are
        defined, then both are processed, and act as if they were
        both part of the same original file (the "lines" array
        acts like the top of the file).
        
        Raise SyntaxError on failure.
        
        If you want to support any type of line endings (such as
        Mac or Unix), be sure to open the file in mode 'rU'.
        
        (Below are REAL testcases run by doctest!)
        
        >>> p = Parser(lines=[
        ... 'xyz = {"one", "two", "three"}\\n',
        ... 'a = 1\\n',
        ... 'c = {1,2,3}\\n',
        ... 'b = "2"\\n',
        ... ])
        >>> d = p.results()
        >>> k = d.keys()[:]
        >>> k.sort()
        >>> print k
        ['a', 'b', 'c', 'xyz']
        >>> print d['a']
        1
        >>> print d['b']
        2
        >>> print d['c']
        (1, 2, 3)
        >>> print d['xyz']
        ('one', 'two', 'three')
        
        >>> try:
        ...        p = Parser(lines=['this is garbage input'])
        ... except SyntaxError, e:
        ...        print str(e)
        session file line 1: need more than 1 value to unpack
        
        """
        self._definitions = dict()
        if lines is not None:
            lines = lines[:]
        else:
            lines = list()
        if file_object is not None:
            lines.extend(file_object.readlines())
        i = 1
        for line in lines:
            try:
                key, value = str(line).split('=')
                key = key.strip()
                value = value.strip(" \t\n\"\'")
                if len(value) > 0:
                    if value[0] == '{':
                        value = Parser._parse_list_value(value)
            except ValueError, e:
                raise SyntaxError("session file line %d: %s" % (i, str(e)))
            self._definitions[key] = value
            i = i + 1

    def results(self):
        """results() -> dict
        
        Return the settings parsed from the file as key-value
        pairs.  Though originally strings, recognized syntax for
        things like lists and quoted strings will automatically
        have been parsed out and converted to Python types.
        
        The encoding of strings is not specified.  For the sake
        of completeness, files without a specified encoding must
        use UTF-8.  An encoding can be given with the key named
        "encoding", whose value should be an IANA encoding name
        (but this is just convention; nothing in this module will
        enforce it).
        
        """
        return self._definitions

def _test():
    import doctest
    import pymacterm.file.kvp
    return doctest.testmod(pymacterm.file.kvp)

if __name__ == '__main__':
    _test()
