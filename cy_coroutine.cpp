// �򵥵���ջЭ��
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


// ����ϵ���Ϣ����ת��yield
void breakpoint(CoroutineControlBlock& ccb) {
    auto loopContext = get_current_loop();
    int64_t* regster = ccb.regster_value;
    loopContext->recover_flag = false;
    ccb.RIP = &&recover;
    RECOVER_LABLE = &&recover;
    __asm {
        // ����
        push rax
        push rbx
        mov rax, [regster]
        // ����rax
        mov rbx, [rsp + 8]
        mov [rax], rbx
        add rax, 8
        // ����rbx
        mov rbx, [rsp]
        mov [rax], rbx
        add rax, 8
        mov [rax], rcx
        add rax, 8
        mov [rax], rdx
        add rax, 8
        // ����rsp
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
    // ��ʵ���flag����û����
    loopContext->recover_flag = false;
    return;
}

// ��ͣһ��Э��
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
        // �������˵����һ������������

        // �ͷ�ջ�ռ�
        auto ccb = *loopContext->current_coro;
        delete[] ccb.stack_base_addr;
        // �Ӿ���������ɾ��
        auto finished = loopContext->current_coro;
        //ȷ��current_coroָ���ֵ����Ч��(�������Ѿ���ɾ���ķǷ�ֵ)
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



int64_t TEMP_RAX;  // ��ʱ����һ��rax
void loop_start() {

    YIELD_LABLE = &&yield;
    FINISH_LABLE = &&finish;
    LOOP_START_LABLE = &&loop_start;
    LoopContext* loopContext = get_current_loop();
    // ����ʱ�Ķ�ջ�ͼĴ���״̬��Ϊ��׼
    // Э���˳�ʱ�ָ�������
    __asm {
        // ����
        push rax
        push rbx
        mov rax, [loopContext]
        add rax, [MAIN_REGSTER_OFFSET]
        // ����rax
        mov rbx, [rsp + 8]
        mov [rax], rbx
        add rax, 8
        // ����rbx
        mov rbx, [rsp]
        mov [rax], rbx
        add rax, 8
        mov [rax], rcx
        add rax, 8
        mov [rax], rdx
        add rax, 8
        // ����rsp
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
    // ������Բ��������κ��й��캯���Ķ���
    // ��Ϊ����������������

    if (loopContext->coroutine_list.empty())
        return;

    // ��β��һ������
    deal_previous_task(loopContext);

    if (loopContext->coroutine_list.empty())
        return;
    
    // �ƶ�����һ������
    select_next_task(loopContext);

    auto ccb = *loopContext->current_coro;
    auto function = ccb.function;
    // ������
    loopContext->yield_flag = false;
    loopContext->recover_flag = false;

    if (ccb.state == CoroState::UNSTART) {
        loopContext->current_coro->state = CoroState::READY;
        void* rsp_ = (int64_t*)ccb.stack_base_addr + ccb.size / 8;
        __asm {
            // ��ջŲ������
            mov rsp, [rsp_]
            // ���ص�ַ
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
            jmp [RECOVER_LABLE]  // ��ת��ȥ

        }
    }

finish:
yield:
    __asm {
        // ��ʱ��ջ֡����õģ�����RSPָ��λ�ò��ԣ��Ĵ��������ֵҲ����
        // ���ʾֲ�����֮ǰ������Ӧ�ø�λRSP��RBP

        // �ָ�
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
