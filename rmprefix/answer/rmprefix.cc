#include <iostream>
#include <string>
#include <cstdlib>

int main() {
	std::size_t found = std::string::npos;
    for (std::string line; std::getline(std::cin, line);) {
    	found = line.find_first_of(" \t");
    	while(found != std::string::npos){
    		line.erase(found,1);
    		found = line.find_first_of(" \t",found);
    	}

        std::cout << line << std::endl;
    }
	exit(EXIT_SUCCESS);
}
