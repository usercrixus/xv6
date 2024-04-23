#ifndef type_h
#define type_h

typedef unsigned int uint;
typedef unsigned short ushort;
typedef unsigned char uchar;
/*
Can be seen as metadata of a page directory (a bundle of page). Contain a 20bit
physical adress who point on a page directory and some information like
permission present or writable.

Bit 0: Present (PTE_P)
Bit 1: Writable (PTE_W)
Bit 2: User-level permission (PTE_U)
Bits 12-31: Physical address of the page table
*/
typedef struct {
    uint present : 1;
    uint writable : 1;
    uint permission : 1;
    uint padding : 9;
    uint physicalAdress : 20;
} pageDirecoryEntry;

/*
Can be seen as metadata of a page. Contain a 20bit physical adress who point on
a page and some information like permission present or writable.
*/
typedef pageDirecoryEntry pageTableEntry;

#endif