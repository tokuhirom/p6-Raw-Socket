use LibraryMake;

my $destdir = 'lib/Raw/Socket/';
my %vars = get-vars($destdir);
process-makefile('.', %vars);
