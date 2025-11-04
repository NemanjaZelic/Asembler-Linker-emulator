#include <iostream>
#include <fstream>
#include <sstream>
#include <string>
#include <vector>
#include <map>
#include <set>
#include <iomanip>
#include <algorithm>
using namespace std;

struct Section {
    string name;
    map<unsigned int, vector<int>> data; 
};

enum class SymbolType { GLOBAL, LOCAL, EXTERN };

struct Symbol {
    string name;
    string section;
    unsigned int offset;
    SymbolType type;
};

struct Relocation {
    string symbolName;
    string section;
    unsigned int offset;
};

enum class ParseState { NONE, SECTION, SYMBOLS, RELOCS };

unsigned int parseIntAuto(const string &s) {
    if (s.size() > 2 && (s[0]=='0') && (s[1]=='x' || s[1]=='X'))
        return stoul(s.substr(2), nullptr, 16);
    for (char c : s) {
        if ((c >= 'A' && c <= 'F') || (c >= 'a' && c <= 'f')) {
            return stoul(s, nullptr, 16);
        }
    }
    return stoul(s, nullptr, 10);
}

void parseFileLocal(const string &fname,
               map<string, Section> &sections,
               vector<Symbol> &symbols,
               vector<Relocation> &relocs)
{
    ifstream in(fname);
    if (!in.is_open()) {
        cerr << "ERROR " << fname << endl;
        exit(1);
    }

    string line;
    string currentSection;
    ParseState state = ParseState::NONE;

    while (getline(in, line)) {
        if (line.empty()) continue;
        size_t start = line.find_first_not_of(" \t\r\n");
        if (start == string::npos) continue;
        if (start > 0) line = line.substr(start);

        if (line.rfind("SECTION:", 0) == 0) {
            istringstream iss(line);
            string tmp;
            iss >> tmp >> currentSection;
            if (!sections.count(currentSection))
                sections[currentSection] = Section{currentSection, {}};
            state = ParseState::SECTION;
        }
        else if (line.rfind("SYMBOLS:", 0) == 0) {
            currentSection.clear();
            state = ParseState::SYMBOLS;
        }
        else if (line.rfind("REALOCS:", 0) == 0 || line.rfind("RELOCS:", 0) == 0) {
            currentSection.clear();
            state = ParseState::RELOCS;
        }
        else if (state == ParseState::SECTION) {
            size_t colonPos = line.find(':');
            if (colonPos == string::npos) continue;
            string addrStr = line.substr(0, colonPos);
            size_t astart = addrStr.find_first_not_of(" \t");
            size_t aend = addrStr.find_last_not_of(" \t");
            if (astart==string::npos) continue;
            addrStr = addrStr.substr(astart, aend - astart + 1);

            unsigned int addr = parseIntAuto(addrStr);

            istringstream iss(line.substr(colonPos+1));
            string byteStr;
            vector<int> bytes;
            while (iss >> byteStr) {
                int b;
                if (byteStr.size() > 2 && (byteStr[0]=='0' && (byteStr[1]=='x' || byteStr[1]=='X')))
                    b = stoi(byteStr.substr(2), nullptr, 16);
                else {
                  
                    b = stoi(byteStr, nullptr, 16);
                }
                bytes.push_back(b & 0xFF);
            }

            sections[currentSection].data[addr] = bytes;
        }
        else if (state == ParseState::SYMBOLS) {
            istringstream iss(line);
            Symbol sym;
            string typeStr;
            unsigned int off;
            if (!(iss >> sym.name >> sym.section >> off >> typeStr)) {
                cerr << "WARNING: malformed symbol line: " << line << endl;
                continue;
            }
            sym.offset = off;
            if (typeStr == "global") sym.type = SymbolType::GLOBAL;
            else if (typeStr == "local") sym.type = SymbolType::LOCAL;
            else sym.type = SymbolType::EXTERN;
            symbols.push_back(sym);
        }
        else if (state == ParseState::RELOCS) {
            istringstream iss(line);
            Relocation r;
            unsigned int off;
            if (!(iss >> r.symbolName >> r.section >> off)) {
                cerr << "WARNING: malformed reloc line: " << line << endl;
                continue;
            }
            r.offset = off;
            relocs.push_back(r);
        }
    }
}

void generateHex(const string &outFile,
                 map<string, Section> &sections,
                 map<string, unsigned int> &sectionBase,
                 const vector<Symbol> &symbols,
                 const vector<Relocation> &relocs)
{
    ofstream out(outFile);
    if (!out.is_open()) {
        cerr << "ERROR " << outFile << endl;
        exit(1);
    }

    map<unsigned long long, int> memory; 
    for (auto &kv : sections) {
        unsigned long long base = 0;
        if (sectionBase.count(kv.first)) base = sectionBase[kv.first];
        for (auto &line : kv.second.data) {
            unsigned long long addr = line.first;
            size_t numBytes = line.second.size();
            for (size_t i = 0; i < numBytes; i += 4) {
                if (i + 3 < numBytes) {
                    memory[base + addr + i + 0] = line.second[i + 3];
                    memory[base + addr + i + 1] = line.second[i + 2];
                    memory[base + addr + i + 2] = line.second[i + 1];
                    memory[base + addr + i + 3] = line.second[i + 0];
                } else {
                    for (size_t j = i; j < numBytes; j++) {
                        memory[base + addr + j] = line.second[j];
                    }
                    break;
                }
            }
        }
    }

    map<string, unsigned long long> symAddr;
    for (auto &sym : symbols) {
        if (sym.section == "UND") continue; 
        if (!sectionBase.count(sym.section)) {
            cerr << "ERROR " << sym.name << " in " << sym.section << endl;
            exit(1);
        }
        symAddr[sym.name] = (unsigned long long)sectionBase[sym.section] + sym.offset;
    }

    for (auto &r : relocs) {
        unsigned long long loc = (unsigned long long)sectionBase[r.section] + r.offset;
        if (!symAddr.count(r.symbolName)) {
            cerr << "ERROR " << r.symbolName << endl;
            exit(1);
        }
        unsigned long long target = symAddr[r.symbolName];

        memory[loc]   = (int)(target & 0xFF);
        memory[loc+1] = (int)((target >> 8) & 0xFF);
        memory[loc+2] = (int)((target >> 16) & 0xFF);
        memory[loc+3] = (int)((target >> 24) & 0xFF);
    }

    unsigned long long prevAddr = (unsigned long long)-1;
    int count = 0;
    for (auto &kv : memory) {
        unsigned long long addr = kv.first;
        if (prevAddr == (unsigned long long)-1 || addr != prevAddr + 1 || count % 8 == 0) {
            if (count != 0) out << "\n";
            out << hex << setw(8) << setfill('0') << (unsigned int)addr << ": ";
            count = 0;
        }
        out << setw(2) << setfill('0') << (kv.second & 0xFF) << " ";
        prevAddr = addr;
        count++;
    }
    out << dec << "\n";
    out.close();
}

int main(int argc, char* argv[]) {
    if (argc < 2) {
        cerr << "Usage: " << argv[0] << " [options] file1.o file2.o ..." << endl;
        return 1;
    }

    string outputFile = "a.out";
    bool hexOutput = false;
    bool relocatableOutput = false;
    map<string, unsigned int> placeMap;
    vector<string> inputFiles;

    for (int i = 1; i < argc; i++) {
        string arg = argv[i];
        if (arg == "-hex") {
            hexOutput = true;
        } else if (arg == "-relocatable") {
            relocatableOutput = true;
        } else if (arg == "-o" && i + 1 < argc) {
            outputFile = argv[++i];
        } else if (arg.rfind("-place=", 0) == 0) {
            string param = arg.substr(7);
            size_t atPos = param.find('@');
            if (atPos == string::npos) {
                cerr << "ERROR" << param << endl;
                return 1;
            }
            string secName = param.substr(0, atPos);
            unsigned int addr = parseIntAuto(param.substr(atPos+1));
            placeMap[secName] = addr;
        } else {
            inputFiles.push_back(arg);
        }
    }

    if (hexOutput == relocatableOutput) {
        cerr << "ERROR: must specify exactly one of -hex or -relocatable" << endl;
        return 1;
    }

        if (inputFiles.empty()) {
            cerr << "ERROR: No input files provided." << endl;
            return 1;
    }

    map<string, Section> allSections;       
    vector<Symbol> allSymbols;
    vector<Relocation> allRelocs;
    map<string, unsigned int> sectionSizes; 
    vector<string> sectionOrder;           
    set<string> seenSections;

    map<string, string> globalSymbolDefinedIn;

    for (auto &fname : inputFiles) {
        map<string, Section> localSections;
        vector<Symbol> localSymbols;
        vector<Relocation> localRelocs;

        parseFileLocal(fname, localSections, localSymbols, localRelocs);

        map<string, unsigned int> localSize;
        for (auto &kv : localSections) {
            unsigned int maxOffset = 0;
            for (auto &line : kv.second.data) {
                unsigned int end = line.first + (unsigned int)line.second.size();
                if (end > maxOffset) maxOffset = end;
            }
            localSize[kv.first] = maxOffset;
        }

        for (auto &kv : localSections) {
            string secName = kv.first;
            unsigned int base = 0;
            if (sectionSizes.count(secName)) base = sectionSizes[secName];

            if (!allSections.count(secName)) {
                allSections[secName] = Section{secName, {}};
                if (!seenSections.count(secName)) {
                    sectionOrder.push_back(secName);
                    seenSections.insert(secName);
                }
            }

            for (auto &line : kv.second.data) {
                unsigned int newAddr = base + line.first;
                allSections[secName].data[newAddr] = line.second;
            }

            sectionSizes[secName] = base + localSize[secName];
        }

        for (auto &sym : localSymbols) {
            Symbol s = sym;
            unsigned int base = 0;
            if (sectionSizes.count(s.section)) {
                base = sectionSizes[s.section] - localSize[s.section];
            } else base = 0;
            s.offset = base + s.offset;

            if (s.type == SymbolType::GLOBAL) {
                if (globalSymbolDefinedIn.count(s.name)) {
                    cerr << "ERROR" << s.name
                         << " (in " << fname << " and " << globalSymbolDefinedIn[s.name] << ")" << endl;
                    return 1;
                } else {
                    globalSymbolDefinedIn[s.name] = fname;
                }
            }
            allSymbols.push_back(s);
        }

        for (auto &r : localRelocs) {
            Relocation rr = r;
            unsigned int base = 0;
            if (sectionSizes.count(rr.section)) {
                base = sectionSizes[rr.section] - localSize[rr.section];
            } else base = 0;
            rr.offset = base + rr.offset;
            allRelocs.push_back(rr);
        }
    }

    map<string, unsigned int> sectionBase;

    if (relocatableOutput) {
        unsigned long long cur = 0;
        for (auto &secName : sectionOrder) {
            sectionBase[secName] = (unsigned int)cur;
            unsigned int sz = sectionSizes.count(secName) ? sectionSizes[secName] : 0;
            cur += sz;
        }
    } else {
        bool anyPlaced = false;
        unsigned long long highestEnd = 0;
        for (auto &p : placeMap) {
            const string &sec = p.first;
            unsigned int base = p.second;
            if (!allSections.count(sec)) continue;
            sectionBase[sec] = base;
            unsigned int sz = sectionSizes.count(sec) ? sectionSizes[sec] : 0;
            unsigned long long end = (unsigned long long)base + sz;
            if (!anyPlaced || end > highestEnd) {
                anyPlaced = true;
                highestEnd = end;
            }
        }
        unsigned long long cur = anyPlaced ? highestEnd : 0;


        for (auto &secName : sectionOrder) {
            if (sectionBase.count(secName)) continue;
            sectionBase[secName] = (unsigned int)cur;
            unsigned int sz = sectionSizes.count(secName) ? sectionSizes[secName] : 0;
            cur += (unsigned long long)sz;
        }

        vector<pair<unsigned long long, unsigned long long>> ranges;
        for (auto &secName : sectionOrder) {
            unsigned long long base = (unsigned long long)sectionBase[secName];
            unsigned long long sz = (unsigned long long)(sectionSizes.count(secName) ? sectionSizes[secName] : 0);
            if (sz == 0) continue;
            ranges.push_back({base, base + sz});
        }
        sort(ranges.begin(), ranges.end());
        for (size_t i = 1; i < ranges.size(); ++i) {
            if (ranges[i].first < ranges[i-1].second) {
                cerr << "ERROR "
                     << "0x" << hex << ranges[i-1].first << "-" << ranges[i-1].second
                     << " and 0x" << ranges[i].first << "-" << ranges[i].second << dec << endl;
                return 1;
            }
        }
    }


    map<string, unsigned long long> symbolAddr;
    for (auto &sym : allSymbols) {
        if (sym.type == SymbolType::EXTERN) continue;
        if (sym.section == "UND") continue; 
        if (!sectionBase.count(sym.section)) {
            cerr << "ERROR: symbol " << sym.name << " refers to unknown section " << sym.section << endl;
            return 1;
        }
        symbolAddr[sym.name] = (unsigned long long)sectionBase[sym.section] + sym.offset;
    }
    for (auto &r : allRelocs) {
        if (!symbolAddr.count(r.symbolName)) {
            cerr << "ERROR" << r.symbolName << endl;
            return 1;
        }
    }

    if (hexOutput) {
        generateHex(outputFile, allSections, sectionBase, allSymbols, allRelocs);
    } else {
        ofstream out(outputFile);
        if (!out.is_open()) {
            cerr << "ERROR " << outputFile << endl;
            exit(1);
        }
        for (auto &secName : sectionOrder) {
            auto &kv = allSections[secName];
            out << "SECTION: " << kv.name << "\n";
            for (auto &line : kv.data) {
                out << line.first << ": ";
                for (auto b : line.second) {
                    out << hex << setw(2) << setfill('0') << (b & 0xFF) << " ";
                }
                out << dec << "\n";
            }
            out << "\n";
        }
        out << "SYMBOLS:\n";
        for (auto &sym : allSymbols) {
            string typeStr = (sym.type == SymbolType::GLOBAL ? "global" :
                             sym.type == SymbolType::LOCAL ? "local" : "extern");
            out << sym.name << " " << sym.section << " "
                << sym.offset << " " << typeStr << "\n";
        }
        out << "\nREALOCS:\n";
        for (auto &r : allRelocs) {
            out << r.symbolName << " " << r.section << " " << r.offset << "\n";
        }
        out.close();
    }

    return 0;
}
