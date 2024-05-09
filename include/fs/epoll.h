#ifndef EPOLL_H
#define EPOLL_H

#include "fs/fsobject.h"

namespace kernel {

    class Epoll : public FsObject {
    public:
        explicit Epoll(FS* fs) : FsObject(fs) { }

        bool isEpoll() const override { return true; }

        void close() override;
        bool keepAfterClose() const override { return false; }
    private:

    };

}

#endif