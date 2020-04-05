#include <iostream>

#include "bas/vector.h"

using bas::Vector;

int main()
{
    Vector<int> vec = {1, 2, 3, 4, 5};
    vec.as_ref().print_as_lines("vec");
    std::cout << "Hello World\n";
    return 0;
}
