#!/usr/bin/perl -w

# FindDeprecatedCalls.pl
#
# Scans the specified application source files and
# looks for apparent calls to APIs that Apple has
# deprecated (suggesting they may not be supported
# soon).  Warnings are printed to standard output,
# one per line, for each case.
#
# Kevin Grant (kmg@mac.com)
# January 5, 2005

use strict;
use FileHandle;
use FindBin;

# read deprecated list from a data file
#
my @deprecated_APIs;
my $fh = new FileHandle("$FindBin::RealBin/DeprecatedListOfAPIs");
(defined $fh) or die "cannot find deprecated APIs file\n";
while (defined($_ = <$fh>))
{
	chomp;
	next if /^\s*$/; # skip blank lines
	push @deprecated_APIs, $_;
}

# find calls to deprecated APIs
#
my $old_ARGV = '';
my $i = 0;
system('/bin/date', '+Started scanning files at %T.');
while (<>)
{
	if ($ARGV ne $old_ARGV)
	{
		$i = 1;
		$old_ARGV = $ARGV;
		warn "...scanning $ARGV...\n";
	}
	foreach my $api (@deprecated_APIs)
	{
		# if searching for Function, search for all of:
		#	)Function(...
		#	 Function(...\n
		#	 Function    \n
		(/(\s|\))\Q$api\E\s*\(/ || /(\s|\))\Q$api\E\s*$/) and warn "$ARGV:$i: use of deprecated API $api()\n";
	}
	++$i;
}
system('/bin/date', '+Finished scanning files at %T.');

