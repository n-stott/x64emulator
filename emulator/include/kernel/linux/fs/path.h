#ifndef PATH_H
#define PATH_H

#include "span.h"
#include "verify.h"
#include <memory>
#include <string>
#include <vector>

namespace kernel::gnulinux {

    class Path {
    public:
        static std::unique_ptr<Path> tryCreate(std::string pathname);

        static std::unique_ptr<Path> tryJoin(const std::string& prefix, const std::string& suffix);

        Span<const std::string> components() const {
            const std::string* data = components_.data();
            size_t size = components_.size();
            return Span<const std::string>(data, data+size);
        }

        Span<const std::string> componentsExceptLast() const {
            const std::string* data = components_.data();
            size_t size = components_.size();
            if(size <= 1) {
                return Span<const std::string>(nullptr, nullptr);
            } else {
                return Span<const std::string>(data, data+size-1);
            }
        }

        const std::string& last() const {
            verify(!components_.empty(), "Path has empty components");
            return components_.back();
        }

        bool isRoot() const {
            return components_.empty();
        }

    private:
        explicit Path(std::vector<std::string> components);
        std::vector<std::string> components_;
    };

}

#endif