import os
os.system("gcc -Wall pksfuse.c rbtree.c `pkg-config fuse --cflags --libs` -o main")
os.system("./main -f 0A")
