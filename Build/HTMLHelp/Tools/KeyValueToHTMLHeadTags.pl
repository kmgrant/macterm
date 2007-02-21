#!/usr/bin/perl

# KeyValueToHTMLHeadTags.pl
#
# Converts a simple key:value file into a series of
# meta tags and other HTML header components.
#
# Keys are generally converted directly into meta
# tags, except for special ones:
#
#	content-type	becomes <meta http-equiv...>
#			with content="value"
#
#	stylesheet	becomes <link rel=...> with
#			href="value"
#
#	title		becomes <title>value</title>
#
#	<other>		becomes a meta tag of that name
#			with content="value"
#
# Kevin Grant (kevin@ieee.org)
# March 7, 2005

while (<>)
{
	chomp;
	/^\s*$/ and next;
	/^\s*([^\s\:]+)\s*\:\s*(.*)$/ or die "$0: unexpected syntax: $_\n";
	my $key = $1;
	my $value = $2;
	($key =~ /^\-/) and warn "$0: ignoring $key because of leading '-'\n" and next;
	($value =~ /([\"])/) and die "$0: cannot have $1 in $value\n";
	if ($key eq 'content-type')
	{
		print "<meta http-equiv=\"Content-Type\" content=\"$value\">\n";
	}
	elsif ($key eq 'stylesheet')
	{
		print "<link rel=\"stylesheet\" type=\"text/css\" href=\"$value\">\n";
	}
	elsif ($key eq 'title')
	{
		print "<title>$value</title>\n";
	}
	else
	{
		print "<meta name=\"$key\" content=\"$value\">\n";
	}
}

