// ===== MISC =====
uint8_t getchoice(int client_sock) {
    send(client_sock, "> ", 2, 0);

    unsigned char _buf[8] = {0};
    int err = recv(client_sock, _buf, 8, 0);
    if (err <= 0) {
        return 255;
    }
    uint8_t choice;
    choice = atoi(_buf);
    return choice;
};

int map_selector(int client_sock, unsigned int *x, unsigned int *y) {
    unsigned char _buf[8] = {0};
    int err;

    send(client_sock, "Map Position (x) > ", 20, 0);
    err = recv(client_sock, _buf, 8, 0);
    if (err <= 0) {
        return -1;
    }
    *x = atoi(_buf);

    send(client_sock, "Map Position (y) > ", 20, 0);
    err = recv(client_sock, _buf, 8, 0);
    if (err <= 0) {
        return -1;
    }
    *y = atoi(_buf);

    return 0;
};

int block_selector(int client_sock, int *block) {
    send(client_sock, "Select a block\n", 16, 0);
    send(client_sock, "[1] Normal Block\n", 18, 0);
    send(client_sock, "[2] Chest\n", 11, 0);
    send(client_sock, "> ", 2, 0);

    unsigned char _buf[8] = {0};
    int err = recv(client_sock, _buf, 8, 0);
    if (err <= 0) return -1;
    *block = atoi(_buf);

    return 0;
};

int backpack_selector(int client_sock, int *bpslot) {
    send(client_sock, "Select a backpack slot\n", 24, 0);
    send(client_sock, "> ", 2, 0);

    unsigned char _buf[8] = {0};
    int err = recv(client_sock, _buf, 8, 0);
    if (err <= 0) return -1;
    *bpslot = atoi(_buf);

    return 0;
};

// ===== INTERACT =====
int item_rename(int client_sock, struct item *item) {
    send(client_sock, "Name > ", 8, 0);

    unsigned char namebuf[8] = {0};
    int err = recv(client_sock, namebuf, 8, 0);
    if (err <= 0) return -1;
    strncpy(item->name, namebuf, 8);

    send(client_sock, "Done!\n", 7, 0);
    return 0;
};

int chest_push(int client_sock, struct item *item) {
    // get count
    send(client_sock, "Push [Normal block]\n", 21, 0);
    send(client_sock, "Count > ", 9, 0);
    unsigned char countbuf[10];
    int err = recv(client_sock, countbuf, 10, 0);
    if (err <= 0) return -1;
    uint32_t count = atoi(countbuf);

    // make chain
    struct inchest *now = item->itemlist, *last = NULL;
    for (uint32_t i=0; i<count; i++) {
        last = now;

        now = malloc(sizeof(struct inchest));

        now->item = malloc(sizeof(struct item));
        now->item->id = 1;
        memset(now->item->name, 0, 8);

        now->next = last;
    }
    item->itemlist = now;

    return 0;
};

int chest_pop(int client_sock, unsigned int usernow, struct item *item) {
    int err = 0;
    // backpack slot
    int bpslot;
    err = backpack_selector(client_sock, &bpslot);
    if (err) return -1;
    if (bpslot < 0 || bpslot >= BACKPACKSIZE) {
        send(client_sock, "Invalid slot!\n", 15, 0);
        return 0;
    }

    // pop
    if (item->itemlist) {
        struct inchest *tmp;
        tmp = item->itemlist;
        item->itemlist = item->itemlist->next;

        userlist[usernow]->backpack->item[bpslot] = tmp->item;
        tmp->next = NULL;
        tmp->item = NULL;
        free(tmp);
    } else {
        send(client_sock, "Nothing in chest.\n", 19, 0);
    }

    return 0;
};

// ===== MAIN FUNCTIONS =====
void menu(int client_sock) {
    // map
    send(client_sock, "| Map\n", 7, 0);
    unsigned char _buf[8] = {0};
    for (unsigned int y=0; y<MAP_SIZE_Y; y++) {
        for (unsigned int x=0; x<MAP_SIZE_X; x++) {
            if (map[y][x].item) {
                if (map[y][x].item->id == 1) { // normal block
                    send(client_sock, "# ", 2, 0);
                } else { // chest
                    send(client_sock, "C ", 2, 0);
                }
            } else {
                send(client_sock, ". ", 2, 0);
            }
        }
        send(client_sock, "\n", 1, 0);
    }

    // menu
    send(client_sock, "| Menu\n", 8, 0);
    send(client_sock, "[1] Put block\n", 15, 0);
    send(client_sock, "[2] Get block\n", 15, 0);
    send(client_sock, "[3] Destory block\n", 19, 0);
    send(client_sock, "[4] Interact with block\n", 25, 0);
    send(client_sock, "[5] Profile\n", 13, 0);
    send(client_sock, "[6] Lottery (10% chance - VIP)\n", 32, 0);
    send(client_sock, "[255] Exit\n", 12, 0);

    return;
};


int put_block(int client_sock, unsigned long usernow) {
    int err;
    // map selector
    unsigned int x, y;
    err = map_selector(client_sock, &x, &y);
    if (err) return -1;
    if (x < 0 || x >= MAP_SIZE_X || y < 0 || y >= MAP_SIZE_Y) {
        send(client_sock, "Invalid position!\n", 19, 0);
        return 0;
    }

    if (userlist[usernow]->perm_vip == 1) { // vip
        // block selector
        int block = 0;
        err = block_selector(client_sock, &block);
        if (err) return -1;
        if (block != 1 && block != 2) {
            send(client_sock, "Invalid block!\n", 16, 0);
            return 0;
        }

        // put
        if (map[y][x].item) { // map unit / no item on it
            send(client_sock, "The map unit which you selected is not clear.\n", 47, 0);
            return 0;
        }
        map[y][x].item = malloc(sizeof(struct item));
        map[y][x].item->id = block;
        memset(map[y][x].item->name, 0, 8);
        if (block == 2) {
            map[y][x].item->owner = usernow; // chest owner protect
            map[y][x].item->itemlist = NULL;
        }
    } else { // not vip
        // backpack selector
        int bpslot = 0;
        err = backpack_selector(client_sock, &bpslot);
        if (err) return -1;
        if (bpslot < 0 || bpslot >= BACKPACKSIZE) {
            send(client_sock, "Invalid slot!\n", 15, 0);
            return 0;
        }

        // put (backpack -> map)
        if (!userlist[usernow]->backpack->item[bpslot]) { // backpack slot / item in it
            send(client_sock, "Nothing in the slot which you selected.\n", 41, 0);
            return 0;
        }
        if (map[y][x].item) { // map unit / no item on it
            send(client_sock, "The map unit which you selected is not clear.\n", 47, 0);
            return 0;
        }
        map[y][x].item = userlist[usernow]->backpack->item[bpslot];
        userlist[usernow]->backpack->item[bpslot] = NULL;
        if (map[y][x].item->id == 2) { // chest owner protect
            map[y][x].item->owner = usernow;
        }
    }

    return 0;
};


int get_block(int client_sock, unsigned long usernow) {
    int err;
    // map selector
    unsigned int x, y;
    err = map_selector(client_sock, &x, &y);
    if (err) return -1;
    if (x < 0 || x >= MAP_SIZE_X || y < 0 || y >= MAP_SIZE_Y) {
        send(client_sock, "Invalid position!\n", 19, 0);
        return 0;
    }

    // backpack selector
    int bpslot = 0;
    err = backpack_selector(client_sock, &bpslot);
    if (err) return -1;
    if (bpslot < 0 || bpslot >= BACKPACKSIZE) {
        send(client_sock, "Invalid slot!\n", 15, 0);
        return 0;
    }

    // get (map -> backpack)
    if (!map[y][x].item) { // map unit / item on it
        send(client_sock, "Nothing in the map unit which you selected.\n", 45, 0);
        return 0;
    }
    if (userlist[usernow]->backpack->item[bpslot]) { // backpack slot / no item in it
        send(client_sock, "The slot which you selected is not clear.\n", 43, 0);
        return 0;
    }
    userlist[usernow]->backpack->item[bpslot] = map[y][x].item;
    map[y][x].item = NULL;

    return 0;
};


int destory_block(int client_sock, unsigned long usernow) {
    int err;
    // map selector
    unsigned int x, y;
    err = map_selector(client_sock, &x, &y);
    if (err) return -1;
    if (x < 0 || x >= MAP_SIZE_X || y < 0 || y >= MAP_SIZE_Y) {
        send(client_sock, "Invalid position!\n", 19, 0);
        return 0;
    }
    struct item *tmpit = map[y][x].item;
    if (!tmpit) { // map unit / item on it
        send(client_sock, "Nothing in the map unit which you selected.\n", 45, 0);
        return 0;
    }

    // destory!!
    if (tmpit->id == 1) { // normal block
        free(tmpit);
        map[y][x].item = NULL;
    } else { // chest
        if (tmpit->owner != usernow) { // chest owner protect
            send(client_sock, "This box is not yours!\n", 24, 0);
            return 0;
        }
        // remove items
        struct inchest *now = tmpit->itemlist, *next = NULL;
        free(tmpit);
        while (1) {
            if (!now) break;

            next = now->next;

            free(now->item);
            free(now);
            
            now = next;
        }
        // remove chest
        map[y][x].item = NULL;
    }
    
    return 0;
}


int interact_block(int client_sock, unsigned long usernow) {
    int err;
    // map selector
    unsigned int x, y;
    err = map_selector(client_sock, &x, &y);
    if (err) return -1;
    if (x < 0 || x >= MAP_SIZE_X || y < 0 || y >= MAP_SIZE_Y) {
        send(client_sock, "Invalid position!\n", 19, 0);
        return 0;
    }
    if (!map[y][x].item) { // map unit / item on it
        send(client_sock, "Nothing in the map unit which you selected.\n", 45, 0);
        return 0;
    }

    // interact
    if (map[y][x].item->id == 1) { // normal block
        unsigned char _buf[64] = {0};
        snprintf(_buf, 64, "Block [Normal block] : %s\n", map[y][x].item->name);
        send(client_sock, _buf, 64, 0);
        send(client_sock, "[1] Rename\n", 12, 0);

        int choice = getchoice(client_sock);
        if (choice == 255) return -1;
        if (choice == 1) {
            err = item_rename(client_sock, map[y][x].item);
            if (err) return -1;
        } else {
            send(client_sock, "Invalid choice!\n", 17, 0);
        }
    } else { // chest
        unsigned char _buf[64] = {0};
        snprintf(_buf, 64, "Block [Chest] : %s\n", map[y][x].item->name);
        send(client_sock, _buf, 64, 0);
        send(client_sock, "[1] Rename\n", 12, 0);
        send(client_sock, "[2] Push item\n", 15, 0);
        send(client_sock, "[3] Pop item\n", 14, 0);

        int choice = getchoice(client_sock);
        if (choice == 255) return -1;
        if (choice == 1) {
            err = item_rename(client_sock, map[y][x].item);
            if (err) return -1;
        } else if (choice == 2) {
            err = chest_push(client_sock, map[y][x].item);
            if (err) return -1;
        } else if (choice == 3) {
            err = chest_pop(client_sock, usernow, map[y][x].item);
            if (err) return -1;
        } else {
            send(client_sock, "Invalid choice!\n", 17, 0);
        }
    }

    return 0;
}


void profile(int client_sock, unsigned long usernow) {
    unsigned char _buf[64] = {0};

    snprintf(_buf, 64, "Hello player %s", userlist[usernow]->name);
    send(client_sock, _buf, 64, 0);

    if (userlist[usernow]->perm_vip == 1) {
        send(client_sock, "You are a VIP\n", 15, 0);
    } else {
        send(client_sock, "You are not a VIP\n", 19, 0);
    }

    send(client_sock, "| Backpack\n", 12, 0);

    for (unsigned long i=0; i<BACKPACKSIZE; i++) {
        memset(_buf, 0, 64);
        if (userlist[usernow]->backpack->item[i]) {
            unsigned long itemid = userlist[usernow]->backpack->item[i]->id;
            if (itemid == 1) {
                snprintf(_buf, 64, "[%lu] A normal block\n", i);
            } else {
                snprintf(_buf, 64, "[%lu] Chest\n", i);
            }
        } else {
            snprintf(_buf, 64, "[%lu] \n", i);
        }
        send(client_sock, _buf, 64, 0);
    }
    send(client_sock, "\n", 1, 0);

    return;
}


int lottery(int client_sock, unsigned long usernow) {
    if (vipcount >= VIPMAX) {
        send(client_sock, "Sorry, too many players have vip now.\n", 39, 0);
        return 0;
    };

    unsigned long result = rand();
    unsigned char rnbuf[20] = {0};
    unsigned char gubuf[20] = {0};

    send(client_sock, "Guess a number > ", 18, 0);
    int err = recv(client_sock, gubuf, 19, 0);
    if (err <= 0) return -1;
    
    snprintf(rnbuf, 20, "%lu", result);
    gubuf[19] = 0;

    if (!memcmp(rnbuf, gubuf, strlen(gubuf))) {
        userlist[usernow]->perm_vip = 1;
        vipcount++;
        send(client_sock, "You are a VIP now!\n", 20, 0);

        // vip gift
        char _buf[32] = {0};
        snprintf(_buf, 32, "Gift -> %p\n", printf);
        send(client_sock, _buf, 32, 0);
    } else {
        send(client_sock, "Try again...\n", 14, 0);
    }

    return 0;
}