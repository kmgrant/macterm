#!/usr/bin/env perl

# Doxygen runs this on every source file and generates its
# documentation based only on what the filter produces; therefore
# this script is used to make minor tweaks.
#
# Kevin Grant (kmg@mac.com)
# June 11, 2017

use strict;
use warnings;

while (<>)
{
	# since for decades function comment blocks have ended with a
	# parenthesized dotted version and/or YYYY.MM value, the filter
	# translates them into "\since" directives (which is what these
	# numbers mean) rather than requiring hundreds of lines to be
	# modified across dozens of files to use "\since" directly...
	if (/\(\d+\.\d+\)$/)
	{
		s/\((\d+\.\d+)\)$/\\since $1/;
	}
	print;
}
