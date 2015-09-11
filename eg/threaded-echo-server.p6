use v6;
use NativeCall;

use Raw::Socket::INET;

# threaded echo server.

# -------------------------------------------------------------------------

sub info($msg as Str) {
    say "[{$*THREAD.id}] $msg";
}

# -------------------------------------------------------------------------

class Echod {
    has @!threads;
    has $!sock;

    method listen($port) {
        $!sock = Raw::Socket::INET.new(
            listen => True,
            localport => $port,
        );
    }

    method spawn-child() {
        @!threads.push(start {
            self.work();
        });
    }

    method work() {
        while (1) {
            info 'accepting..';
            my $csock = $!sock.accept();
            info "clientfd: $csock";
            # say inet_ntoa($client_addr.sin_addr);
            # say ntohs($client_addr.sin_port);

            my $buf = buf8.new;
            $buf[100-1] = 0; # extend buffer

            loop {
                my $readlen = $csock.recv($buf, 100, 0);
                if ($readlen <= 0) {
                    info("closed");
                    $csock.close();
                    last;
                }
                my $sent = $csock.send($buf.subbuf(0, $readlen), 0);
            }
        }
    }

    method run($n) {
        for 1..$n {
            self.spawn-child();
        }

        for @!threads {
            .join
        }
    }
}

my $port = @*ARGS.elems > 0 ?? @*ARGS[0].Int !! 9800;

say "listening $port";

my $echod = Echod.new();
$echod.listen($port);
$echod.run(10);

