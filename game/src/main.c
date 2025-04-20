#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <stdint.h>
#include <arpa/inet.h>
#include <pthread.h>
#include <fcntl.h>

#include "var.h"
#include "struct.h"

// variables
struct map_unit map[MAP_SIZE_Y][MAP_SIZE_X];
int user_clientsock[USERMAX]; // for announcements
struct user *userlist[USERMAX];
unsigned long usercount = 0;
unsigned long vipcount = 0;

#include "func.h"
#include "misc.h"

// function
void init_map() {
    for (unsigned int y=0; y<MAP_SIZE_Y; y++) {
        for (unsigned int x=0; x<MAP_SIZE_X; x++) {
            map[y][x].item = NULL;
        }
    }
};
void init_user() {
    for (int i=0; i<USERMAX; i++) {
        userlist[i] = NULL;
    }
};
void init_fd() {
    setvbuf(stdin, 0, 2, 0);
    setvbuf(stdout, 0, 2, 0);
    setvbuf(stderr, 0, 2, 0);

    memset(user_clientsock, 0, sizeof(int)*USERMAX);
}

// handle socket client
void *handle_client(void *arg) {
    int client_sock = *(int *)arg;
    free(arg);
    printf("New connection established.\n");

    int err;
    char _buf[32] = {0};
    snprintf(_buf, 32, "Gift -> %p\n", win);
    send(client_sock, _buf, 32, 0);

    // ::handle login
    // register
    char username[16];
    send(client_sock, "Username > ", 12, 0);
    err = recv(client_sock, username, 16, 0);
    if (err <= 0) { // disconnected or err
        close(client_sock);
        return NULL;
    }
    // check max
    if (usercount >= USERMAX) {
        send(client_sock, "Sorry, too many players in server now.\n", 40, 0);
        close(client_sock);
        return NULL;
    }
    // check exist
    for (unsigned long i = 0; i < usercount; i++) {
        if (!strncmp(userlist[i]->name, username, 16)) {
            send(client_sock, "User existed.\n", 15, 0);
            close(client_sock);
            return NULL;
        }
    }
    // create user
    userlist[usercount] = malloc(sizeof(struct user));
    strncpy(userlist[usercount]->name, username, 16); // username
    unsigned long usernow = usercount;
    usercount++;
    userlist[usernow]->backpack = malloc(sizeof(struct backpack)); // backpack
    for (unsigned long i=0; i<BACKPACKSIZE; i++) {
        userlist[usernow]->backpack->item[i] = NULL;
    }
    userlist[usernow]->perm_vip = 0; // vip
    user_clientsock[usernow] = client_sock;
    // init item
    userlist[usernow]->backpack->item[0] = malloc(sizeof(struct item));
    userlist[usernow]->backpack->item[0]->id = 1;
    
    // ::main
    uint8_t choice;
    while (1) {
        menu(client_sock);
        choice = getchoice(client_sock);
        
        if (choice == 1) {
            err = put_block(client_sock, usernow);
            if (err) break;
        } else if (choice == 2) {
            err = get_block(client_sock, usernow);
            if (err) break;
        } else if (choice == 3) {
            err = destory_block(client_sock, usernow);
            if (err) break;
        } else if (choice == 4) {
            err = interact_block(client_sock, usernow);
            if (err) break;
        } else if (choice == 5) {
            profile(client_sock, usernow);
        } else if (choice == 6) {
            err = lottery(client_sock, usernow);
            if (err) break;
        } else if (choice == 255) {
            break;
        } else {
            send(client_sock, "Invalid choice!\n", 17, 0);
        }
    }

    // exit
    close(client_sock);
    return NULL;
};

int main() {
    // init
    srand(time(0));
    init_fd();
    init_map();
    init_user();

    // socket
    int server_sock, new_sock;
    struct sockaddr_in server_addr, client_addr;
    socklen_t addr_len = sizeof(client_addr);

    if ((server_sock = socket(AF_INET, SOCK_STREAM, 0)) == -1) {
        perror("Socket creation failed");
        exit(EXIT_FAILURE);
    }

    int opt = 1;
    setsockopt(server_sock, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt)); // force reuse port

    server_addr.sin_family = AF_INET;
    server_addr.sin_addr.s_addr = INADDR_ANY;
    server_addr.sin_port = htons(PORT);

    if (bind(server_sock, (struct sockaddr *)&server_addr, sizeof(server_addr)) == -1) {
        perror("Bind failed");
        close(server_sock);
        exit(EXIT_FAILURE);
    }

    if (listen(server_sock, 5) == -1) {
        perror("Listen failed");
        close(server_sock);
        exit(EXIT_FAILURE);
    }

    printf("Server listening on port %d...\n", PORT);

    while (1) {
        if ((new_sock = accept(server_sock, (struct sockaddr *)&client_addr, &addr_len)) == -1) {
            perror("Accept failed");
            continue;
        }

        pthread_t thread_id;
        int *client_sock = malloc(sizeof(int));
        if (client_sock == NULL) {
            perror("Malloc failed");
            close(new_sock);
            continue;
        }
        *client_sock = new_sock;

        if (pthread_create(&thread_id, NULL, handle_client, client_sock) != 0) {
            perror("Thread creation failed");
            free(client_sock);
            close(new_sock);
        }

        pthread_detach(thread_id);
    }

    close(server_sock);
    return 0;
};
