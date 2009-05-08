#!/usr/bin/python
# vim: set fileencoding=UTF-8 :
"""Routines to distill various kinds of URLs into components.

Every function returns a dictionary with one or more of the
following keys defined (which ones depends on the URL type):

    'user': user ID for login to server
    'host': domain name or IP address (v4 or v6) of server
    'port': TCP port of server
    'section': (man pages) section to look up resource in
    'cmd': (man pages) command to look up
    'path': the remainder after the host name and port

Below, example URLs fully specify all parts, although each type
has a number of optional parts that may be omitted (e.g. user
and port, man page section).

file -- handle URLs of the form "file:///path/to/some/file"
ftp -- handle URLs of the form "ftp://user@host:port"
sftp -- handle URLs of the form "sftp://user@host:port"
ssh -- handle URLs of the form "ssh://user@host:port"
telnet -- handle URLs of the form "telnet://user@host:port"
x_man_page -- handle URLs of the form "x-man-page://section/cmd"
"""
__author__ = 'Kevin Grant <kevin@ieee.org>'
__date__ = '24 August 2006'
__version__ = '4.0.0'

from pymactelnet.utilities import \
    slash_free_path as _slash_free_path, \
    sort_dict as _sort_dict

def _host_port(netloc):
    """_host_port(netloc) -> (host, port)
    
    Return a tuple of the host and port elements of a net
    location component in a URL.  Either of the elements may be
    None if it is not present in the original URL.
    
    (Below are REAL testcases run by doctest!)
    
    >>> _host_port("nowhere.com")
    ('nowhere.com', None)
    
    >>> _host_port("nowhere.com:123")
    ('nowhere.com', 123)
    
    >>> _host_port(":")
    (None, None)
    
    """
    elements = netloc.split(':')
    host = None
    port = None
    if len(elements) > 2 or len(elements) == 0:
        print "MacTelnet: unexpected number of host:port elements in URL"
    else:
        host = elements[0]
        if len(host) == 0: host = None
        try:
            port = int(elements[1])
        except IndexError:
            pass # no port - fine
        except ValueError:
            port = None # e.g. if port = "string"
    return (host, port)

def _user_host_port(netloc):
    """_user_host_port(netloc) -> (user, host, port)
    
    Return a tuple of the user, host and port elements of a net
    location component in a URL.  Any of the elements may be None
    if it is not present in the original URL.
    
    (Below are REAL testcases run by doctest!)
    
    >>> _user_host_port("nowhere.com")
    (None, 'nowhere.com', None)
    
    >>> _user_host_port("me@nowhere.com")
    ('me', 'nowhere.com', None)
    
    >>> _user_host_port("me@nowhere.com:123")
    ('me', 'nowhere.com', 123)
    
    >>> _user_host_port("@")
    (None, None, None)
    
    >>> _user_host_port("nowhere.com:123")
    (None, 'nowhere.com', 123)
    
    >>> _user_host_port(":")
    (None, None, None)
    
    >>> _user_host_port("@:")
    (None, None, None)
    
    """
    elements = netloc.split('@')
    user = None
    host = None
    port = None
    if len(elements) > 2 or len(elements) == 0:
        print "MacTelnet: unexpected number of @ elements in URL"
    else:
        if len(elements) == 2:
            user = elements[0]
            if len(user) == 0: user = None
            remainder = elements[1]
        else:
            remainder = elements[0]
        (host, port) = _host_port(remainder)
    return (user, host, port)

def file(url):
    """file(url) -> None
    
    Return dictionary with the 'path' component of the given
    "file://" URL.
    
    (Below are REAL testcases run by doctest!)
    
    >>> _sort_dict(file('file://'))
    'path:/'
    
    >>> _sort_dict(file('file:///Users/kevin/Library'))
    'path:/Users/kevin/Library'
    
    >>> try:
    ...    file('telnet://userid@yourserver.com:12345')
    ... except ValueError, e:
    ...    print e
    not a file URL
    
    """
    result = dict()
    import urlparse
    allow_pound_notation = False
    (scheme, netloc, path, params, query, fragment) = \
        urlparse.urlparse(url, 'ftp', allow_pound_notation)
    if 'file' != scheme: raise ValueError("not a file URL")
    if path == '': path = '/'
    result['path'] = path
    return result

def ftp(url):
    """ftp(url) -> None
    
    Return dictionary with 'host', 'user' and 'port' components of
    the given "ftp://" URL (if defined).
    
    (Below are REAL testcases run by doctest!)
    
    >>> _sort_dict(ftp('ftp://yourserver.com'))
    'host:yourserver.com port:None user:None'
    
    >>> _sort_dict(ftp('ftp://userid@yourserver.com:12345'))
    'host:yourserver.com port:12345 user:userid'
    
    >>> try:
    ...    ftp('telnet://userid@yourserver.com:12345')
    ... except ValueError, e:
    ...    print e
    not an ftp URL
    
    """
    result = dict()
    import urlparse
    allow_pound_notation = False
    (scheme, netloc, path, params, query, fragment) = \
        urlparse.urlparse(url, 'ftp', allow_pound_notation)
    if 'ftp' != scheme: raise ValueError("not an ftp URL")
    (user, host, port) = _user_host_port(netloc)
    result['user'] = user
    result['host'] = host
    result['port'] = port
    return result

def sftp(url):
    """sftp(url) -> None
    
    Return dictionary with 'host', 'user' and 'port' components of
    the given "sftp://" URL (if defined).
    
    WARNING: The specification for this URL type is not complete.
    This implementation does not yet provide support for all
    possible bits of information suggested in the proposals to
    date.  It probably needs to be updated when this becomes a
    standard.  (Aren't you glad it's in Python so it's easy to
    fix when that day comes?  I am!!!)
    
    (Below are REAL testcases run by doctest!)
    
    >>> _sort_dict(sftp('sftp://yourserver.com'))
    'host:yourserver.com port:None user:None'
    
    >>> _sort_dict(sftp('sftp://userid@yourserver.com:12345'))
    'host:yourserver.com port:12345 user:userid'
    
    >>> try:
    ...    sftp('ftp://userid@yourserver.com:12345')
    ... except ValueError, e:
    ...    print e
    not an sftp URL
    
    """
    result = dict()
    # the Python urlparse module does not support sftp:// because it
    # is not fully specified; so, parse manually
    if not url.startswith('sftp://'): raise ValueError("not an sftp URL")
    netloc = _slash_free_path(url.replace('sftp:', '', 1))
    (user, host, port) = _user_host_port(netloc)
    result['user'] = user
    result['host'] = host
    result['port'] = port
    return result

def ssh(url):
    """ssh(url) -> None
    
    Return dictionary with 'host', 'user' and 'port' components of
    the given "ssh://" URL (if defined).
    
    WARNING: The specification for this URL type is not complete.
    This implementation does not yet provide support for all
    possible bits of information suggested in the proposals to
    date.  It probably needs to be updated when this becomes a
    standard.  (Aren't you glad it's in Python so it's easy to
    fix when that day comes?  I am!!!)
    
    (Below are REAL testcases run by doctest!)
    
    >>> _sort_dict(ssh('ssh://yourserver.com'))
    'host:yourserver.com port:None user:None'
    
    >>> _sort_dict(ssh('ssh://userid@yourserver.com:12345'))
    'host:yourserver.com port:12345 user:userid'
    
    >>> try:
    ...    ssh('telnet://userid@yourserver.com:12345')
    ... except ValueError, e:
    ...    print e
    not an ssh URL
    
    """
    result = dict()
    # the Python urlparse module does not support ssh:// because it
    # is not fully specified; so, parse manually
    if not url.startswith('ssh://'): raise ValueError("not an ssh URL")
    netloc = _slash_free_path(url.replace('ssh:', '', 1))
    (user, host, port) = _user_host_port(netloc)
    result['user'] = user
    result['host'] = host
    result['port'] = port
    return result

def telnet(url):
    """telnet(url) -> None
    
    Return dictionary with 'host', 'user' and 'port' components of
    the given "telnet://" URL (if defined).
    
    (Below are REAL testcases run by doctest!)
    
    >>> _sort_dict(telnet('telnet://yourserver.com'))
    'host:yourserver.com port:None user:None'
    
    >>> _sort_dict(telnet('telnet://userid@yourserver.com:12345'))
    'host:yourserver.com port:12345 user:userid'
    
    >>> try:
    ...    telnet('ftp://userid@yourserver.com:12345')
    ... except ValueError, e:
    ...    print e
    not a telnet URL
    
    """
    result = dict()
    import urlparse
    allow_pound_notation = False
    (scheme, netloc, path, params, query, fragment) = \
        urlparse.urlparse(url, 'telnet', allow_pound_notation)
    if 'telnet' != scheme: raise ValueError("not a telnet URL")
    (user, host, port) = _user_host_port(netloc)
    result['user'] = user
    result['host'] = host
    result['port'] = port
    return result

def x_man_page(url):
    """x_man_page(url) -> None
    
    Return dictionary with 'section' and 'cmd' components of the
    given "x-man-page://" URL (if defined).
    
    (Below are REAL testcases run by doctest!)
    
    >>> _sort_dict(x_man_page('x-man-page://ls'))
    'cmd:ls section:None'
    
    >>> _sort_dict(x_man_page('x-man-page://3/printf'))
    'cmd:printf section:3'
    
    >>> try:
    ...    x_man_page('ftp://userid@yourserver.com:12345')
    ... except ValueError, e:
    ...    print e
    not an x-man-page URL
    
    """
    result = dict()
    import urlparse
    allow_pound_notation = False
    (scheme, netloc, path, params, query, fragment) = \
        urlparse.urlparse(url, 'x-man-page', allow_pound_notation)
    if 'x-man-page' != scheme: raise ValueError("not an x-man-page URL")
    path = _slash_free_path(path).rstrip()
    elements = path.split('/')
    # pull the command name and optional section number out of the URL path
    section = None
    cmd = None
    if len(elements) == 2:
        section = elements[0]
        cmd = elements[1]
    elif len(elements) == 1:
        cmd = elements[0]
    else:
        print "MacTelnet: unsupported form of x-man-page URL"
        pass
    result['section'] = section
    result['cmd'] = cmd
    return result

def _test():
    import doctest, pymactelnet.url.parse
    return doctest.testmod(pymactelnet.url.parse)

if __name__ == '__main__':
    _test()
