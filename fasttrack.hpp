#ifndef FASTTRACK_HPP__
#define FASTTRACK_HPP__

#include <stdio.h>
#include <map>
#include "pin.H"

using namespace std;

namespace stm {
  
	class VC {
	public:
    		pthread_mutex_t lock;
    		map<unsigned int, int> vc;
		
		bool isRacy;
    		VC(){ 
			pthread_mutex_init(&lock, NULL);
			isRacy=false;
		}
	};
	
	typedef map<unsigned int, int> vc;
	typedef VC* VCPOINTER;
	typedef vc Epoch;
	
	class VCs {
	public:
	   	 pthread_mutex_t lock;
   		 map<THREADID, VCPOINTER> vcs;
		 
		 VCs(){ pthread_mutex_init(&lock, NULL); }
	};
	
	typedef VCs* VCsptr;
	typedef map<THREADID, VCPOINTER> vcs;
	static VCs glbC;
	static VCs glbL;
	static VCs glbR;
	static VCs glbW;
	static unsigned int totalThreads = 1;
	
	static map<ADDRINT, THREADID> parentIDs; // to get parent thread ids
	
	/************************************************/
	VCPOINTER get_vector_clock(VCsptr m, THREADID k);
	void vc_compare_and_update(VCPOINTER vc1, VCPOINTER vc2);
	int get_clock(VCPOINTER vcptr, THREADID t);
	void vc_increment(VCPOINTER vcptr, THREADID t);
	Epoch vc_epoch(THREADID t, int clk);
	bool vc_epoch_eq(VCPOINTER vp1, Epoch e2);
	bool vc_leq_epoch(VCPOINTER e, VCPOINTER vc);
	void vc_reset(VCPOINTER vc);
	bool vc_is_epoch(VCPOINTER vc_);
	vc vc_set(vc _vc, int t, int c);
	int vc_epoch_tid(Epoch e);
	int vc_epoch_clock(Epoch e);
	bool vc_leq_all(VCPOINTER vc1, VCPOINTER vc2);
	void vc_copy(VCPOINTER vc1, VCPOINTER vc2);
	/************************************************/
	
	static void ThreadCreate(void* addr, ADDRINT parent_process_id, ADDRINT my_process_id, THREADID tid) {
		THREADID t = tid;
		
		pthread_mutex_lock(&glbC.lock);
		parentIDs[my_process_id] = tid;
		
		unsigned int u = t;
		
		if(parentIDs.find(parent_process_id) != parentIDs.end())
			u = (parentIDs.find(parent_process_id))->second;
		
		totalThreads++;
		pthread_mutex_unlock(&glbC.lock);
		
		VCPOINTER Ct = get_vector_clock(&glbC, t); // pthread's
		VCPOINTER Cu = get_vector_clock(&glbC, u); // parent's
	
		vc_compare_and_update(Ct, Cu);        //vc_set_vc(glbC, u, vc_compare_and_update(Ct, Cu));
		vc_increment(Cu, u);         //vc_set_vc(glbC, t, vc_increment(Ct, t));
	}
	
	static void ThreadJoin(ADDRINT addr, ADDRINT parent_process_id, THREADID tid) {
		THREADID t = tid; //PIN_ThreadId();
		unsigned int u = tid; // parent thread id
		
		if(parentIDs.find(parent_process_id) != parentIDs.end())
			u = (parentIDs.find(parent_process_id))->second; 
		
		VCPOINTER Ct = get_vector_clock(&glbC, t); // thread's
 		VCPOINTER Cu = get_vector_clock(&glbC, u); // parent's
		
		vc_compare_and_update(Cu, Ct);    //vc_set_vc(glbC, t, vc_compare_and_update(Ct, Cu));
		vc_increment(Ct, t);        //vc_set_vc(glbC, u, vc_increment(Cu, u));
	}
	
	static bool READ_RACE_NOT_OCCURS(ADDRINT ea, THREADID tid)
	{ // dynamic instrumentation. It records correct thread ids. For performance reasons remove PIN_ThreadId()
	  	THREADID t = tid;
		unsigned int x = (unsigned int)ea;
		bool not_racy = true;

		VCPOINTER Rx = get_vector_clock(&glbR, x);    //get read vector clocks
		
		pthread_mutex_lock(&Rx->lock);      //  lock
	//	if(!Rx->isRacy) {
			//pthread_mutex_lock(&glbW.lock);
			VCPOINTER Wx = get_vector_clock(&glbW, x);    //get write vector clock
			//pthread_mutex_unlock(&glbW.lock);
			
			VCPOINTER Ct = get_vector_clock(&glbC, t);    //get thread vector clock	
			pthread_mutex_lock(&Wx->lock);      //  lock
			Epoch Et = vc_epoch(t, get_clock(Ct, t));   // make epoch of the thread
	
			if(vc_epoch_eq(Rx, Et)) { // if(x.R == t.epoch) return; // Same epoch 63.4%
				//Rx->same_epoch++;
				pthread_mutex_unlock(&Wx->lock); //unlocking
				pthread_mutex_unlock(&Rx->lock); // unlocking
				return true;
			}
			
			if(! vc_leq_epoch(Wx, Ct)) { //if(x.W > t.C[TID(x.W)]) error;     // write-read race
				
				Rx->isRacy = true; /* printf("Read-Race!\n");*/ 
				not_racy = false;	//reset vector clock;
				vc_reset(Rx);
				vc_reset(Wx);					
			}
			
			if(vc_is_epoch(Rx)) { // if(x.R != READ_SHARED) 
	
				if(vc_leq_epoch(Rx, Ct)) { //if(x.R <= t.C[TID(x.R)]) //exclusive 15.7%
					Rx->vc = Et;       // Rx check for this assignment
				}
				else {                     //Share 0.1% //4. (SLOW PATH)
					vc _vc;
					vc_set(_vc, t, get_clock(Ct, t));        // set Et
					vc_set(_vc, vc_epoch_tid(Rx->vc), vc_epoch_clock(Rx->vc)); // set Rx
					Rx->vc = _vc;
				}
			}
			else { //if(x.R == READ_SHARED) //   //shared 20.8% 2. READ SHARED
	 			vc_set(Rx->vc, t, get_clock(Ct, t));  //vc_set changes the clock
			}
			pthread_mutex_unlock(&Wx->lock); //unlock

		pthread_mutex_unlock(&Rx->lock); // unlock
		//return not_racy;
		printf("READ_RACE_NOT_OCCURS\n");
		return true; // Always no race occurred.
	}
	
	static bool WRITE_RACE_NOT_OCCURS(ADDRINT ea, THREADID tid)
	{
		bool not_racy = true;
		THREADID t = tid;
		ADDRINT x = (ADDRINT)ea;

		VCPOINTER Rx = get_vector_clock(&glbR, x); // get read vector clock
		
		pthread_mutex_lock(&Rx->lock); // lock read vc
	//	if(!Rx->isRacy) {
		//pthread_mutex_lock(&glbW.lock);
		VCPOINTER Wx = get_vector_clock(&glbW, x); // get write vector clock 
		//pthread_mutex_unlock(&glbW.lock);
		
		VCPOINTER Ct = get_vector_clock(&glbC, t); // get thread clockvector
		pthread_mutex_lock(&Wx->lock); // lock write vc
		
		Epoch Et = vc_epoch(t, get_clock(Ct, t));

		if(vc_epoch_eq(Wx, Et)) {     // same epoch 71.0%
//		      Wx->same_epoch++;
			pthread_mutex_unlock(&Wx->lock);
			pthread_mutex_unlock(&Rx->lock);
			return true;	
		}
		
		if(! vc_leq_epoch(Wx, Ct)) {//if(W.x > Ct)  race!!! //
			Rx->isRacy = true; //printf("Write-Write-Race!\n");
			vc_reset(Rx);
			vc_reset(Wx);
			not_racy = false;
		}
		
		// NOT SAME EPOCH
		if(vc_is_epoch(Rx))    // if(Rx != READ_SHARED ) // 2.  NOT WRITE SHARED it is EPOCH (EXCLUSIVE)
		{// Shared 28.9%
//			Wx->write_shared++;
			if(!vc_leq_epoch(Rx, Ct))
			{ 
				Rx->isRacy = true; /* printf("Write-Race!\n"); */
				vc_reset(Rx);
				vc_reset(Wx);
				not_racy = false;
			}
		}
		else  {// if(Rx == READ_SHARED) // 3. (SLOW PATH) it is read shared
			//Exclusive 0.1%
//			Wx->slowpath++;
			if(!vc_leq_all(Rx, Ct)) // check all clocks
				{
					Rx->isRacy = true; /*printf("Write-Race!\n");*/ 
					vc_reset(Rx);
					vc_reset(Wx);
					not_racy = false;
				}
			else
				Rx->vc = vc_epoch(t,0);   // vc_set_vc(glbR, x, vc_epoch(0,0));
		}
		Wx->vc = Et;              // vc_set_vc(glbW, x, Et);
		pthread_mutex_unlock(&Wx->lock);
//              }
		pthread_mutex_unlock(&Rx->lock);
		//return not_racy;
// 		printf("WRITE_RACE_NOT_OCCURS\n");
		return true; // to avoid rollback
	}

	static void LOCKING_UPDATE_FASTTRACK(ADDRINT lok, THREADID tid) 
	{
		VCPOINTER Ct = get_vector_clock(&glbC, tid);    // get pointer to thread VC
		
		//pthread_mutex_lock(&glbL.lock);
		VCPOINTER Lm = get_vector_clock(&glbL, lok);  // get pointer to lock VC
		//pthread_mutex_unlock(&glbL.lock);
	
		pthread_mutex_lock(&Lm->lock);     //lock the lock vector(avoid others accesses)
		vc_compare_and_update(Ct, Lm);     // set
		
		pthread_mutex_unlock(&Lm->lock);   // release locks
	}
	
	static void UNLOCKING_UPDATE_FASTTRACK(ADDRINT lok, THREADID tid) {

		VCPOINTER Ct = get_vector_clock(&glbC, tid); //get thread vectorclock pointer
		
		//pthread_mutex_lock(&glbL.lock);
		VCPOINTER Lm = get_vector_clock(&glbL, lok); // get lock thread pointer
		//pthread_mutex_unlock(&glbL.lock);
	
		pthread_mutex_lock(&Lm->lock);
		
		vc_copy(Lm, Ct);                   //set 
		vc_increment(Ct, tid);             // increment
		
		pthread_mutex_unlock(&Lm->lock);
	}
	
	
	inline VCPOINTER get_vector_clock(VCsptr m, THREADID k) 
	{
			
	 	map<THREADID, VCPOINTER>::iterator it = (m->vcs).find(k);
	 	if(it != (m->vcs).end()) {
	 		return it->second;
	 	}
	 	VCPOINTER tmp = new VC();
		tmp->vc  = map<THREADID, int>();
		if(m == &glbC)
		  tmp->vc[k] = 1;
		else
		  tmp->vc[k] = 0;
	 	(m->vcs)[k] = tmp;
	 	return tmp;
	}
	
	inline int get_clock(VCPOINTER vcptr, THREADID t) {
		vc::iterator it = vcptr->vc.find(t);
		if(it != vcptr->vc.end()) {
			int c = it->second;
			assert(c > 0);
			return c;
		}
		return 0; // rest is 0
	}
	
	inline void vc_compare_and_update(VCPOINTER vc1, VCPOINTER vc2) {
	
		for (vc::iterator it=vc2->vc.begin() ; it != vc2->vc.end(); it++) {
	 		THREADID t = it->first;
	 		int c1 = get_clock(vc1, t);
	 		int c2 = it->second;
	 		if(c2 > c1) {
	 			vc1->vc[t] = c2;
	 		}
		}
	}
	
	inline void vc_reset(VCPOINTER vc) 
	{
		for (vc::iterator it=vc->vc.begin() ; it != vc->vc.end(); it++)
	 		it->second = 0;
	}
	
	inline void vc_copy(VCPOINTER vc1, VCPOINTER vc2) {
		vc1->vc = vc2->vc;
	}
	
	inline Epoch vc_epoch(THREADID t, int clk) {
		vc vc_;
		vc_[t] = clk;
		return vc_;
	}
	
	inline void vc_increment(VCPOINTER vcptr, THREADID t) {  
		int c = get_clock(vcptr, t);
		vcptr->vc[t] = c + 1;
	}
	
	inline bool vc_is_epoch(VCPOINTER vc_){
		return vc_->vc.size() <= 1;
	}
	
	inline bool is_epoch(vc vc_){
	        return vc_.size() <= 1;
	}
		
	inline int vc_epoch_tid(Epoch e) {
		if(e.size() == 0) {
			return 0;
		}
		assert (e.size() == 1);
		vc::iterator it=e.begin();
		return it->first;
	}
	
	inline int vc_epoch_clock(Epoch e) {
		if(e.size() == 0) {
			return 0;
		}
		assert (e.size() == 1);
		vc::iterator it=e.begin();
		return it->second;
	}
	
	inline bool vc_epoch_eq(VCPOINTER vp1, Epoch e2) {
		assert(vc_is_epoch(vp1));
		assert(is_epoch(e2));
		return ((vc_epoch_tid(vp1->vc) == vc_epoch_tid(e2)) && (vc_epoch_clock(vp1->vc) == vc_epoch_clock(e2)));
	}
	
	inline bool vc_leq_epoch(VCPOINTER e, VCPOINTER vc) {
	        assert(vc_is_epoch(e));
	        int t = vc_epoch_tid(e->vc);
	        int c1 = vc_epoch_clock(e->vc);
	        int c2 = get_clock(vc, t);
	        return c1 <= c2;
	}
	
	inline vc vc_set(vc _vc, int t, int c) {
	        if(c > 0) {
	                _vc[t] = c;
	        } else {
	                _vc.erase(t);
	        }
	        return _vc;
	}
	
	inline bool vc_leq_all(VCPOINTER vc1, VCPOINTER vc2) {
		for (vc::iterator it=vc2->vc.begin() ; it != vc2->vc.end(); it++ ) {
			int t = it->first;
			int c1 = get_clock(vc1, t);
			int c2 = it->second;
			if(c1 > c2) {
				return false;
			}
		}
		return true;
	}	
	
}

#endif
