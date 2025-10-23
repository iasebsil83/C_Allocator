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






// ---------------- PAGE SCALE ----------------

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



//add - remove
void* newMap(ulng len) {

	//allocate new map
	ulng* p = syscall_mmap(
		null,                //adr
		sizeof(ulng) + len,  //len
		MM__PROT_READ   | MM__PROT_WRITE,    //prot
		MM__MAP_PRIVATE | MM__MAP_ANONYMOUS, //flags
		-1, //fd
		0   //offset
	);

	//err
	if(p == MM__MAP_FAILED){
		syscall_write(0, "[ ERROR ] > Unable to allocate more memory for process.", 59);
		syscall_exit(43);
	}

	//keep track of map len
	p[0] = len;
	return p+1;
}

void freeMap(void* adr) {

	//retrieve map len
	ulng len = ((ulng*)adr)[-1];

	//free map
	if(syscall_munmap(adr, len) == -1){
		syscall_write(0, "[ ERROR ] > Unable to de-allocate memory from process.", 54);
		syscall_exit(54);
	}
}






// ---------------- USER SCALE ----------------

//new - free
/*ref new(unt_m len){

	//empty segment => err
	if(len == 0){ return NULL; }

	return adr;
}

void free(ref r){
}*/
