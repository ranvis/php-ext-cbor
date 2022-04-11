use 5.012;
use warnings;

# usage: perl -pi .\msvc_mod_make.pl Makefile

s{^(CFLAGS=.+)}{
	my $cFrags = $1;
	# https://github.com/ulfjack/ryu/pull/70#issuecomment-412168459
	$cFrags =~ s{ /Ox }{ /O2 };
	$cFrags;
}me;
