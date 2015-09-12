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
  int err;
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

const char* p6_socket_strerror(p6_socket* sock) {
  return strerror(sock->err);
}

int p6_socket_inet_socket(p6_socket* self) {
  assert(self != NULL);
  self->fd = socket(AF_INET, SOCK_STREAM|SOCK_CLOEXEC, 0);
  self->err = errno;
  return self->fd;
}

// On success,  zero is returned.  On error, -1 is returned, and errno is
// set appropriately.
int p6_socket_set_so_reuseaddr(p6_socket* self, int n) {
  int retval = setsockopt(self->fd, 0, SO_REUSEADDR, &n, sizeof(int));
  self->err = errno;
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
  self->err = errno;
  return n;
}

int p6_socket_listen(p6_socket* self, int backlog) {
  int retval = listen(self->fd, backlog);
  self->err = errno;
  return retval;
}

int p6_socket_accept(p6_socket* self, p6_socket* csock) {
  int retval;
  socklen_t peer_addr_size;
  peer_addr_size = sizeof(csock->addr);
  retval = accept(self->fd, (struct sockaddr*)&(csock->addr), &peer_addr_size);
  if (retval < 0) {
    self->err = errno;
  } else {
    csock->fd = retval;
  }
  return retval;
}

int p6_socket_recv(p6_socket* self, char* buf, size_t len, int flags) {
  int retval = recv(self->fd, buf, len, flags);
  self->err = errno;
  return retval;
}

int p6_socket_close(p6_socket* self) {
  int retval = close(self->fd);
  self->err = errno;
  return retval;
}

int p6_socket_send(p6_socket* self, const char* buf, size_t len, int flags) {
  int retval = send(self->fd, buf, len, flags);
  self->err = errno;
  return retval;
}

#if 0

tinysocket* tinysocket_accept(tinysocket* self, tinysocket* csock) {
  struct sockaddr_in addr;
  socklen_t size = sizeof(struct sockaddr_in);

  int cfd = accept(self->fd, (struct sockaddr*)&addr, &size);
  if (cfd >= 0) {
    csock->fd = cfd;
    return csock;
  } else {
    return NULL;
  }
}

// @return NULL if succeeded, error message otherwise.
const char* tinysocket_inet_connect(tinysocket* self, const char* host, const char *service) {
  int sfd;

  struct addrinfo hints;
  memset(&hints, 0, sizeof(struct addrinfo));
  hints.ai_family = AF_UNSPEC;    /* Allow IPv4 or IPv6 */
  hints.ai_socktype = SOCK_DGRAM; /* Datagram socket */
  hints.ai_flags = AI_PASSIVE;    /* For wildcard IP address */
  hints.ai_protocol = 0;          /* Any protocol */
  hints.ai_canonname = NULL;
  hints.ai_addr = NULL;
  hints.ai_next = NULL;

  struct addrinfo *result;

  int s = getaddrinfo(host, service, &hints, &result);
  if (s != 0) {
    return gai_strerror(s);
  }

  struct addrinfo* rp;
  for (rp = result; rp != NULL; rp = rp->ai_next) {
    sfd = socket(rp->ai_family, rp->ai_socktype,
                rp->ai_protocol);
    if (sfd == -1)
      continue;

    if (connect(sfd, rp->ai_addr, rp->ai_addrlen) != -1)
      break;                  /* Success */

    close(sfd);
  }

  freeaddrinfo(result);

  if (rp == NULL) {               /* No address succeeded */
    return strerror(errno);
  }

  self->fd = sfd;

  return NULL;
}

void tinysocket_close(tinysocket* self) {
  close(self->fd);
}


ssize_t tinysocket_read(tinysocket* self, void* buf, size_t count) {
  return read(self->fd, buf, count);
}

const char* tinysocket_strerror() {
  return strerror(errno);
}

// release resource.
void tinysocket_free(tinysocket* self) {
  free(self);
}

int main() {
  tinysocket* sock = tinysocket_new();
  if (sock == NULL) {
    printf("oops\n");
    return;
  }

  const char * err = tinysocket_inet_connect(sock, "127.0.0.1", "http");
  if (err!=NULL) {
    printf("%s\n", err);
    return;
  }
  int wrote = tinysocket_write(sock, "GET / HTTP/1.0\015\012\015\012", sizeof("GET / HTTP/1.0\r\n\r\n"));
  printf("wrote: %d\n", wrote);
  char buf[1024];
  int read = tinysocket_read(sock, buf, sizeof(buf)-1);
  buf[read] = '\0';
  perror("WTF");
  printf("read: %d\n", read);
  printf("%s\n", buf);

  tinysocket_close(sock);
  tinysocket_free(sock);
}
#endif
