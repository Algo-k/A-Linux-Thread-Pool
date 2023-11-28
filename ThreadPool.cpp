
#include "ThreadPool.h"

ThreadPool::ThreadPool(int initialTrdNum, int stackSize)
{
    _initialTrdNum = initialTrdNum;
    _trdNum.set(0);
    _runningTasksNum.set(0);
    _decreaseNumber.set(0);
    StackSize = stackSize;
}

void ThreadPool::ExecuteJob(Task* task)
{
    MSleep(task->Delay);
    task->Func();
    task->Mutex.lock();
    task->Completed.set(true);
    task->Condition.notify_all();
    task->Mutex.unlock();

}

void ThreadPool::ControlLoop()
{
    while (true)
    {
        CheckToIncrease();
        MSleep(50);
        CheckToDecrease();
        MSleep(50);
    }
}

void ThreadPool::ThreadLoop()
{
    while (true)
    {
        std::shared_ptr<Task> task;
        {
            std::unique_lock<std::mutex> lock(queue_mutex);
            mutex_condition.wait(lock, [this]
                {
                    return !tasks.empty() || should_terminate || _decreaseNumber.get()>0;
                });
            if (should_terminate) {
                break;
            }
            if (_decreaseNumber.get() > 0)
            {
                _decreaseNumber.decrease();
                break;
            }
            task = tasks.front();
            tasks.pop();
        }
        ExecuteJob(task.get());     // new method Execute, join in task class (wait)
        _runningTasksNum.decrease();
    }
    _trdNum.decrease();
}

void ThreadPool::Start()
{
    Increase(_initialTrdNum);
    controlTrd = new std::thread(&ThreadPool::ControlLoop, this);
}

void* ThreadPool::TrdCore(void* ptr)
{
    ((ThreadPool*)ptr)->ThreadLoop();
}

void ThreadPool::Increase(int num)
{
    for (int i = 0; i < num; i++)
    {
        _trdNum.increase();
        pthread_t trd;
        pthread_attr_t attr;
        pthread_attr_init(&attr);
        pthread_attr_setstacksize(&attr, StackSize);
        pthread_create(&trd, &attr, &ThreadPool::TrdCore, (void*)this);
        threads.push_back(trd);
    }
}

void ThreadPool::Decrease(int num)
{
    _decreaseNumber.set(num);
    for (int i = 0; i < num; i++)
        mutex_condition.notify_one();
}

void ThreadPool::CheckToIncrease()
{
    if (std::min(_runningTasksNum.get() + DistanceToTop, MaxTrdNum) - _trdNum.get()>0)
    {
        Increase(std::min(_runningTasksNum.get() + DistanceToTop,MaxTrdNum) - _trdNum.get());
    }
}

void ThreadPool::CheckToDecrease()
{
    if (_trdNum.get() - std::max(_runningTasksNum.get() + DistanceFromBottom, MinTrdNum)>0)
    {
        Decrease(_trdNum.get() - std::max(_runningTasksNum.get() + DistanceFromBottom, MinTrdNum));
    }
}

void ThreadPool::_run(std::shared_ptr<Task> task)
{
    queue_mutex.lock();
    tasks.push(task);
    queue_mutex.unlock();
    _runningTasksNum.increase();
    mutex_condition.notify_one();
}

bool ThreadPool::busy() 
{
    bool poolbusy;
    queue_mutex.lock();
    poolbusy = tasks.empty();
    queue_mutex.unlock();
    return !poolbusy;
}

void ThreadPool::Stop()
{
    queue_mutex.lock();
    should_terminate = true;
    queue_mutex.unlock();
    mutex_condition.notify_all();
    for (auto active_thread : threads)
    {
        pthread_join(active_thread, NULL);
    }
    threads.clear();
}