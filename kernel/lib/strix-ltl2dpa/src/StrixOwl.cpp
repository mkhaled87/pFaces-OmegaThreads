#include <iostream>
#include <string>

#include "StrixOwl.h"

namespace strix_owl {
    std::string owl_object_to_string(JVM owl, void* object) {
        constexpr size_t SIZE = 256;
        char buffer[SIZE];
        size_t len = print_object_handle(owl, object, buffer, SIZE);
        if (len < SIZE) {
            return std::string(buffer, len);
        }
        else {
            // large string, use dynamic allocation
            char* dyn_buffer;
            size_t size = SIZE;
            while (len >= size) {
                size *= 2;
                dyn_buffer = new char[size];
                len = print_object_handle(owl, object, dyn_buffer, size);
            }
            std::string string(dyn_buffer, len);
            delete[] dyn_buffer;
            return string;
        }
    }
}