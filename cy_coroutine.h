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
    UNSTART = 0,  // û��ʼ
    READY = 1,  // �ȴ�����
    FINISHING = 2,  // �Ѿ����أ��ȴ����յ���ջ
};

// Э�̿��ƿ�
struct CoroutineControlBlock {
    // ջ֡��ʼλ��
    int64_t* stack_base_addr = NULL;
    // ջ֡�����С
    size_t size;
    // �Ĵ�����ֵ rsp = regster_value[4]��rax rbx rcx rdx rsp rbp rsi rdi r8~r15
    int64_t regster_value[16] = { 0 };
    // ��һ��ָ��ĵ�ַ
    void* RIP = NULL;
    // Э�̵ĵ�ǰ״̬
    CoroState state = CoroState::UNSTART;
    void* function = NULL;
    std::vector<void*> args;


    void add_arg_ptr(void* arg) {
        args.push_back(arg);
    }
};

typedef std::list<CoroutineControlBlock> CcbList;
typedef CcbList::iterator CcbIter;


// ������ȫ�ֵģ�ÿ��ϵͳ�̶߳�����һ����demo������
struct LoopContext {
    // ��ʾ���ڻָ�
    bool recover_flag = false;
    // ��ʾ����yield
    bool yield_flag = false;
    // ���еȴ����ȵ�Э��
    std::list<CoroutineControlBlock> coroutine_list;
    // ��ѭ����ʼʱ�ļĴ���״̬
    int64_t MAIN_REGSTER[16] = { 0 };
    // ��ǰ�������е�Э��
    CcbIter current_coro;
    LoopContext() { current_coro = coroutine_list.end(); }
    CcbIter add_coro(void* function,
                     size_t size = 4 * 1024 * 1024) {
        CoroutineControlBlock ccb;
        // �ڶ��д���һ��ջ��������64λ����֪�����������cpu�᲻������
        ccb.stack_base_addr = new int64_t[size / 8];
        ccb.function = function;
        ccb.size = size;
        coroutine_list.push_back(ccb);
        return --coroutine_list.end();
    }
};

// ����˵,ÿ���߳�Ӧ����һ��������loopContext����
// ����Ŀǰû���Ƕ��̵߳����
extern LoopContext __loopContext;

// ������,�ǻ�õ�ǰ����ѭ��������
// ʵ����,ʼ�շ��� &__loopContext
DLL_API LoopContext* get_current_loop();

// ��ͣһ��Э��
DLL_API void yield_coro();

// ��ʼ����
DLL_API void loop_start();

// һ���ݽ�
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