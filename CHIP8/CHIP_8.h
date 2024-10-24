#ifndef CHIP_8_H
#define CHIP_8_H

#include <chrono>
#include <random>
#include <iostream>
#include <SDL.h>

typedef signed char int8_t;
typedef unsigned char uint8_t;
typedef signed short int16_t;
typedef unsigned short uint16_t;
typedef signed int int32_t;
typedef unsigned int uint32_t;
typedef signed long long int64_t;
typedef unsigned long long uint64_t;

class CHIP_8
{
public:
    CHIP_8();
    virtual ~CHIP_8();

    void LoadROM(char const* filename);

    /*The CHIP 8 ISA*/
    void MC_00E0();    //clear        --> Clear The Display
    void MC_00EE();    //return       --> Exit a subroutine
    void MC_1NNN();    //jump NNN     --> jump to this memory address
    void MC_2NNN();    //NNN          --> Call a subroutine
    void MC_3XNN();    //SNE Vx, NN   --> if Vx != NN then
    void MC_4XNN();    //SE Vx, NN    --> if Vx == NN then
    void MC_5XY0();    //SNE Vx, Vy   --> if Vx != Vy then
    void MC_6XNN();    //LD Vx, NN    --> Vx = NN
    void MC_7XNN();    //ADD Vx, NN   --> Vx += NN
    void MC_8XY0();    //LD Vx, Vy    --> Vx = Vy
    void MC_8XY1();    //Bitwise OR   --> Vx |= Vy
    void MC_8XY2();    //Bitwise AND  --> Vx &= Vy
    void MC_8XY3();    //Bitwise XOR  --> Vx ^= Vy
    void MC_8XY4();    //SUM Vx, Vy   --> Vx += Vy      (Vf = 1 on carry)
    void MC_8XY5();    //SUB Vx, Vy   --> Vx -= Vy      (Vf = 0 on borrow)
    void MC_8XY6();    //SHR Vx, 1    --> Vx = Vx >> 1. (Vf = 1 on carry)
    void MC_8XY7();    //SUB Vy, Vx   --> Vx = Vy - Vx  (Vf = 0 on borrow)
    void MC_8XYE();    //SHL Vx, 1    --> VX = VX << 1  (VF = 1 on carry)
    void MC_9XY0();    //SNE Vx, Vy   --> if Vx != Vy then
    void MC_ANNN();    //LD IR, NNN   --> IR = NNN
    void MC_BNNN();    //BNNN	      --> Jump to NNN + V0
    void MC_CXNN();    //CXNN         --> Vx = Random number & NN
    void MC_DXYN();    //DRW Vx,Vy, N --> sprite Vx Vy N  (VF = 1 on collision)
    void MC_EX9E();	   //SKP Vx       --> Skip next instruction if key VX pressed
    void MC_EXA1();	   //SKNP Vx	  --> Skip next instruction if key VX not pressed
    void MC_FX07();	   //LD Vx, DT	  --> VX = Delay timer
    void MC_FX0A();	   //LD Vx, K     --> Waits a keypress and stores it in VX
    void MC_FX15();    //LD DT, Vx    --> Delay timer = VX
    void MC_FX18();    //LD ST, Vx    --> Sound timer = VX
    void MC_FX1E();    //ADD I, Vx    --> I = I + VX
    void MC_FX29();    //LD F, Vx     --> I points to the 4 x 5 font sprite of hex char in VX
    void MC_FX33();    //LD B, Vx     --> Store BCD representation of VX in M(I)...M(I+2)
    void MC_FX55();    //LD [I], Vx   --> Save V0...VX in memory starting at M(I)
    void MC_FX65();    //LD Vx, [I]   --> Load V0...VX from memory starting at M(I)

    // Decode Tables
    void Table0();
    void Table8();
    void TableE();
    void TableF();
    void OP_NULL();

    // Emulation Cycle
    void Cycle();

   
    uint32_t Display[32][64];
    uint8_t keypad[16]{};
    uint8_t Sound_Timer;

protected:

private:
    uint8_t V0VF_Registers[16];
    uint8_t Memory[4096];
    uint8_t SP;
    uint8_t Delay_Timer;
    
    uint16_t Index_REG;
    uint16_t PC;
    uint16_t Stack[16];

    SDL_AudioDeviceID deviceId; 
    Uint8* wavBuffer;           
    Uint32 wavLength;
    
    uint16_t Inst_Reg;
    typedef void (CHIP_8::* Chip8Func)();
    Chip8Func table[0xF + 1];
    Chip8Func table0[0xE + 1];
    Chip8Func table8[0xE + 1];
    Chip8Func tableE[0xE + 1];
    Chip8Func tableF[0x65 + 1];

    //pointer to void function
    //void (* func_x_ptr)(void); ------>  void func_x(void);
    //func_x_ptr = &func_x;
    //void (CHIP_8::*opcode_MC_FX1E)(void); -----> void CHIP_8::MC_FX0A(void);
    //opcode_MC_FX1E = &MC_FX0A;
    void (CHIP_8::* opcode_MC_FX1E)(void);


    std::default_random_engine randGen;
    std::uniform_int_distribution<short> randByte;
};

#endif // CHIP_8_H
