#ifndef ELFREADER_H
#define ELFREADER_H

#include <memory>
#include <string>
#include <vector>

namespace elf {

    class Elf {
    public:

    private:
    };

    class ElfReader {
    public:
        static std::unique_ptr<Elf> tryCreate(const std::string& filename, const std::vector<char>& bytebuffer);
    };
}


#endif
