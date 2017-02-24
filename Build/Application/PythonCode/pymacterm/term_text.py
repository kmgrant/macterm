#!/usr/bin/python
# vim: set fileencoding=UTF-8 :

"""Routines to handle text in terminal screen buffers.

find_word -- scan around a starting point to find the range of the word
get_dumb_rendering -- string to describe a Unicode character in a dumb terminal

"""
from __future__ import absolute_import
from __future__ import division
from __future__ import print_function

__author__ = 'Kevin Grant <kmg@mac.com>'
__date__ = '28 November 2010'
__version__ = '4.0.0'

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

    >>> find_word('"quoted"', 2)
    (1, 6)

    >>> find_word("'quoted'", 2)
    (1, 6)

    >>> find_word("<quoted>", 2)
    (1, 6)

    >>> find_word("{quoted}", 2)
    (1, 6)

    >>> find_word("[quoted]", 2)
    (1, 6)

    >>> find_word("(quoted)", 2)
    (1, 6)

    >>> find_word("quoted)", 2)
    (0, 6)

    >>> find_word("(quoted", 2)
    (1, 6)

    >>> find_word("unquoted", 2)
    (0, 8)

    """
    result = [pos, 1]
    if pos < 0:
        raise ValueError("word-seeking callback expected nonnegative offset")
    try:
        ustr = unicode(text_utf8, "utf-8", "ignore")
        len_ustr = len(ustr)
        if pos >= len_ustr:
            raise ValueError("word-seeking callback expected offset to",
                             "fall within range of characters")
        nonword_chars = str(string.whitespace)
        # an easy way to customize this is to add characters to
        # the string variable "nonword_chars", e.g. the following
        # would consider dots (.) to be word-breaking characters:
        #     nonword_chars = nonword_chars + '.'
        invert = False
        if ustr[pos] in nonword_chars:
            # special case; when starting on non-word characters, look for all
            # non-word characters
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
        while j < len_ustr:
            if (invert and ustr[j] not in nonword_chars) or \
               (not invert and ustr[j] in nonword_chars):
                j = j - 1
                break
            j = j + 1
        if j >= len_ustr:
            j = len_ustr - 1
        result[0] = i
        result[1] = j - i + 1
        # strip certain trailing punctuation marks to make word selections more
        # sensible
        if result[1] > 0:
            # WARNING: variable helpers are synchronized "as needed", not
            # "always"; be careful when adding new code to make sure the
            # variables are actually up-to-date before depending on them
            first = ustr[result[0]]
            last = ustr[result[0] + result[1] - 1]
            # strip basic punctuation off the end (this is repeated below)
            if (last == ".") or (last == ",") or (last == ";") or (last == ":"):
                result[1] = result[1] - 1
                last = ustr[result[0] + result[1] - 1] # synchronize variable
            if result[1] > 1:
                open_paren_count = 0
                close_paren_count = 0
                double_quote_count = 0
                single_quote_count = 0
                # study the word's characters; note that due to GNU's broken
                # behavior of treating a backquote like an open-quote, this
                # algorithm assumes that "`" is a type of single quotation mark
                for i in range(result[0], result[0] + result[1]):
                    if ustr[i] == '"':
                        double_quote_count = double_quote_count + 1
                    elif ustr[i] == "'" or ustr[i] == "`":
                        single_quote_count = single_quote_count + 1
                    elif ustr[i] == "(":
                        open_paren_count = open_paren_count + 1
                    elif ustr[i] == ")":
                        close_paren_count = close_paren_count + 1
                # strip trailing punctuation as long as the word doesn't
                # contain balanced brackets (e.g. keep "xyz()" but change
                # "xyz)" to "xyz")
                tail_ok = False
                while not tail_ok and result[1] > 0:
                    if last == ")":
                        if close_paren_count > open_paren_count:
                            close_paren_count = close_paren_count - 1
                            result[1] = result[1] - 1
                        else:
                            tail_ok = True
                    elif last == '"':
                        if (double_quote_count % 2) != 0:
                            double_quote_count = double_quote_count - 1
                            result[1] = result[1] - 1
                        else:
                            tail_ok = True
                    elif last == "'" or last == "`":
                        if (single_quote_count % 2) != 0:
                            single_quote_count = single_quote_count - 1
                            result[1] = result[1] - 1
                        else:
                            tail_ok = True
                    else:
                        tail_ok = True
                    last = ustr[result[0] + result[1] - 1] # sync. variable
                # strip leading punctuation as long as the word doesn't contain
                # balanced brackets (e.g. keep "(xyz)", change "(xyz" to "xyz")
                head_ok = False
                while not head_ok and result[1] > 0:
                    if first == "(":
                        if open_paren_count > close_paren_count:
                            open_paren_count = open_paren_count - 1
                            result[0] = result[0] + 1
                            result[1] = result[1] - 1
                        else:
                            head_ok = True
                    elif first == '"':
                        if (double_quote_count % 2) != 0:
                            double_quote_count = double_quote_count - 1
                            result[0] = result[0] + 1
                            result[1] = result[1] - 1
                        else:
                            head_ok = True
                    elif first == "'" or first == "`":
                        if (single_quote_count % 2) != 0:
                            single_quote_count = single_quote_count - 1
                            result[0] = result[0] + 1
                            result[1] = result[1] - 1
                        else:
                            head_ok = True
                    else:
                        head_ok = True
                    first = ustr[result[0]] # synchronize variable
                    last = ustr[result[0] + result[1] - 1] # sync. variable
            # repeat this rule, as punctuation sometimes appears inside brackets
            if (last == ".") or (last == ",") or (last == ";") or (last == ":"):
                result[1] = result[1] - 1
                last = ustr[result[0] + result[1] - 1] # synchronize variable
        # strip any brackets that appear balanced at both ends
        if result[1] > 1:
            first = ustr[result[0]]
            last = ustr[result[0] + result[1] - 1]
            end_caps_ok = False
            while not end_caps_ok and result[1] > 0:
                if (first == '"' and last == '"') or \
                   (first == "'" and last == "'") or \
                   (first == "`" and last == "`") or \
                   (first == "`" and last == "'") or \
                   (first == '<' and last == '>') or \
                   (first == '(' and last == ')') or \
                   (first == '[' and last == ']') or \
                   (first == '{' and last == '}'):
                    # strip brackets
                    result[0] = result[0] + 1
                    result[1] = result[1] - 2
                    first = ustr[result[0]] # synchronize variable
                    last = ustr[result[0] + result[1] - 1] # sync. variable
                else:
                    end_caps_ok = True
    except Exception as _:
        print("warning, exception while trying to find words:", _)
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
        if as_ascii == 27:
            result = '<ESC>'
        # a "proper" control symbol is preferred, but MacTerm cannot render
        # higher Unicode characters just yet...
        #elif as_ascii < ord(' '):
        #    result = '⌃%c' % chr(ord('@') + as_ascii)
        elif as_ascii < ord(' '):
            result = '^%c' % chr(ord('@') + as_ascii)
        elif chr(as_ascii) in string.printable:
            result = chr(as_ascii)
        else: result = '<%i>' % as_ascii
    else:
        result = '<u%i>' % ord_unicode_16
    return result

def _test():
    """Runs all of this module's "doctest" test cases.
    """
    import doctest
    from . import term_text
    return doctest.testmod(term_text)
