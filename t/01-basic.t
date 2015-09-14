#!/usr/bin/env perl6 -Ilib
use v6;
use lib 'lib';
use Test;
use Raw::Socket::INET;
plan 1;

use NativeCall;

sub fork()
    returns Int
    is native { ... }

constant SIGTERM = 15;

sub kill(int $pid, int $sig)
    returns Int
    is native { ... }

my $sock = Raw::Socket::INET.new(
    listen => 60,
    localport => 0,
    reuseaddr => True,
);
my $port = $sock.localport;

note "PORT: $port";

my $pid = fork();
if ($pid == 0) { # child
    while my $csock = $sock.accept {
        my $buf = Buf.new;
        $buf[1024+1] = 0;
        my $received = $csock.recv($buf, 1024, 0);
        my $sent = $csock.send($buf, $received, 0);
        $csock.close;
    }
} elsif ($pid > 0) { # parent
    sleep 1;
    my $client = Raw::Socket::INET.new(
        host => '127.0.0.1',
        port => $port,
    );
    my $msg = "hoge".encode('utf-8');
    my $sent = $client.send($msg, $msg.elems, 0);
    my $buf = Buf.new;
    $buf[1024+1] = 0;
    my $readlen = $client.recv($buf, 1024, 0);
    $buf[$readlen] = 0;

    is($buf.subbuf(0, $readlen).decode('utf-8'), 'hoge');

    kill($pid, SIGTERM);
} else {
    die "fork failed";
}

