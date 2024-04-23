#ifndef STAT_H
#define STAT_H

/*
represents a directory
*/
#define T_DIR 1
/*
represents a regular file
*/
#define T_FILE 2
/*
represents a device file.
*/
#define T_DEV 3

/*
used to store information about a file. When a program queries the file system
for information about a particular file, this information is returned in a stat
structure.
*/
struct stat {
    /*
    represents the type of the file. It can have values T_DIR, T_FILE, or T_DEV.
    */
    short type;
    /*
    represents the file system's disk device associated with the file.
    */
    int dev;
    /*
    represents the inode number of the file.
    */
    uint ino;
    /*
    represents the number of links (hard links) to the file.
    */
    short nlink;
    /*
    represents the size of the file in bytes.
    */
    uint size;
};

#endif