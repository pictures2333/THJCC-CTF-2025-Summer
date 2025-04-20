// item
struct item {
    // common attr
    unsigned char name[8];
    unsigned long id;
    // chest attr
    struct inchest *itemlist;
    unsigned long owner;
};

// chest item
struct inchest {
    struct inchest *next;
    struct item *item;
};

// map
struct map_unit {
    struct item *item;
};

// user
struct user {
    unsigned char name[16];
    unsigned long perm_vip;
    struct backpack *backpack;
};

struct backpack {
    struct item *item[BACKPACKSIZE];
};
