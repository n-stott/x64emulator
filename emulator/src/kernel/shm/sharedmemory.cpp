#include "kernel/shm/sharedmemory.h"
#include "kernel/shm/sharedmemorysegment.h"
#include "x64/mmu.h"
#include "verify.h"
#include <algorithm>
#include <errno.h>

namespace kernel {

    SharedMemory::SharedMemory(x64::Mmu& mmu) : mmu_(mmu) { }

    SharedMemory::~SharedMemory() = default;

    ErrnoOr<SharedMemory::Id> SharedMemory::get(Key key, size_t size, int mode, BitFlags<GetFlags> flags) {
        using ReturnType = ErrnoOr<SharedMemory::Id>;

        verify(key.value == IPC_PRIVATE.value, "Only IPC_PRIVATE is supported");
        verify(flags.test(GetFlags::CREATE), "Only create is supported");
        verify(!flags.test(GetFlags::EXCL), "Excl is not supported");
        
        size = x64::Mmu::pageRoundUp(size);

        auto segment = SharedMemorySegment::tryCreate(mmu_, mode, size);
        Id id { segment->id() };

        segments_.push_back(std::make_pair(id, std::move(segment)));

        return ReturnType(id.value);
    }

    ErrnoOr<u64> SharedMemory::attach(Id id, u64 preferredAddress, BitFlags<AtFlags> flags) {
        using ReturnType = ErrnoOr<u64>;

        verify(preferredAddress == 0, "Can only attach at kernel-chosen address");
        verify(!flags.test(AtFlags::REMAP), "REMAP is not supported");

        auto it = std::find_if(segments_.begin(), segments_.end(), [&](const auto& p) {
            return p.first.value == id.value;
        });
        if(it == segments_.end()) return ReturnType(-EINVAL);

        SharedMemorySegment* segment = it->second.get();
        auto ErrnoOraddr = segment->attach(flags.test(AtFlags::READ_ONLY), flags.test(AtFlags::EXEC));
        return ErrnoOraddr;
    }

    int SharedMemory::detach(u64 address) {
        auto it = std::find_if(segments_.begin(), segments_.end(), [&](const auto& p) {
            return p.second->attachedAddress() == address;
        });
        if(it == segments_.end()) return -EINVAL;

        SharedMemorySegment* segment = it->second.get();
        return segment->detach();
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