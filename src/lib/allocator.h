#ifndef ALLOCATOR_H
#define ALLOCATOR_H






// ---------------- DEFINITIONS ----------------

//root types: signed
typedef signed char      byt;
typedef signed long long lng;

//root types: unsigned
typedef unsigned char      ubyt;
typedef unsigned int       uint;
typedef unsigned long long ulng;

//root types: other
typedef void* ref;

//null ref
//#define null (0(ref))

//<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<< tmp
//static uint a_len = 0;






// ---------------- PAGE SCALE ----------------

//csts
#define MM__PAGE_SIZE 4096

//memory protection
#define MM__PROT_NONE  0x00000000
#define MM__PROT_READ  0x00000001
#define MM__PROT_WRITE 0x00000002
#define MM__PROT_EXEC  0x00000004

//mapping type
#define MM__MAP_SHARED          0x00000001
#define MM__MAP_PRIVATE         0x00000002
#define MM__MAP_SHARED_VALIDATE 0x00000003
#define MM__MAP_TYPE            0x0000000f
#define MM__MAP_FIXED           0x00000010
#define MM__MAP_ANONYMOUS       0x00000020
#define MM__MAP_NORESERVE       0x00004000
#define MM__MAP_GROWSDOWN       0x00000100
#define MM__MAP_DENYWRITE       0x00000800
#define MM__MAP_EXECUTABLE      0x00001000
#define MM__MAP_LOCKED          0x00002000
#define MM__MAP_POPULATE        0x00008000
#define MM__MAP_NONBLOCK        0x00010000
#define MM__MAP_STACK           0x00020000
#define MM__MAP_HUGETLB         0x00040000
#define MM__MAP_SYNC            0x00080000
#define MM__MAP_FIXED_NOREPLACE 0x00100000
#define MM__MAP_FILE            0x00000000

//err
#define MM__MAP_FAILED         0xffffffff //<=> -1

//page add - remove
void* newMap(ulng len);
void  freeMap(void* adr);






// ---------------- USER SCALE ----------------

//new - free
/*ref  new(unt_m len);
void free(ref p);*/

#endif
