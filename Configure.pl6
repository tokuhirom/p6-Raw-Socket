use LibraryMake;

my $destdir = 'lib/Raw/Socket/';
my %vars = get-vars($destdir);
%vars{'WINSOCK'} = "-lws2_32" if $*DISTRO.is-win;
process-makefile('.', %vars);
