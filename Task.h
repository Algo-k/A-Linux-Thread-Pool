
#pragma once

#include <functional>
#include <condition_variable>
#include "Locked.h"

class Task
{
public:
	std::function<void()> Func;
	Locked<bool> Completed;
	std::mutex Mutex;                  // Prevents data races to the job queue
	std::condition_variable Condition; // Allows threads to wait on new jobs or termination 
	int Delay = 0;
	Task()
	{
		Completed.set(false);
	}
	Task(int delay)
	{
		Delay = delay;
	}
	void Join()
	{
		std::unique_lock<std::mutex> lock(Mutex);
		Condition.wait(lock, [this] {return Completed.get(); });
	}
};

