#include <stdio.h>
#include <stdlib.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <unistd.h>
#include <poll.h>
#include <stddef.h>

#ifndef INFTIM
#define INFTIM -1
#endif

#define MAX_CAP 100

#define error(msg) do {\
   printf("ERROR %s \n", msg);\
} while(0);\

int create_tcp_socket(unsigned int tcp_port) {
   
   const int server_socket = socket(PF_INET, SOCK_STREAM, 0);
   if (server_socket < 0) {
      error("Failed to create server socket");
   }

   struct sockaddr_in address = {0};
   address.sin_family = AF_INET;
   address.sin_port = htons(tcp_port);
   address.sin_addr.s_addr = htonl(INADDR_ANY);

   if (bind(server_socket, (struct sockaddr *)&address, sizeof address) != 0) {
      error("bind() failed");
   }

   if (listen(server_socket, SOMAXCONN) != 0) {
      error("listen() failed");
   }

   return server_socket;
}

typedef void (*Handle)(void* ev, int fd);

typedef struct handler_t {
   int fd;
   Handle handle;
} handler_t;

/* ---------------- EVENT LOOP ---------------- */

typedef struct EventLoop {
   handler_t *handlers;
   int size;
   int cap;
} EventLoop;

EventLoop* evloop_init(int cap) {
   EventLoop* ev = (EventLoop*) malloc(sizeof(EventLoop*));
   ev->handlers = (handler_t *) malloc(cap * sizeof(handler_t));
   ev->size = 0;
   ev->cap = cap;

   return ev;
}

int evloop_is_full(EventLoop* ev) {
   return ev->cap == ev->size;
}

void evloop_register(EventLoop *ev, handler_t handler) {
   printf("[register] %d\n", handler.fd);
   if (evloop_is_full(ev)) {
      /* ignore for know, maybe later expand */
      return;
   }
   ev->handlers[ev->size] = handler;
   ev->size += 1;
}

void evloop_unregister(EventLoop* ev, int fd){
   if(ev->size == 0) return;
   
   int pos = -1;
   for(int i=0; i<ev->size; i++)
      if(ev->handlers[i].fd == fd){
         pos = i;
         break;
      }
   ev->handlers[pos] = ev->handlers[ev->size-1];
   ev->size -= 1;
}

void evloop_emit(EventLoop* ev, struct pollfd* fds) {
   for(int i=0; i<ev->size; i++)
      if ((POLLRDNORM | POLLERR) & fds[i].revents) {
         printf("Hi[%d]\n", fds[i].fd);
         ev->handlers[i].handle(ev, fds[i].fd);
      }
}

void evloop_handle(EventLoop* ev) {
   if(ev->size == 0) return;

   struct pollfd fds[MAX_CAP] = {0};
   for(int i=0; i<ev->size; i++) {
      fds[i].fd = ev->handlers[i].fd;
      fds[i].events = POLLRDNORM;
   }

   if(0 < poll(fds, ev->size, INFTIM)){
      evloop_emit(ev, fds);
      return;
   }
   error("Poll failure");
}

/* ---------------- Client socket event handler ---------------- */

void client_handle(void * loop, int fd) {
   EventLoop* ev = loop;

   char msg[1024] = {0};
   const ssize_t receiveResult = recv(fd, msg, sizeof msg, 0);
   
   if(0 < receiveResult) {
      printf("Client[%d]: received - %s\n", fd, msg);
      // TODO(mahmednabil): handle sending back :)
      return;
   }
   evloop_unregister(ev, fd);
}

/* ---------------- Server socket event handler ---------------- */

void server_handle(void* ev, int fd) {

   struct sockaddr_in client_addr = {0};
   socklen_t addr_size = sizeof client_addr;
   int client_socket = accept(
      fd, 
      (struct sockaddr *)&client_addr, 
      &addr_size
   );

   handler_t handler = {
      .fd = client_socket,
      .handle = client_handle
   };
   evloop_register(ev, handler);
}


int main() {
   EventLoop* ev = evloop_init(10);

   int sock = create_tcp_socket(8088);
   // register server itself
   handler_t handler = {
      .fd = sock,
      .handle = server_handle,
   };
   evloop_register(ev, handler);

   for(;;) {
      evloop_handle(ev);
   }
}