#include <iostream>
#include <cstdint> // uint8_t, uint16_t, etc.
#include <chrono>  // std::chrono::high_resolution_clock, duration
#include <thread>  // std::this_thread::sleep_for
#include <random>  // for random_device, mt19937, uniform_int_distribution
#include "../include/memory.h"
#include "../include/flags.h"
#include "../include/speed.h"

// Base cycle counts for NMOS 6502 opcodes (0x00–0xFF)
// Includes official + stable undocumented opcodes
// Page-cross penalties still applied separately
constexpr uint8_t cycle_table[256] = {
    // 0x00
    7, 6, 2, 2, 3, 3, 5, 5, 3, 2, 2, 2, 4, 4, 6, 6, // 00–0F
    2, 5, 2, 2, 4, 4, 6, 6, 2, 4, 2, 2, 4, 4, 7, 7, // 10–1F
    // 0x20
    6, 6, 2, 2, 3, 3, 5, 5, 4, 2, 2, 2, 4, 4, 6, 6, // 20–2F
    2, 5, 2, 2, 4, 4, 6, 6, 2, 4, 2, 2, 4, 4, 7, 7, // 30–3F
    // 0x40
    6, 6, 2, 2, 3, 3, 5, 5, 3, 2, 2, 2, 3, 4, 6, 6, // 40–4F
    2, 5, 2, 2, 4, 4, 6, 6, 2, 4, 2, 2, 4, 4, 7, 7, // 50–5F
    // 0x60
    6, 6, 2, 2, 3, 3, 5, 5, 4, 2, 2, 2, 5, 4, 6, 6, // 60–6F
    2, 5, 2, 2, 4, 4, 6, 6, 2, 4, 2, 2, 4, 4, 7, 7, // 70–7F
    // 0x80
    2, 6, 2, 2, 3, 3, 5, 5, 2, 2, 2, 2, 4, 4, 6, 6, // 80–8F
    2, 5, 2, 2, 4, 4, 6, 6, 2, 4, 2, 2, 4, 4, 7, 7, // 90–9F
    // 0xA0
    2, 6, 2, 2, 3, 3, 5, 5, 2, 2, 2, 2, 4, 4, 6, 6, // A0–AF
    2, 5, 2, 2, 4, 4, 6, 6, 2, 4, 2, 2, 4, 4, 7, 7, // B0–BF
    // 0xC0
    2, 6, 2, 2, 3, 3, 5, 5, 2, 2, 2, 2, 4, 4, 6, 6, // C0–CF
    2, 5, 2, 2, 4, 4, 6, 6, 2, 4, 2, 2, 4, 4, 7, 7, // D0–DF
    // 0xE0
    2, 6, 2, 2, 3, 3, 5, 5, 2, 2, 2, 2, 4, 4, 6, 6, // E0–EF
    2, 5, 2, 2, 4, 4, 6, 6, 2, 4, 2, 2, 4, 4, 7, 7  // F0–FF
};

// 1 = opcode can incur a +1 cycle page-cross penalty
// 0 = no page-cross penalty possible
constexpr uint8_t page_cross_table[256] = {
    // 0x00
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 0, 0, // 00–0F
    1, 0, 0, 0, 0, 0, 0, 0, 0, 1, 0, 0, 0, 1, 0, 0, // 10–1F
    // 0x20
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 0, 0, // 20–2F
    1, 0, 0, 0, 0, 0, 0, 0, 0, 1, 0, 0, 0, 1, 0, 0, // 30–3F
    // 0x40
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 0, 0, // 40–4F
    1, 0, 0, 0, 0, 0, 0, 0, 0, 1, 0, 0, 0, 1, 0, 0, // 50–5F
    // 0x60
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 0, 0, // 60–6F
    1, 0, 0, 0, 0, 0, 0, 0, 0, 1, 0, 0, 0, 1, 0, 0, // 70–7F
    // 0x80
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 0, 0, // 80–8F
    1, 0, 0, 0, 0, 0, 0, 0, 0, 1, 0, 0, 0, 1, 0, 0, // 90–9F
    // 0xA0
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 0, 0, // A0–AF
    1, 0, 0, 0, 0, 0, 0, 0, 0, 1, 0, 0, 0, 1, 0, 0, // B0–BF
    // 0xC0
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 0, 0, // C0–CF
    1, 0, 0, 0, 0, 0, 0, 0, 0, 1, 0, 0, 0, 1, 0, 0, // D0–DF
    // 0xE0
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 0, 0, // E0–EF
    1, 0, 0, 0, 0, 0, 0, 0, 0, 1, 0, 0, 0, 1, 0, 0  // F0–FF
};

const double SECONDS_PER_CYCLE = 1.0 / CPU_FREQ;

struct CPU6502
{
    uint8_t A = 0;     // Accumulator
    uint8_t X = 0;     // X register
    uint8_t Y = 0;     // Y register
    uint8_t SP = 0xFD; // Stack Pointer
    uint16_t PC = 0;   // Program Counter
    Flags P;           // Processor Status
    Memory *mem = nullptr;
    bool isNMOS6507 = false;

    void Reset(Memory &memory, bool is6507 = false)
    {
        this->mem = &memory;
        this->isNMOS6507 = is6507;
        mem->use6507addresspace = is6507;

        // Randomise A, X, Y to simulate undefined power-on state
        static std::random_device rd;
        static std::mt19937 gen(rd());
        static std::uniform_int_distribution<uint8_t> dist(0, 255);

        A = dist(gen);
        X = dist(gen);
        Y = dist(gen);
        halted = false;

        // Stack pointer after reset sequence
        SP = 0xFD;

        // Processor status: Interrupt Disable + Unused bit set
        P.reg = Flags::I | Flags::U;

        // Simulate the "phantom pushes" the 6502 does on reset
        // These don't actually store meaningful values, but they burn cycles
        uint8_t dummy;
        dummy = mem->Read(0x0100 | SP); // pretend push PCH
        SP--;
        dummy = mem->Read(0x0100 | SP); // pretend push PCL
        SP--;
        dummy = mem->Read(0x0100 | SP); // pretend push P
        SP--;

        // Load reset vector into PC
        uint8_t lo = mem->Read(0xFFFC);
        uint8_t hi = mem->Read(0xFFFD);
        PC = static_cast<uint16_t>(lo | (hi << 8));

        // Account for reset timing (NMOS 6502 = 7 cycles)
        cycles += 7;
    }

    void LDA_Immediate()
    {
        uint8_t value = mem->Read(PC++);
        A = value;
        P.SetZN(A);
    }

    void STA_Absolute()
    {
        uint16_t lo = mem->Read(PC++);
        uint16_t hi = mem->Read(PC++);
        uint16_t addr = lo | (hi << 8);
        mem->Write(addr, A);
    }

    // --- Addressing helpers ---
    uint8_t Fetch8() { return mem->Read(PC++); }
    uint16_t Fetch16()
    {
        uint8_t lo = Fetch8();
        uint8_t hi = Fetch8();
        return (uint16_t)lo | ((uint16_t)hi << 8);
    }
    uint16_t Addr_Immediate() { return PC++; }
    uint16_t Addr_ZeroPage() { return Fetch8(); }
    uint16_t Addr_ZeroPageX() { return (uint8_t)(Fetch8() + X); }
    uint16_t Addr_ZeroPageY() { return (uint8_t)(Fetch8() + Y); }
    uint16_t Addr_Absolute() { return Fetch16(); }
    uint16_t Addr_AbsoluteX()
    {
        uint16_t base = Fetch16();
        uint16_t addr = base + X;
        page_crossed = ((base & 0xFF00) != (addr & 0xFF00));
        return addr;
    }

    uint16_t Addr_AbsoluteY()
    {
        uint16_t base = Fetch16();
        uint16_t addr = base + Y;
        page_crossed = ((base & 0xFF00) != (addr & 0xFF00));
        return addr;
    }

    // --- Core operations ---
    void LDA(uint16_t addr)
    {
        A = mem->Read(addr);
        P.SetZN(A);
    }
    void LDX(uint16_t addr)
    {
        X = mem->Read(addr);
        P.SetZN(X);
    }
    void LDY(uint16_t addr)
    {
        Y = mem->Read(addr);
        P.SetZN(Y);
    }
    void STA(uint16_t addr) { mem->Write(addr, A); }
    void STX(uint16_t addr) { mem->Write(addr, X); }
    void STY(uint16_t addr) { mem->Write(addr, Y); }

    uint16_t Addr_IndirectX()
    {
        uint8_t zpAddr = (uint8_t)(Fetch8() + X);
        uint8_t lo = mem->Read(zpAddr);
        uint8_t hi = mem->Read((uint8_t)(zpAddr + 1));
        return (uint16_t)lo | ((uint16_t)hi << 8);
    }

    uint16_t Addr_IndirectY()
    {
        uint8_t zp = Fetch8();
        uint16_t base = mem->Read(zp) | (mem->Read((uint8_t)(zp + 1)) << 8);
        uint16_t addr = base + Y;
        page_crossed = ((base & 0xFF00) != (addr & 0xFF00));
        return addr;
    }

    void TAX()
    {
        X = A;
        P.SetZN(X);
    }
    void TAY()
    {
        Y = A;
        P.SetZN(Y);
    }
    void TXA()
    {
        A = X;
        P.SetZN(A);
    }
    void TYA()
    {
        A = Y;
        P.SetZN(A);
    }

    void INX()
    {
        X++;
        P.SetZN(X);
    }
    void INY()
    {
        Y++;
        P.SetZN(Y);
    }
    // --- Full BRK ---
    void BRK_full()
    {
        PC++; // BRK increments PC before pushing
        Push((PC >> 8) & 0xFF);
        Push(PC & 0xFF);
        Push(P.reg | Flags::B | Flags::U);
        P.Set(Flags::I, true);
        uint8_t lo = mem->Read(0xFFFE);
        uint8_t hi = mem->Read(0xFFFF);
        PC = (uint16_t)lo | ((uint16_t)hi << 8);
    }

    // --- RTI ---
    void RTI()
    {
        P.reg = (Pop() & ~Flags::B) | Flags::U;
        uint8_t lo = Pop();
        uint8_t hi = Pop();
        PC = (uint16_t)lo | ((uint16_t)hi << 8);
    }

    // --- INC / DEC (memory) ---
    void INC(uint16_t addr)
    {
        uint8_t val = mem->Read(addr);
        val++;
        mem->Write(addr, val);
        P.SetZN(val);
    }
    void DEC(uint16_t addr)
    {
        uint8_t val = mem->Read(addr);
        val--;
        mem->Write(addr, val);
        P.SetZN(val);
    }
    void DEX()
    {
        X--;
        P.SetZN(X);
    }
    void DEY()
    {
        Y--;
        P.SetZN(Y);
    }

    void ADC(uint16_t addr)
    {
        uint8_t value = mem->Read(addr);

        if (P.Get(Flags::D))
        {
            // -------- Decimal mode (NMOS 6502 behaviour) --------
            uint8_t carry_in = P.Get(Flags::C) ? 1 : 0;
            uint8_t lo = (A & 0x0F) + (value & 0x0F) + carry_in;
            uint8_t hi = (A >> 4) + (value >> 4);

            if (lo > 9)
            {
                lo += 6;
                hi++;
            }
            if (hi > 9)
            {
                hi += 6;
            }

            P.Set(Flags::C, hi > 15);
            uint8_t result = (uint8_t)((hi << 4) | (lo & 0x0F));

            // V flag still from binary add
            uint16_t bin_sum = (uint16_t)A + value + carry_in;
            P.Set(Flags::V, (~(A ^ value) & (A ^ (uint8_t)bin_sum) & 0x80) != 0);

            A = result;
            P.SetZN(A);
        }
        else
        {
            // -------- Binary mode --------
            uint16_t sum = (uint16_t)A + value + (P.Get(Flags::C) ? 1 : 0);
            P.Set(Flags::C, sum > 0xFF);
            uint8_t result = (uint8_t)sum;
            P.Set(Flags::V, (~(A ^ value) & (A ^ result) & 0x80) != 0);
            A = result;
            P.SetZN(A);
        }
    }

    void SBC(uint16_t addr)
    {
        uint8_t value = mem->Read(addr);

        if (P.Get(Flags::D))
        {
            // -------- Decimal mode (NMOS 6502 behaviour) --------
            uint8_t carry_in = P.Get(Flags::C) ? 0 : 1; // In SBC, C=1 means no borrow
            uint8_t lo = (A & 0x0F) - (value & 0x0F) - carry_in;
            uint8_t hi = (A >> 4) - (value >> 4);

            if ((int8_t)lo < 0)
            {
                lo -= 6;
                hi--;
            }
            if ((int8_t)hi < 0)
            {
                hi -= 6;
            }

            P.Set(Flags::C, hi >= 0);
            uint8_t result = (uint8_t)((hi << 4) | (lo & 0x0F));

            // V flag still from binary subtract
            uint8_t m = value ^ 0xFF;
            uint16_t bin_sum = (uint16_t)A + m + (P.Get(Flags::C) ? 1 : 0);
            P.Set(Flags::V, (~(A ^ m) & (A ^ (uint8_t)bin_sum) & 0x80) != 0);

            A = result;
            P.SetZN(A);
        }
        else
        {
            // -------- Binary mode --------
            uint8_t m = value ^ 0xFF;
            uint16_t sum = (uint16_t)A + m + (P.Get(Flags::C) ? 1 : 0);
            P.Set(Flags::C, sum > 0xFF);
            uint8_t result = (uint8_t)sum;
            P.Set(Flags::V, (~(A ^ m) & (A ^ result) & 0x80) != 0);
            A = result;
            P.SetZN(A);
        }
    }

    void AND(uint16_t addr)
    {
        A &= mem->Read(addr);
        P.SetZN(A);
    }
    void ORA(uint16_t addr)
    {
        A |= mem->Read(addr);
        P.SetZN(A);
    }
    void EOR(uint16_t addr)
    {
        A ^= mem->Read(addr);
        P.SetZN(A);
    }

    void CMP(uint16_t addr)
    {
        uint8_t value = mem->Read(addr);
        P.Set(Flags::C, A >= value);
        P.SetZN(A - value);
    }
    void CPX(uint16_t addr)
    {
        uint8_t value = mem->Read(addr);
        P.Set(Flags::C, X >= value);
        P.SetZN(X - value);
    }
    void CPY(uint16_t addr)
    {
        uint8_t value = mem->Read(addr);
        P.Set(Flags::C, Y >= value);
        P.SetZN(Y - value);
    }

    void ASL_A()
    {
        P.Set(Flags::C, A & 0x80);
        A <<= 1;
        P.SetZN(A);
    }
    void LSR_A()
    {
        P.Set(Flags::C, A & 0x01);
        A >>= 1;
        P.SetZN(A);
    }
    void ROL_A()
    {
        bool carry = P.Get(Flags::C);
        P.Set(Flags::C, A & 0x80);
        A = (A << 1) | (carry ? 1 : 0);
        P.SetZN(A);
    }
    void ROR_A()
    {
        bool carry = P.Get(Flags::C);
        P.Set(Flags::C, A & 0x01);
        A = (A >> 1) | (carry ? 0x80 : 0);
        P.SetZN(A);
    }

    void CLC() { P.Set(Flags::C, false); }
    void SEC() { P.Set(Flags::C, true); }
    void CLI() { P.Set(Flags::I, false); }
    void SEI() { P.Set(Flags::I, true); }
    void CLV() { P.Set(Flags::V, false); }
    void CLD() { P.Set(Flags::D, false); }
    void SED() { P.Set(Flags::D, true); }

    void NOP() { /* do nothing */ }

    void BRK()
    {
        // Minimal: stop execution
        running = false;
    }

    // --- Branching ---
    void BranchIf(bool condition)
    {
        int8_t offset = (int8_t)Fetch8();
        branch_taken = false;
        page_crossed = false;

        if (condition)
        {
            branch_taken = true;
            uint16_t oldPC = PC;
            PC += offset;
            if ((oldPC & 0xFF00) != (PC & 0xFF00))
            {
                page_crossed = true;
            }
        }
    }

    void BEQ() { BranchIf(P.Get(Flags::Z)); }
    void BNE() { BranchIf(!P.Get(Flags::Z)); }
    void BCS() { BranchIf(P.Get(Flags::C)); }
    void BCC() { BranchIf(!P.Get(Flags::C)); }
    void BMI() { BranchIf(P.Get(Flags::N)); }
    void BPL() { BranchIf(!P.Get(Flags::N)); }
    void BVS() { BranchIf(P.Get(Flags::V)); }
    void BVC() { BranchIf(!P.Get(Flags::V)); }

    // --- Stack helpers ---
    void Push(uint8_t value) { mem->Write(0x0100 + SP--, value); }
    uint8_t Pop() { return mem->Read(0x0100 + ++SP); }

    // --- Stack ops ---
    void PHA() { Push(A); }
    void PHP() { Push(P.reg | Flags::B | Flags::U); }
    void PLA()
    {
        A = Pop();
        P.SetZN(A);
    }
    void PLP() { P.reg = (Pop() & ~Flags::B) | Flags::U; }

    // --- Official missing transfers ---
    void TSX()
    {
        X = SP;
        P.SetZN(X);
    }
    void TXS() { SP = X; }

    // --- Unofficial opcodes ---
    void LAX(uint16_t addr)
    {
        uint8_t val = mem->Read(addr);
        A = val;
        X = val;
        P.SetZN(val);
    }

    void SAX(uint16_t addr)
    {
        mem->Write(addr, A & X);
    }

    void DCP(uint16_t addr)
    {
        uint8_t val = mem->Read(addr);
        val--;
        mem->Write(addr, val);
        P.Set(Flags::C, A >= val);
        P.SetZN(A - val);
    }

    void ISC(uint16_t addr)
    {
        uint8_t val = mem->Read(addr);
        val++;
        mem->Write(addr, val);
        // SBC with incremented value
        val ^= 0xFF;
        uint16_t sum = A + val + (P.Get(Flags::C) ? 1 : 0);
        P.Set(Flags::C, sum > 0xFF);
        uint8_t result = (uint8_t)sum;
        P.Set(Flags::V, (~(A ^ val) & (A ^ result) & 0x80) != 0);
        A = result;
        P.SetZN(A);
    }

    void SLO(uint16_t addr)
    {
        uint8_t val = mem->Read(addr);
        P.Set(Flags::C, val & 0x80);
        val <<= 1;
        mem->Write(addr, val);
        A |= val;
        P.SetZN(A);
    }

    void RLA(uint16_t addr)
    {
        uint8_t val = mem->Read(addr);
        bool carry = P.Get(Flags::C);
        P.Set(Flags::C, val & 0x80);
        val = (val << 1) | (carry ? 1 : 0);
        mem->Write(addr, val);
        A &= val;
        P.SetZN(A);
    }

    void SRE(uint16_t addr)
    {
        uint8_t val = mem->Read(addr);
        P.Set(Flags::C, val & 0x01);
        val >>= 1;
        mem->Write(addr, val);
        A ^= val;
        P.SetZN(A);
    }

    // --- RRA: ROR then ADC
    void RRA(uint16_t addr)
    {
        uint8_t val = mem->Read(addr);
        bool carry = P.Get(Flags::C);
        P.Set(Flags::C, val & 0x01);
        val = (val >> 1) | (carry ? 0x80 : 0);
        mem->Write(addr, val);
        // ADC with rotated value
        uint16_t sum = A + val + (P.Get(Flags::C) ? 1 : 0);
        P.Set(Flags::C, sum > 0xFF);
        uint8_t result = (uint8_t)sum;
        P.Set(Flags::V, (~(A ^ val) & (A ^ result) & 0x80) != 0);
        A = result;
        P.SetZN(A);
    }

    // ANE (aka XAA) — A = (A | magic_const) & X & imm
    // Magic constant varies; C64 NMOS 6510 often behaves like 0xEE.
    static inline uint8_t ANE(uint8_t A, uint8_t X, uint8_t imm)
    {
        const uint8_t magic = 0xEE; // best guess
        return (A | magic) & X & imm;
    }

    // LAX #imm (unstable immediate) — A = X = imm & magic_const
    static inline uint8_t LAXimm(uint8_t imm)
    {
        const uint8_t magic = 0xEE; // best guess
        return imm & magic;
    }

    // LAS (aka LAR) — A = X = SP = mem & SP
    static inline uint8_t LAS(uint8_t memVal, uint8_t &SP)
    {
        uint8_t val = memVal & SP;
        SP = val;
        return val;
    }

    // SHA (aka AHX) — store A & X & (high_byte+1)
    static inline uint8_t SHA(uint8_t A, uint8_t X, uint16_t addr)
    {
        uint8_t high = (addr >> 8) + 1;
        return A & X & high;
    }

    // SHX (aka SXH) — store X & (high_byte+1)
    static inline uint8_t SHX(uint8_t X, uint16_t addr)
    {
        uint8_t high = (addr >> 8) + 1;
        return X & high;
    }

    // SHY (aka SYH) — store Y & (high_byte+1)
    static inline uint8_t SHY(uint8_t Y, uint16_t addr)
    {
        uint8_t high = (addr >> 8) + 1;
        return Y & high;
    }

    // --- SHA (AHX Absolute,Y and Indirect,Y)
    void SHA(uint16_t addr)
    {
        uint8_t high = (addr >> 8) + 1;
        uint8_t val = A & X & high;
        mem->Write(addr, val);
    }

    // --- SHX (Absolute,Y)
    void SHX(uint16_t addr)
    {
        uint8_t high = (addr >> 8) + 1;
        uint8_t val = X & high;
        mem->Write(addr, val);
    }

    // --- SHY (Absolute,X)
    void SHY(uint16_t addr)
    {
        uint8_t high = (addr >> 8) + 1;
        uint8_t val = Y & high;
        mem->Write(addr, val);
    }

    // --- TAS (Absolute,Y)
    void TAS(uint16_t addr)
    {
        SP = A & X;
        uint8_t high = (addr >> 8) + 1;
        uint8_t val = SP & high;
        mem->Write(addr, val);
    }

    // --- Jumps / subroutines ---
    void JSR()
    {
        uint16_t addr = Fetch16();
        uint16_t retAddr = PC - 1;
        Push((retAddr >> 8) & 0xFF);
        Push(retAddr & 0xFF);
        PC = addr;
    }
    void RTS()
    {
        uint8_t lo = Pop();
        uint8_t hi = Pop();
        PC = ((uint16_t)hi << 8) | lo;
        PC++;
    }
    void JMP_Absolute() { PC = Fetch16(); }
    void JMP_Indirect()
    {
        uint16_t ptr = Fetch16();
        uint8_t lo = mem->Read(ptr);
        // emulate 6502 page boundary bug
        uint8_t hi = mem->Read((ptr & 0xFF00) | ((ptr + 1) & 0x00FF));
        PC = (uint16_t)lo | ((uint16_t)hi << 8);
    }

    // --- Memory shifts/rotates ---
    void ASL_Mem(uint16_t addr)
    {
        uint8_t val = mem->Read(addr);
        P.Set(Flags::C, val & 0x80);
        val <<= 1;
        mem->Write(addr, val);
        P.SetZN(val);
    }
    void LSR_Mem(uint16_t addr)
    {
        uint8_t val = mem->Read(addr);
        P.Set(Flags::C, val & 0x01);
        val >>= 1;
        mem->Write(addr, val);
        P.SetZN(val);
    }
    void ROL_Mem(uint16_t addr)
    {
        uint8_t val = mem->Read(addr);
        bool carry = P.Get(Flags::C);
        P.Set(Flags::C, val & 0x80);
        val = (val << 1) | (carry ? 1 : 0);
        mem->Write(addr, val);
        P.SetZN(val);
    }
    void ROR_Mem(uint16_t addr)
    {
        uint8_t val = mem->Read(addr);
        bool carry = P.Get(Flags::C);
        P.Set(Flags::C, val & 0x01);
        val = (val >> 1) | (carry ? 0x80 : 0);
        mem->Write(addr, val);
        P.SetZN(val);
    }

    // ANC: A = A & value; C = bit7(A); Z/N set from A
    void ANC(uint16_t addr)
    {
        A &= mem->Read(addr);
        P.SetZN(A);
        P.Set(Flags::C, A & 0x80);
    }

    // ALR (ASR): A = (A & value) >> 1; C = old bit0
    void ALR(uint16_t addr)
    {
        A &= mem->Read(addr);
        P.Set(Flags::C, A & 0x01);
        A >>= 1;
        P.SetZN(A);
    }

    // ARR: A = (A & value) ROR 1; C = bit6 of result; V = bit6 ^ bit5
    void ARR(uint16_t addr)
    {
        A &= mem->Read(addr);
        bool carry_in = P.Get(Flags::C);
        uint8_t oldA = A;
        A = (A >> 1) | (carry_in ? 0x80 : 0);
        P.SetZN(A);
        P.Set(Flags::C, A & 0x40);
        P.Set(Flags::V, ((A & 0x40) >> 6) ^ ((A & 0x20) >> 5));
    }

    // LAS: A = X = SP = mem[addr] & SP
    void LAS(uint16_t addr)
    {
        uint8_t val = mem->Read(addr) & SP;
        A = val;
        X = val;
        SP = val;
        P.SetZN(val);
    }

    // --- Memory shifts/rotates with indexed addressing ---
    void ASL_MemAbsX(uint16_t addr)
    {
        addr += X;
        uint8_t val = mem->Read(addr);
        P.Set(Flags::C, val & 0x80);
        val <<= 1;
        mem->Write(addr, val);
        P.SetZN(val);
    }

    void LSR_MemAbsX(uint16_t addr)
    {
        addr += X;
        uint8_t val = mem->Read(addr);
        P.Set(Flags::C, val & 0x01);
        val >>= 1;
        mem->Write(addr, val);
        P.SetZN(val);
    }

    void ROL_MemAbsX(uint16_t addr)
    {
        addr += X;
        uint8_t val = mem->Read(addr);
        bool carry = P.Get(Flags::C);
        P.Set(Flags::C, val & 0x80);
        val = (val << 1) | (carry ? 1 : 0);
        mem->Write(addr, val);
        P.SetZN(val);
    }

    void ROR_MemAbsX(uint16_t addr)
    {
        addr += X;
        uint8_t val = mem->Read(addr);
        bool carry = P.Get(Flags::C);
        P.Set(Flags::C, val & 0x01);
        val = (val >> 1) | (carry ? 0x80 : 0);
        mem->Write(addr, val);
        P.SetZN(val);
    }

    void BIT(uint16_t addr)
    {
        uint8_t value = mem->Read(addr);
        // Z flag = (A & value) == 0
        P.Set(Flags::Z, (A & value) == 0);
        // N flag = bit 7 of value
        P.Set(Flags::N, value & 0x80);
        // V flag = bit 6 of value
        P.Set(Flags::V, value & 0x40);
    }

    void Execute(uint8_t opcode)
    {
        switch (opcode)
        {
        // --- LDA ---
        case 0xA9:
            LDA(Addr_Immediate());
            break;
        case 0xA5:
            LDA(Addr_ZeroPage());
            break;
        case 0xB5:
            LDA(Addr_ZeroPageX());
            break;
        case 0xAD:
            LDA(Addr_Absolute());
            break;
        case 0xBD: // LDA abs,X
        {
            uint16_t addr = Addr_AbsoluteX();
            LDA(addr);
        }
        break;

        case 0xB9:
            LDA(Addr_AbsoluteY());
            break;

        // --- LDX ---
        case 0xA2:
            LDX(Addr_Immediate());
            break;
        case 0xA6:
            LDX(Addr_ZeroPage());
            break;
        case 0xB6:
            LDX(Addr_ZeroPageY());
            break;
        case 0xAE:
            LDX(Addr_Absolute());
            break;
        case 0xBE:
            LDX(Addr_AbsoluteY());
            break;

        // --- LDY ---
        case 0xA0:
            LDY(Addr_Immediate());
            break;
        case 0xA4:
            LDY(Addr_ZeroPage());
            break;
        case 0xB4:
            LDY(Addr_ZeroPageX());
            break;
        case 0xAC:
            LDY(Addr_Absolute());
            break;
        case 0xBC:
            LDY(Addr_AbsoluteX());
            break;

        // --- STA ---
        case 0x85:
            STA(Addr_ZeroPage());
            break;
        case 0x95:
            STA(Addr_ZeroPageX());
            break;
        case 0x8D:
            STA(Addr_Absolute());
            break;
        case 0x9D:
            STA(Addr_AbsoluteX());
            break;
        case 0x99:
            STA(Addr_AbsoluteY());
            break;

        // --- STX ---
        case 0x86:
            STX(Addr_ZeroPage());
            break;
        case 0x96:
            STX(Addr_ZeroPageY());
            break;
        case 0x8E:
            STX(Addr_Absolute());
            break;

        // --- STY ---
        case 0x84:
            STY(Addr_ZeroPage());
            break;
        case 0x94:
            STY(Addr_ZeroPageX());
            break;
        case 0x8C:
            STY(Addr_Absolute());
            break;

        // --- Transfers ---
        case 0xAA:
            TAX();
            break;
        case 0xA8:
            TAY();
            break;
        case 0x8A:
            TXA();
            break;
        case 0x98:
            TYA();
            break;

        // --- INC / DEC ---
        case 0xE8:
            INX();
            break;
        case 0xC8:
            INY();
            break;
        case 0xCA:
            DEX();
            break;
        case 0x88:
            DEY();
            break;

        // --- ADC ---
        case 0x69:
            ADC(Addr_Immediate());
            break;
        case 0x65:
            ADC(Addr_ZeroPage());
            break;
        case 0x6D:
            ADC(Addr_Absolute());
            break;

        // --- SBC ---
        case 0xE9:
            SBC(Addr_Immediate());
            break;
        case 0xE5:
            SBC(Addr_ZeroPage());
            break;
        case 0xED:
            SBC(Addr_Absolute());
            break;

        // --- Logical ---
        case 0x29:
            AND(Addr_Immediate());
            break;
        case 0x25:
            AND(Addr_ZeroPage());
            break;
        case 0x2D:
            AND(Addr_Absolute());
            break;

        case 0x09:
            ORA(Addr_Immediate());
            break;
        case 0x05:
            ORA(Addr_ZeroPage());
            break;
        case 0x0D:
            ORA(Addr_Absolute());
            break;

        case 0x49:
            EOR(Addr_Immediate());
            break;
        case 0x45:
            EOR(Addr_ZeroPage());
            break;
        case 0x4D:
            EOR(Addr_Absolute());
            break;

        // --- Compare ---
        case 0xC9:
            CMP(Addr_Immediate());
            break;
        case 0xE0:
            CPX(Addr_Immediate());
            break;
        case 0xC0:
            CPY(Addr_Immediate());
            break;

        // --- Shifts / Rotates (accumulator only for now) ---
        case 0x0A:
            ASL_A();
            break;
        case 0x4A:
            LSR_A();
            break;
        case 0x2A:
            ROL_A();
            break;
        case 0x6A:
            ROR_A();
            break;

        // --- Flags ---
        case 0x18:
            CLC();
            break;
        case 0x38:
            SEC();
            break;
        case 0x58:
            CLI();
            break;
        case 0x78:
            SEI();
            break;
        case 0xB8:
            CLV();
            break;
        case 0xD8:
            CLD();
            break;
        case 0xF8:
            SED();
            break;

        // --- NOP ---
        case 0xEA:
            NOP();
            break;

            // --- Branches ---
        case 0xF0:
            BEQ();
            break;
        case 0xD0:
            BNE();
            break;
        case 0xB0:
            BCS();
            break;
        case 0x90:
            BCC();
            break;
        case 0x30:
            BMI();
            break;
        case 0x10:
            BPL();
            break;
        case 0x70:
            BVS();
            break;
        case 0x50:
            BVC();
            break;

        // --- Stack ops ---
        case 0x48:
            PHA();
            break;
        case 0x08:
            PHP();
            break;
        case 0x68:
            PLA();
            break;
        case 0x28:
            PLP();
            break;

        // --- Jumps / subroutines ---
        case 0x20:
            JSR();
            break;
        case 0x60:
            RTS();
            break;
        case 0x4C:
            JMP_Absolute();
            break;
        case 0x6C:
            JMP_Indirect();
            break;

        // --- Memory shifts/rotates (Zero Page) ---
        case 0x06:
            ASL_Mem(Addr_ZeroPage());
            break;
        case 0x46:
            LSR_Mem(Addr_ZeroPage());
            break;
        case 0x26:
            ROL_Mem(Addr_ZeroPage());
            break;
        case 0x66:
            ROR_Mem(Addr_ZeroPage());
            break;

        // --- Memory shifts/rotates (Absolute) ---
        case 0x0E:
            ASL_Mem(Addr_Absolute());
            break;
        case 0x4E:
            LSR_Mem(Addr_Absolute());
            break;
        case 0x2E:
            ROL_Mem(Addr_Absolute());
            break;
        case 0x6E:
            ROR_Mem(Addr_Absolute());
            break;

        // --- Memory shifts/rotates (Zero Page,X) ---
        case 0x16:
            ASL_Mem(Addr_ZeroPageX());
            break;
        case 0x56:
            LSR_Mem(Addr_ZeroPageX());
            break;
        case 0x36:
            ROL_Mem(Addr_ZeroPageX());
            break;
        case 0x76:
            ROR_Mem(Addr_ZeroPageX());
            break;

        // --- Memory shifts/rotates (Absolute,X) ---
        case 0x1E:
            ASL_MemAbsX(Addr_Absolute());
            break;
        case 0x5E:
            LSR_MemAbsX(Addr_Absolute());
            break;
        case 0x3E:
            ROL_MemAbsX(Addr_Absolute());
            break;
        case 0x7E:
            ROR_MemAbsX(Addr_Absolute());
            break;

            // --- AND (indexed forms) ---
        case 0x35:
            AND(Addr_ZeroPageX());
            break;
        case 0x3D:
            AND(Addr_AbsoluteX());
            break;
        case 0x39:
            AND(Addr_AbsoluteY());
            break;

        // --- ORA (indexed forms) ---
        case 0x15:
            ORA(Addr_ZeroPageX());
            break;
        case 0x1D:
            ORA(Addr_AbsoluteX());
            break;
        case 0x19:
            ORA(Addr_AbsoluteY());
            break;

        // --- EOR (indexed forms) ---
        case 0x55:
            EOR(Addr_ZeroPageX());
            break;
        case 0x5D:
            EOR(Addr_AbsoluteX());
            break;
        case 0x59:
            EOR(Addr_AbsoluteY());
            break;

        // --- CMP (indexed forms) ---
        case 0xC5:
            CMP(Addr_ZeroPage());
            break; // if not already present
        case 0xD5:
            CMP(Addr_ZeroPageX());
            break;
        case 0xCD:
            CMP(Addr_Absolute());
            break; // if not already present
        case 0xDD:
            CMP(Addr_AbsoluteX());
            break;
        case 0xD9:
            CMP(Addr_AbsoluteY());
            break;

        // --- CPX (indexed form) ---
        case 0xE4:
            CPX(Addr_ZeroPage());
            break; // if not already present
        case 0xEC:
            CPX(Addr_Absolute());
            break; // CPX has no indexed forms beyond this

        // --- CPY (indexed form) ---
        case 0xC4:
            CPY(Addr_ZeroPage());
            break; // if not already present
        case 0xCC:
            CPY(Addr_Absolute());
            break; // CPY has no indexed forms beyond this

        // --- LDA (indirect) ---
        case 0xA1:
            LDA(Addr_IndirectX());
            break;
        case 0xB1:
            LDA(Addr_IndirectY());
            break;

        // --- LDX has no indirect forms ---

        // --- LDY has no indirect forms ---

        // --- STA (indirect) ---
        case 0x81:
            STA(Addr_IndirectX());
            break;
        case 0x91:
            STA(Addr_IndirectY());
            break;

        // --- AND (indirect) ---
        case 0x21:
            AND(Addr_IndirectX());
            break;
        case 0x31:
            AND(Addr_IndirectY());
            break;

        // --- ORA (indirect) ---
        case 0x01:
            ORA(Addr_IndirectX());
            break;
        case 0x11:
            ORA(Addr_IndirectY());
            break;

        // --- EOR (indirect) ---
        case 0x41:
            EOR(Addr_IndirectX());
            break;
        case 0x51:
            EOR(Addr_IndirectY());
            break;

        // --- CMP (indirect) ---
        case 0xC1:
            CMP(Addr_IndirectX());
            break;
        case 0xD1:
            CMP(Addr_IndirectY());
            break;

        // --- BIT ---
        case 0x24:
            BIT(Addr_ZeroPage());
            break;
        case 0x2C:
            BIT(Addr_Absolute());
            break;

            // --- BRK (full) ---
        case 0x00:
            BRK_full();
            break;

        // --- RTI ---
        case 0x40:
            RTI();
            break;

        // --- INC ---
        case 0xE6:
            INC(Addr_ZeroPage());
            break;
        case 0xF6:
            INC(Addr_ZeroPageX());
            break;
        case 0xEE:
            INC(Addr_Absolute());
            break;
        case 0xFE:
            INC(Addr_AbsoluteX());
            break;

        // --- DEC ---
        case 0xC6:
            DEC(Addr_ZeroPage());
            break;
        case 0xD6:
            DEC(Addr_ZeroPageX());
            break;
        case 0xCE:
            DEC(Addr_Absolute());
            break;
        case 0xDE:
            DEC(Addr_AbsoluteX());
            break;

        // --- ADC (remaining forms) ---
        case 0x75:
            ADC(Addr_ZeroPageX());
            break;
        case 0x7D:
            ADC(Addr_AbsoluteX());
            break;
        case 0x79:
            ADC(Addr_AbsoluteY());
            break;
        case 0x61:
            ADC(Addr_IndirectX());
            break;
        case 0x71:
            ADC(Addr_IndirectY());
            break;

        // --- SBC (remaining forms) ---
        case 0xF5:
            SBC(Addr_ZeroPageX());
            break;
        case 0xFD:
            SBC(Addr_AbsoluteX());
            break;
        case 0xF9:
            SBC(Addr_AbsoluteY());
            break;
        case 0xE1:
            SBC(Addr_IndirectX());
            break;
        case 0xF1:
            SBC(Addr_IndirectY());
            break;

            // --- TSX / TXS ---
        case 0xBA:
            TSX();
            break;
        case 0x9A:
            TXS();
            break;

        // --- LAX ---
        case 0xA7:
            LAX(Addr_ZeroPage());
            break;
        case 0xB7:
            LAX(Addr_ZeroPageY());
            break;
        case 0xAF:
            LAX(Addr_Absolute());
            break;
        case 0xBF:
            LAX(Addr_AbsoluteY());
            break;
        case 0xA3:
            LAX(Addr_IndirectX());
            break;
        case 0xB3:
            LAX(Addr_IndirectY());
            break;

        // --- SAX ---
        case 0x87:
            SAX(Addr_ZeroPage());
            break;
        case 0x97:
            SAX(Addr_ZeroPageY());
            break;
        case 0x8F:
            SAX(Addr_Absolute());
            break;
        case 0x83:
            SAX(Addr_IndirectX());
            break;

        // --- DCP ---
        case 0xC7:
            DCP(Addr_ZeroPage());
            break;
        case 0xD7:
            DCP(Addr_ZeroPageX());
            break;
        case 0xCF:
            DCP(Addr_Absolute());
            break;
        case 0xDF:
            DCP(Addr_AbsoluteX());
            break;
        case 0xDB:
            DCP(Addr_AbsoluteY());
            break;
        case 0xC3:
            DCP(Addr_IndirectX());
            break;
        case 0xD3:
            DCP(Addr_IndirectY());
            break;

        // --- ISC ---
        case 0xE7:
            ISC(Addr_ZeroPage());
            break;
        case 0xF7:
            ISC(Addr_ZeroPageX());
            break;
        case 0xEF:
            ISC(Addr_Absolute());
            break;
        case 0xFF:
            ISC(Addr_AbsoluteX());
            break;
        case 0xFB:
            ISC(Addr_AbsoluteY());
            break;
        case 0xE3:
            ISC(Addr_IndirectX());
            break;
        case 0xF3:
            ISC(Addr_IndirectY());
            break;

        // --- SLO ---
        case 0x07:
            SLO(Addr_ZeroPage());
            break;
        case 0x17:
            SLO(Addr_ZeroPageX());
            break;
        case 0x0F:
            SLO(Addr_Absolute());
            break;
        case 0x1F:
            SLO(Addr_AbsoluteX());
            break;
        case 0x1B:
            SLO(Addr_AbsoluteY());
            break;
        case 0x03:
            SLO(Addr_IndirectX());
            break;
        case 0x13:
            SLO(Addr_IndirectY());
            break;

        // --- RLA ---
        case 0x27:
            RLA(Addr_ZeroPage());
            break;
        case 0x37:
            RLA(Addr_ZeroPageX());
            break;
        case 0x2F:
            RLA(Addr_Absolute());
            break;
        case 0x3F:
            RLA(Addr_AbsoluteX());
            break;
        case 0x3B:
            RLA(Addr_AbsoluteY());
            break;
        case 0x23:
            RLA(Addr_IndirectX());
            break;
        case 0x33:
            RLA(Addr_IndirectY());
            break;

        // --- SRE ---
        case 0x47:
            SRE(Addr_ZeroPage());
            break;
        case 0x57:
            SRE(Addr_ZeroPageX());
            break;
        case 0x4F:
            SRE(Addr_Absolute());
            break;
        case 0x5F:
            SRE(Addr_AbsoluteX());
            break;
        case 0x5B:
            SRE(Addr_AbsoluteY());
            break;
        case 0x43:
            SRE(Addr_IndirectX());
            break;
        case 0x53:
            SRE(Addr_IndirectY());
            break;

            // ANC (immediate)
        case 0x0B:
            ANC(Addr_Immediate());
            break;
        case 0x2B:
            ANC(Addr_Immediate());
            break; // second variant

        // ALR (immediate)
        case 0x4B:
            ALR(Addr_Immediate());
            break;

        // ARR (immediate)
        case 0x6B:
            ARR(Addr_Immediate());
            break;

        // LAS (Absolute,Y)
        case 0xBB:
            LAS(Addr_AbsoluteY());
            break;

            // --- RRA ---
        case 0x67:
            RRA(Addr_ZeroPage());
            break;
        case 0x77:
            RRA(Addr_ZeroPageX());
            break;
        case 0x6F:
            RRA(Addr_Absolute());
            break;
        case 0x7F:
            RRA(Addr_AbsoluteX());
            break;
        case 0x7B:
            RRA(Addr_AbsoluteY());
            break;
        case 0x63:
            RRA(Addr_IndirectX());
            break;
        case 0x73:
            RRA(Addr_IndirectY());
            break;

        // --- SHA (AHX) ---
        case 0x9F:
            SHA(Addr_AbsoluteY());
            break;
        case 0x93:
            SHA(Addr_IndirectY());
            break;

        // --- SHX ---
        case 0x9E:
            SHX(Addr_AbsoluteY());
            break;

        // --- SHY ---
        case 0x9C:
            SHY(Addr_AbsoluteX());
            break;

        // --- TAS ---
        case 0x9B:
            TAS(Addr_AbsoluteY());
            break;

            // --- ANE (aka XAA) immediate ---
        case 0x8B:
        {
            uint8_t imm = Fetch8();
            A = ANE(A, X, imm);
            P.SetZN(A);
        }
        break;

        // --- LAX immediate (unstable) ---
        case 0xAB:
        {
            uint8_t imm = Fetch8();
            uint8_t val = LAXimm(imm);
            A = X = val;
            P.SetZN(val);
        }
        break;

        case 0x02:
        case 0x12:
        case 0x22:
        case 0x32:
        case 0x42:
        case 0x52:
        case 0x62:
        case 0x72:
        case 0x92:
        case 0xB2:
        case 0xD2:
        case 0xF2:
            halted = true; // JAM/KIL — CPU locked until reset
            break;

        default:
            std::cerr << "Unknown opcode: $" << std::hex << (int)opcode
                      << " at PC=$" << (PC - 1) << "\n";
            running = false;
            break;
        }
    }

    bool running = true;
    bool page_crossed;
    bool branch_taken;
    bool halted = false;

    uint32_t cycles;
    uint64_t throttle_counter = 0;

    void HandleNMI()
    {
        // Push PC high byte, then low byte
        Push((PC >> 8) & 0xFF);
        Push(PC & 0xFF);

        // Push status with B flag cleared, bit 5 (U) set
        this->P.Set(Flags::B, false);
        this->P.Set(Flags::U, true);
        Push(this->P.reg);

        // Set I flag to disable further IRQs (NMI ignores it, but IRQs will be masked)
        this->P.Set(Flags::I, false);

        // Fetch new PC from NMI vector
        uint8_t lo = mem->Read(0xFFFA);
        uint8_t hi = mem->Read(0xFFFB);
        PC = (uint16_t)hi << 8 | lo;

        // NMI takes 7 cycles on a real 6502
        cycles += 7;
    }

    void HandleIRQ()
    {
        // Only respond if interrupts are enabled (I flag clear)
        if ((this->P.Get(Flags::I)) == false)
        {
            // Push PC high byte, then low byte
            Push((PC >> 8) & 0xFF);
            Push(PC & 0xFF);

            // Push status with B flag cleared, bit 5 set
            this->P.Set(Flags::B, false);
            this->P.Set(Flags::U, true);
            Push(this->P.reg);

            // Set I flag to disable further IRQs
            this->P.Set(Flags::I, false);

            // Fetch new PC from IRQ vector ($FFFE/$FFFF)
            uint8_t lo = mem->Read(0xFFFE);
            uint8_t hi = mem->Read(0xFFFF);
            PC = (uint16_t)hi << 8 | lo;

            // IRQ takes 7 cycles on a real 6502
            cycles += 7;
        }
    }

    void Run()
    {
        auto start_time = std::chrono::high_resolution_clock::now();
        uint64_t total_cycles = 0;

        while (running && !halted)
        {
            if (cycles > 0)
            {
                cycles--;
                total_cycles++;
                mem->Clock(); // tick peripherals every CPU cycle
                if (!isNMOS6507 && mem->CheckIRQLines())
                {
                    // Intrupt for US!
                    HandleIRQ();
                }
                // TODO: add Support Later for NMI.
                if (throttle_counter >= 2000)
                {
                    // Throttle here per cycle
                    double emu_time = total_cycles * SECONDS_PER_CYCLE;
                    auto now = std::chrono::high_resolution_clock::now();
                    double real_time = std::chrono::duration<double>(now - start_time).count();
                    if (emu_time > real_time)
                    {
                        std::this_thread::sleep_for(
                            std::chrono::duration<double>(emu_time - real_time));
                    }
                    throttle_counter = 0;
                }
                continue;
            }

            // Fetch and execute next instruction
            uint8_t opcode = mem->Read(PC++);
            page_crossed = false;
            branch_taken = false;

            Execute(opcode);

            cycles += cycle_table[opcode];
            if (branch_taken)
                cycles++;
            if (page_crossed)
                cycles++;
        }
    }
};

int main(int argc, char *argv[])
{
    Memory mem;
    CPU6502 cpu;
    RomSpace romSpace = RomSpace::NONE; // default until set by user/system

    uint16_t startAddr = 0x8000;
    // Simple test program: LDA #$42; STA $0200; BRK
    mem.Write(startAddr + 0, 0xA9);
    mem.Write(startAddr + 1, 0x42);
    mem.Write(startAddr + 2, 0x8D);
    mem.Write(startAddr + 3, 0x00);
    mem.Write(startAddr + 4, 0x02);
    mem.Write(startAddr + 5, 0x00);

    cpu.Reset(mem, false);

    cpu.Run();

    return 0;
}
