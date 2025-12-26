#include "kernel/linux/fs/path.h"
#include "verify.h"

namespace kernel::gnulinux {

    std::unique_ptr<Path> Path::tryCreate(std::string pathname, std::string cwd) {
        verify(!pathname.empty(), "Cannot create path from empty pathname");
        if(pathname[0] != '/') {
            return tryJoin(cwd, pathname);
        }
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
        if(components_.empty()) {
            absolutePath_ = "/";
        } else {
            for(const auto& component : components_) {
                absolutePath_ += "/" + component;
            }
        }
    }

    Path::Path(std::string file) : Path(std::vector<std::string>{file}) {

    }

    Path::Path(std::string dir, std::string file) : Path(std::vector<std::string>{dir, file}) {

    }

    Path::Path(std::string dir0, std::string dir1, std::string file) : Path(std::vector<std::string>{dir0, dir1, file}) {

    }

    std::optional<Path> Path::tryAppend(const std::string& element) const {
        auto attempt = tryJoin(absolute(), element);
        if(!attempt) return {};
        return *attempt;
    }


}