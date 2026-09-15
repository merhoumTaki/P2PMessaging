
#include "main.h"

Node *head = NULL;
Threads *threads = NULL;
atomic_bool Exit = true;
CRITICAL_SECTION iplist;
CRITICAL_SECTION Threadslist;
SOCKET server_socket = INVALID_SOCKET;

int main() {
#if defined(_WIN32) || defined(_WIN64)
  InitializeCriticalSection(&iplist);
  InitializeCriticalSection(&Threadslist);

  WSADATA wsaData;
  if (WSAStartup(MAKEWORD(2, 2), &wsaData) != 0) {
    printf("\n[Error]: Winsock initialization failed. Error code: %d\n",
           WSAGetLastError());
    return 1;
  }

  HANDLE hThread = (HANDLE)_beginthreadex(NULL, 0, ListenThread, NULL, 0, NULL);
  if (hThread != NULL) {
    addBeginningThread(hThread);
  }
#else
  pthread_mutex_init(&iplist, NULL);
  pthread_mutex_init(&Threadslist, NULL);

  pthread_t hThread;
  if (pthread_create(&hThread, NULL, ListenThread, NULL) == 0) {
    addBeginningThread(hThread);
  } else {
    printf("\n[Error]: Failed to open listening thread.\n");
  }
#endif

  char choice;
  char target_ip[INET_ADDRSTRLEN];
  char message[BUFFER_SIZE];
  char hFilename[HISTORY_FILENAME_SIZE];
  char filePath[PATH_MAX];
  char fileName[NAME_MAX + 1];
  size_t bytesRead;

  while (atomic_load(&Exit)) {
    printf("1: IP Connection History\n");
    printf("2: Message someone\n");
    printf("3: Send a file\n");
    printf("4: Exit\n");
    printf("Enter choice: ");

    if (scanf("%c", &choice) != 1) {
      printf("\n[Input closed. Exiting...]\n");
      atomic_store(&Exit, false);
      break;
    }
    clearInputBuffer();

    switch (choice) {
    case '1': {
      printIp();
      break;
    }
    case '2': {
      SOCKET sock = socket(AF_INET, SOCK_STREAM, 0);
      struct sockaddr_in peer_addr;
      peer_addr.sin_family = AF_INET;
      peer_addr.sin_port = htons(PORT);

      getInput("Enter target IP address: ", target_ip, INET_ADDRSTRLEN);

      if (inet_pton(AF_INET, target_ip, &peer_addr.sin_addr) == 1) {
        if (connect(sock, (struct sockaddr *)&peer_addr, sizeof(peer_addr)) !=
            SOCKET_ERROR) {
          printf("Enter message: ");
          snprintf(hFilename, HISTORY_FILENAME_SIZE, "Hof_%s.txt", target_ip);
          FILE *messageHF = fopen(hFilename, "a");
          message[0] = '1';
          fgets(message + 1, BUFFER_SIZE - 1, stdin);

          if (send(sock, message, (int)strlen(message), 0) > 0) {
            if (messageHF != NULL) {
              fprintf(messageHF, "\n[Message sent to %s]: %s\n", target_ip,
                      message + 1);
            } else {
              printf("\n[Error]: Failed to open message history file.\n");
            }

            while (strchr(message, '\n') == NULL) {
              if (fgets(message, BUFFER_SIZE - 1, stdin) == NULL) {
                break;
              }

              if (send(sock, message, (int)strlen(message), 0) <= 0) {
                break;
              }

              if (messageHF != NULL) {
                fprintf(messageHF, "\n[Message sent to %s]: %s\n", target_ip,
                        message);
              } else {
                printf("\n[Error]: Failed to open message history file.\n");
              }
            }
            addBeginning(target_ip);
            printf("\n[Sent successfully!]\n");
          } else {
            printf("\n[Error]: Failed to send the message.\n");
          }

          if (messageHF != NULL) {
            fclose(messageHF);
          }
        } else {
          printf("\n[Error]: Failed to connect to specified peer!\n");
        }
      } else {
        printf("\n[Error]: Invalid IP address.\n");
      }

      closesocket(sock);
      break;
    }
    case '3': {
      getInput("Enter target IP address: ", target_ip, INET_ADDRSTRLEN);
      getInput("Enter file location: ", filePath, PATH_MAX);

      FILE *filetoS = fopen(filePath, "rb");
      if (filetoS != NULL) {
#if defined(_WIN32) || defined(_WIN64)
        char *last = strrchr(filePath, '\\');
        char *slash = strrchr(filePath, '/');
        if (last != NULL || slash != NULL) {
          if (slash != NULL) {
            if (last != NULL) {
              if (last < slash) {
                last = slash;
              }
            } else {
              last = slash;
            }
          }
          strcpy(fileName, last + 1);
        } else {
          strcpy(fileName, filePath);
        }
#else
        if (strrchr(filePath, '/') != NULL) {
          snprintf(fileName, NAME_MAX + 1, "%s", strrchr(filePath, '/') + 1);
        } else {
          snprintf(fileName, NAME_MAX + 1, "%s", filePath);
        }
#endif
        size_t fileNameSize = strlen(fileName);
        message[0] = '2';
        sprintf(message + 1, "%08zu", fileNameSize);
        strcpy(message + MSIZE_OF_fileNameSize + 1, fileName);
        bytesRead = fileNameSize + MSIZE_OF_fileNameSize + 1;

        SOCKET sock = socket(AF_INET, SOCK_STREAM, 0);
        struct sockaddr_in peer_addr;
        peer_addr.sin_family = AF_INET;
        peer_addr.sin_port = htons(PORT);

        if (inet_pton(AF_INET, target_ip, &peer_addr.sin_addr) == 1) {
          if (connect(sock, (struct sockaddr *)&peer_addr, sizeof(peer_addr)) !=
              SOCKET_ERROR) {
            do {
              if (send_all(sock, message, bytesRead) <= 0) {
                printf("\n[Error]: Failed to send file data.\n");
                break;
              }
            } while ((bytesRead = fread(message, 1, BUFFER_SIZE, filetoS)) > 0);

            addBeginning(target_ip);
            fileExchangeH(hFilename, target_ip, filePath);
            printf("\n[File sent successfully!]\n");
          } else {
            printf("\n[Error]: Failed to connect to specified peer!\n");
          }
        } else {
          printf("\n[Error]: Invalid IP address.\n");
        }
        fclose(filetoS);
        closesocket(sock);
      } else {
        printf("\n[Error]: Failed to open the specified file.\n");
      }

      break;
    }
    case '4': {
      atomic_store(&Exit, false);
      break;
    }
    default: {
      printf("\n[Warning]: Invalid choice. Please try again.\n");
      break;
    }
    }
  }

  if (server_socket != INVALID_SOCKET) {
    shutdown(server_socket, SHUT_RDWR);
    closesocket(server_socket);
    server_socket = INVALID_SOCKET;
  }

  joinThread();
  freeList();

#if defined(_WIN32) || defined(_WIN64)
  DeleteCriticalSection(&iplist);
  DeleteCriticalSection(&Threadslist);
  WSACleanup();
#else
  pthread_mutex_destroy(&iplist);
  pthread_mutex_destroy(&Threadslist);
#endif

  return 0;
}

void addBeginning(const char *ip) {
  EnterCriticalSection(&iplist);
  Node *temp = head;
  while (temp != NULL) {
    if (strcmp(temp->ip, ip) == 0) {
      temp->connectionC += 1;
      LeaveCriticalSection(&iplist);
      return;
    }
    temp = temp->next;
  }

  Node *newNode = (Node *)malloc(sizeof(Node));
  if (newNode != NULL) {
    newNode->connectionC = 1;
    strcpy(newNode->ip, ip);
    newNode->next = head;
    head = newNode;
  } else {
    printf("\n[Error]: Memory allocation failed.\n");
  }
  LeaveCriticalSection(&iplist);
}

void addBeginningThread(const HANDLE hThread) {
  EnterCriticalSection(&Threadslist);
  Threads *newThreads = (Threads *)malloc(sizeof(Threads));
  if (newThreads != NULL) {
    newThreads->hThread = hThread;
    newThreads->next = threads;
    threads = newThreads;
  } else {
    printf("\n[Error]: Memory allocation failed.\n");
  }
  LeaveCriticalSection(&Threadslist);
}

void deleteThread(const HANDLE hThread) {
  EnterCriticalSection(&Threadslist);
  Threads *temp = threads;
  if (temp == NULL) {
    LeaveCriticalSection(&Threadslist);
    return;
  }

  if (temp->hThread == hThread) {
    threads = temp->next;
#if defined(_WIN32) || defined(_WIN64)
    CloseHandle(temp->hThread);
#else
    pthread_detach(temp->hThread);
#endif
    free(temp);
    LeaveCriticalSection(&Threadslist);
    return;
  }

  while (temp->next != NULL) {
    if (temp->next->hThread == hThread) {
      Threads *temp2 = temp->next;
      temp->next = temp->next->next;
#if defined(_WIN32) || defined(_WIN64)
      CloseHandle(temp2->hThread);
#else
      pthread_detach(temp2->hThread);
#endif
      free(temp2);
      LeaveCriticalSection(&Threadslist);
      return;
    }
    temp = temp->next;
  }
  LeaveCriticalSection(&Threadslist);
}

void joinThread(void) {
  Threads *temp;
  EnterCriticalSection(&Threadslist);
  temp = threads;
  threads = NULL;
  LeaveCriticalSection(&Threadslist);

  while (temp != NULL) {
#if defined(_WIN32) || defined(_WIN64)
    WaitForSingleObject(temp->hThread, INFINITE);
    CloseHandle(temp->hThread);
#else
    pthread_join(temp->hThread, NULL);
#endif
    Threads *next = temp->next;
    free(temp);
    temp = next;
  }
}

void printIp() {
  EnterCriticalSection(&iplist);
  Node *temp = head;
  if (temp == NULL) {
    printf("\nNo IP addresses have contacted you yet.\n");
    LeaveCriticalSection(&iplist);
    return;
  }

  printf("\n--- IP addresses that contacted you ---\n");
  printf("Count\tIP Address\n");
  printf("---------------------------------------\n");
  while (temp != NULL) {
    printf("%d\t%s\n", temp->connectionC, temp->ip);
    temp = temp->next;
  }
  printf("---------------------------------------\n");
  LeaveCriticalSection(&iplist);
}

void freeList() {
  EnterCriticalSection(&iplist);
  Node *current = head;
  while (current != NULL) {
    Node *next = current->next;
    free(current);
    current = next;
  }
  head = NULL;
  LeaveCriticalSection(&iplist);
}

void clearInputBuffer(void) {
  int c;
  while ((c = getchar()) != '\n' && c != EOF) {
  }
}

void getInput(const char *demand, char *buffer, size_t size) {
  printf("%s", demand);
  if (fgets(buffer, size, stdin) == NULL) {
    buffer[0] = '\0';
    return;
  }

  if (strchr(buffer, '\n') == NULL) {
    clearInputBuffer();
  } else {
    buffer[strcspn(buffer, "\n")] = '\0';
  }
}

void fileExchangeH(char *historyFile, char *ip, char *file) {
  snprintf(historyFile, HISTORY_FILENAME_SIZE, "Hof_%s.txt", ip);
  FILE *messageHF = fopen(historyFile, "a");
  if (messageHF != NULL) {
    fprintf(messageHF, "\n[File sent to %s]: %s\n", ip, file);
    fclose(messageHF);
  } else {
    printf("\n[Error]: Failed to open message history file.\n");
  }
}

int send_all(const SOCKET sock, char *buffer, const int length) {
  int total_send = 0;
  while (total_send < length) {
    int result = send(sock, buffer + total_send, length - total_send, 0);
    if (result <= 0) {
      return result;
    }
    total_send += result;
  }
  return total_send;
}

int recv_all(const SOCKET sock, char *buffer, const int length) {
  int total_received = 0;
  while (total_received < length) {
    int result =
        recv(sock, buffer + total_received, length - total_received, 0);
    if (result <= 0) {
      return result;
    }
    total_received += result;
  }
  return total_received;
}

THREAD_RET ReceivingThread(void *arg) {
  ClientInformation *information = (ClientInformation *)arg;
  char buffer[BUFFER_SIZE];
  char client_ip[INET_ADDRSTRLEN];
  char hFilename[HISTORY_FILENAME_SIZE];
  char fileName[NAME_MAX + 1];
  size_t bytesReceived;
  SOCKET new_socket = information->new_socket;
  strcpy(client_ip, information->client_ip);
#if defined(_WIN32) || defined(_WIN64)
  HANDLE hThread = information->hThread;
#else
  HANDLE hThread = pthread_self();
  addBeginningThread(hThread);
#endif
  free(information);

  if (recv(new_socket, buffer, 1, 0) > 0) {
    if (buffer[0] == '1') {
      buffer[0] = '\0';
      snprintf(hFilename, HISTORY_FILENAME_SIZE, "Hof_%s.txt", client_ip);
      FILE *messageHF = fopen(hFilename, "a");
      do {
        int result = recv(new_socket, buffer, BUFFER_SIZE - 1, 0);
        if (result <= 0) {
          break;
        }
        buffer[result] = '\0';
        printf("\n[Message received from %s]: %s\n", client_ip, buffer);
        if (messageHF != NULL) {
          fprintf(messageHF, "\n[Message received from %s]: %s\n", client_ip,
                  buffer);
        }
      } while (strchr(buffer, '\n') == NULL);

      if (messageHF != NULL) {
        fclose(messageHF);
      } else {
        printf("\n[Error]: Failed to open message history file.\n");
      }
    } else if (buffer[0] == '2') {
      buffer[0] = '\0';
      size_t fileNameSize;
      if (recv_all(new_socket, buffer, MSIZE_OF_fileNameSize) > 0) {
        buffer[MSIZE_OF_fileNameSize] = '\0';
        fileNameSize = (size_t)atoi(buffer);
        if (fileNameSize > NAME_MAX) {
          fileNameSize = NAME_MAX;
        }

        if (recv_all(new_socket, buffer, fileNameSize) > 0) {
          buffer[fileNameSize] = '\0';
          strcpy(fileName, buffer);
#if defined(_WIN32) || defined(_WIN64)
          if (strrchr(fileName, '\\') != NULL) {
            strcpy(fileName, strrchr(fileName, '\\') + 1);
          }
          while (strrchr(fileName, '/') != NULL) {
            fileName[strcspn(fileName, "/")] = '_';
          }
#else
          if (strrchr(fileName, '/') != NULL) {
            strcpy(fileName, strrchr(fileName, '/') + 1);
          }
#endif
          FILE *filetoR = fopen(fileName, "wb");
          if (filetoR != NULL) {
            while ((bytesReceived = recv(new_socket, buffer, BUFFER_SIZE, 0)) >
                   0) {
              fwrite(buffer, 1, bytesReceived, filetoR);
            }
            fclose(filetoR);
            printf("\n[File received from %s]: %s\n", client_ip, fileName);
            fileExchangeH(hFilename, client_ip, fileName);
          } else {
            printf("\n[Error]: Failed to create file for incoming transfer.\n");
          }
        }
      }
    }
  }

  addBeginning(client_ip);
  closesocket(new_socket);

  if (atomic_load(&Exit)) {
    deleteThread(hThread);
  }
  return 0;
}

THREAD_RET ListenThread(void *arg) {
  struct sockaddr_in server_addr, client_addr;
  socklen_t client_len;
  char client_ip[INET_ADDRSTRLEN];

  server_socket = socket(AF_INET, SOCK_STREAM, 0);
  if (server_socket == INVALID_SOCKET) {
    return EXIT_THREAD;
  }

  server_addr.sin_family = AF_INET;
  server_addr.sin_addr.s_addr = INADDR_ANY;
  server_addr.sin_port = htons(PORT);

  if (bind(server_socket, (struct sockaddr *)&server_addr,
           sizeof(server_addr)) == SOCKET_ERROR) {
    closesocket(server_socket);
    server_socket = INVALID_SOCKET;
    printf("\n[Server Error]: Failed to bind socket on port %d.\n", PORT);
    return EXIT_THREAD;
  }

  listen(server_socket, SOMAXCONN);

  while (atomic_load(&Exit)) {
    client_len = sizeof(client_addr);
    SOCKET new_socket =
        accept(server_socket, (struct sockaddr *)&client_addr, &client_len);

    if (new_socket != INVALID_SOCKET) {
      inet_ntop(AF_INET, &client_addr.sin_addr, client_ip, sizeof(client_ip));

      ClientInformation *information =
          (ClientInformation *)malloc(sizeof(ClientInformation));
      if (information != NULL) {
        strcpy(information->client_ip, client_ip);
        information->new_socket = new_socket;

#if defined(_WIN32) || defined(_WIN64)
        information->hThread = (HANDLE)_beginthreadex(
            NULL, 0, ReceivingThread, information, CREATE_SUSPENDED, NULL);
        if (information->hThread != NULL) {
          addBeginningThread(information->hThread);
          ResumeThread(information->hThread);
        } else {
          free(information);
          closesocket(new_socket);
        }
#else
        if (pthread_create(&information->hThread, NULL, ReceivingThread,
                           information) != 0) {
          free(information);
          closesocket(new_socket);
        }
#endif
      } else {
        printf("\n[Error]: Memory allocation failed.\n");
        closesocket(new_socket);
      }
    }
  }

  if (server_socket != INVALID_SOCKET) {
    closesocket(server_socket);
    server_socket = INVALID_SOCKET;
  }
  return 0;
}