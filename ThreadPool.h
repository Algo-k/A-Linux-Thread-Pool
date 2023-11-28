#pragma once
#include <mutex>
#include <condition_variable>
#include <vector>
#include <queue>
#include <functional>
#include <thread>
#include "Locked.h"
#include "Task.h"
#include <pthread.h>
#include <memory>

class ThreadPool
{
public:
    int MaxTrdNum = 1000;
    int MinTrdNum = 50;
    int DistanceToTop = 10;
    int DistanceFromBottom = 150;
    int StackSize = 0;

    ThreadPool(int initialTrdNum, int stackSize);
    void Start();
    void Stop();
    bool busy();
    void CheckToIncrease();
    void CheckToDecrease();
    int GetNumber()
    {
        return _trdNum.get();
    }

        // ---- Caller is resposible for deleting pointer
    template<typename _Callable, typename... _Args>
    std::shared_ptr<Task> RunStatic(_Callable __f, _Args... __args)
    {
        auto f = [__f, __args...]
        {
            __f(__args...);
        };
        std::shared_ptr<Task> task = std::make_shared<Task>();
        task->Func = f;
        _run(task);
        return task;
    }

        // ---- Caller is resposible for deleting pointer
    template<typename _Callable, typename... _Args>
    std::shared_ptr<Task> RunStaticDelay(_Callable __f, _Args... __args, int delay)
    {
        auto f = [__f, __args...]
        {
            __f(__args...);
        };
        std::shared_ptr<Task> task = std::make_shared<Task>();
        task->Func = f;
        _run(task);
        return task;
    }

        // ---- Caller is resposible for deleting pointer
    template<typename _Callable, typename _Cls, typename... _Args>
    std::shared_ptr<Task> RunInClass(_Callable __f, _Cls __cls, _Args... __args)
    {
        auto f = [__f, __cls, __args...]
        {
            (__cls->*__f)(__args...);
        };
        std::shared_ptr<Task> task = std::make_shared<Task>();
        task->Func = f;
        _run(task);
        return task;
    }

        // ---- Caller is resposible for deleting pointer
    template<typename _Callable, typename _Cls, typename... _Args>
    std::shared_ptr<Task> RunInClassDelay(int delay, _Callable __f, _Cls __cls, _Args... __args)
    {
        auto f = [__f, __cls, __args...]
        {
            (__cls->*__f)(__args...);
        };
        std::shared_ptr<Task> task = std::make_shared<Task>();
        task->Func = f;
        _run(task);
        return task;
    }

private:
    void ExecuteJob(Task* task);
    void _run(std::shared_ptr<Task> task);
    static void* TrdCore(void* ptr);
    int _initialTrdNum = 0;
    Locked<int> _trdNum;
    Locked<int> _runningTasksNum;
    void ThreadLoop();
    void ControlLoop();
    void Increase(int num);
    void Decrease(int num);

    bool should_terminate = false;
    Locked<int> _decreaseNumber;
    std::mutex queue_mutex;
    std::queue<std::shared_ptr<Task>> tasks;
    std::condition_variable mutex_condition;
    std::vector<pthread_t> threads;
    std::thread* controlTrd;
};
