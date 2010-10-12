#!/usr/bin/perl

# KeyValueToExactMatchProperties.pl
#
# Converts a simple key:value file into the
# "ExactMatch.plist" format required by Apple for
# ensuring that specific terms can find specific
# help pages.
#
# Kevin Grant (kmg@mac.com)
# November 5, 2008

print <<'ENDHEADER';
<?xml version="1.0" encoding="UTF-8"?>
<!DOCTYPE plist SYSTEM "file://localhost/System/Library/DTDs/PropertyList.dtd">
<plist version="0.9">
<dict>
ENDHEADER
while (<>)
{
	chomp;
	/^#/ and next; # comments
	/^\s*$/ and next;
	/^\s*([^\s\:]+)\s*\:\s*(.*)$/ or die "$0: unexpected syntax: $_\n";
	my $key = $1;
	my $value = $2;
	($key =~ /^\-/) and warn "$0: ignoring $key because of leading '-'\n" and next;
	($value =~ /([\"])/) and die "$0: cannot have $1 in $value\n";
	print "\t<key>$key</key>\n\t<string>$value</string>\n";
}
print "</dict>\n</plist>\n";
