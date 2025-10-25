// ---------------- IMPORTATIONS ----------------

//own header
#include "allocator.h"






/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ ALLOCATOR [0.1.0] ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
                                 Allocator by I.A.

        This program is a bare metal library that allows you to allocate and free
    memory dynamically on linux systems, the same way as malloc/free does in
    standard C libraries.

        Of course, as it is the main goal of this library, this implementation is
    completely independant from any C library related resources and stands only
    by using linux syscalls.

    This program adds 2 functions:
     - new()
     - free()

    Contact: https://github.com/iasebsil83

    Let's Code !                                  By I.A..
******************************************************************************************

    LICENCE :

    C_Allocator
    Copyright (C) 2025 Sebastien SILVANO

    This library is free software; you can redistribute it and/or
    modify it under the terms of the GNU Lesser General Public
    License as published by the Free Software Foundation; either
    version 2.1 of the License, or (at your option) any later version.

    This library is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
    Lesser General Public License for more details.

    You should have received a copy of the GNU Lesser General Public
    License along with this library.

    If not, see <https://www.gnu.org/licenses/>.
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */






// ---------------- GENERIC TOOLS ----------------

//padding tool, pads the given value to the next multiple of the power of 2:
//    37,4096 => 4096
//  2000,4096 => 4096
//  4096,4096 => 4096
//  4097,4096 => 8192
//  8191,8192 => 8192
//  ...
ulng padP2(ulng n, ulng p2) { return (n + p2 - 1) & ~(p2 - 1); }






// ---------------- PC SCALE ----------------

//syscalls
extern void* syscall_mmap(
	void* adr,
	ulng  len,
	ulng  prot,
	ulng  flags,
	ulng  fd,
	ulng  ofs
);
extern int  syscall_munmap(void* adr, ulng len);
extern void syscall_write(uint fd, char* c, uint len);
extern void syscall_exit(ulng err);

//latest PC
ulng* a_latestPC = null;



//new - free PC
static void* newPC(ulng len) {



	//STEP 1: ALLOCATE

	//need 5 additionnal ulng than requested: 3 for PC metadata (prev PC + freeLstRef + PC size), 2 for allocated block metadata (len + freeLst ref)
	ulng PCLen = len + 5*sizeof(ulng);

	//pad len asked, and also deduce remaining space left (can be 0 btw)
	ulng paddedPCLen   = padP2(PCLen, MM__PAGE_SIZE);
	ulng remainingInPC = paddedPCLen - PCLen;

	//allocate new page(s)
	ulng* pc = syscall_mmap(
		null,        //adr
		paddedPCLen, //len
		MM__PROT_READ   | MM__PROT_WRITE,    //prot
		MM__MAP_PRIVATE | MM__MAP_ANONYMOUS, //flags
		-1, //fd
		0   //offset
	);

	//system refuses => stop program
	if(pc == MM__MAP_FAILED){
		syscall_write(0, "[ ERROR ] > Unable to allocate more memory for process.", 59);
		syscall_exit(43);
	}



	//STEP 2: CHAIN

	//save PC size also
	pc[2] = paddedPCLen;

	//chain with other PCs: 1st ulng of PC contains prev PC adr
	pc[0]      = (ulng)a_latestPC;
	a_latestPC = pc;               //update latestPC, now we are working on this new one in priority now



	//STEP 3: SET INITIAL FREE LST (inside PC)

	//too few space left => let's give the remaining to our data
	ulng* firstFreeSlot = null;
	if(remainingInPC < 2*sizeof(ulng)){ len += remainingInPC; } //<-------- !WARNING IF USING "MM__PAD_ALLOC_LEN_ULNG": Will make "len" unpadded with sizeof(ulng) if MM__PAGE_SIZE is not a multiple of size(ulng).

	//enough space => create 1st slot with it
	else{
		firstFreeSlot    = pc + 4 + len;                   //2 metadata for PC + 2 metadata for allocated block + len of allocated block
		firstFreeSlot[0] = remainingInPC - 2*sizeof(ulng); //slot size (available space - 2 metadata ulng for the potential new block to be set here)
		firstFreeSlot[1] = (ulng)null;                     //next slot adr (none here)
	}

	//set initial freeLst
	pc[1] = (ulng)firstFreeSlot;

	//set allocated region
	pc[3] =          len; //size            as 1st ulng in allocated chunk (4th ulng in PC)
	pc[4] = (ulng)(pc+1); //freeLstHead ref as 2nd ulng in allocated chunk (5th ulng in PC)
	return pc+5;          //adr             as 3rd ulng in allocated chunk (6th ulng in PC)
}

static void freePC(ulng* adr, ulng len) {
	if(syscall_munmap(adr, len) == -1){
		syscall_write(0, "[ ERROR ] > Unable to de-allocate memory from process.", 54);
		syscall_exit(54);
	}
}






// ---------------- USER SCALE ----------------

//new - free
void* new(ulng len){

	//0 bytes to allocate => silent err
	if(!len){ return null; }

	#ifdef MM__PAD_ALLOC_LEN_ULNG
	//padding to only multiple of ulng
	len = padP2(len, sizeof(ulng));
	#else
	//at least sizeof(ulng) bytes required to allocate
	if(len < sizeof(ulng)) { len = sizeof(ulng); }
	#endif

	//potential matches: IN RANGE
	ulng* bestPtl_inRange_prevRef = null;
	ulng* bestPtl_inRange         = null;
	ulng  bestPtl_inRange_size    = 0;

	//potential matches: OUT RANGE
	ulng* bestPtl_outRange_prevRef     = null;
	ulng* bestPtl_outRange             = null;
	ulng  bestPtl_outRange_size        = 0;
	ulng* bestPtl_outRange_freeLstHead = null;

	//all researches at once
	ulng* curPC = a_latestPC; //start by a LOCAL search
	while(curPC != null){
		ulng* curPC_freeLstHead   = curPC+1;
		ulng* curFreeSlot_prevRef = curPC_freeLstHead;
		while(true){

			//as long as we have a free slot
			if(!curFreeSlot_prevRef){ break; }

			//cur free slot
			ulng* curFreeSlot = (ulng*)(curFreeSlot_prevRef[0]);
			ulng  size        = curFreeSlot[0]; //size of free block as 1st ulng inside slot

			//exact match => use it
			if(size == len){
				curFreeSlot_prevRef[0] = curFreeSlot[1];          //disconnect slot from freeLst, <=> prevSlot->nxt = curSlot->nxt
				curFreeSlot[1]         = (ulng)curPC_freeLstHead; //set freeLstHead in newly allocated block
				return curFreeSlot+2;                             //direct return adr, no need to set the size (exact match)
			}

			//bigger => potential
			if(size > len){

				//in-range => update best potential
				if(size < len + MM__BEST_FIT_BET__THRESHOLD){
					if(size < bestPtl_inRange_size){
						bestPtl_inRange_size    = size;
						bestPtl_inRange_prevRef = curFreeSlot_prevRef;
						bestPtl_inRange         = curFreeSlot;
					}
				}

				//out-range => update best potential
				else {
					if(size < bestPtl_outRange_size){
						bestPtl_outRange_size        = size;
						bestPtl_outRange_prevRef     = curFreeSlot_prevRef;
						bestPtl_outRange             = curFreeSlot;
						bestPtl_outRange_freeLstHead = curPC_freeLstHead;
					}
				}
			}

			//next free slot adr as 2nd ulng inside slot
			curFreeSlot_prevRef = (ulng*)(curFreeSlot[1]);
		}

		//in-range slot found in that page => use it
		if(bestPtl_inRange){
			bestPtl_inRange_prevRef[0] = bestPtl_inRange[1];      //disconnect slot from freeLst, <=> prevSlot->nxt = bestPtl_inRangeSlot->nxt
			bestPtl_inRange[0]         = bestPtl_inRange_size;    //set size        in newly allocated block
			bestPtl_inRange[1]         = (ulng)curPC_freeLstHead; //set freeLstHead in newly allocated block
			return bestPtl_inRange+2;
		}

		//go to previous PC (neighbor check)
		curPC = (ulng*)curPC[0];
	}

	//still no exact match or in-range potential found => use out-range potential
	if(bestPtl_outRange){
		bestPtl_outRange_prevRef[0] = bestPtl_outRange[1];                //disconnect from freeLst, <=> prevSlot->nxt = bestPtl_outRangeSlot->nxt
		bestPtl_outRange[0]         = bestPtl_outRange_size;              //set size        in newly allocated block
		bestPtl_outRange[1]         = (ulng)bestPtl_outRange_freeLstHead; //set freeLstHead in newly allocated block
		return bestPtl_outRange+2;
	}

	//no even having any out-range potential => allocate new PC
	return newPC(len);
}

void free(void* r){
	ulng* freedSlot  = ((ulng*)r)-2;
	ulng* tgtFreeLst = (ulng*)(freedSlot[1]);



	//CASE 1: MERGING WITH PREV/NEXT SLOTS

	//adr right after our block
	ulng* followingAdr = freedSlot + freedSlot[0] + 2*sizeof(ulng);

	//go over the whole freeLst for that PC
	ulng  totalFreeLstSize    = 0;     //total size of every free slots (including metadata)
	boo   mergedPrev          = false;
	boo   mergedNext          = false;
	ulng* prevNeedingUpdate   = null;
	ulng* curFreeSlot_prevRef = tgtFreeLst;
	while(true){

		//as long as we have anything in free lst
		if(!curFreeSlot_prevRef){ break; }

		//cur free slot
		ulng* curFreeSlot = (ulng*)(curFreeSlot_prevRef[0]);
		totalFreeLstSize += curFreeSlot[0] = 2*sizeof(ulng);

		//found free slot RIGHT BEFORE ours => merge them
		if(curFreeSlot + curFreeSlot[0] + 2*sizeof(ulng) == freedSlot){
			curFreeSlot[0] += freedSlot[0] + 2*sizeof(ulng); //add our complete freedSlot as raw available data in curFreeSlot

			//VERY IMPORTANT !!! If having both merge prev & after, freedSlot must be updated so that we will merge the prevMergedSlot with the next one.
			freedSlot = curFreeSlot;
			if(prevNeedingUpdate){ prevNeedingUpdate[0] = curFreeSlot; } //also, the prevRef that has been updated must be updated once more to the even-before chunk (=current one) !

			//optimization
			if(mergedNext){ return; }
			mergedPrev = true;
		}

		//found free slot RIGHT AFTER ours => merge them also
		else if(followingAdr == curFreeSlot){
			freedSlot[0]          += curFreeSlot[0] + 2*sizeof(ulng); //add complete curFreeSlot as raw available data in our freedSlot
			freedSlot[1]           = curFreeSlot[1];                  //also take the "next" field from curFreeSlot
			curFreeSlot_prevRef[0] = (ulng)freedSlot;                 //<=> prevSlot->nxt = curSlot

			//keep aside the prevRef in case the following freeSlot is RIGHT BEFORE the freed one => we will have to update prevRef one more time
			prevNeedingUpdate = curFreeSlot_prevRef;

			//optimization
			if(mergedPrev){ return; }
			mergedNext = true;
		}

		//check next free slot
		curFreeSlot_prevRef = (ulng*)(curFreeSlot[1]);
	}

	//full PC has been freed => de-allocate memory
	ulng PCSize = (tgtFreeLst+1)[0];
	if(totalFreeLstSize + 3*sizeof(ulng) == PCSize){ //3 ulng of PC metadata + each free block occupies the whole PC
		freePC(tgtFreeLst-1, PCSize);
		return;
	}

	//at least one merge => no need to go further
	if(mergedPrev || mergedNext) { return; }



	//CASE 2: CREATE INDEPENDANT SLOT (unmerged)

	//set free slot data (size is unchanged)
	freedSlot[1] = (ulng)(tgtFreeLst); //<=> curSlot->nxt = freeLstHead

	//add slot to free lst, as 1st element of lst (which means it will be accessed in priority in further allocations btw)
	tgtFreeLst[0] = (ulng)(freedSlot); //<=> freeLstHead = curSlot
}



// ---------------- MEMORY SAFETY ----------------

//unsafe: read - write
/*void* unsafe_read(void* r, ulng size){
	
}

void unsafe_write(void* r, void* dat, ulng size){
	
}




//safe: read - write
void* safe_readN(void* r, ulng size){
	
}

boo safe_write(void* r, void* dat, ulng size){
	
}
*/
