#include "Helpers.h"

#include <regex>

namespace engine {

std::vector<std::string> splitNamespaces(std::string const &className) {
    std::regex re("::");
    std::sregex_token_iterator first { className.begin(), className.end(), re, -1 };
    std::sregex_token_iterator last;
    return {first, last};
}

}
