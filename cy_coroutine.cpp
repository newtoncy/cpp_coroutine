// 简单的有栈协程
#define CY_COROUTINE_DLL_EXPORT
#include "cy_coroutine.h"
#include <iostream>
using namespace std;


namespace cy_coroutine {
LoopContext __loopContext;
const int64_t MAIN_REGSTER_OFFSET = (int64_t)__loopContext.MAIN_REGSTER - (int64_t)&__loopContext;
void* LOOP_START_LABLE = NULL;
void* YIELD_LABLE = NULL;
void* FINISH_LABLE = NULL;
void* RECOVER_LABLE = NULL;


// 保存断点信息后跳转到yield
void breakpoint(CoroutineControlBlock& ccb) {
    auto loopContext = get_current_loop();
    int64_t* regster = ccb.regster_value;
    loopContext->recover_flag = false;
    ccb.RIP = &&recover;
    RECOVER_LABLE = &&recover;
    __asm {
        // 保存
        push rax
        push rbx
        mov rax, [regster]
        // 保存rax
        mov rbx, [rsp + 8]
        mov [rax], rbx
        add rax, 8
        // 保存rbx
        mov rbx, [rsp]
        mov [rax], rbx
        add rax, 8
        mov [rax], rcx
        add rax, 8
        mov [rax], rdx
        add rax, 8
        // 保存rsp
        mov rbx, rsp
        add rbx, 16
        mov [rax], rbx
        add rax, 8
        mov [rax], rbp
        add rax, 8
        mov [rax], rsi
        add rax, 8
        mov [rax], rdi
        add rax, 8
        mov [rax], r8
        add rax, 8
        mov [rax], r9
        add rax, 8
        mov [rax], r10
        add rax, 8
        mov [rax], r11
        add rax, 8
        mov [rax], r12
        add rax, 8
        mov [rax], r13
        add rax, 8
        mov [rax], r14
        add rax, 8
        mov [rax], r15
 
        pop rbx
        pop rax
        jmp [YIELD_LABLE]
    }
recover:
    // 其实这个flag现在没用了
    loopContext->recover_flag = false;
    return;
}

// 暂停一个协程
void yield_coro() {
    auto loopContext = get_current_loop();
    auto ccb = loopContext->current_coro;
    loopContext->yield_flag = true;
    breakpoint(*ccb);
}


LoopContext* get_current_loop() {
    return &__loopContext;
}

void deal_previous_task(LoopContext* loopContext) {
    CcbIter begin = loopContext->coroutine_list.begin(), end = loopContext->coroutine_list.end();
    
    if (loopContext->current_coro == end) {
        return;
    }

    if (loopContext->current_coro->state == CoroState::READY && !loopContext->yield_flag)
        loopContext->current_coro->state = CoroState::FINISHING;

    if (loopContext->current_coro->state == CoroState::FINISHING) {
        // 这种情况说明上一个任务做完了

        // 释放栈空间
        auto ccb = *loopContext->current_coro;
        delete[] ccb.stack_base_addr;
        // 从就绪队列中删除
        auto finished = loopContext->current_coro;
        //确保current_coro指向的值是有效的(而不是已经被删除的非法值)
        if (finished == begin)
            loopContext->current_coro = end;
        else
            --(loopContext->current_coro);
        loopContext->coroutine_list.erase(finished);
        
    }
}

void select_next_task(LoopContext* loopContext) {
    CcbIter begin = loopContext->coroutine_list.begin(), end = loopContext->coroutine_list.end();
    if (loopContext->current_coro == end) {
        loopContext->current_coro = begin;
    } else {
        ++loopContext->current_coro;
        if (loopContext->current_coro == end)
            loopContext->current_coro = begin;
    }
}



int64_t TEMP_RAX;  // 临时保存一下rax
void loop_start() {

    YIELD_LABLE = &&yield;
    FINISH_LABLE = &&finish;
    LOOP_START_LABLE = &&loop_start;
    LoopContext* loopContext = get_current_loop();
    // 将此时的堆栈和寄存器状态作为基准
    // 协程退出时恢复到这里
    __asm {
        // 保存
        push rax
        push rbx
        mov rax, [loopContext]
        add rax, [MAIN_REGSTER_OFFSET]
        // 保存rax
        mov rbx, [rsp + 8]
        mov [rax], rbx
        add rax, 8
        // 保存rbx
        mov rbx, [rsp]
        mov [rax], rbx
        add rax, 8
        mov [rax], rcx
        add rax, 8
        mov [rax], rdx
        add rax, 8
        // 保存rsp
        mov rbx, rsp
        add rbx, 16
        mov [rax], rbx
        add rax, 8
        mov [rax], rbp
        add rax, 8
        mov [rax], rsi
        add rax, 8
        mov [rax], rdi
        add rax, 8
        mov [rax], r8
        add rax, 8
        mov [rax], r9
        add rax, 8
        mov [rax], r10
        add rax, 8
        mov [rax], r11
        add rax, 8
        mov [rax], r12
        add rax, 8
        mov [rax], r13
        add rax, 8
        mov [rax], r14
        add rax, 8
        mov [rax], r15
        pop rbx
        pop rax

    }
loop_start:
    // 这里绝对不允许定义任何有构造函数的东西
    // 因为析构函数不会运行

    if (loopContext->coroutine_list.empty())
        return;

    // 收尾上一个任务
    deal_previous_task(loopContext);

    if (loopContext->coroutine_list.empty())
        return;
    
    // 移动到下一个任务
    select_next_task(loopContext);

    auto ccb = *loopContext->current_coro;
    auto function = ccb.function;
    // 清除标记
    loopContext->yield_flag = false;
    loopContext->recover_flag = false;

    if (ccb.state == CoroState::UNSTART) {
        loopContext->current_coro->state = CoroState::READY;
        void* rsp_ = (int64_t*)ccb.stack_base_addr + ccb.size / 8;
        __asm {
            // 将栈挪到堆中
            mov rsp, [rsp_]
            // 返回地址
            push [FINISH_LABLE]
            // call
            jmp [function]
        }
    } else if (ccb.state == CoroState::READY) {
        int64_t* regster = ccb.regster_value;
        loopContext->recover_flag = true;
        __asm {
            mov rax, [regster]
            mov rbx, [rax]
            mov [TEMP_RAX], rbx
            add rax, 8
            mov rbx, [rax]
            add rax, 8
            mov rcx, [rax]
            add rax, 8
            mov rdx, [rax]
            add rax, 8
            mov rsp, [rax]
            add rax, 8
            mov rbp, [rax]
            add rax, 8
            mov rsi, [rax]
            add rax, 8
            mov rdi, [rax]
            add rax, 8
            mov r8, [rax]
            add rax, 8
            mov r9, [rax]
            add rax, 8
            mov r10, [rax]
            add rax, 8
            mov r11, [rax]
            add rax, 8
            mov r12, [rax]
            add rax, 8
            mov r13, [rax]
            add rax, 8
            mov r14, [rax]
            add rax, 8
            mov r15, [rax]
            mov rax, [TEMP_RAX]
            jmp [RECOVER_LABLE]  // 跳转出去

        }
    }

finish:
yield:
    __asm {
        // 此时，栈帧是完好的，但是RSP指针位置不对，寄存器里面的值也不对
        // 访问局部变量之前，我们应该复位RSP和RBP

        // 恢复
        call get_current_loop
        add rax, [MAIN_REGSTER_OFFSET]
        add rax, 8
        mov rbx, [rax]
        add rax, 8
        mov rcx, [rax]
        add rax, 8
        mov rdx, [rax]
        add rax, 8
        mov rsp, [rax]
        add rax, 8
        mov rbp, [rax]
        add rax, 8
        mov rsi, [rax]
        add rax, 8
        mov rdi, [rax]
        add rax, 8
        mov r8, [rax]
        add rax, 8
        mov r9, [rax]
        add rax, 8
        mov r10, [rax]
        add rax, 8
        mov r11, [rax]
        add rax, 8
        mov r12, [rax]
        add rax, 8
        mov r13, [rax]
        add rax, 8
        mov r14, [rax]
        add rax, 8
        mov r15, [rax]
        mov rax, loopContext
        add rax, [MAIN_REGSTER_OFFSET]
        mov rax, [rax]
        jmp[LOOP_START_LABLE]
    }
}


}
