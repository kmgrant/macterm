#!/usr/bin/perl

# PrintHTMLDocType.pl
#
# Converts a simple shorthand into a fully-compliant
# <!DOCTYPE...> header.
#
# Valid keys are (add more as needed):
# HTML3.2 HTML4.01 XHTML1.0-UTF-8 HTML5
#
# Kevin Grant (kmg@mac.com)
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
	elsif (/^XHTML1.0-(.*)$/)
	{
		my $encoding = lc($1);
		print "<?xml version=\"1.0\" encoding=\"$encoding\"?>\n";
		print "<!DOCTYPE html PUBLIC \"-//W3C//DTD XHTML 1.0 Strict//EN\" \"http://www.w3.org/TR/xhtml1/DTD/xhtml1-strict.dtd\">\n";
	}
	elsif ($_ eq 'HTML5')
	{
		print "<!DOCTYPE html>\n";
	}
	else
	{
		warn "$0: unknown doctype: $_ (generating default)\n";
		print "<!DOCTYPE HTML PUBLIC \"-//W3C//DTD HTML 3.2 Final//EN\">\n";
	}
}

