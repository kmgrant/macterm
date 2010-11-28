#!/usr/bin/python
# vim: set fileencoding=UTF-8 :
"""Routines to handle text in terminal screen buffers.

find_word -- scan around a starting point to find the range of the word
get_dumb_rendering -- string to describe a Unicode character in a dumb terminal

"""
__author__ = 'Kevin Grant <kmg@mac.com>'
__date__ = '28 November 2010'
__version__ = '4.0.0'

import sys
import string

# note: Quills is a compiled module, library path must be set properly
import quills

def find_word(text_utf8, pos):
    """find_word(text_utf8, pos) -> (pos, count)
    
    Return a pair of integers as a tuple, where the first
    is a zero-based CHARACTER offset into the given string,
    and the second is a CHARACTER count from that offset.
    This range identifies a word that is found by scanning
    forwards and backwards from the given starting CHARACTER
    in the given string of BYTES.  Don't use byte offsets!
    
    This is designed to be compatible with the callback
    format required by quills.Terminal.on_seekword_call().
    
    (Below are REAL testcases run by doctest!)
    
    >>> find_word("this is a sentence", 0)
    (0, 4)
    
    >>> find_word("this is a sentence", 1)
    (0, 4)
    
    >>> find_word("this is a sentence", 3)
    (0, 4)
    
    >>> find_word("this is a sentence", 4)
    (4, 1)
    
    >>> find_word("this is a sentence", 5)
    (5, 2)
    
    >>> find_word("  well   spaced      words  ", 17)
    (15, 6)
    
    >>> find_word("  well   spaced      words  ", 13)
    (9, 6)
    
    """
    result = [pos, 1]
    if pos < 0:
        raise ValueError("word-seeking callback expected nonnegative offset")
    try:
        ustr = unicode(text_utf8, "utf-8", "ignore")
        if pos >= len(ustr):
            raise ValueError("word-seeking callback expected offset to fall within range of characters")
        nonword_chars = str(string.whitespace)
        # an easy way to customize this is to add characters to
        # the string variable "nonword_chars", e.g. the following
        # would consider dots (.) to be word-breaking characters:
        #     nonword_chars = nonword_chars + '.'
        invert = False
        if ustr[pos] in nonword_chars:
            # special case; when starting on non-word characters, look for all non-word characters
            invert = True
        i = pos
        j = pos
        while i >= 0:
            if (invert and ustr[i] not in nonword_chars) or \
               (not invert and ustr[i] in nonword_chars):
                i = i + 1
                break
            i = i - 1
        if i < 0:
            i = 0
        while j < len(ustr):
            if (invert and ustr[j] not in nonword_chars) or \
               (not invert and ustr[j] in nonword_chars):
                j = j - 1
                break
            j = j + 1
        if j >= len(ustr):
            j = len(ustr) - 1
        result[0] = i
        result[1] = j - i + 1
    except Exception, e:
        print "warning, exception while trying to find words:", e
    return (result[0], result[1])

def get_dumb_rendering(ord_unicode_16):
    """get_dumb_rendering(char_utf16) -> text_utf8
    
    Return the string that dumb terminals should use to
    render the specified character.  The idea is for EVERY
    character to have a visible representation, whereas
    with normal terminals many invisible characters have
    special meaning.
    
    (Below are REAL testcases run by doctest!)
    
    >>> get_dumb_rendering(0)
    '^@'
    
    >>> get_dumb_rendering(13)
    '^M'
    
    >>> get_dumb_rendering(27)
    '<ESC>'
    
    >>> get_dumb_rendering(97)
    'a'
    
    """
    result = '<?>' # must use UTF-8 encoding
    if ord_unicode_16 < 128:
        as_ascii = ord_unicode_16
        if as_ascii == 27: result = '<ESC>'
        # a "proper" control symbol is preferred, but MacTelnet cannot render
        # higher Unicode characters just yet...
        #elif as_ascii < ord(' '): result = '⌃%c' % chr(ord('@') + as_ascii)
        elif as_ascii < ord(' '): result = '^%c' % chr(ord('@') + as_ascii)
        elif (chr(as_ascii) in string.printable): result = chr(as_ascii)
        else: result = '<%i>' % as_ascii
    else:
        result = '<u%i>' % ord_unicode_16
    return result

def _test():
    import doctest
    import pymactelnet.term.text
    return doctest.testmod(pymactelnet.term.text)
