#include <iostream>
#include <fstream>

int main() {
    std::ifstream fin("spsa.txt");
    std::ofstream fout("temp.txt");
    std::string s;
    double x;
    while (fin >> s >> x) {
        fout << "TUNE_PARAM_DOUBLE(" << s << " " << x << ", -10.0, 10.0);\n";
    }

    return 0;
}