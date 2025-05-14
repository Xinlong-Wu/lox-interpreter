#ifndef SCANNER_H
#define SCANNER_H

#include "compiler/Token.h"
#include <string_view>

using namespace std;
namespace lox {

    class Scanner
    {
    private:
        std::string_view buffer;
        std::string_view::iterator current;
        int line = 1;
        int column = 1;
        bool inInterpolation = false;
        bool isInterpolationStart = false;

        bool isAtEnd() {return current == buffer.end();};
        bool isDigit(char c) {return (c >= '0' && c <= '9') || (c == '.' && isDigit(peek(1)));};
        bool isAlpha(char c) {return (c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z');};
        bool isAlphaOrUnderScore(char c) {return isAlpha(c) || c == '_';};

        Token string();
        Token identifier();
        Token number();
        void skipWhitespace();
        char advance();
    public:
        Scanner(const char *source): buffer(source), current(buffer.begin()) {};
        ~Scanner() {}

        char peek(unsigned pos = 0) {return current[pos];};
        Token next();
        bool match(char expected);
    };

}

#endif
