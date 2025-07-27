#ifndef SHAREDMEMORYSEGMENT_H
#define SHAREDMEMORYSEGMENT_H

#include "kernel/utils/erroror.h"
#include "utils.h"
#include <cstddef>
#include <memory>
#include <optional>

namespace x64 {
    class Mmu;
}

namespace kernel {

    class SharedMemorySegment {
    public:
        struct Key {
            int value;
        };
        static constexpr Key IPC_PRIVATE { 0 };

        static std::unique_ptr<SharedMemorySegment> tryCreate(x64::Mmu& mmu, int mode, size_t size);
        ~SharedMemorySegment();
        
        int id() const { return id_; }
        std::optional<u64> attachedAddress() const { return attachedAddress_; }
        
        ErrnoOr<u64> attach(bool readonly, bool executable);
        int detach();

        void rm();
        
    private:
        SharedMemorySegment(x64::Mmu& mmu, int id, int mode, size_t size);

        x64::Mmu& mmu_;
        int id_ { -1 };
        int mode_ { 0 };
        size_t size_ { 0 };
        std::optional<u64> attachedAddress_;
        size_t numAttach_ { 0 };
        bool markedForRemoval_ { false };

        // struct ipc_perm {
        //     key_t          __key;    /* Key supplied to shmget(2) */
        //     uid_t          uid;      /* Effective UID of owner */
        //     gid_t          gid;      /* Effective GID of owner */
        //     uid_t          cuid;     /* Effective UID of creator */
        //     gid_t          cgid;     /* Effective GID of creator */
        //     unsigned short mode;     /* Permissions + SHM_DEST and SHM_LOCKED flags */
        //     unsigned short __seq;    /* Sequence number */
        // };

        // struct shmid_ds {
        //     struct ipc_perm shm_perm;    /* Ownership and permissions */
        //     size_t          shm_segsz;   /* Size of segment (bytes) */
        //     time_t          shm_atime;   /* Last attach time */
        //     time_t          shm_dtime;   /* Last detach time */
        //     time_t          shm_ctime;   /* Creation time/time of last modification via shmctl() */
        //     pid_t           shm_cpid;    /* PID of creator */
        //     pid_t           shm_lpid;    /* PID of last shmat(2)/shmdt(2) */
        //     shmatt_t        shm_nattch;  /* No. of current attaches */
        // };

        
    };

}

#endif