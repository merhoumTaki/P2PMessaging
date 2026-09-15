#ifndef MAIN_H_INCLUDED
#define MAIN_H_INCLUDED

#include <stdatomic.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

#define PORT 8080
#define BUFFER_SIZE 1024
#define HISTORY_FILENAME_SIZE 24
#define MSIZE_OF_fileNameSize 8

#if defined(_WIN32) || defined(_WIN64)
#include <process.h>
#include <stddef.h>
#include <winsock2.h>
#include <ws2tcpip.h>

#define THREAD_RET unsigned __stdcall
#define NAME_MAX 255
#define PATH_MAX MAX_PATH
#define SHUT_RDWR SD_BOTH
#define EXIT_THREAD 1
#define socklen_t int

#else

#include <arpa/inet.h>
#include <limits.h>
#include <pthread.h>
#include <sched.h>
#include <sys/socket.h>
#include <unistd.h>

#define SOCKET int
#define closesocket(c) close(c)
#define INVALID_SOCKET -1
#define SOCKET_ERROR -1

#define THREAD_RET void *
#define HANDLE pthread_t
#define _beginthreadex pthread_create
#define CRITICAL_SECTION pthread_mutex_t
#define EnterCriticalSection(c) pthread_mutex_lock(c)
#define LeaveCriticalSection(c) pthread_mutex_unlock(c)
#define EXIT_THREAD NULL

#endif

typedef struct Node {
  int connectionC;
  char ip[INET_ADDRSTRLEN];
  struct Node *next;
} Node;

typedef struct ClientInformation {
  SOCKET new_socket;
  char client_ip[INET_ADDRSTRLEN];
  HANDLE hThread;
} ClientInformation;

typedef struct Threads {
  HANDLE hThread;
  struct Threads *next;
} Threads;

void addBeginning(const char *ip);
void addBeginningThread(const HANDLE hThread);
void deleteThread(const HANDLE hThread);
void joinThread();
void printIp();
void freeList();
void clearInputBuffer();
void getInput(const char *demand, char *buffer, size_t size);
void fileExchangeH(char *historyFile, char *ip, char *file);
int send_all(const SOCKET sock, char *buffer, const int length);
int recv_all(const SOCKET sock, char *buffer, const int length);
THREAD_RET ReceivingThread(void *arg);
THREAD_RET ListenThread(void *arg);

#endif // MAIN_H_INCLUDED