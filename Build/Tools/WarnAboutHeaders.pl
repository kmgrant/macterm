#!/usr/bin/perl -w

# WarnAboutHeaders.pl
#
# Scans the specified application source files and
# looks for #include or #import statements that
# may not be necessary.  Warnings are printed to
# standard output, one per line, for each case.
#
# The algorithm is quite simple; since MacTerm
# follows the naming standard of prefixing symbol
# names with "<filename>_", this script just looks
# for at least one reference to "foo_" when it
# encounters an #include/#import "foo.h".
#
# Kevin Grant (kmg@mac.com)
# December 29, 2002

use strict;
use FileHandle;

# the following MacTerm modules do not follow the
# proper convention, or are expected to be included
# frequently, so no warnings are issued for them
#
my @skip_modules = qw
(
	AlertMessages
	Commands
	Console
	ConstantsRegistry
	DialogUtilities
	QuillsBase
	QuillsEvents
	QuillsPrefs
	QuillsSession
	QuillsSWIG
	QuillsTerminal
	Terminology
	UIStrings_PrefsWindow
	UniversalDefines
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
	# "application includes" marker is found
	#
	while (defined($_ = <$fh>))
	{
		last if /application includes/; # skip Mac headers, etc.
	}
	
	# figure out which application modules are included
	#
	while (defined($_ = <$fh>))
	{
		if (/^#\s*(include|import)\s+\"([^\.]+)\.h\"/)
		{
			#print "$file: found module $1\n";
			$module_info{$2} = 1;
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
