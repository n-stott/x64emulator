#ifndef SHAREDMEMORY_H
#define SHAREDMEMORY_H

#include "kernel/utils/erroror.h"
#include "bitflags.h"
#include "utils.h"
#include <cstddef>
#include <memory>
#include <utility>
#include <vector>

namespace x64 {
    class Mmu;
}

namespace kernel {

    class SharedMemorySegment;

    class SharedMemory {
    public:
        explicit SharedMemory(x64::Mmu& mmu);
        ~SharedMemory();

        struct Key {
            int value;
        };
        static constexpr Key IPC_PRIVATE { 0 };

        struct Id {
            int value;
        };

        enum class GetFlags {
            CREATE = (1 << 0),
            EXCL = (1 << 1),
        };

        enum class AtFlags {
            EXEC = (1 << 0),
            READ_ONLY = (1 << 1),
            REMAP = (1 << 2),
        };

        ErrnoOr<Id> get(Key key, size_t size, int mode, BitFlags<GetFlags> flags);
        ErrnoOr<u64> attach(Id id, u64 preferredAddress, BitFlags<AtFlags> flags);
        int detach(u64 address);
        int rmid(Id id);

    private:
        x64::Mmu& mmu_;
        std::vector<std::pair<Id, std::unique_ptr<SharedMemorySegment>>> segments_;
    };

}


#endif