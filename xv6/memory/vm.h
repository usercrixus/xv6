#include "../type/types.h"

void seginit(void);
void kvmalloc(void);
pageDirecoryEntry* setupkvm(void);
char* uva2ka(pageDirecoryEntry*, char*);
int allocuvm(pageDirecoryEntry*, uint, uint);
int deallocuvm(pageDirecoryEntry*, uint, uint);
void freevm(pageDirecoryEntry*);
void inituvm(pageDirecoryEntry*, char*, uint);
int loaduvm(pageDirecoryEntry*, char*, struct inode*, uint, uint);
pageDirecoryEntry* copyuvm(pageDirecoryEntry*, uint);
void switchuvm(struct proc*);
void switchkvm(void);
int copyout(pageDirecoryEntry*, uint, void*, uint);
void clearpteu(pageDirecoryEntry* pgdir, char* uva);