#!/usr/bin/env python
# -*- coding: utf-8 -*-

# File Name: err_str_gen.py
# Author: Kevin
# Created Time: 2020-02-19
# Description:

import os
import copy
from jinja2 import Environment, FileSystemLoader

in_file = "work/projs/nwmng/include/utils/err.h"
out_file = "work/projs/nwmng/utils/err_str.c"


def get_enums(err_h):
    names = []
    enums = []

    with open(err_h, "r") as fp_err_h:
        fp_err_h.seek(0)
        lines = fp_err_h.readlines()
        for line in lines:
            if line.find("ec_") == -1 or line.find(" = ") == -1:
                continue
            kv = line.strip().split("=")
            names.append(kv[0].strip())
            val = kv[1].strip(',').strip()
            if val.find("0x") == -1:
                enums.append(val)
            else:
                enums.append(str(int(val, 16)))
    return names, enums


if __name__ == '__main__':
    in_file = env_home = os.getenv('HOME') + '/' + in_file
    out_file = env_home = os.getenv('HOME') + '/' + out_file
    names, enums = get_enums(in_file)
    #  print names
    #  print enums
    #

    errs = []
    for i in range(0, 256):
        if str(i) in enums:
            idx = enums.index(str(i))
            errs.append({"name" : "\"" + names[idx] + "\"", "no" : enums[idx]})
        else:
            errs.append({"name" : "NULL", "no" : "Not Used Yet"})

    ct_path = os.getenv('HOME') + '/' + 'work/projs/nwmng/tools'
    env = Environment(loader=FileSystemLoader(ct_path))
    template = env.get_template("err_str.ct")
    content = template.render(errs = errs)
    content = "".join([s for s in content.splitlines(True) if s.strip()])
    with open(out_file, "w") as fp:
        fp.seek(0)
        fp.truncate()
        fp.write(content)
