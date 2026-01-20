#include "x64/codesegment.h"
#include "verify.h"
#include <ostream>

#define JIT_THRESHOLD 1024

namespace x64 {

    CodeSegment::CodeSegment(BasicBlock cpuBasicBlock) : cpuBasicBlock_(std::move(cpuBasicBlock)) {
        verify(!cpuBasicBlock_.instructions().empty(), "Basic block is empty");
        endsWithFixedDestinationJump_ = cpuBasicBlock_.endsWithFixedDestinationJump();
        std::fill(fixedDestinationInfo_.next.begin(), fixedDestinationInfo_.next.end(), nullptr);
        std::fill(fixedDestinationInfo_.nextCount.begin(), fixedDestinationInfo_.nextCount.end(), 0);
        callsForCompilation_ = JIT_THRESHOLD;
    }

    u64 CodeSegment::start() const {
        return cpuBasicBlock_.instructions()[0].first.address();
    }

    u64 CodeSegment::end() const {
        return cpuBasicBlock_.instructions().back().first.nextAddress();
    }

    CodeSegment* CodeSegment::findNext(u64 address) {
        if(endsWithFixedDestinationJump_) {
            return fixedDestinationInfo_.findNext(address);
        } else {
            auto it = successors_.find(address);
            if(it != successors_.end()) {
                return it->second;
            } else {
                return nullptr;
            }
        }
    }

    CodeSegment* CodeSegment::FixedDestinationInfo::findNext(u64 address) {
        for(size_t i = 0; i < next.size(); ++i) {
            if(!next[i]) return nullptr;
            if(next[i]->start() != address) continue;
            CodeSegment* result = next[i];
            ++nextCount[i];
            if(i > 0 && nextCount[i] > nextCount[i-1]) {
                std::swap(next[i], next[i-1]);
                std::swap(nextCount[i], nextCount[i-1]);
            }
            return result;
        }
        return nullptr;
    }

    void CodeSegment::FixedDestinationInfo::addSuccessor(CodeSegment* other) {
        size_t firstAvailableSlot = next.size()-1;
        bool foundSlot = false;
        for(size_t i = 0; i < next.size(); ++i) {
            if(!next[i]) {
                firstAvailableSlot = i;
                foundSlot = true;
                break;
            }
        }
        verify(foundSlot);
        next[firstAvailableSlot] = other;
        nextCount[firstAvailableSlot] = 1;
    }

    void CodeSegment::VariableDestinationInfo::addSuccessor(CodeSegment* other) {
        next.push_back(other);
        nextJit.push_back(other->jitBasicBlock());
        nextStart.push_back(other->start());
        nextCount.push_back(1);
    }

    void CodeSegment::syncBlockLookupTable() {
        if(!jitBasicBlock_) return;
        for(size_t i = 0; i < variableDestinationInfo_.next.size(); ++i) {
            variableDestinationInfo_.nextJit[i] = variableDestinationInfo_.next[i]->jitBasicBlock();
        }
        jitBasicBlock_->syncBlockLookupTable(
                variableDestinationInfo_.nextJit.size(),
                variableDestinationInfo_.nextStart.data(),
                (const JitBasicBlock**)variableDestinationInfo_.nextJit.data(),
                variableDestinationInfo_.nextCount.data());
    }

    void CodeSegment::addSuccessor(CodeSegment* other) {
        if(endsWithFixedDestinationJump_) {
            fixedDestinationInfo_.addSuccessor(other);
        }
        auto res = successors_.insert(std::make_pair(other->start(), other));
        if(res.second && !endsWithFixedDestinationJump_) {
            variableDestinationInfo_.addSuccessor(other);
            syncBlockLookupTable();
        }
        other->predecessors_.insert(std::make_pair(start(), this));
    }

    void CodeSegment::removePredecessor(CodeSegment* other) {
        predecessors_.erase(other->start());
    }

    void CodeSegment::FixedDestinationInfo::removeSuccessor(CodeSegment* other) {
        for(size_t i = 0; i < next.size(); ++i) {
            const auto* bb1 = next[i];
            if(bb1 == other) {
                next[i] = nullptr;
                nextCount[i] = 0;
            }
        }
    }

    void CodeSegment::VariableDestinationInfo::removeSuccessor(CodeSegment*) {
        next.clear();
        nextJit.clear();
        nextStart.clear();
        nextCount.clear();
    }

    void CodeSegment::removeSucessor(CodeSegment* other) {
        if(endsWithFixedDestinationJump_) {
            fixedDestinationInfo_.removeSuccessor(other);
        } else {
            variableDestinationInfo_.removeSuccessor(other);
            syncBlockLookupTable();
        }
        successors_.erase(other->start());
    }

    void CodeSegment::removeFromCaches() {
        for(auto prev : predecessors_) prev.second->removeSucessor(this);
        predecessors_.clear();
        for(auto succ : successors_) succ.second->removePredecessor(this);
        successors_.clear();
        jitBasicBlock_ = nullptr;
    }

    size_t CodeSegment::size() const {
        return successors_.size() + predecessors_.size();
    }

    void CodeSegment::onCall(Jit* jit, CompilationQueue& compilationQueue) {
        if(!jit) return;
        compilationQueue.process(*jit, this);
    }

    void CodeSegment::onCpuCall() {
        ++calls_;
    }

    void CodeSegment::onJitCall() {
        // Nothing yet
    }

    void CodeSegment::tryCompile(Jit& jit, CompilationQueue& queue) {
        if(calls_ < callsForCompilation_) {
            callsForCompilation_ /= 2;
            return;
        }
        if(!compilationAttempted_) {
            jitBasicBlock_ = jit.tryCompile(cpuBasicBlock_, this);

            if(!!jitBasicBlock_) {
                if(jit.jitChainingEnabled()) {
                    tryPatch(jit);
                    for(auto prev : predecessors_) {
                        prev.second->tryPatch(jit);
                    }
                }
            }
            compilationAttempted_ = true;
            if(!!fixedDestinationInfo_.next[0]) queue.push(fixedDestinationInfo_.next[0]);
            if(!!fixedDestinationInfo_.next[1]) queue.push(fixedDestinationInfo_.next[1]);
            for(CodeSegment* next : variableDestinationInfo_.next) {
                queue.push(next);
            }
        }
    }

    void CodeSegment::tryPatch(Jit& jit) {
        if(!jitBasicBlock_) return;
        if(jitBasicBlock_->needsPatching()) {
            u64 continuingBlockAddress = end();

            auto tryPatch = [&](CodeSegment* next) {
                if(!next) return;
                if(!next->jitBasicBlock()) return;
                jitBasicBlock_->forAllPendingPatches(next->start() == continuingBlockAddress, [&](std::optional<size_t>* pendingPatch) {
                    jitBasicBlock_->tryPatch(pendingPatch, next->jitBasicBlock(), jit.compiler());
                });
            };
            tryPatch(fixedDestinationInfo_.next[0]);
            tryPatch(fixedDestinationInfo_.next[1]);
        }
        syncBlockLookupTable();
    }

    void CodeSegment::dumpGraphviz(std::ostream& stream, std::unordered_map<void*, u32>& counter) const {
        auto get_id = [&](const CodeSegment* seg) -> u32 {
            auto it = counter.find((void*)seg);
            if(it != counter.end()) {
                return it->second;
            } else {
                u32 id = (u32)counter.size();
                counter[(void*)seg] = id;
                return id;
            }
        };

        auto write_edge = [&](const CodeSegment* u, const CodeSegment* v) {
            stream << fmt::format("{}", get_id(u));
            stream << " -> ";
            stream << fmt::format("{}", get_id(v));
            stream << ';' << '\n';
        };

        for(const CodeSegment* succ : fixedDestinationInfo_.next) {
            if(!succ) continue;
            write_edge(this, succ);
        }

        for(const CodeSegment* succ : variableDestinationInfo_.next) {
            if(!succ) continue;
            write_edge(this, succ);
        }
    }

}