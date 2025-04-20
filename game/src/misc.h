void win() {
    int fd = open("/flag.txt", O_RDONLY);
    if (fd == -1) return;

    char buf[128];
    memset(buf, 0, 128);
    read(fd, buf, 128);

    close(fd);

    for (unsigned long i=0; i<usercount; i++) {
        send(user_clientsock[i], buf, 128, 0);
    }

    return;
}