#include <iostream>

void helperFunction() {
    std::cout << "This is a helper function that should be extracted.\n";
    for(int i=0; i<10; ++i) {
        // dummy code to increase size
    }
}

int main() {
    std::cout << "This main function should NOT be extracted.\n";
    helperFunction();
    return 0;
}
