use LibraryMake;

my $destdir = 'lib/Raw/Socket/';
my %vars = get-vars($destdir);
%vars{'WINSOCK'} = $*DISTRO.is-win ?? "-lws2_32" !! "";
process-makefile('.', %vars);
