#include "arch/arm64/codesegment.h"
#include "verify.h"
#include <ostream>

namespace arm64 {

    CodeSegment::CodeSegment(BasicBlock cpuBasicBlock) : cpuBasicBlock_(std::move(cpuBasicBlock)) {
        verify(!cpuBasicBlock_.instructions().empty(), "Basic block is empty");
        endsWithFixedDestinationJump_ = cpuBasicBlock_.endsWithFixedDestinationJump()
                || cpuBasicBlock_.endsWithDirectCall();
        std::fill(fixedDestinationInfo_.next.begin(), fixedDestinationInfo_.next.end(), nullptr);
        std::fill(fixedDestinationInfo_.nextCount.begin(), fixedDestinationInfo_.nextCount.end(), 0);
    }

    CodeSegment::~CodeSegment() = default;

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
            return successors_.find(address);
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

    void CodeSegment::FixedDestinationInfo::addSuccessor(CodeSegment* succ) {
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
        next[firstAvailableSlot] = succ;
        nextCount[firstAvailableSlot] = 1;
    }

    void CodeSegment::VariableDestinationInfo::addSuccessor(CodeSegment* succ) {
        next.push_back(succ);
        nextStart.push_back(succ->start());
        nextCount.push_back(1);
    }

    void CodeSegment::addSuccessor(CodeSegment* succ) {
        if(endsWithFixedDestinationJump_) {
            fixedDestinationInfo_.addSuccessor(succ);
        }
        auto inserted = successors_.insert(succ->start(), succ);
        if(inserted && !endsWithFixedDestinationJump_) {
            variableDestinationInfo_.addSuccessor(succ);
        }
        succ->predecessors_.insert(std::make_pair(start(), this));
    }

    void CodeSegment::addReturn(CodeSegment* ret) {
        returnDestinationInfo_.addReturn(ret);
        verify(ret->start() == end());
        ret->callPredecessors_.insert(start(), this);
    }

    void CodeSegment::ReturnDestinationInfo::addReturn(CodeSegment* retseg) {
        verify(!ret || ret == retseg);
        ret = retseg;
    }

    void CodeSegment::removePredecessor(CodeSegment* other) {
        predecessors_.erase(other->start());
    }

    void CodeSegment::removeCallPredecessor(CodeSegment* other) {
        callPredecessors_.erase(other->start());
    }

    void CodeSegment::FixedDestinationInfo::removeSuccessor(CodeSegment* succ) {
        for(size_t i = 0; i < next.size(); ++i) {
            const auto* bb1 = next[i];
            if(bb1 == succ) {
                next[i] = nullptr;
                nextCount[i] = 0;
            }
        }
    }

    void CodeSegment::VariableDestinationInfo::removeSuccessor(CodeSegment*) {
        next.clear();
        nextStart.clear();
        nextCount.clear();
    }

    void CodeSegment::removeSucessor(CodeSegment* succ) {
        if(endsWithFixedDestinationJump_) {
            fixedDestinationInfo_.removeSuccessor(succ);
        } else {
            variableDestinationInfo_.removeSuccessor(succ);
        }
        successors_.erase(succ->start());
    }

    void CodeSegment::removeReturn(CodeSegment* ret) {
        verify(returnDestinationInfo_.ret == ret);
        returnDestinationInfo_.ret = nullptr;
    }

    void CodeSegment::removeFromCaches() {
        for(auto prev : predecessors_) prev.second->removeSucessor(this);
        predecessors_.clear();
        successors_.forEach([&](u64, CodeSegment* prev) {
            prev->removePredecessor(this);
        });
        successors_.clear();
        callPredecessors_.forEach([&](u64, CodeSegment* prev) {
            prev->removeReturn(this);
        });
        callPredecessors_.clear();
    }

    size_t CodeSegment::size() const {
        return successors_.size() + predecessors_.size() + callPredecessors_.size();
    }

    void CodeSegment::onCpuCall() {
        ++calls_;
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