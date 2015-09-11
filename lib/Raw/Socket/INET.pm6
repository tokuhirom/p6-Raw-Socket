use v6;

use Raw::Socket;

use NativeCall;

constant SOCK_STREAM = 1;
constant AF_INET = 2;

constant INADDR_ANY = 0;

my class sockaddr_in is repr('CStruct') {
    has int16  $.sin_family is rw; # sa_family_t
    has uint16 $.sin_port   is rw;
    has uint32 $.sin_addr   is rw;
    has uint64 $.dummy;

    method size() {
        16 # 2+2+4+8
    }

    method pack_sockaddr_in($port, $ip_address) {
        my $addr = sockaddr_in.new();
        $addr.sin_family = AF_INET;
        $addr.sin_port = htons($port);
        $addr.sin_addr = $ip_address;
        return $addr;
    }
}


my module private {
    our sub connect(int $sockfd, sockaddr_in $addr, int32 $len)
        returns int
        is native { }
    our sub bind(int $sockfd, sockaddr_in $addr, int32 $len)
        returns int
        is native { }
    our sub accept(int $sockfd, sockaddr_in $addr, CArray[int32] $len)
        returns int
        is native { }
    our sub send(int $sockfd, Blob $buf, uint64 $len, int $flags)
        returns int64
        is native { ... };
    our sub recv(int $sockfd, buf8 $buf, uint64 $len, int $flags)
        returns int64
        is native { ... };
}

sub socket(Int $domain, Int $type, Int $protocol)
    returns Int is native { ... }

sub perror(Str $s)
    returns Str
    is native { ... };

sub bind(int $sock-fd, sockaddr_in $addr) {
    return private::bind($sock-fd, $addr, sockaddr_in.size);
}

sub connect(int $sock-fd, sockaddr_in $addr) {
    return private::connect($sock-fd, $addr, sockaddr_in.size);
}

sub htons(uint16 $hostshort)
    returns uint16
    is native { ... };

sub listen(int $sockfd, int $backlog)
    returns int
    is native { ... };

# int accept(int sockfd, struct sockaddr *addr, socklen_t *addrlen);
sub accept(int $sockfd, sockaddr_in $sockaddr) {
    my $len = CArray[int32].new;
    $len[0] = 16;
    return private::accept($sockfd, $sockaddr, $len);
}

sub inet_ntoa(int32 $sockaddr)
    returns Str
    is native
    { ... }

sub ntohs(uint16 $sockaddr)
    returns uint16
    is native
    { ... }

sub close(int $fd)
    returns int
    is native { ... }

sub inet_addr(Str $src)
    returns int32
    is native { ... }

sub send(int $sockfd, Blob $buf, int $flags) {
    return private::send($sockfd, $buf, $buf.elems, $flags);
}

sub recv(int $sockfd, Buf $buf, int64 $len, int $flags) {
    return private::recv($sockfd, $buf, $len, $flags);
}

class Raw::Socket::INET {
    has int $.fd;

    has $.listen;
    has $.localhost;
    has $.localport;
    has $.host;
    has $.port;

    method new(*%args is copy) {
        fail "Nothing given for new socket to connect or bind to" unless %args<host> || %args<listen>;

        self.bless(|%args)!initialize()
    }

    method !initialize() {
        if ($.listen) {
            self!socket(SOCK_STREAM, 0);
            self.bind($.localport, INADDR_ANY);
            self!listen(20);
        } elsif ($.host) {
            fail "missing port inforamtion" unless $.port.defined;
            # TODO: use getaddrinfo. ref https://github.com/h2o/h2o/blob/master/examples/libh2o/socket-client.c#L93
            my $addr = sockaddr_in.pack_sockaddr_in($.port, inet_addr($.host));
            $!fd = socket(AF_INET, SOCK_STREAM, 0);
            if ($!fd < 0) {
                # TODO: strerror_r
                die "cannot open socket";
            }
            if (connect($!fd, $addr) == -1) {
                die "cannot connect: $.host:$.port";
            }
        }
        return self;
    }

    method !socket($type, $protocol) {
        $!fd = socket(AF_INET, $type, $protocol);
        if ($!fd < 0) {
            die "cannot open socket";
        }
    }

    method bind($port, $host) {
        my $sockaddr = sockaddr_in.pack_sockaddr_in($port, $host);
        if (bind($!fd, $sockaddr) != 0) {
            die "cannot bind";
        }
    }

    method !listen($backlog) {
        if (listen($!fd, 20) != 0 ) {
            die "cannot listen";
        }
    }

    method accept() {
        my $client_addr = sockaddr_in.new();
        my $clientfd = accept($!fd, $client_addr);
        my $new_sock := $?CLASS.bless(fd => $clientfd);
        return $new_sock;
    }

    method recv(Buf $buf, int64 $len, int $flags) {
        return recv($!fd, $buf, $len, $flags);
    }

    method send(Blob $buf, int $flags) {
        return send($!fd, $buf, $flags);
    }

    method close() {
        return close($!fd);
    }
}
