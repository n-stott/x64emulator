#include "kernel/fs/path.h"
#include "verify.h"

namespace kernel {

    std::unique_ptr<Path> Path::tryCreate(std::string pathname) {
        verify(!pathname.empty(), "Cannot create path from empty pathname");
        verify(pathname[0] == '/', "Cannot create path from non-absolute pathname");
        std::vector<std::string> components;
        size_t pos = 1;
        while(true) {
            size_t nextPos = pathname.find('/', pos);
            std::string component = pathname.substr(pos, nextPos-pos);
            if(component.empty() || component == ".") {
                // do nothing
            } else if(component == "..") {
                if(components.empty()) return {};
                components.pop_back();
            } else {
                components.push_back(std::move(component));
            }
            if(nextPos == std::string::npos) break;
            pos = nextPos+1;
        }
        return std::unique_ptr<Path>(new Path(std::move(components)));
    }

    std::unique_ptr<Path> Path::tryJoin(const std::string& prefix, const std::string& suffix) {
        return tryCreate(prefix + "/" + suffix);
    }

    Path::Path(std::vector<std::string> components) : components_(std::move(components)) {

    }

}