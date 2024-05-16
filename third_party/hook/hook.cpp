// hook.cpp
#include <cstdio>
#include <dlfcn.h>
#include <stdlib.h>
#include <cxxabi.h>
#include "hook.hpp"

unordered_map<int64_t,string> addr_to_name;
unordered_map<string,int64_t> name_to_addr;

inline const char *get_funcname(const char *src) {
        int status = 99;
        const char *f = abi::__cxa_demangle(src, nullptr, nullptr, &status);
        return f == nullptr ? src : f;
}

extern "C" void __attribute__((constructor)) traceBegin(void) {}

extern "C" void __attribute__((destructor)) traceEnd(void) {}

extern "C" void __cyg_profile_func_enter(void *func, void *caller) {
    Dl_info info1, info2;
    if(!dladdr(func, &info1) || !dladdr(caller, &info2)){
        fprintf(stderr, "Failed to dl\n");
        exit(-1);
    }
    fprintf(stdout, "enter func: 0x%016lx, func name: %s father: 0x%016lx, father name:%s\n", func,get_funcname(info1.dli_sname), caller, get_funcname(info2.dli_sname));
}

extern "C" void __cyg_profile_func_exit(void *func, void *caller) {
    // Dl_info info1, info2;
    // if(!dladdr(func, &info1) || !dladdr(caller, &info2)){
    //     fprintf(stderr, "Failed to dl\n");
    //     exit(-1);
    // }
    // fprintf(stdout, "exit func: 0x%016lx, func name: %s father: 0x%016lx, father name:%s\n", func,get_funcname(info1.dli_sname), caller, get_funcname(info2.dli_sname));
}