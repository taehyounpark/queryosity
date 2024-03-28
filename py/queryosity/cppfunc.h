#pragma once

#include <Python.h>

#include <functional>

namespace queryosity {

namespace py {

template <typename Ret, typename... Args>
std::function<Ret(Args...)> cppfunc(const std::string& module_name, const std::string& fn_name) {
    Py_Initialize();

    // Import the module where your cfunc is defined
    PyObject* myModule = PyImport_ImportModule(module_name.c_str());
    if (!myModule) {
        PyErr_Print();
    }

    // Get the function object from the module
    PyObject* cfuncObj = PyObject_GetAttrString(myModule, fn_name.c_str());
    if (!cfuncObj) {
        PyErr_Print();
    }

    // Get the address attribute
    PyObject* cfuncAddr = PyObject_GetAttrString(cfuncObj, "address");
    if (!cfuncAddr) {
        PyErr_Print();
    }

    // Convert address to a callable function pointer in C++
    void* ptr = PyLong_AsVoidPtr(cfuncAddr);
    if (!ptr) {
        PyErr_Print();
    }
    typedef Ret(*Callable)(Args...);
    Callable callable = reinterpret_cast<Callable>(ptr);

    Py_Finalize();

    return std::function<Ret(Args...)>(callable);
  }

}

}