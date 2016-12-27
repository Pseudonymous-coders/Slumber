#include <iostream> //Test printing
#include <chrono> //Test std C++11
#include <thread> //Test time

int main() {
	std::cout << "This shouldn't work on amd64, if you see this you must be on the gnuaebi, great!" << std::endl; //Test the print
	std::this_thread::sleep_for(std::chrono::milliseconds(1000)); //Sleep one second
	std::cout << "One second later.. Test complete!" << std::endl; //Amazing print
	return 0; //Return success
}
