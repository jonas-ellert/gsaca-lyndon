#include <chrono>
#include <fstream>
#include <iostream>
#include <string>
#include <sstream>

#include <unistd.h>
#include <sys/resource.h>
#include <stdio.h>

#include <parallel_divsufsort.h>


using namespace std;

typedef int32_t num_type; // Currently only int32_t and int64_t are supported.
int times = 3; // How often the time measurement is repeated.

size_t getPeakRSS() {
	struct rusage rusage;
	getrusage( RUSAGE_SELF, &rusage);
	return (size_t)(rusage.ru_maxrss * 1024L);
}

int main(int argc, char* args[]) {
	if (argc < 2 || argc > 3) {
		cout << "Usage: <input file> [prefix]" << endl;
		return -1;
	}
	uint64_t prefix = ~0ull;
	if (argc == 3) {
        uint64_t mult = 1;
        std::string pres = args[2];
        if (pres.size() == 0) return -1;
        if (pres.back() == 'B' || pres.back() == 'b') {
            pres.pop_back();
        }
        if (pres.back() == 'i') {
            pres.pop_back();
        }
        if (pres.back() == 'K' || pres.back() == 'k') {
            pres.pop_back();
            mult = 1024ull;
        } else if (pres.back() == 'M' || pres.back() == 'm') {
            pres.pop_back();
            mult = 1024ull * 1024ull;
        } else if (pres.back() == 'G' || pres.back() == 'g') {
            pres.pop_back();
            mult = 1024ull * 1024ull * 1024ull;
        }
        for (auto c : pres) {
            if ((c < '0') || (c > '9')) {
                std::cout << "integer parser error in '" << pres << "'" << std::endl;
                return 1;
            }
        }
        prefix = std::stoi(pres);
        prefix *= mult;
    }
	std::cout.precision(4);
	string text;
	{ // Read input file.
		ifstream input_file(args[1]);
		input_file.seekg(0, ios::end);
        uint64_t text_size = input_file.tellg();
		input_file.seekg(0, ios::beg);
        uint64_t actual_prefix = std::min(prefix, text_size);
		text.resize(actual_prefix);
        std::cout << "reading a prefix of " << text.size() << " bytes" << std::endl;
        input_file.read(text.data(), actual_prefix);
	}
	num_type size = text.size();
	num_type *SA = new num_type[size];
	for (int i = 0; i < times; ++i) {
		auto start = chrono::steady_clock::now();
		pdivsufsort((sauchar_t*)text.data(), SA, size);
		auto end = chrono::steady_clock::now();
		auto diff = end - start;
		cout <<	chrono::duration <double, milli> (diff).count() / 1000.0 << ", ";
		cout.flush();
	}
	cout << getPeakRSS() / (1024*1024)<< endl;
	if (sufcheck_labeit((sauchar_t*)text.data(), SA, size, false)) {
		cout << "Sufcheck failed!" << endl;
		return -1;
	}
	return 0;
}
