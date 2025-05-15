
#ifndef LOCATION_H
#define LOCATION_H

#include <iostream>
#include <sstream>

namespace lox
{
    class Location {
        public:
            Location() = default;
            Location(int line, int column) : line(line), column(column) {};
            ~Location() {};

            int getLine() const { return line; }
            int getColumn() const { return column; }

            Location getNextColumn() const {
                return Location(line, column + 1);
            }

            Location getNextLine() const {
                return Location(line + 1, 1);
            }

            bool operator==(const Location& other) const {
                return line == other.line && column == other.column;
            }

            bool operator!=(const Location& other) const {
                return !(*this == other);
            }

            friend std::ostream& operator<<(std::ostream& os, const Location& loc) {
                loc.print(os);
                return os;
            }

            void print(std::ostream& os = std::cout) const {
                os << "[ line ";
                if (line > 0) {
                    os << line;
                } else {
                    os << "unknown";
                }
                os << ":";
                if (column > 0) {
                    os << column;
                } else {
                    os << "unknown";
                }
                os << "]";
            }

        private:
            int line = -1;
            int column = -1;
    };
} // namespace lox

#endif // LOCATION_H
