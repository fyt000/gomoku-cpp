#include <iostream>

void printBoard() {
	std::cout << " ";
	for (int j = 0; j < 15; j++) {
		std::cout.width(3);
		std::cout << j;
	}
	std::cout << std::endl;
	for (int i = 0; i < 15; i++) {
		std::cout.width(2);
		std::cout << i;
		for (int j = 0; j < 15; j++) {
			std::cout << " - ";
		}
		std::cout << std::endl;
	}
}

int main() {
	printBoard();
	std::cin.get();
}