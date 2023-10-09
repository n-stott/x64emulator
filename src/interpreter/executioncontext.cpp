#include "interpreter/executioncontext.h"
#include "interpreter/interpreter.h"

namespace x64 {

    Mmu* ExecutionContext::mmu() const {
        return &interpreter_->mmu_;
    }

    std::string ExecutionContext::readString(u64 address) const {
        std::string buffer;
        Ptr8 ptr { Segment::DS, address };
        while(true) {
            char c = mmu()->read8(ptr);
            if(c == '\0') break;
            buffer.push_back(c);
            ++ptr;
        }
        return buffer;
    }

    u64 ExecutionContext::rdi() const {
        return interpreter_->cpu_.regs_.rdi_;
    }

    u64 ExecutionContext::rsi() const {
        return interpreter_->cpu_.regs_.rsi_;
    }

    u64 ExecutionContext::rdx() const {
        return interpreter_->cpu_.regs_.rdx_;
    }

    u64 ExecutionContext::rcx() const {
        return interpreter_->cpu_.regs_.rcx_;
    }

    u64 ExecutionContext::rax() const {
        return interpreter_->cpu_.regs_.rax_;
    }

    u64 ExecutionContext::rbx() const {
        return interpreter_->cpu_.regs_.rbx_;
    }

    void ExecutionContext::set_rax(u64 val) const {
        interpreter_->cpu_.regs_.rax_ = val;
    }

    void ExecutionContext::set_rbx(u64 val) const {
        interpreter_->cpu_.regs_.rbx_ = val;
    }

    void ExecutionContext::stop() const {
        interpreter_->stop();
    }
}