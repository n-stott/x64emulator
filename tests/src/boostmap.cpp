#include <boost/container/map.hpp>
#include <string>
#include <vector>

void testA() {
    boost::container::map<std::string, std::string> map;
    std::string key0 = "info0";
    std::string val0 = "oizeqrugqiouzeyrgvubniqeygrbvi_uqezygrvuq";
    map[key0] = val0;
    std::string key1 = "info1";
    std::string val1 = "oizeqrugqiouzeyrgvubnqsdqsdqiqeygrbvi_uqezygrvuq";
    map[key1] = val1;
    std::string key2 = "info2";
    std::string val2 = "oizeqrugqioqsdqsdqsduzeyrgvubniqeygrbvi_uqezygrvuq";
    map[key2] = val2;
    std::string key3 = "info3";
    std::string val3 = "oizeqrugqiouzeyrgvubniqeygrbvi_uqsdqsdqsdqqezygrvuq";
    map[key3] = val3;
}

void testB() {
    boost::container::map<std::string, std::string> map;
    std::string key0 = "0";
    std::string val0 = "a";
    map[key0] = val0;
    std::string key1 = "1";
    std::string val1 = "b";
    map[key1] = val1;
    std::string key2 = "2";
    std::string val2 = "cccccccccccccccccccccccccccccccccccccccccccccccccccccc";
    map[key2] = val2;
    std::string key3 = "3";
    std::string val3 = "d";
    map[key3] = val3;
}

void addValue(boost::container::map<int, std::string>& map, int key, const std::string& val) {
    map[key] = val;
}

void testC() {
    boost::container::map<int, std::string> map;
    int key = 2;
    std::string val("aaaaabbbbbcccccddd");
    addValue(map, key, val);
}


void testD() {
    boost::container::map<std::string, int> map;
    std::string key0 = "0";
    int val0 = 0;
    map[key0] = val0;
    std::string key1 = "1";
    int val1 = 0;
    map[key1] = val1;
}

int main() {
    testA();
    testB();
    testC();
    testD();
}