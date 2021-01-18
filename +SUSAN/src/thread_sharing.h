#ifndef THREAD_SHARING_H
#define THREAD_SHARING_H

#include <pthread.h>
#include "datatypes.h"
#include "particles.h"

class PBarrier {
	
public:
	pthread_barrier_t barrier;

	PBarrier(const int n_th) {
		pthread_barrier_init(&barrier, NULL, n_th);
	}
	
	~PBarrier() {
		pthread_barrier_destroy(&barrier);
	}
	
	void wait() {
		pthread_barrier_wait(&barrier);
	}
	
};

class DoubleBufferHandler {
private:
	void     *buffer[2];
	Status_t status[2];
	int      ix_readonly;
	int      ix_writeonly;
	PBarrier *barrier;
	
public:
	DoubleBufferHandler(void*ptr_1,void*ptr_2,PBarrier*in_barrier) {
		status[0]    = EMPTY;
		status[1]    = EMPTY;
		ix_readonly  = 0;
		ix_writeonly = 1;
		buffer[0]    = ptr_1;
		buffer[1]    = ptr_2;
		barrier      = in_barrier;
	}
	
	void* RO_get_buffer() {
		return buffer[ix_readonly];
	}
	
	Status_t RO_get_status() {
		return status[ix_readonly];
	}
	
	void RO_sync() {
		barrier->wait();
		barrier->wait();
	}
	
	void* WO_get_buffer() {
		return buffer[ix_writeonly];
	}
	
	Status_t WO_get_status() {
		return status[ix_writeonly];
	}
	
	void WO_sync(Status_t new_status) {
		barrier->wait();
		status[ix_writeonly] = new_status;
		int tmp = ix_writeonly;
		ix_writeonly = ix_readonly;
		ix_readonly  = tmp;
		barrier->wait();
	}
	
};

class StackBuffer {
	
public:
	float *stack;
	ParticlesSubset ptcls;
	int tomo_ix;
	int numel;
	
	StackBuffer(int in_numel) {
		numel = in_numel;
		stack = new float[numel];
		tomo_ix = 0;
	}
	
	~StackBuffer() {
		delete [] stack;
	}
	
};

class WorkerCommand  {
	
public:
	int command;
	
	typedef enum {
		CMD_END = -1,
		CMD_IDLE = 0
	} BasicCommands;
	
protected:
	PBarrier barrier;
	int sending_status;
	
public:
	WorkerCommand(const int n_th) : barrier(n_th){
		command = CMD_IDLE;
		sending_status = 0;
	}
	
	void presend_sync() {
		if(sending_status==0){
			barrier.wait();
			sending_status = 1;
		}
	}
	
	void send_command(int cmd) {
		if(sending_status==0){
			barrier.wait();
		}
		command = cmd;
		barrier.wait();
		sending_status = 0;
	}
	
	int read_command() {
		barrier.wait();
		barrier.wait();
		return command;
	}
	
};

#endif /// THREAD_SHARING_H

