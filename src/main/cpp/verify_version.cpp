#include <iostream>

int main() {
    long version = __cplusplus;
    std::cout << "Running C++ Version: ";

    if (version == 202302L) std::cout << "C++23";
    else if (version == 202002L) std::cout << "C++20";
    else if (version == 201703L) std::cout << "C++17";
    else if (version == 201402L) std::cout << "C++14";
    else if (version == 201103L) std::cout << "C++11";
    else if (version == 199711L) std::cout << "C++98";
    else std::cout << "Unknown (" << version << ")";

    std::cout << "\nGCC Version: " << __VERSION__ << std::endl;
    return 0;
}