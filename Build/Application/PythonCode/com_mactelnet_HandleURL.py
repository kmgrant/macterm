#!/usr/bin/python
"""Routines to open MacTelnet sessions using information in URLs.

Below, example URLs fully specify all parts, although each type
has a number of optional parts that may be omitted (e.g. user
and port, man page section).

file -- handle URLs of the form "file:///path/to/some/file"
ftp -- handle URLs of the form "ftp://user@host:port"
rlogin -- handle URLs of the form "rlogin://user@host:port"
sftp -- handle URLs of the form "sftp://user@host:port"
ssh -- handle URLs of the form "ssh://user@host:port"
telnet -- handle URLs of the form "telnet://user@host:port"
x_man_page -- handle URLs of the form "x-man-page://section/cmd"
"""
__author__ = 'Kevin Grant <kevin@ieee.org>'
__date__ = '24 August 2006'
__version__ = '3.1.0'

from com_mactelnet_ParseURL import \
	file as _parse_file, \
	ftp as _parse_ftp, \
	sftp as _parse_sftp, \
	ssh as _parse_ssh, \
	telnet as _parse_telnet, \
	x_man_page as _parse_x_man_page
try:
	import Quills
except ImportError, err:
	import sys, os
	if "DYLD_LIBRARY_PATH" in os.environ:
		print >>sys.stderr, "Shared library path:", os.environ["DYLD_LIBRARY_PATH"]
	print >>sys.stderr, "Python path:", sys.path
	raise err

def file(url):
	"""file(url) -> None
	
	Asynchronously open a session with "emacs" in file browser
	mode, looking at the resource from a given "file://" URL.
	
	(Below are REAL testcases run by doctest!)
	
	>>> file('file:///System/Library')
	
	>>> file('file:///usr/bin')
	
	"""
	try:
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
					if path is None: path = '/'
			# emacs is a good choice because it has a command line
			# option to set a start location, it is an excellent file
			# browser and it can remain running to perform other tasks
			args = ['/usr/bin/emacs', "--file=%s" % path];
			session = Quills.Session(args)
		else:
			print "MacTelnet: unsupported form of file URL"
			pass
	except:
		# catching all exceptions is normally bad, but MacTelnet should
		# not crash simply because a URL handler fails
		pass

def ftp(url):
	"""ftp(url) -> None
	
	Asynchronously open a session with the '/usr/bin/ftp'
	command based on the given "ftp://" URL.
	
	(Below are REAL testcases run by doctest!)
	
	>>> ftp('ftp://yourserver.com')
	
	>>> ftp('ftp://userid@yourserver.com:12345')
	
	"""
	try:
		url_info = _parse_ftp(url)
		host = url_info.get('host', None)
		user = url_info.get('user', None)
		port = url_info.get('port', None)
		if host is not None:
			args = ['/usr/bin/ftp'];
			# ftp uses "user@host" form
			if user is not None: host = "%s@%s" % (user, host)
			args.append(host)
			if port is not None: args.append(str(port)) # standalone port
			session = Quills.Session(args)
		else:
			print "MacTelnet: unsupported form of ftp URL"
			pass
	except:
		# catching all exceptions is normally bad, but MacTelnet should
		# not crash simply because a URL handler fails
		pass

def rlogin(url):
	"""rlogin(url) -> None
	
	Considered an alias for telnet(); converts an "rlogin://..."
	URL into a "telnet://..." URL.
	
	(Below are REAL testcases run by doctest!)
	
	>>> rlogin('rlogin://yourserver.com')
	
	>>> rlogin('rlogin://userid@yourserver.com:12345')
	
	"""
	url = url.replace('telnet:', 'rlogin:', 1)
	return telnet(url)

def sftp(url):
	"""sftp(url) -> None
	
	Asynchronously open a session with the '/usr/bin/sftp'
	command based on the given "sftp://" URL.
	
	WARNING: The specification for this URL type is not complete.
	This implementation does not yet provide support for all
	possible bits of information suggested in the proposals to
	date.  It probably needs to be updated when this becomes a
	standard.  (Aren't you glad it's in Python so it's easy to
	fix when that day comes?  I am!!!)
	
	(Below are REAL testcases run by doctest!)
	
	>>> sftp('sftp://yourserver.com')
	
	>>> sftp('sftp://userid@yourserver.com:12345')
	
	"""
	try:
		url_info = _parse_sftp(url)
		host = url_info.get('host', None)
		user = url_info.get('user', None)
		port = url_info.get('port', None)
		if host is not None:
			args = ['/usr/bin/sftp'];
			if port is not None: args.append('-oPort=%s' % str(port))
			# sftp uses "user@host" form
			if user is not None: host = "%s@%s" % (user, host)
			args.append(host)
			session = Quills.Session(args)
		else:
			print "MacTelnet: unsupported form of sftp URL"
			pass
	except:
		# catching all exceptions is normally bad, but MacTelnet should
		# not crash simply because a URL handler fails
		pass

def ssh(url):
	"""ssh(url) -> None
	
	Asynchronously open a session with the '/usr/bin/ssh' command
	based on the given "ssh://" URL.
	
	WARNING: The specification for this URL type is not complete.
	This implementation does not yet provide support for all
	possible bits of information suggested in the proposals to
	date.  It probably needs to be updated when this becomes a
	standard.  (Aren't you glad it's in Python so it's easy to
	fix when that day comes?  I am!!!)
	
	(Below are REAL testcases run by doctest!)
	
	>>> ssh('ssh://yourserver.com')
	
	>>> ssh('ssh://userid@yourserver.com:12345')
	
	"""
	try:
		url_info = _parse_ssh(url)
		host = url_info.get('host', None)
		user = url_info.get('user', None)
		port = url_info.get('port', None)
		if host is not None:
			args = ['/usr/bin/ssh'];
			args.append('-2') # SSH protocol version 2 by default
			if user is not None: args.extend(['-l', user])
			if port is not None: args.extend(['-p', str(port)])
			args.append(host)
			session = Quills.Session(args)
		else:
			print "MacTelnet: unsupported form of ssh URL"
			pass
	except:
		# catching all exceptions is normally bad, but MacTelnet should
		# not crash simply because a URL handler fails
		pass

def telnet(url):
	"""telnet(url) -> None
	
	Asynchronously open a session with the '/usr/bin/telnet'
	command based on the given "telnet://" URL.
	
	(Below are REAL testcases run by doctest!)
	
	>>> telnet('telnet://yourserver.com')
	
	>>> telnet('telnet://userid@yourserver.com:12345')
	
	"""
	try:
		url_info = _parse_telnet(url)
		host = url_info.get('host', None)
		user = url_info.get('user', None)
		port = url_info.get('port', None)
		if host is not None:
			args = ['/usr/bin/telnet'];
			if user is not None: args.extend(['-l', user])
			args.append(host)
			if port is not None: args.append(str(port)) # standalone port
			session = Quills.Session(args)
		else:
			print "MacTelnet: unsupported form of telnet URL"
			pass
	except:
		# catching all exceptions is normally bad, but MacTelnet should
		# not crash simply because a URL handler fails
		pass

def x_man_page(url):
	"""x_man_page(url) -> None
	
	Asynchronously open a session with the '/usr/bin/man' command
	based on the given "x-man-page://" URL.
	
	(Below are REAL testcases run by doctest!)
	
	>>> x_man_page('x-man-page://ls')
	
	>>> x_man_page('x-man-page://3/printf')
	
	"""
	try:
		url_info = _parse_x_man_page(url)
		cmd = url_info.get('cmd', None)
		section = url_info.get('section', None)
		# pull the command name and optional section number out of the URL path
		if section is not None:
			session = Quills.Session(['/usr/bin/man', section, cmd])
		elif cmd is not None:
			session = Quills.Session(['/usr/bin/man', cmd])
		else:
			print "MacTelnet: unsupported form of x-man-page URL"
			pass
	except:
		# catching all exceptions is normally bad, but MacTelnet should
		# not crash simply because a URL handler fails
		pass

def _test():
	import doctest, com_mactelnet_HandleURL
	return doctest.testmod(com_mactelnet_HandleURL)
