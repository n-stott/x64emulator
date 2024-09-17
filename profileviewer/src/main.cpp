#include "profilingdata.h"
#include <fmt/core.h>
#include <fstream>

int main() {
    using namespace profiling;
    
    std::ifstream inputFile("output.json");
    auto profileData = ProfilingData::tryCreateFromJson(inputFile);
    if(!profileData) {
        fmt::print(stderr, "Unable to read {}\n", "output.json");
        return 1;
    } else {
        fmt::print("Read {} : has data from {} thread(s)\n", "output.json", profileData->nbThreads());
    }
    

    return 0;
}