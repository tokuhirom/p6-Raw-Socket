// requirement: c99, IPv6 APIs

#include <stdio.h>
#include <errno.h>
#include <string.h>
#include <stdlib.h>
#include <assert.h>

#include <sys/types.h>
#include <sys/socket.h>
#include <netdb.h>

typedef struct {
  /* TODO: remove */
  const char *errmsg;
  int fd;
  union {
    struct sockaddr_in in;
  } addr;
} p6_socket;

p6_socket* p6_socket_new() {
  p6_socket* self = malloc(sizeof(p6_socket));
  if (self == NULL) {
    return NULL;
  }
  memset(self, 0, sizeof(p6_socket));
  return self;
}

void p6_socket_free(p6_socket* self) {
  free(self);
}

const char* p6_socket_strerror(p6_socket* self) {
  return self->errmsg;
}

int p6_socket_inet_socket(p6_socket* self) {
  assert(self != NULL);
  self->fd = socket(AF_INET, SOCK_STREAM|SOCK_CLOEXEC, 0);
  if (self->fd < 0) {
    self->errmsg = strerror(errno);
  }
  return self->fd;
}

// On success,  zero is returned.  On error, -1 is returned, and errno is
// set appropriately.
int p6_socket_set_so_reuseaddr(p6_socket* self, int n) {
  int retval = setsockopt(self->fd, 0, SO_REUSEADDR, &n, sizeof(int));
  if (retval < 0) {
    self->errmsg = strerror(errno);
  }
  return retval;
}

// On  success,  zero is returned.  On error, -1 is returned, and errno is
// set appropriately.
int p6_socket_inet_bind(p6_socket* self, const char* host, int port) {
  struct sockaddr_in addr;
  int n;
  assert(host != NULL);

  memset(&addr, 0, sizeof(struct sockaddr_in));
  addr.sin_family = AF_INET;
  addr.sin_port = htons(port);
  addr.sin_addr.s_addr = inet_addr(host);

  n = bind(self->fd, (struct sockaddr *)&addr, sizeof(struct sockaddr_in));
  if (n < 0) {
    self->errmsg = strerror(errno);
  }
  return n;
}

int p6_socket_connect(p6_socket* self, const char *host, const char* service) {
  struct addrinfo hints;
  struct addrinfo *result, *rp;
  int sfd;
  int s;

  memset(&hints, 0, sizeof(struct addrinfo));
  hints.ai_family = AF_UNSPEC;    /* Allow IPv4 or IPv6 */
  hints.ai_socktype = SOCK_STREAM;
  hints.ai_flags = AI_ADDRCONFIG;
  hints.ai_protocol = IPPROTO_TCP;
  hints.ai_canonname = NULL;
  hints.ai_addr = NULL;
  hints.ai_next = NULL;

  s = getaddrinfo(host, service, &hints, &result);
  if (s != 0) {
    self->errmsg = gai_strerror(s);
    return -1;
  }

  for (rp = result; rp != NULL; rp = rp->ai_next) {
    sfd = socket(rp->ai_family, rp->ai_socktype,
                 rp->ai_protocol);
    if (sfd == -1)
      continue;

    if (connect(sfd, rp->ai_addr, rp->ai_addrlen) == 0)
      break;                  /* Success */

    close(sfd);
  }

  freeaddrinfo(result);

  if (rp == NULL) {               /* No address succeeded */
    self->errmsg = "could not connect";
    return -1;
  }

  self->fd = sfd;

  return sfd;
}

int p6_socket_listen(p6_socket* self, int backlog) {
  int retval = listen(self->fd, backlog);
  if (retval < 0) {
    self->errmsg = strerror(errno);
  }
  return retval;
}

int p6_socket_accept(p6_socket* self, p6_socket* csock) {
  int retval;
  socklen_t peer_addr_size;
  peer_addr_size = sizeof(csock->addr);
  retval = accept(self->fd, (struct sockaddr*)&(csock->addr), &peer_addr_size);
  if (retval < 0) {
    self->errmsg = strerror(errno);
  } else {
    csock->fd = retval;
  }
  return retval;
}

ssize_t p6_socket_recv(p6_socket* self, char* buf, size_t len, int flags) {
  ssize_t retval = recv(self->fd, buf, len, flags);
  if (retval < 0) {
    self->errmsg = strerror(errno);
  }
  return retval;
}

ssize_t p6_socket_close(p6_socket* self) {
  ssize_t retval = close(self->fd);
  if (retval < 0) {
    self->errmsg = strerror(errno);
  }
  return retval;
}

int p6_socket_send(p6_socket* self, const char* buf, size_t len, int flags) {
  int retval = send(self->fd, buf, len, flags);
  if (retval < 0) {
    self->errmsg = strerror(errno);
  }
  return retval;
}

#ifdef TEST_CLIENT
int main() {
  p6_socket* sock = p6_socket_new();
  if (sock == NULL) {
    printf("cannot allocate memory\n");
    exit(1);
  }

  if (p6_socket_connect(sock, "127.0.0.1", "9999") < 0) {
    printf("%s\n", p6_socket_strerror(sock));
    exit(1);
  }
  const char* msg = "hoge";
  ssize_t sent = p6_socket_send(sock, msg, strlen(msg), 0);
  printf("sent: %d\n", sent);
  char buf[5];
  ssize_t received = p6_socket_recv(sock, buf, sizeof(buf), 0);
  printf("received: %d\n", received);
  if (received < 0) {
    printf("%s\n", p6_socket_strerror(sock));
  } else {
    buf[received] = '\0';
    printf("received: %s\n", buf);
  }
}
#endif
