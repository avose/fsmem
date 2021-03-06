# fsmem

Software library which allows applications to use space on disk instead of RAM for malloc()-based memory. While swap space in Linux is nice, it is very uncommon to create an install with a large (think terabytes) swap partition. Additionally, one might not want to dedicate that much disk space to swap all the time. The fsmem library allows one to enable a specific application to use files on disk for memory instead. This is done by memory mapping large files and then allowing ptmalloc to carve them up transparently. One should be able to download the source which will produce "libptmalloc3.a" when built. Linking with this file will cause the linked application to create a 1 terabyte file on disk which will be carved up into smaller blocks as needed by malloc().

http://www.aaronvose.net/fsmem/
