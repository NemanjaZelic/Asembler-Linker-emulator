#include <iostream>
#include <fstream>
#include <unordered_map>
#include <vector>
#include <string>
#include <iomanip>
#include <cstdint>
#include <sstream>

using namespace std;


string to_hex_string(uint32_t value) {
    stringstream ss;
    ss << hex << showbase << uppercase << value;
    return ss.str();
}


uint32_t reg_gpr[16];   
uint32_t csr_status = 0;
uint32_t csr_handler = 0;
uint32_t csr_cause = 0;
unordered_map<uint32_t, uint8_t> mem; 
bool running = true;


#define STATUS_IE 0x1 

uint32_t read32(uint32_t addr) {
    uint32_t val = 0;
    for (int i = 0; i < 4; i++) {
        val |= (uint32_t(mem[addr + i]) << (8 * i)); 
    }
    return val;
}

void write32(uint32_t addr, uint32_t value) {
    for (int i = 0; i < 4; i++)
        mem[addr + i] = (value >> (8 * i)) & 0xFF;
}

uint32_t readCSR(uint8_t idx) {
    switch (idx) {
        case 0x0: return csr_status;
        case 0x1: return csr_handler;
        case 0x2: return csr_cause;
        default:  return 0;
    }
}
void writeCSR(uint8_t idx, uint32_t v) {
    switch (idx) {
        case 0x0: csr_status = v; break;
        case 0x1: csr_handler = v; break;
        case 0x2: csr_cause = v; break;
        default:  break;
    }
}

void triggerInterrupt(uint32_t cause) {
    cerr << "[DEBUG] triggerInterrupt: csr_status=" << hex << csr_status << " STATUS_IE=" << STATUS_IE << dec << endl;
    if (!(csr_status & STATUS_IE)) {
        cerr << "[DEBUG] Interrupts disabled, returning" << endl;
        return;
    }
    
    csr_cause = cause;

    reg_gpr[14] -= 4;
    write32(reg_gpr[14], reg_gpr[15]);
    
    csr_status &= ~STATUS_IE; 
    
    reg_gpr[15] = csr_handler;
    cerr << "[DEBUG] Interrupt triggered: cause=" << cause << " jumping to " << hex << csr_handler << dec << endl;
}

uint32_t fetch() {
    uint32_t pc = reg_gpr[15];
    uint32_t inst = read32(pc);
    reg_gpr[15] += 4;
    return inst;
}

void execute(uint32_t inst) {
    uint8_t OC  = (inst >> 28) & 0xF;
    uint8_t MOD = (inst >> 24) & 0xF;
    uint8_t RA  = (inst >> 20) & 0xF;
    uint8_t RB  = (inst >> 16) & 0xF;
    uint8_t RC  = (inst >> 12) & 0xF;
    uint32_t raw_disp = inst & 0xFFF; 
    
    int32_t DISP; 
    if (raw_disp & 0x800) {
        DISP = (int32_t)(raw_disp | 0xFFFFF000); 
    } else {
        DISP = (int32_t)raw_disp;
    }
    
    // DEBUG: Print instruction being executed
    cerr << "[DEBUG] PC=" << hex << uppercase << setfill('0') << setw(8) << (reg_gpr[15]-4) 
         << " INST=" << setw(8) << inst 
         << " OC=" << (int)OC << " MOD=" << (int)MOD 
         << " RA=" << (int)RA << " RB=" << (int)RB << " RC=" << (int)RC 
         << " DISP=" << DISP << dec << endl;

    reg_gpr[0] = 0; 

    switch (OC) {
        case 0x0: 
            running = false;
            break;

        case 0x1: 
            triggerInterrupt(4); 
            break;

        case 0x2: 
            switch (MOD) {
                case 0x0:
                    cerr << "[DEBUG] CALL: pushing PC=" << hex << reg_gpr[15] 
                         << " jumping to " << (reg_gpr[RA] + reg_gpr[RB] + DISP) << dec << endl;
                    reg_gpr[14] -= 4;
                    write32(reg_gpr[14], reg_gpr[15]);
                    reg_gpr[15] = reg_gpr[RA] + reg_gpr[RB] + DISP;
                    break;
                case 0x1:
                    cerr << "[DEBUG] CALL (indirect): pushing PC=" << hex << reg_gpr[15] 
                         << " jumping to " << read32(reg_gpr[RA] + reg_gpr[RB] + DISP) << dec << endl;
                    reg_gpr[14] -= 4;
                    write32(reg_gpr[14], reg_gpr[15]);
                    reg_gpr[15] = read32(reg_gpr[RA] + reg_gpr[RB] + DISP);
                    break;
            }
            break;

        case 0x3: 
            switch (MOD) {
                case 0x0: reg_gpr[15] = reg_gpr[RA] + DISP; break;
                case 0x1: if (reg_gpr[RB] == reg_gpr[RC]) reg_gpr[15] = reg_gpr[RA] + DISP; break;
                case 0x2: if (reg_gpr[RB] != reg_gpr[RC]) reg_gpr[15] = reg_gpr[RA] + DISP; break;
            }
            break;

        case 0x4: 
                reg_gpr[15] = read32(reg_gpr[14]); 
                reg_gpr[14] += 4;
                csr_status |= STATUS_IE; 
            break;

        case 0x5:
            switch (MOD) {
                case 0x0: reg_gpr[RA] = reg_gpr[RB] + reg_gpr[RC]; break;
                case 0x1: reg_gpr[RA] = reg_gpr[RB] - reg_gpr[RC]; break;
                case 0x2: reg_gpr[RA] = reg_gpr[RB] * reg_gpr[RC]; break;
                case 0x3: if (reg_gpr[RC]!=0) reg_gpr[RA] = reg_gpr[RB] / reg_gpr[RC]; break;
            }
            break;

        case 0x6:
            switch (MOD) {
                case 0x0: reg_gpr[RA] = ~reg_gpr[RB]; break;
                case 0x1: reg_gpr[RA] = reg_gpr[RB] & reg_gpr[RC]; break;
                case 0x2: reg_gpr[RA] = reg_gpr[RB] | reg_gpr[RC]; break;
                case 0x3: reg_gpr[RA] = reg_gpr[RB] ^ reg_gpr[RC]; break;
            }
            break;

        case 0x7: 
            switch (MOD) {
                case 0x0: reg_gpr[RA] = reg_gpr[RB] << reg_gpr[RC]; break;
                case 0x1: reg_gpr[RA] = reg_gpr[RB] >> reg_gpr[RC]; break;
            }
            break;

             case 0x8:
            switch (MOD) {
                case 0x0: 
                    cerr << "[DEBUG] STORE: [" << hex << (reg_gpr[RA] + reg_gpr[RB] + DISP) 
                         << "] = r" << dec << (int)RC << "=" << hex << reg_gpr[RC] << dec << endl;
                    write32(reg_gpr[RA] + reg_gpr[RB] + DISP, reg_gpr[RC]); 
                    break;
                case 0x1: 
                    reg_gpr[RA] += DISP; 
                    cerr << "[DEBUG] STORE (post-inc): [" << hex << reg_gpr[RA] 
                         << "] = r" << dec << (int)RC << "=" << hex << reg_gpr[RC] << dec << endl;
                    write32(reg_gpr[RA], reg_gpr[RC]); 
                    break;
                case 0x2: {
                    uint32_t addr = read32(reg_gpr[RA] + reg_gpr[RB] + DISP);
                    cerr << "[DEBUG] STORE (indirect): [" << hex << addr 
                         << "] = r" << dec << (int)RC << "=" << hex << reg_gpr[RC] << dec << endl;
                    write32(addr, reg_gpr[RC]);
                    break;
                }
            }
            break;

              case 0x9:
            switch (MOD) {
                case 0x0: 
                    reg_gpr[RA] = read32(reg_gpr[RB] + DISP);
                    cerr << "[DEBUG] LOAD: r" << dec << (int)RA << " = [" << hex 
                         << (reg_gpr[RB] + DISP) << "] = " << reg_gpr[RA] << dec << endl;
                    break;
                case 0x1:
                    reg_gpr[RA] = reg_gpr[RB] + DISP;
                    cerr << "[DEBUG] LOAD (immed): r" << dec << (int)RA << " = r" << (int)RB 
                         << "+" << DISP << " = " << hex << reg_gpr[RA] << dec << endl;
                    break;
                case 0x2:
                    reg_gpr[RA] = read32(reg_gpr[RB] + reg_gpr[RC] + DISP);
                    cerr << "[DEBUG] LOAD: r" << dec << (int)RA << " = [" << hex 
                         << (reg_gpr[RB] + reg_gpr[RC] + DISP) << "] = " << reg_gpr[RA] << dec << endl;
                    break;
                case 0x3: 
                    reg_gpr[RA] = read32(reg_gpr[RB]);
                    reg_gpr[RB] += DISP;
                    cerr << "[DEBUG] LOAD (post-inc): r" << dec << (int)RA << " = [" << hex 
                         << (reg_gpr[RB]-DISP) << "] = " << reg_gpr[RA] << ", r" << dec << (int)RB 
                         << " += " << DISP << " = " << hex << reg_gpr[RB] << dec << endl;
                    break;
                
                case 0x4: 
                    reg_gpr[RA] = readCSR(RB);
                    cerr << "[DEBUG] LOAD CSR: r" << dec << (int)RA << " = CSR[" << (int)RB 
                         << "] = " << hex << reg_gpr[RA] << dec << endl;
                    break;
                case 0x5: 
                    writeCSR(RB, reg_gpr[RA]);
                    break;
                case 0x6: 
                    reg_gpr[RA] = readCSR(RB);
                    break;
                case 0x7: 
                    writeCSR(RB, reg_gpr[RA]);
                    break;

                default:
                    cerr << "Unsupported LOAD/CSR MOD=" << dec << (int)MOD
                         << " at PC=0x" << hex << (reg_gpr[15] - 4) << endl;
                    running = false;
                    break;
            }
            break;

        default:
            cerr << "Unknown instruction at 0x" << hex << reg_gpr[15] - 4 << endl;
            running = false;
            break;
    }

    reg_gpr[0] = 0; 
}

uint8_t parseByte(const string &s) {
    uint8_t val = 0;
    for (char c : s) {
        val <<= 4;
        if (c >= '0' && c <= '9') val += c - '0';
        else if (c >= 'A' && c <= 'F') val += 10 + (c - 'A');
        else if (c >= 'a' && c <= 'f') val += 10 + (c - 'a');
    }
    return val;
}

void loadHex(const string &file) {
    ifstream in(file);
    if (!in.is_open()) {
        cerr << "Cannot open " << file << endl;
        exit(1);
    }

    string line;
    while (getline(in, line)) {
        if (line.empty()) continue;

        size_t colon = line.find(':');
        if (colon == string::npos) continue;

        string addrStr = line.substr(0, colon);
        uint32_t addr = stoul(addrStr, nullptr, 16);

        string rest = line.substr(colon + 1);
        string byteStr;
        stringstream ss(rest);
        while (ss >> byteStr) {
            uint8_t val = parseByte(byteStr);
            mem[addr++] = val;
        }
    }
}

int main(int argc, char* argv[]) {
    if (argc != 2) {
        cerr << "Usage: " << argv[0] << " <input.hex>\n";
        return 1;
    }

    loadHex(argv[1]);

    uint32_t init_table_addr = 0xF0000118;
    while (true) {
        uint32_t value = read32(init_table_addr);
        uint32_t addr = read32(init_table_addr + 4);
        
        if (addr == 0) break; 
        
        write32(addr, value);
        
        init_table_addr += 8;
    }


    for (int i = 0; i < 16; i++) reg_gpr[i] = 0;
    reg_gpr[15] = 0x40000000; 
    reg_gpr[14] = 0xFFFFFF00; 
    csr_handler = 0xF0000000; 
    csr_status = STATUS_IE;
    csr_cause = 0;

    cout << "Starting emulator...\n";

    while (running) {
        uint32_t inst = fetch();
        execute(inst);


    }

    cout << "-----------------------------------------------------------------\n";
    cout << "Emulated processor executed halt instruction\n";
    cout << "Emulated processor state:\n";
    for (int i = 0; i < 16; i++) {
        cout << "r" << dec << i << "=0x" << hex << setw(8) << setfill('0') << reg_gpr[i];
        if ((i + 1) % 4 == 0) cout << "\n"; else cout << " ";
    }

    cout << "\nCSR_STATUS=" << to_hex_string(csr_status)
         << " CSR_HANDLER=" << to_hex_string(csr_handler)
         << " CSR_CAUSE=" << to_hex_string(csr_cause) << endl;

    return 0;
}
