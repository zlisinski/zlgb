#pragma once

#include "gbemu.h"
#include "IoRegisterProxy.h"
#include "Interrupt.h"
#include "TimerObserver.h"

class DisplayInterface;
class Memory;

const uint SCREEN_X = 160;
const uint SCREEN_Y = 144;

class Display : public IoRegisterProxy, public TimerObserver
{
public:
    Display(Memory *memory, Interrupt *interrupts, DisplayInterface *displayInterface, TimerSubject *timerSubject);
    virtual ~Display();

    bool SaveState(FILE *file);
    bool LoadState(uint16_t version, FILE *file);

    // Inherited from IoRegisterProxy.
    virtual bool WriteByte(uint16_t address, uint8_t byte);
    virtual uint8_t ReadByte(uint16_t address) const;

    // Inherited from TimerObserver.
    virtual void UpdateTimer(uint value);

private:
    enum DisplayModes
    {
        eMode0HBlank = 0,
        eMode1VBlank = 1,
        eMode2SearchingOAM = 2,
        eMode3TranferData = 3,
    };

    struct SpriteData
    {
        int16_t x;
        uint8_t i;
    };

    uint16_t GetMode3ClockCount(uint8_t scanline);
    void SetMode(DisplayModes mode);
    void UpdateScanline();

    bool GetStatCheck();

    void DrawScanline(uint8_t scanline);
    void DrawBackgroundScanline(uint8_t scanline, uint8_t scrollX, uint8_t scrollY);
    void DrawWindowScanline(uint8_t scanline, uint8_t windowX, uint8_t windowY);
    void DrawSprites(uint8_t scanline);
    void DrawTileLine(uint8_t byte1, uint8_t byte2, uint8_t xPos, uint8_t yPos, uint8_t paletteReg, bool flipX, bool bgPriority, bool isBg);

    void DrawScreen();

    static bool SpriteSort(const SpriteData &a, const SpriteData &b);

    Memory *memory;
    Interrupt *interrupts;

    uint8_t *regLCDC;  // 0xFF40 LCD control
    uint8_t *regSTAT;  // 0xFF41 LCDC status
    uint8_t *regSCY;   // 0xFF42 Scroll Y
    uint8_t *regSCX;   // 0xFF43 Scroll X
    uint8_t *regLY;    // 0xFF44 LCDC Y coordinate
    uint8_t *regLYC;   // 0xFF45 LY compare
    uint8_t *regBGP;   // 0xFF47 BG & window palette data
    uint8_t *regOBP0;  // 0xFF48 Object palette 0 data
    uint8_t *regOBP1;  // 0xFF49 Object palette 1 data
    uint8_t *regWY;    // 0xFF4A Window Y position
    uint8_t *regWX;    // 0xFF4B Window X position

    DisplayModes displayMode;
    uint16_t mode3Clocks;

    uint32_t frameBuffer[SCREEN_X * SCREEN_Y];
    uint8_t bgColorMap[SCREEN_X * SCREEN_Y];

    uint16_t counter;

    DisplayInterface *displayInterface;
};