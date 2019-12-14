#!/usr/bin/env python
# -*- coding: utf-8 -*-

# File Name: err_gen.py
# Author: Kevin
# Created Time: 2019-10-22
# Description:
import os
from jinja2 import Environment, FileSystemLoader

root_dir = ['work/projs/nwmng']
out_file = "work/projs/nwmng/utils/src_names.c"


def get_files(loadfile=root_dir):
    files = []
    while(loadfile):
        try:
            path = loadfile.pop()
            for x in os.listdir(path):
                if os.path.isfile(os.path.join(path, x)):
                    if(os.path.splitext(x)[1] == '.c'):
                        #  print os.path.splitext(x)[0]
                        files.append(os.path.splitext(x)[0])
                        pass
                elif os.path.isdir(os.path.join(path, x)):
                    if x == 'build' or x == 'apps' or x == '.git':
                        continue
                    #  print x
                    loadfile.append(os.path.join(path, x))
                else:
                    pass

        except Exception, e:
            print str(e) + path

    return files


if __name__ == '__main__':
    env_home = os.getenv('HOME') + '/'

    for i in range(len(root_dir)):
        root_dir[i] = env_home + root_dir[i]
        print root_dir[i]

    out_file = env_home + out_file

    f = get_files()
    if "main" not in f:
        f.append("main")
    #  print f

    ct_path = env_home + "work/projs/nwmng/tools"
    env = Environment(loader=FileSystemLoader(ct_path))
    template = env.get_template("file_names.ct")
    content = template.render(source_files=f)
    content = "".join([s for s in content.splitlines(True) if s.strip()])

    l = 0
    for s in f:
        #  print s + " --- " + str(len(s))
        l = max(l, len(s))
    print "Max Length of Files --- " + str(l)
    #  print content

    with open(out_file, "w") as fp:
        fp.seek(0)
        fp.truncate()
        fp.write(content)
