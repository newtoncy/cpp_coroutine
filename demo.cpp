#include <iostream>
#define CY_COROUTINE_STATIC_LINK
#include "cy_coroutine.h"
#include <string>
using namespace std;

#define cyc cy_coroutine

void nest(const char* str) {
    for (int i = 0; i < 10; i++) {
        cout << str << "_nest_function_yield"<< i << endl;
        cyc::yield_coro();
    }
    
    
}

void function_a() {
    cout << "function_a started" << endl;
    auto str = (string*)cyc::get_arg(0);
    cout << *str;
    delete str;
    cout << "before yield" << endl;
    cyc::yield_coro();
    cout << "after yield" << endl;
    nest("function_a");
}

void function_b() {
    cout << "function_b started" << endl;
    nest("function_b");
}

int main() {
    auto ccb = cyc::add_coro((void*)function_a);
    ccb->add_arg_ptr(new string("Passing param\n"));
    ccb = cyc::add_coro((void*)function_b);
    cyc::loop_start();
    return 0;
}
