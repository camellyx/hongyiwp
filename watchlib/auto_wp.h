//////////////////////////////////////////////////////
//													//
//		This is the headder file of the automatic	//
//		watchpoint implementation. It includes		//
//		automatic watchpoint adding and removing	//
//													//
//													//
//////////////////////////////////////////////////////

int open_ctl (char * file_path);
//MODIFIES:
//EFFECTS:
//

void close_ctl (int ctlfd);
//MODIFIES:
//EFFECTS:
//

void set_byte (int target_addr, int target_wflags, int ctlfd);
//INPUTS:	target_addr specifies where you want to put the watchpoint, and target_wflags specifies which flag you want to add; ctlfd determine which
//			process you are adding.
//MODIFIES: Watchpoint array (by writing into ctl file)
//EFFECTS:	Add a watchpoint on a single byte. If the watchpoint already exist but have a different flag, then it will add the target_flag by ORing them
//			If the flag is the same, then do noting.
//SPECIAL NOTICE:	It does automatic splitting if it changes a byte of flag in the middle of a wide watchpoint.
//					It also automatic merging the watchpoints if the newly added watchpoints is adjacent to the watchpoint before it or right after it.

//Below are basicly the same with set_byte only with a difference of range(2bytes, 4bytes & 8bytes).
void set_2byte (int target_addr, int target_wflags, int ctlfd);
void set_4byte (int target_addr, int target_wflags, int ctlfd);
void set_8byte (int target_addr, int target_wflags, int ctlfd);

void rm_range (int target_addr, int target_size, int ctlfd);
//INPUTS:	target_addr specifies where you want to put the watchpoint, and target_size specifies the range you want to remove; ctlfd determine which
//			process you are adding.
//MODIFIES:	Watchpoint array (by writting into ctl file)
//EFFECTS:	It remove all the watchpoints staring with target_addr, with a intervle of size.

void set_range (int target_addr, int target_size, int target_wflags, int ctlfd);
//MODIFIES: Watchpoint array (by writing into ctl file)
//EFFECTS:	Add a watchpoint on target_addr with a length of target_size. If the watchpoint already exist but have a different flag, then it will add the target_flag by ORing them. If the flag is the same, then do noting.
//SPECIAL NOTICE:	It does automatic splitting if it changes a byte of flag in the middle of a wide watchpoint.
//					It also automatic merging the watchpoints if the newly added watchpoints is adjacent to the watchpoint before it or right after it.

void watch_print();
//MODIFIE:	Nothing
//EFFECTS:	Print out all the watchpoints to the screen.
