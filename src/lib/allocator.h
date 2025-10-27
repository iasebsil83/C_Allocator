#ifndef ALLOCATOR_H
#define ALLOCATOR_H






// ---------------- DEFINITIONS ----------------

//common types: signed
typedef signed char      byt;
typedef signed long long lng;

//common types: unsigned
typedef unsigned char      ubyt;
typedef unsigned int       uint;
typedef unsigned long long ulng;

//other
typedef byt boo;

//root types
typedef unsigned char       u8;
typedef unsigned short     u16;
typedef unsigned int       u32;
#ifdef ARCH64
typedef unsigned long long u64;
#endif

//default dat itms
#define null ((void*)0)
#define false 0
#define true  1

//some info
/*
    ================================================ INFORMATION ================================================
    |                                                                                                           |
    | I] INTRODUCTION                                                                                           |
    |                                                                                                           |
    | In this allocator, PC means "Page Collection".                                                            |
    |                                                                                                           |
    | A PC is nothing more than a mmap (using mmap syscall).                                                    |
    | It is a segment of heap allocated memory, with a total length that can only be a multiple of PAGE_SIZE.   |
    | Mind that this program uses MM__PAGE_SIZE as constant for his computation:                                |
    |     Please check that this one is correctly set or you will surely have undesired behavior !!!            |
    |                                                                                                           |
    | Also, in the following explanations, I will talk a bit about some technical concepts like CPU cache and   |
    | memory paging but only in surface, no deep dive will be given as it is not the subject of this project.   |
    |                                                                                                           |
    | Last thing before starting: mind that I am mainly considering "PC" instead of "page" or "cache" in these  |
    | paragraphs. It makes a real difference ! System and hardware may deal with different kind of units that   |
    | they will consider "local" to work with. This allocator works with Page Collections as their "local"      |
    | memory regions available.                                                                                 |
    |                                                                                                           |
    |                                                                                                           |
    |                                                                                                           |
    | II] GLOBAL ACCESS TO ALLOCATOR                                                                            |
    |                                                                                                           |
    | The algorithm of that allocator relies on the use of a static pointer "a_latestPC", which seems to be a   |
    | security issue as it can be predictable to target in the DATA section of our program. This sould be fixed |
    | later using some more low-level contraptions (make everyone use a global stack value for example).        |
    |                                                                                                           |
    | For the moment, we need this static field to give every scope access to the allocator. Indeed, as memory  |
    | allocation should be accessible from any scope, anywhere, we have to keep somewhere a reference to some   |
    | allocator resources, ALWAYS ACCESSIBLE.                                                                   |
    |                                                                                                           |
    | In this allocator, the "global accessible" resource that must be shared with every other scope is a       |
    | reference to the latest PC created. This is the starting point of each new/free() execution.              |
    |                                                                                                           |
    |                                                                                                           |
    |                                                                                                           |
    | III] FULL STACK ALLOCATION                                                                                |
    |                                                                                                           |
    | If your program does not call any new/free, no PC will be created at all, you will be in full-stack       |
    | allocation. As soon as you call new(), you will create a first PC, and so, use some heap allocation.      |
    |                                                                                                           |
    | This algorithm is "infinitely providing", meaning that you can allocate memory as much as you need,       |
    | without any fixed limit or anything. The only way to have this end is to have the system refusing to give |
    | more when asking for new PC (=mmap) via syscalls.                                                         |
    |                                                                                                           |
    | In those cases, an error message will be printed in stderr and the program will exit with a specific      |
    | error code (everything set as constants in the header file).                                              |
    |                                                                                                           |
    |                                                                                                           |
    |                                                                                                           |
    | IV] LOCALITY / WASTE COMPROMISE                                                                           |
    |                                                                                                           |
    | By accessing any PC, we can access every other one that has been created from the beginning of the        |
    | program because they are chained together. By the way, this chain is made only in one sens, each PC       |
    | holding a reference to the PREVIOUS one only.                                                             |
    |                                                                                                           |
    | Why ? LOCALITY ! To preserve CPU from loading memory pages in cache (which can be time consuming and      |
    | inefficient), we should try to use the most recent memory chunks as much as possible. This is LOCALITY.   |
    | If we have to look for something in another PC, at least we should look in the PREVIOUS one first because |
    | it is the most recent memory space we used so it may still be in cache (no load to operate).              |
    |                                                                                                           |
    | This is also the reason why I decided to always keep THE LATEST PC address as global access to allocator, |
    | to always give priority to the latest PC when looking for a new allocation. That way, we ensure maximum   |
    | LOCALITY, trying to work on the same PC (=> page) as our latest allocated blocks.                         |
    |                                                                                                           |
    | HOWEVER, having only this does not always offer the best place for our new allocation as the "best fit"   |
    | algorithm says. It even goes a bit in contradiction with it and can lead to huge memory waste because of  |
    | the impossibility of re-using freed memory efficiently. Let's see this in detail.                         |
    |                                                                                                           |
    |                                                                                                           |
    |                                                                                                           |
    | V] RECALL ON THE MAIN PRINCIPLE OF "BEST-FIT"                                                             |
    |                                                                                                           |
    | Recall on the "best fit" main principle:                                                                  |
    |                                                                                                           |
    |   The "best fit" main principle is related to the SIZE of the chunks we want to allocate.                 |
    |   During the lifetime of a program, we will have some free slots available somewhere in a mmap or another |
    |   (in our case, in a PC or another, exactly the same thing).                                              |
    |                                                                                                           |
    |   These slots offer contiguous memory that could fit for new allocations with the exact same size OR it   |
    |   can also fit for SMALLER chunks. Therefore, the more a chunk to be allocated is BIG, the harder it will |
    |   be to find somewhere to put it.                                                                         |
    |                                                                                                           |
    |   Also, if a small chunk is allocated in a large freed area instead of somewhere else, we will no longer  |
    |   be able to use that large area for large allocations. Instead, we will have to use more memory pages    |
    |   (=> mmap, or PC in our case). This is ineficient as we end with:                                        |
    |     - Waste of unused memory                                                                              |
    |     - Bigger system resource usage                                                                        |
    |     - Slower execution (new page allocation implies syscall => kernel interupt... => CPU time!)           |
    |                                                                                                           |
    |   The "best fit" algorithm solves these issues by going over EVERY free chunk available and then :        |
    |     > if current chunk has exact size => use it (can't have better, stop research here)                   |
    |     > if current chunk is at least the desired size:                                                      |
    |        > check whether it is SMALLER than the latest potential match:                                     |
    |          - Definitely smaller => set as new potential                                                     |
    |          - Not smaller        => skip it                                                                  |
    |                                                                                                           |
    |   At the end, the best potential slot remains, so we are using the smallest chunk that can contain our    |
    |   data, leaving the big chunks for bigger data. If we weren't able to find any potential, that means we   |
    |   have no place to hold that chunk, so we have to allocate more pages.                                    |
    |                                                                                                           |
    | In our case, using the best fit principle would implie to go through every PC to check whether we have a  |
    | better place to re-use some memory. The problem is... if we do so, we will go and read over different     |
    | pages, potentially tons of them, so we are being inefficient in terms of locality (maximize page miss).   |
    | Therefore, we have used an alternative strategy to try keeping both sides efficient as much as possible.  |
    |                                                                                                           |
    |                                                                                                           |
    |                                                                                                           |
    | V] "BEST-FIT-BET" STRATEGY                                                                                |
    |                                                                                                           |
    | Trying to get the best locality while also saying that you will have the "best fit" together is like      |
    | saying that you will take the best card from a game that is split in several rooms, but you can't go into |
    | these rooms to check this out. This is a bet. Seems completely random... isn't it ?                       |
    |                                                                                                           |
    | Well, even if we can't be sure at 100%, we can at least reduce the risk by making a "best-fit-bet".       |
    | Let's take our previous methophore with rooms to explain this strategy here:                              |
    |                                                                                                           |
    |   In each PC, you have potental free space available for your data.                                       |
    |   It is like having available storage drawers in different rooms that can potentially fit to the object   |
    |   you want to store, but you can't know which drawer will fit the best (avoid wasting place if we have a  |
    |   bigger object later).                                                                                   |
    |                                                                                                           |
    |   Also, you would like to avoid going into every room because it will cost a lot of time to do so.        |
    |   The only thing you know is that you have an object with a certain size, and one room, the lastest room  |
    |   used, and the drawers available in that room.                                                           |
    |                                                                                                           |
    |   To reduce to risk of wasting large space, the "best-fit-bet" strategy will consider 2 cases and make a  |
    |   choice depending on the size of the element to be stored:                                               |
    |                                                                                                           |
    |     > Either we will bet on LOCALITY over best-fit (choose local slot, whatever is in other PCs)          |
    |                 or                                                                                        |
    |     > Pay the price to look in the previous PC for a better place.                                        |
    |                                                                                                           |
    | To decide whether to bet on locality or not, we will consider the available slots in our current PC where |
    | our data can be stored UNTIL A FIXED RANGE in bytes. For a certain size of data to be stored, we make a   |
    | difference between the "tiny bit bigger" slots and the "VERY MUCH BIGGER" slots.                          |
    | This threshold is configurable via the constant MM__BEST_FIT_BET__THRESHOLD.                              |
    |                                                                                                           |
    |                                                                                                           |
    |                                                                                                           |
    | VI] "BEST-FIT-BET" CHOSING LOCALITY                                                                       |
    |                                                                                                           |
    | If the data we want to store founds an available free slot IN CURRENT PC that is greater than what we     |
    | need, we will do as best-fit does, keeping track of this slot as potential fit, BUT, we also check        |
    | whether it is in the allowed range:                                                                       |
    |                   availableChunkSize > size(dataToStore) + MM__BEST_FIT_BEST_THRESHOLD                    |
    |                                                                                                           |
    | We keep both the best potential match that is IN THAT RANGE, and the best potential match OUT OF RANGE.   |
    | Then, after having these 2 potentials, we are now able to make a choice:                                  |
    |                                                                                                           |
    | If there is a potential IN RANGE, that means, we have a free slot in our local PC that is a bit bigger    |
    | than what we need, but also not SO big, so we might not waste that much space by using it.                |
    |                                                                                                           |
    |    => Bet on LOCALITY, we potentially have better somewhere else, but it won't be SO MUCH waste.          |
    |                                                                                                           |
    | Otherwise, we will consider having too big free slots in that PC (or no one available actually), so we    |
    | won't take the risk of wasting too much memory yet. In that case, we choose the NEIGHBOR CHECK option.    |
    |                                                                                                           |
    |                                                                                                           |
    |                                                                                                           |
    | VII] "BEST-FIT-BET" CHOSING NEIGHBOR CHECK                                                                |
    |                                                                                                           |
    | In this other case, we are sure that our latest PC does not have an IN RANGE SLOT, so we hope that we can |
    | find one in previous PCs. So, as we did in the local check, we will go over free slots of the previous PC |
    | trying to find out a potential IN RANGE slot.                                                             |
    |                                                                                                           |
    | If we don't find any IN RANGE slot in that previous PC neither, we move on again to the previous one      |
    | until no more PC can be found. The first IN RANGE slot available will be considered efficient enough (as  |
    | we considered in local research, this is a bet). Even if we are not checking EVERY PC, we consider it is  |
    | enough efficient for us.                                                                                  |
    |                                                                                                           |
    | If at the end of all pages, we still don't find any IN RANGE slot that could fit, we still can re-use     |
    | memory by taking one of the OUT RANGE slots we have found during analysis. For that reason, when we look  |
    | for potentials in a PC, we not only keep track of the best IN RANGE only but also the best OUT RANGE.     |
    |                                                                                                           |
    | It doesn't matter whether this OUT RANGE is from the local PC or not, we keep only the best one, whatever |
    | PC it is located on. So if no IN RANGE found, use the best OUT RANGE. But if no OUT RANGE has been found  |
    | at all ? Well that means we don't even have one slot that fits with our data requirement so we have to    |
    | allocate more to hold that memory item.                                                                   |
    |                                                                                                           |
    |                                                                                                           |
    |                                                                                                           |
    | VIII] "BEST-FIT-BET" EXACT MATCH INTERRUPTION                                                             |
    |                                                                                                           |
    | It is important to recall that, as "best-fit" implies, if an EXACT match is found during slot research,   |
    | (both local or neighbor research), WE ARE USING THIS EXACT SLOT: this is DEFINITELY the best fit for our  |
    | data. Everything stops here and we allocate this slot for the given data, without asking further          |
    | questions.                                                                                                |
    |                                                                                                           |
    |                                                                                                           |
    |                                                                                                           |
    | IX] "BEST-FIT-BET" NEIGHBOR CHECK LIMIT (technical!)                                                      |
    |                                                                                                           |
    | We could have stopped our neigbourg check here, and it would be nice. However, there is a tiny small      |
    | feature that we have also added to this allocator to still preserve maximum locality, even in neighbor    |
    | check: NEIGHBOR LIMIT.                                                                                    |
    |                                                                                                           |
    | When looking for the best IN or OUT RANGE in neighbor PCs, you may find the best place far away from the  |
    | latest PC. But actually, having a page miss because the available slot is 2 or 3 pages away will involve  |
    | the same delay as if it was 10 or 30 pages away.                                                          |
    |                                                                                                           |
    | In all cases, having to change PC seems to have the same impact, no matter how far in previous PCs we     |
    | found a slot to re-use. Well, this is not exactly true. Depending on your hardware and the size of your   |
    | CPU cache, you are probably able to load several PCs at once and so the impact of going to one PC or      |
    | another will be the same. Same locality, no additionnal load to operate.                                  |
    |                                                                                                           |
    | That means, even if we don't go slower by going far away in PCs, we can still go faster by limiting our   |
    | neighbor check to only a certain number of PCs. This will reduce the performance of "best-fit" as we      |
    | shorthen the number of PCs to be accessed but it allows you to guarantee an optimal locality.             |
    |                                                                                                           |
    | This is the role of the constant MM__BEST_FIT_BET__NEIGHBOR_LIMIT. It corresponds to the maximum number   |
    | of PCs we allow to switch in neighbor check before considering we are going too far.                      |
    |                                                                                                           |
    | This kind of feature however, may be useful ONLY with programs that allocate a lot !                      |
    |                                                                                                           |
    | In these particular case, we may have a huge number of PCs, so locality over several PCs makes more sens  |
    | over "best-fit". Current execution may be not only centered on a few pages for everything, but rather on  |
    | the N latest PCs. Then, if you omit the N-1 previous PCs, it might be even better for you because no one  |
    | is using these pages for the moment (and they might no longer be in cache by the way).                    |
    |                                                                                                           |
    |            ============================ /!\ WARNING /!\ ================================                  |
    |          USING NEIGHBOR CHECK LIMIT UNEFFICIENTLY CAN CAUSE MORE DRAWBACKS THAN IMPROVEMENT !!!           |
    |          BY DEFAULT AND TO AVOID MISUSE, THIS FEATURE IS DISABLED. UNCOMMENT THE LINE BELOW               |
    |          TO ENABLE IT:                                                                                    |*/
//                              #define MM__BEST_FIT_BEST__NEIGHBOR_LIMIT_ENABLED
/*  |            =============================================================================                  |
    |                                                                                                           |
    |                                                                                                           |
    |                                                                                                           |
    | X] PC COMPOSITION                                                                                         |
    |                                                                                                           |
    | Each PC starts with 2 ulng of metadata:                                                                   |
    ||
    ||
    ||
    =============================================================================================================
*/

//latest PC
extern ulng* heap__latestPC;

//csts
#define MM__PAGE_SIZE 4096

//memory protection
#define MM__PROT_NONE  0x00000000
#define MM__PROT_READ  0x00000001
#define MM__PROT_WRITE 0x00000002
#define MM__PROT_EXEC  0x00000004

//maps
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
#define MM__MAP_FAILED ((void*)0xffffffff) //<=> -1

//std streams
#define STDIN  0
#define STDOUT 1
#define STDERR 2

//err
#define ERR__SUCCESS               0
#define ERR__FAILURE               1
#define ERR__SHELL_BUILTIN         2
#define ERR__EXECUTION_DENIED    126
#define ERR__CMD_NOT_FOUND       127
#define ERR__INVALID_EXIT_CODE   128
#define ERR__SIGINT              130
#define ERR__SIGKILL             137
#define ERR__SEG_FAULT           139
#define ERR__SIGTERM             143
#define ERR__SYSTEM_REFUSE_ALLOC 144 //allocator related
#define ERR__SYSTEM_REFUSE_FREE  145 //allocator related
#define ERR__ILLEGAL_HEAP_RU8    146 //allocator related
#define ERR__ILLEGAL_HEAP_RU16   147 //allocator related
#define ERR__ILLEGAL_HEAP_RU32   148 //allocator related
#define ERR__ILLEGAL_HEAP_RU64   149 //allocator related
#define ERR__ILLEGAL_HEAP_WU8    150 //allocator related
#define ERR__ILLEGAL_HEAP_WU16   151 //allocator related
#define ERR__ILLEGAL_HEAP_WU32   152 //allocator related
#define ERR__ILLEGAL_HEAP_WU64   153 //allocator related
#define ERR__EXIT_OUT_OF_RANGE   255

//pad every allocation request into a multiple of sizeof(ulng)
#define MM__PAD_ALLOC_LEN_ULNG

//best-fit-bet !WARNING: CHECK EXPLANATIONS ABOVE BEFORE MODIFYING
#define MM__BEST_FIT_BET__THRESHOLD      8
#define MM__BEST_FIT_BET__NEIGHBOR_LIMIT 18446744073709551616

//memory access
#define MM__ILLEGAL_ACCESS_OUTPUT //comment to disable output msg on illegal access






// ---------------- GENERIC TOOLS ----------------

//padding tool
ulng padP2(ulng n, ulng p2);






// ---------------- USER SCALE ----------------

//new - free
void* heap__new(ulng len);
void  heap__free(void* r);






// ---------------- MEMORY ACCESS ----------------

//fixed size, unsafe: read
u8  heap__unsafe_ru8( void* r, ulng offset);
u16 heap__unsafe_ru16(void* r, ulng offset);
u32 heap__unsafe_ru32(void* r, ulng offset);
#ifdef ARCH64
u64 heap__unsafe_ru64(void* r, ulng offset);
#endif

//fixed size, unsafe: write
void heap__unsafe_wu8( void* r, ulng offset, u8  e);
void heap__unsafe_wu16(void* r, ulng offset, u16 e);
void heap__unsafe_wu32(void* r, ulng offset, u32 e);
#ifdef ARCH64
void heap__unsafe_wu64(void* r, ulng offset, u64 e);
#endif

//fixed size, safe: read
u8  heap__safe_ru8( void* r, ulng offset);
u16 heap__safe_ru16(void* r, ulng offset);
u32 heap__safe_ru32(void* r, ulng offset);
#ifdef ARCH64
u64 heap__safe_ru64(void* r, ulng offset);
#endif

//fixed size, safe: write
void heap__safe_wu8( void* r, ulng offset, u8  e);
void heap__safe_wu16(void* r, ulng offset, u16 e);
void heap__safe_wu32(void* r, ulng offset, u32 e);
#ifdef ARCH64
void heap__safe_wu64(void* r, ulng offset, u64 e);
#endif

//makes no sens to read on variable size:
//  How can we know where to put the result if we don't know the size statically (at compile time) ?

//variable size, unsafe: write
void heap__unsafe_w(void* src, ulng srcOffset, void* dst, ulng dstOffset, ulng size);

//variable size, safe: write
void heap__safe_w(void* src, ulng srcOffset, void* dst, ulng dstOffset, ulng size);

#endif
