#include "arguments.hpp"
 
#include <stdexcept>
#include <iostream>

int main(int argc, char **argv) { 

    try {
        auto output = cmkr::args::handle_args(argc, argv);
        std::string format = "[cmkr]" + std::string(output) + "\n";

        if ( format.find( '\n' ) != std::string::npos )
            format = std::string(output) + "\n";

        std::cerr << format;
        return true;
    }
    catch ( const std::exception &err ) {
        std::string error = err.what();
        std::string format = "[cmkr] error: " + error + "\n";

        if (error.find('\n') != std::string::npos)
            format = error + "\n";

        std::cerr << format;
        return false;
    }

}  
