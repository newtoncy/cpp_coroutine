#pragma once
#if defined(CY_COROUTINE_DLL_EXPORT)
#define DLL_API __declspec(dllexport)
#elif defined(CY_COROUTINE_STATIC_LINK)
#define DLL_API
#else
#define DLL_API __declspec(dllimport)
#endif

#include "cy_coroutine.h"
#include <list>
#include <stddef.h>
#include <vector>

namespace cy_coroutine {

enum class CoroState {
    UNSTART = 0,  // 没开始
    READY = 1,  // 等待调度
    FINISHING = 2,  // 已经返回，等待回收调用栈
};

// 协程控制块
struct CoroutineControlBlock {
    // 栈帧开始位置
    int64_t* stack_base_addr = NULL;
    // 栈帧分配大小
    size_t size;
    // 寄存器的值 rsp = regster_value[4]，rax rbx rcx rdx rsp rbp rsi rdi r8~r15
    int64_t regster_value[16] = { 0 };
    // 下一条指令的地址
    void* RIP = NULL;
    // 协程的当前状态
    CoroState state = CoroState::UNSTART;
    void* function = NULL;
    std::vector<void*> args;


    void add_arg_ptr(void* arg) {
        args.push_back(arg);
    }
};

typedef std::list<CoroutineControlBlock> CcbList;
typedef CcbList::iterator CcbIter;


// 不该是全局的，每个系统线程都该有一个，demo不考虑
struct LoopContext {
    // 表示正在恢复
    bool recover_flag = false;
    // 表示正在yield
    bool yield_flag = false;
    // 所有等待调度的协程
    std::list<CoroutineControlBlock> coroutine_list;
    // 主循环开始时的寄存器状态
    int64_t MAIN_REGSTER[16] = { 0 };
    // 当前正在运行的协程
    CcbIter current_coro;
    LoopContext() { current_coro = coroutine_list.end(); }
    CcbIter add_coro(void* function,
                     size_t size = 4 * 1024 * 1024) {
        CoroutineControlBlock ccb;
        // 在堆中创建一个栈，并对齐64位（不知道如果不对齐cpu会不会疯掉）
        ccb.stack_base_addr = new int64_t[size / 8];
        ccb.function = function;
        ccb.size = size;
        coroutine_list.push_back(ccb);
        return --coroutine_list.end();
    }
};

// 按理说,每个线程应该有一个独立的loopContext对象
// 但是目前没考虑多线程的情况
extern LoopContext __loopContext;

// 语义上,是获得当前调度循环上下文
// 实际上,始终返回 &__loopContext
DLL_API LoopContext* get_current_loop();

// 暂停一个协程
DLL_API void yield_coro();

// 开始调度
DLL_API void loop_start();

// 一个捷近
inline CcbIter add_coro(void* function,
                        size_t size = 4 * 1024 * 1024) {
    return get_current_loop()->add_coro(function, size);
}


inline void* get_arg(size_t index) {
    auto coro = get_current_loop()->current_coro;
    if (index < coro->args.size())
        return coro->args[index];
    throw std::exception("index error");
}
}  // namespace cy_coroutine

#undef DLL_API