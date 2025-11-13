#include "kernel/linux/shm/sharedmemorysegment.h"
#include "x64/mmu.h"
#include "verify.h"
#include <sys/shm.h>


namespace kernel::gnulinux {

    std::unique_ptr<SharedMemorySegment> SharedMemorySegment::tryCreate(x64::Mmu& mmu, int mode, size_t size) {
        if(mode & (~0777)) return {};
        int id = ::shmget(IPC_PRIVATE, size, IPC_CREAT | mode);
        if(id < 0) return {};
        return std::unique_ptr<SharedMemorySegment>(new SharedMemorySegment(mmu, id, mode, size));
    }

    SharedMemorySegment::SharedMemorySegment(x64::Mmu& mmu, int id, int mode, size_t size) :
            mmu_(mmu),
            id_(id),
            mode_(mode),
            size_(size) {
        // we are good !
        (void)mode_;
    }

    SharedMemorySegment::~SharedMemorySegment() {
        verify(markedForRemoval_, "Segment was not marked for removal !");
    }

    ErrnoOr<u64> SharedMemorySegment::attach(bool readonly, bool executable) {
        BitFlags<x64::PROT> prot;
        prot.add(x64::PROT::READ);
        if(!readonly) {
            prot.add(x64::PROT::WRITE);
        }
        if(executable) {
            prot.add(x64::PROT::EXEC);
        }
        BitFlags<x64::MAP> flags;
        flags.add(x64::MAP::ANONYMOUS);
        flags.add(x64::MAP::PRIVATE);

        // THIS IS A MASSIVE HACK :-p
        // We reserve the range in the Mmu and in host memory and remap the shared region on top.
        auto addr = mmu_.mmap(0, size_, prot, flags);
        if(!addr) return ErrnoOr<u64>(-ENOMEM);
        void* ret = ::shmat(id_, (void*)(mmu_.base() + addr.value()), SHM_REMAP);
        if(ret == (void*)(-1)) {
            return ErrnoOr<u64>(-errno);
        }

        attachedAddress_ = addr;
        ++numAttach_;
        return ErrnoOr<u64>(addr.value());
    }

    int SharedMemorySegment::detach() {
        verify(numAttach_ > 0);
        verify(attachedAddress_.has_value(), "Detaching non-attached SharedMemorySegment");
        --numAttach_;

        int ret = ::shmdt((void*)(mmu_.base() + attachedAddress_.value()));
        // We cannot mprotect due to the hack above
        attachedAddress_ = {};

        return ret;
    }

    void SharedMemorySegment::rm() {
        markedForRemoval_ = true;
    }
}