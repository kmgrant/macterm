#!/usr/bin/perl -w

# WarnAboutHeaders.pl
#
# Scans the specified MacTelnet source files and
# looks for #include statements that may not be
# necessary.  Warnings are printed to standard
# output, one per line, for each case.
#
# The algorithm is quite simple; since MacTelnet
# follows the naming standard of prefixing symbol
# names with "<filename>_", this script just looks
# for at least one reference to "foo_" when it
# encounters an #include "foo.h".
#
# Kevin Grant (kevin@ieee.org)
# December 29, 2002

use strict;
use FileHandle;

# the following MacTelnet modules do not follow the
# proper convention and therefore no warnings are
# issued if they are used (or they are expected to
# always be included)
#
my @skip_modules = qw
(
	AlertMessages
	Commands
	Configure
	ConnectionData
	Console
	DialogUtilities
	InternetConfig
	ObjectClassesAE
	MacroManager
	QuillsBase
	QuillsEvents
	QuillsPrefs
	QuillsSession
	QuillsTerminal
	RasterGraphicsKernel
	RasterGraphicsScreen
	tekdefs
	TektronixFont
	TektronixRealGraphics
	TektronixVirtualGraphics
	Terminology
	UIStrings_PrefsWindow
	UniversalDefines
	vskeys
	VTKeys
);

my @files = @ARGV;
system('/bin/date', '+Started scanning files at %T.');
foreach my $file (@files)
{
	my $fh = new FileHandle($file);
	defined($fh) or warn "$file: cannot read\n" and next;
	
	my %module_info; # retain information about which modules are used
	
	# do not start looking for modules until the
	# "MacTelnet includes" marker is found
	#
	while (defined($_ = <$fh>))
	{
		last if /MacTelnet includes/; # skip Mac headers, etc.
	}
	
	# figure out which MacTelnet modules are included
	#
	while (defined($_ = <$fh>))
	{
		if (/^#\s*include\s+\"([^\.]+)\.h\"/)
		{
			#print "$file: found module $1\n";
			$module_info{$1} = 1;
		}
		
		# do not read more of the file than necessary
		#
		last if /constants|types|variables|internal|public/ or
    			/\#pragma mark (constants|types|variables|internal|public)/i;
	}
	
	# ensure there is at least one "<filename>_" API usage
	# for each "<filename>.h" that was included
	#
	while (defined($_ = <$fh>))
	{
		foreach my $module (keys %module_info)
		{
			(/\Q$module\E_/) and delete $module_info{$module} and last;
		}
		
		# stop when at least one API reference for
		# each included module is found
		last if (scalar(keys %module_info) == 0);
	}
	
	# if there's nothing to warn about, say that
	#
	#scalar(keys %module_info) or print "$file: appears clean\n";
	
	# warn about any modules that were not apparently used
	#
	foreach my $module (sort keys %module_info)
	{
		next if (grep /\Q$module\E/, @skip_modules);
		warn "$file: may not need $module.h\n";
	}
}
system('/bin/date', '+Finished scanning files at %T.');
