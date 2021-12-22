# vim: set fileencoding=UTF-8 :

"""Routines to open MacTerm sessions using information in URLs.

Below, example URLs fully specify all parts, although each type
has a number of optional parts that may be omitted (e.g. user
and port, man page section).

file -- handle URLs of the form "file:///path/to/some/file"
sftp -- handle URLs of the form "sftp://user@host:port"
ssh -- handle URLs of the form "ssh://user@host:port"
x_man_page -- handle URLs of the form "x-man-page://section/cmd"

"""

__author__ = 'Kevin Grant <kmg@mac.com>'
__date__ = '24 August 2006'
__version__ = '4.0.0'

# note: Quills is a compiled module, library path must be set properly
import quills
from .url_parse import \
    file as _parse_file, \
    sftp as _parse_sftp, \
    ssh as _parse_ssh, \
    x_man_page as _parse_x_man_page

def file(url):
    """file(url) -> None

    Asynchronously open a session with "emacs" in file browser
    mode, looking at the resource from a given "file://" URL.

    Raises an exception for some failures.  Note, however, that
    as an asynchronous process spawn, other types of failures
    could occur later that are not trapped at call time.

    (Below are REAL testcases run by doctest!)

    >>> file('file:///System/Library')

    >>> try:
    ...        file('not a file url!')
    ... except ValueError as e:
    ...        print(e)
    not a file URL

    """
    url_info = _parse_file(url)
    path = url_info.get('path', None)
    if path is not None:
        if len(path) == 0:
            # default to the home directory
            from os import getuid as _getuid
            from pwd import getpwuid as _getpwuid
            (pw_name, pw_passwd, pw_uid, pw_gid, pw_gecos, \
                pw_dir, pw_shell) = _getpwuid(_getuid())
            path = pw_dir
            if path is None or len(path) == 0:
                from os import getenv as _getenv
                path = _getenv('HOME')
                if path is None:
                    path = '/'
        # emacs is a good choice because it has a command line
        # option to set a start location, it is an excellent file
        # browser and it can remain running to perform other tasks
        args = ['/usr/bin/emacs', "--file=%s" % path]
        ignored_session = quills.Session(args)
    else:
        raise ValueError("unsupported form of file URL")

def sftp(url):
    """sftp(url) -> None

    Asynchronously open a session with the '/usr/bin/sftp'
    command based on the given "sftp://" URL.

    Raises an exception for some failures.  Note, however, that
    as an asynchronous process spawn, other types of failures
    could occur later that are not trapped at call time.

    WARNING: The specification for this URL type is not complete.
    This implementation does not yet provide support for all
    possible bits of information suggested in the proposals to
    date.  It probably needs to be updated when this becomes a
    standard.  (Aren't you glad it's in Python so it's easy to
    fix when that day comes?  I am!!!)

    (Below are REAL testcases run by doctest!)

    >>> sftp('sftp://yourserver.com')

    >>> sftp('sftp://userid@yourserver.com:12345')

    >>> try:
    ...        sftp('not an sftp url!')
    ... except ValueError as e:
    ...        print(e)
    not an sftp URL

    """
    url_info = _parse_sftp(url)
    host = url_info.get('host', None)
    user = url_info.get('user', None)
    port = url_info.get('port', None)
    if host is not None:
        args = ['/usr/bin/sftp']
        if port is not None:
            args.append('-oPort=%s' % str(port))
        # sftp uses "user@host" form
        if user is not None:
            host = "%s@%s" % (user, host)
        args.append(host)
        ignored_session = quills.Session(args)
    else:
        raise ValueError("unsupported form of sftp URL")

def ssh(url):
    """ssh(url) -> None

    Asynchronously open a session with the '/usr/bin/ssh' command
    based on the given "ssh://" URL.

    Raises an exception for some failures.  Note, however, that
    as an asynchronous process spawn, other types of failures
    could occur later that are not trapped at call time.

    WARNING: The specification for this URL type is not complete.
    This implementation does not yet provide support for all
    possible bits of information suggested in the proposals to
    date.  It probably needs to be updated when this becomes a
    standard.  (Aren't you glad it's in Python so it's easy to
    fix when that day comes?  I am!!!)

    (Below are REAL testcases run by doctest!)

    >>> ssh('ssh://yourserver.com')

    >>> ssh('ssh://userid@yourserver.com:12345')

    >>> try:
    ...        ssh('not an ssh url!')
    ... except ValueError as e:
    ...        print(e)
    not an ssh URL

    """
    url_info = _parse_ssh(url)
    host = url_info.get('host', None)
    user = url_info.get('user', None)
    port = url_info.get('port', None)
    if host is not None:
        args = ['/usr/bin/ssh']
        args.append('-2') # SSH protocol version 2 by default
        if user is not None:
            args.extend(['-l', user])
        if port is not None:
            args.extend(['-p', str(port)])
        args.append(host)
        ignored_session = quills.Session(args)
    else:
        raise ValueError("unsupported form of ssh URL")

def x_man_page(url):
    """x_man_page(url) -> None

    Asynchronously open a session with the '/usr/bin/man' command
    based on the given "x-man-page://" URL.

    Raises an exception for some failures.  Note, however, that
    as an asynchronous process spawn, other types of failures
    could occur later that are not trapped at call time.

    (Below are REAL testcases run by doctest!)

    >>> x_man_page('x-man-page://ls')

    >>> x_man_page('x-man-page://3/printf')

    """
    url_info = _parse_x_man_page(url)
    cmd = url_info.get('cmd', None)
    section = url_info.get('section', None)
    # pull the command name and optional section number out of the URL path
    if section is not None:
        ignored_session = quills.Session(['/usr/bin/man', section, cmd])
    elif cmd is not None:
        ignored_session = quills.Session(['/usr/bin/man', cmd])
    else:
        raise ValueError("unsupported form of x-man-page URL")
