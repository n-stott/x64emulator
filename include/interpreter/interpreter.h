#ifndef INTERPRETER_H
#define INTERPRETER_H

#include "interpreter/vm.h"
#include "interpreter/loader.h"
#include "program.h"
#include <string>
#include <vector>

namespace x64 {

    class SymbolProvider;

    class Interpreter : public Loadable {
    public:
        Interpreter();
        void run(const std::string& programFilePath, const std::vector<std::string>& arguments, const std::vector<std::string>& environmentVariables);

        void crash() { vm_.crash(); }
        bool hasCrashed() const { return vm_.hasCrashed(); }

        void setLogInstructions(bool);
        bool logInstructions() const;

        void setAuxiliary(Auxiliary auxiliary) override;

        u64 mmap(u64 address, u64 length, PROT prot, int flags, int fd, int offset) override;
        int munmap(u64 address, u64 length) override;
        int mprotect(u64 address, u64 length, PROT prot) override;
        void setRegionName(u64 address, std::string name) override;

        void registerTlsBlock(u64 templateAddress, u64 blockAddress) override;
        void setFsBase(u64 fsBase) override;
        void registerInitFunction(u64 address) override;
        void registerFiniFunction(u64 address) override;
        void writeRelocation(u64 relocationSource, u64 relocationDestination) override;
        void writeUnresolvedRelocation(u64 relocationSource, const std::string& name) override;

        void read(u8* dst, u64 srcAddress, u64 nbytes) override;
        void write(u64 dstAddress, const u8* src, u64 nbytes) override;

    private:
        void setupStack();
        void pushProgramArguments(const std::string& programFilePath, const std::vector<std::string>& arguments, const std::vector<std::string>& environmentVariables);

        std::string calledFunctionName(const ExecutableSection* execSection, const CallDirect* insn);

        VM vm_;
        SymbolProvider* symbolProvider_;

        std::vector<u64> initFunctions_;
        std::optional<Loadable::Auxiliary> auxiliary_;

        mutable std::unordered_map<u64, std::string> functionNameCache_;
        std::unordered_map<u64, std::string> bogusRelocations_;

        friend class ExecutionContext;

    };

}

#endif