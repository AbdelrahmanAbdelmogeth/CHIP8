#include "CHIP_8.h"
#include <fstream>
#include <iostream>
#include <cstring>
#include <SDL.h>

const uint8_t FONTSET_SIZE = 80;
uint8_t fontset[FONTSET_SIZE] =
{
    0xF0, 0x90, 0x90, 0x90, 0xF0, // 0
    0x20, 0x60, 0x20, 0x20, 0x70, // 1
    0xF0, 0x10, 0xF0, 0x80, 0xF0, // 2
    0xF0, 0x10, 0xF0, 0x10, 0xF0, // 3
    0x90, 0x90, 0xF0, 0x10, 0x10, // 4
    0xF0, 0x80, 0xF0, 0x10, 0xF0, // 5
    0xF0, 0x80, 0xF0, 0x90, 0xF0, // 6
    0xF0, 0x10, 0x20, 0x40, 0x40, // 7
    0xF0, 0x90, 0xF0, 0x90, 0xF0, // 8
    0xF0, 0x90, 0xF0, 0x10, 0xF0, // 9
    0xF0, 0x90, 0xF0, 0x90, 0x90, // A
    0xE0, 0x90, 0xE0, 0x90, 0xE0, // B
    0xF0, 0x80, 0x80, 0x80, 0xF0, // C
    0xE0, 0x90, 0x90, 0x90, 0xE0, // D
    0xF0, 0x80, 0xF0, 0x80, 0xF0, // E
    0xF0, 0x80, 0xF0, 0x80, 0x80  // F
};


const uint8_t FONTSET_START_ADDRESS = 0x50;
CHIP_8::CHIP_8()
    : randGen(std::chrono::system_clock::now().time_since_epoch().count())
{
    //Initialize The Stack Pointer
    SP = 0x00;

    PC = 0x200;

    Sound_Timer = 0;

    // Load fonts into memory
    for (uint8_t i = 0; i < FONTSET_SIZE; ++i)
    {
        Memory[FONTSET_START_ADDRESS + i] = fontset[i];
    }

    // Initialize RNG
    randByte = std::uniform_int_distribution<short>(0, 255U);

    memset(Display, 0, sizeof(Display));

    // Set up function pointer table
    table[0x0] = &CHIP_8::Table0;
    table[0x1] = &CHIP_8::MC_1NNN;
    table[0x2] = &CHIP_8::MC_2NNN;
    table[0x3] = &CHIP_8::MC_3XNN;
    table[0x4] = &CHIP_8::MC_4XNN;
    table[0x5] = &CHIP_8::MC_5XY0;
    table[0x6] = &CHIP_8::MC_6XNN;
    table[0x7] = &CHIP_8::MC_7XNN;
    table[0x8] = &CHIP_8::Table8;
    table[0x9] = &CHIP_8::MC_9XY0;
    table[0xA] = &CHIP_8::MC_ANNN;
    table[0xB] = &CHIP_8::MC_BNNN;
    table[0xC] = &CHIP_8::MC_CXNN;
    table[0xD] = &CHIP_8::MC_DXYN;
    table[0xE] = &CHIP_8::TableE;
    table[0xF] = &CHIP_8::TableF;

    for (size_t i = 0; i <= 0xE; i++)
    {
        table0[i] = &CHIP_8::OP_NULL;
        table8[i] = &CHIP_8::OP_NULL;
        tableE[i] = &CHIP_8::OP_NULL;
    }

    table0[0x0] = &CHIP_8::MC_00E0;
    table0[0xE] = &CHIP_8::MC_00EE;

    table8[0x0] = &CHIP_8::MC_8XY0;
    table8[0x1] = &CHIP_8::MC_8XY1;
    table8[0x2] = &CHIP_8::MC_8XY2;
    table8[0x3] = &CHIP_8::MC_8XY3;
    table8[0x4] = &CHIP_8::MC_8XY4;
    table8[0x5] = &CHIP_8::MC_8XY5;
    table8[0x6] = &CHIP_8::MC_8XY6;
    table8[0x7] = &CHIP_8::MC_8XY7;
    table8[0xE] = &CHIP_8::MC_8XYE;

    tableE[0x1] = &CHIP_8::MC_EXA1;
    tableE[0xE] = &CHIP_8::MC_EX9E;

    for (size_t i = 0; i <= 0x65; i++)
    {
        tableF[i] = &CHIP_8::OP_NULL;
    }

    tableF[0x07] = &CHIP_8::MC_FX07;
    tableF[0x0A] = &CHIP_8::MC_FX0A;
    tableF[0x15] = &CHIP_8::MC_FX15;
    tableF[0x18] = &CHIP_8::MC_FX18;
    tableF[0x1E] = &CHIP_8::MC_FX1E;
    tableF[0x29] = &CHIP_8::MC_FX29;
    tableF[0x33] = &CHIP_8::MC_FX33;
    tableF[0x55] = &CHIP_8::MC_FX55;
    tableF[0x65] = &CHIP_8::MC_FX65;

    if (SDL_Init(SDL_INIT_AUDIO) < 0) {
        std::cerr << "Failed to initialize SDL audio: " << SDL_GetError() << std::endl;
    }

    SDL_AudioSpec wavSpec;
    if (SDL_LoadWAV("beep.wav", &wavSpec, &wavBuffer, &wavLength) == NULL) {
        std::cerr << "Failed to load WAV: " << SDL_GetError() << std::endl;
    }

    deviceId = SDL_OpenAudioDevice(NULL, 0, &wavSpec, NULL, 0);
    if (deviceId == 0) {
        std::cerr << "Failed to open audio device: " << SDL_GetError() << std::endl;
    }
}

CHIP_8::~CHIP_8()
{
    //dtor
    SDL_CloseAudioDevice(deviceId);
    SDL_FreeWAV(wavBuffer);
    SDL_Quit();
}

const unsigned int START_ADDRESS = 0x200;
void CHIP_8::LoadROM(char const* filename)
{
    // Open the file as a stream of binary and move the file pointer to the end
    std::ifstream file(filename, std::ios::binary | std::ios::ate);

    if (file.is_open())
    {
        // Get size of file and allocate a buffer to hold the contents
        std::streampos size = file.tellg();
        char* buffer = new char[size];

        // Go back to the beginning of the file and fill the buffer
        file.seekg(0, std::ios::beg);
        file.read(buffer, size);
        file.close();

        // Load the ROM contents into the Chip8's memory, starting at 0x200
        for (long i = 0; i < size; ++i)
        {
            //std::cout<<buffer[i]<<"\n";
            Memory[START_ADDRESS + i] = buffer[i];
        }

        // Free the buffer
        delete[] buffer;
    }
}


///////////////////////////////// Instruction Set Functions ///////////////////////////////////////
void CHIP_8::MC_00E0() {
    memset(Display, 0, sizeof(Display));
}

void CHIP_8::MC_00EE() {
    --SP;
    PC = Stack[SP];
}

void CHIP_8::MC_1NNN() {
    //we will and the content of Instruction register with 0FFF to extract
    //the address where the PC will jump to
    PC = Inst_Reg & 0x0FFF;
}

void CHIP_8::MC_2NNN() {
    Stack[SP] = PC;
    ++SP;

    PC = Inst_Reg & 0x0FFF;
}

//Skip next instruction if Vx != NN
void CHIP_8::MC_3XNN() {
    uint8_t NN = (uint8_t)(Inst_Reg & 0x00FF);
    uint8_t Vx = (uint8_t)((Inst_Reg >> 8) & 0x0F);

    if (V0VF_Registers[Vx] == NN) {
        PC += 2;
    }
}

//Skip next instruction if Vx == NN
void CHIP_8::MC_4XNN() {
    uint8_t NN = (uint8_t)(Inst_Reg & 0x00FF);
    uint8_t Vx = (uint8_t)((Inst_Reg >> 8) & 0x0F);

    if (V0VF_Registers[Vx] != NN) {
        PC += 2;
    }
}

//Skip next instruction if Vx == Vy
void CHIP_8::MC_5XY0() {
    uint8_t Vy = (uint8_t)((Inst_Reg >> 4) & 0x0F);
    uint8_t Vx = (uint8_t)((Inst_Reg >> 8) & 0x0F);

    if (V0VF_Registers[Vy] == V0VF_Registers[Vx]) {
        PC += 2;
    }
}

void CHIP_8::MC_6XNN() {
    uint8_t Vx = (Inst_Reg >> 8) & 0x0F;
    V0VF_Registers[Vx] = (uint8_t)(Inst_Reg & 0x00FF);
}

void CHIP_8::MC_7XNN() {
    uint8_t Vx = (uint8_t)((Inst_Reg >> 8) & 0x0F);
    V0VF_Registers[Vx] += (uint8_t)(Inst_Reg & 0x00FF);
}

void CHIP_8::MC_8XY0() {
    uint8_t Vx = (uint8_t)((Inst_Reg >> 8) & 0x0F);
    uint8_t Vy = (uint8_t)((Inst_Reg >> 4) & 0x0F);
    V0VF_Registers[Vx] = V0VF_Registers[Vy];
}

void CHIP_8::MC_8XY1() {
    uint8_t Vx = (uint8_t)((Inst_Reg >> 8) & 0x0F);
    uint8_t Vy = (uint8_t)((Inst_Reg >> 4) & 0x0F);
    V0VF_Registers[Vx] |= V0VF_Registers[Vy];
}

void CHIP_8::MC_8XY2() {
    uint8_t Vx = (uint8_t)((Inst_Reg >> 8) & 0x0F);
    uint8_t Vy = (uint8_t)((Inst_Reg >> 4) & 0x0F);
    V0VF_Registers[Vx] &= V0VF_Registers[Vy];
}

void CHIP_8::MC_8XY3() {
    uint8_t Vx = (uint8_t)((Inst_Reg >> 8) & 0x0F);
    uint8_t Vy = (uint8_t)((Inst_Reg >> 4) & 0x0F);
    V0VF_Registers[Vx] ^= V0VF_Registers[Vy];
}

void CHIP_8::MC_8XY4() {
    uint8_t Vx = (uint8_t)((Inst_Reg >> 8) & 0x0F);
    uint8_t Vy = (uint8_t)((Inst_Reg >> 4) & 0x0F);
    uint16_t sum = V0VF_Registers[Vx] + V0VF_Registers[Vy];

    if (sum > 255)
        V0VF_Registers[0xF] = 1;
    else
        V0VF_Registers[0xF] = 0;

    //V0VF_Registers[Vx] += V0VF_Registers[Vy];
    V0VF_Registers[Vx] = uint8_t(sum & 0x00ff);
}

/*
    When you do VX - VY, VF is set to the negation of the borrow.
    This means that if VX is superior or equal to VY,
    VF will be set to 01, as the borrow is 0.
    If VX is inferior to VY, VF is set to 00, as the borrow is 1.
*/
void CHIP_8::MC_8XY5() {
    uint8_t Vx = (uint8_t)((Inst_Reg >> 8) & 0x0F);
    uint8_t Vy = (uint8_t)((Inst_Reg >> 4) & 0x0F);

    if (V0VF_Registers[Vx] < V0VF_Registers[Vy])
        V0VF_Registers[0xF] = 0x00;
    else
        V0VF_Registers[0xF] = 0x01;

    V0VF_Registers[Vx] -= V0VF_Registers[Vy];
}

void CHIP_8::MC_8XY6() {
    uint8_t Vx = (uint8_t)((Inst_Reg >> 8) & 0x0F);
    //Save The LSB
    V0VF_Registers[0xF] = (V0VF_Registers[Vx] & 0x01);
    //Vx = Vx / 2
    V0VF_Registers[Vx] >>= 1;
}

/*
    When you do VY - VX, VF is set to the negation of the borrow.
    This means that if VY is superior or equal to VX,
    VF will be set to 01, as the borrow is 0.
    If VY is inferior to VX, VF is set to 00, as the borrow is 1.
*/
void CHIP_8::MC_8XY7() {
    uint8_t Vx = (uint8_t)((Inst_Reg >> 8) & 0x0F);
    uint8_t Vy = (uint8_t)((Inst_Reg >> 4) & 0x0F);

    if (V0VF_Registers[Vy] < V0VF_Registers[Vx])
        V0VF_Registers[0xF] = 0x00;
    else
        V0VF_Registers[0xF] = 0x01;

    V0VF_Registers[Vy] -= V0VF_Registers[Vx];
}


void CHIP_8::MC_8XYE() {
    uint8_t Vx = (uint8_t)((Inst_Reg >> 8) & 0x0F);
    V0VF_Registers[0xF] = (V0VF_Registers[Vx] & 0x80);
    //Vx = Vx * 2
    V0VF_Registers[Vx] <<= 1;
}

void CHIP_8::MC_9XY0() {
    uint8_t Vx = (uint8_t)((Inst_Reg >> 8) & 0x0F);
    uint8_t Vy = (uint8_t)((Inst_Reg >> 4) & 0x0F);

    if (V0VF_Registers[Vy] != V0VF_Registers[Vx]) {
        PC += 2;
    }
}

void CHIP_8::MC_ANNN() {
    Index_REG = Inst_Reg & 0x0FFF;
}

void CHIP_8::MC_BNNN() {
    PC = (uint16_t)V0VF_Registers[0x00] + (Inst_Reg & 0x0FFF);
}

void CHIP_8::MC_CXNN() {
    uint8_t Vx = (uint8_t)((Inst_Reg >> 8) & 0x0F);
    V0VF_Registers[Vx] = randByte(randGen) & (uint8_t)(Inst_Reg & 0x00FF);
}

/*
    DXYN	Draws a sprite at (VX,VY) starting at Memory Location index_reg.
            VF = collision.
            If N=0, draws the 16 x 16 sprite, else an 8 x N sprite.

            //Graphics are drawn as 8 x 1...15 sprites (they are byte coded)

*/
void CHIP_8::MC_DXYN() {
    uint8_t Vx = (uint8_t)((Inst_Reg >> 8) & 0x0F);
    uint8_t Vy = (uint8_t)((Inst_Reg >> 4) & 0x0F);
    uint8_t Xpos = V0VF_Registers[Vx] % 64;
    uint8_t Ypos = V0VF_Registers[Vy] % 32;
    uint8_t sprite_height = (uint8_t)(Inst_Reg & 0x0F);
    V0VF_Registers[0xF] = 0;

    for (uint8_t row_index = 0; row_index < sprite_height; row_index++) {
        uint8_t sprite_byte = Memory[Index_REG + row_index];
        for (int sprite_index = 0; sprite_index < 8; sprite_index++) {
            uint8_t sprite_pixel = (sprite_byte >> (7 - sprite_index)) & 0x01;
            uint32_t displayPixel = Display[Ypos + row_index][Xpos + sprite_index];
            if (sprite_pixel) {
                if (displayPixel == 0xFFFFFFFF) {
                    V0VF_Registers[0xF] = 0x1;
                }
                Display[Ypos + row_index][Xpos + sprite_index] ^= 0xFFFFFFFF;
            }
        }
    }
}


void CHIP_8::MC_EX9E() {
    uint8_t Vx = (uint8_t)((Inst_Reg >> 8) & 0x0F);
    uint8_t key = V0VF_Registers[Vx];

    if (keypad[key]) {
        PC += 2;
    }
}

void CHIP_8::MC_EXA1() {
    uint8_t Vx = (uint8_t)((Inst_Reg >> 8) & 0x0F);
    if (!keypad[V0VF_Registers[Vx]]) {
        PC += 2;
    }
}

void CHIP_8::MC_FX07() {
    uint8_t Vx = (uint8_t)((Inst_Reg >> 8) & 0x0F);
    V0VF_Registers[Vx] = Delay_Timer;
}


void CHIP_8::MC_FX0A() {
    uint8_t Vx = (uint8_t)((Inst_Reg >> 8) & 0x0F);
    if (keypad[0])
    {
        V0VF_Registers[Vx] = 0;
    }
    else if (keypad[1])
    {
        V0VF_Registers[Vx] = 1;
    }
    else if (keypad[2])
    {
        V0VF_Registers[Vx] = 2;
    }
    else if (keypad[3])
    {
        V0VF_Registers[Vx] = 3;
    }
    else if (keypad[4])
    {
        V0VF_Registers[Vx] = 4;
    }
    else if (keypad[5])
    {
        V0VF_Registers[Vx] = 5;
    }
    else if (keypad[6])
    {
        V0VF_Registers[Vx] = 6;
    }
    else if (keypad[7])
    {
        V0VF_Registers[Vx] = 7;
    }
    else if (keypad[8])
    {
        V0VF_Registers[Vx] = 8;
    }
    else if (keypad[9])
    {
        V0VF_Registers[Vx] = 9;
    }
    else if (keypad[10])
    {
        V0VF_Registers[Vx] = 10;
    }
    else if (keypad[11])
    {
        V0VF_Registers[Vx] = 11;
    }
    else if (keypad[12])
    {
        V0VF_Registers[Vx] = 12;
    }
    else if (keypad[13])
    {
        V0VF_Registers[Vx] = 13;
    }
    else if (keypad[14])
    {
        V0VF_Registers[Vx] = 14;
    }
    else if (keypad[15])
    {
        V0VF_Registers[Vx] = 15;
    }
    else
    {
        PC -= 2;
    }

}

void CHIP_8::MC_FX15() {
    uint8_t Vx = (uint8_t)((Inst_Reg >> 8) & 0x0F);
    Delay_Timer = V0VF_Registers[Vx];
}

void CHIP_8::MC_FX18() {
    uint8_t Vx = (uint8_t)((Inst_Reg >> 8) & 0x0F);
    Sound_Timer = V0VF_Registers[Vx];
}

void CHIP_8::MC_FX1E() {
    uint8_t Vx = (uint8_t)((Inst_Reg >> 8) & 0x0F);
    Index_REG += V0VF_Registers[Vx];
}

void CHIP_8::MC_FX29() {
    //Vx stores the digit that we want so we can take it as an offset
    uint8_t Vx = (uint8_t)((Inst_Reg >> 8) & 0x0F);
    Index_REG = FONTSET_START_ADDRESS + (V0VF_Registers[Vx] * 5);
}

void CHIP_8::MC_FX33() {
    uint8_t Vx = (uint8_t)((Inst_Reg >> 8) & 0x0F);
    uint8_t temp = V0VF_Registers[Vx];
    /*
        The interpreter takes the decimal value of Vx,
        and places the hundreds digit in memory at location in I,
        the tens digit at location I+1, and the ones digit at location I+2.
    */
    Memory[Index_REG + 2] = temp % 10;
    temp /= 10;
    Memory[Index_REG + 1] = temp % 10;
    temp /= 10;
    Memory[Index_REG + 2] = temp;
}

void CHIP_8::MC_FX55() {
    uint8_t Vx = (uint8_t)((Inst_Reg >> 8) & 0x0F);
    for (uint8_t i = 0; i <= Vx; i++) {
        Memory[Index_REG + i] = V0VF_Registers[i];
    }
}

void CHIP_8::MC_FX65() {
    uint8_t Vx = (uint8_t)((Inst_Reg >> 8) & 0x0F);
    for (uint8_t i = 0; i <= Vx; i++) {
        V0VF_Registers[i] = Memory[Index_REG + i];
    }
}

//////////////////////// Decoder functions ///////////////////////////////////////
void CHIP_8::Table0()
{
    ((*this).*(table0[Inst_Reg & 0x000Fu]))();
}

void CHIP_8::Table8()
{
    ((*this).*(table8[Inst_Reg & 0x000Fu]))();
}

void CHIP_8::TableE()
{
    ((*this).*(tableE[Inst_Reg & 0x000Fu]))();
}

void CHIP_8::TableF()
{
    ((*this).*(tableF[Inst_Reg & 0x00FFu]))();
}

void CHIP_8::OP_NULL()
{}


////////////////////////// CPU Cycle Function /////////////////////////////////
void CHIP_8::Cycle()
{
    // Fetch
    Inst_Reg = (Memory[PC] << 8u) | Memory[PC + 1];

    // Increment the PC before we execute anything
    PC += 2;

    // Decode and Execute
    ((*this).*(table[(Inst_Reg & 0xF000u) >> 12u]))();

    // Decrement the delay timer if it's been set
    if (Delay_Timer > 0)
    {
        --Delay_Timer;
    }

    // Decrement the sound timer if it's been set
    if (Sound_Timer > 0) {
        if (Sound_Timer == 1) {
            SDL_QueueAudio(deviceId, wavBuffer, wavLength);
            SDL_PauseAudioDevice(deviceId, 0);
        }
        --Sound_Timer;
    }
}