#!/usr/bin/perl

# PrintHTMLDocType.pl
#
# Converts a simple shorthand into a fully-compliant
# <!DOCTYPE...> header.
#
# Valid keys are (add more as needed):
# HTML3.2 HTML4.01
#
# Kevin Grant (kevin@ieee.org)
# March 7, 2005

(scalar @ARGV) or die "$0: requires argument\n";

foreach (@ARGV)
{
	if ($_ eq 'HTML3.2')
	{
		print "<!DOCTYPE HTML PUBLIC \"-//W3C//DTD HTML 3.2 Final//EN\">\n";
	}
	elsif ($_ eq 'HTML4.01')
	{
		print "<!DOCTYPE HTML PUBLIC \"-//W3C//DTD HTML 4.01//EN\" \"http://www.w3.org/TR/html4/strict.dtd\">\n";
	}
	else
	{
		warn "$0: unknown doctype: $_ (generating default)\n";
		print "<!DOCTYPE HTML PUBLIC \"-//W3C//DTD HTML 3.2 Final//EN\">\n";
	}
}

