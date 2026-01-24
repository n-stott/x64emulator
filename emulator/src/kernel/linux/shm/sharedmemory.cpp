#include "kernel/linux/shm/sharedmemory.h"
#include "kernel/linux/shm/sharedmemorysegment.h"
#include "x64/mmu.h"
#include "verify.h"
#include <algorithm>
#include <errno.h>

namespace kernel::gnulinux {

    SharedMemory::SharedMemory() { }

    SharedMemory::~SharedMemory() = default;

    ErrnoOr<SharedMemory::Id> SharedMemory::get(Key key, size_t size, int mode, BitFlags<GetFlags> flags) {
        using ReturnType = ErrnoOr<SharedMemory::Id>;

        verify(key.value == IPC_PRIVATE.value, "Only IPC_PRIVATE is supported");
        verify(flags.test(GetFlags::CREATE), "Only create is supported");
        verify(!flags.test(GetFlags::EXCL), "Excl is not supported");
        
        size = x64::Mmu::pageRoundUp(size);

        auto segment = SharedMemorySegment::tryCreate(mode, size);
        Id id { segment->id() };

        segments_.push_back(std::make_pair(id, std::move(segment)));

        return ReturnType(id.value);
    }

    ErrnoOr<u64> SharedMemory::attach(x64::Mmu* mmu, Id id, u64 preferredAddress, BitFlags<AtFlags> flags) {
        using ReturnType = ErrnoOr<u64>;

        verify(preferredAddress == 0, "Can only attach at kernel-chosen address");
        verify(!flags.test(AtFlags::REMAP), "REMAP is not supported");

        auto it = std::find_if(segments_.begin(), segments_.end(), [&](const auto& p) {
            return p.first.value == id.value;
        });
        if(it == segments_.end()) return ReturnType(-EINVAL);

        SharedMemorySegment* segment = it->second.get();
        auto ErrnoOraddr = segment->attach(mmu, flags.test(AtFlags::READ_ONLY), flags.test(AtFlags::EXEC));
        return ErrnoOraddr;
    }

    int SharedMemory::detach(x64::Mmu* mmu, u64 address) {
        auto it = std::find_if(segments_.begin(), segments_.end(), [&](const auto& p) {
            return p.second->attachedAddress() == address;
        });
        if(it == segments_.end()) return -EINVAL;

        SharedMemorySegment* segment = it->second.get();
        return segment->detach(mmu);
    }

    int SharedMemory::rmid(Id id) {
        auto it = std::find_if(segments_.begin(), segments_.end(), [&](const auto& p) {
            return p.first.value == id.value;
        });
        if(it == segments_.end()) return -EINVAL;

        SharedMemorySegment* segment = it->second.get();
        segment->rm();
        return 0;
    }
}