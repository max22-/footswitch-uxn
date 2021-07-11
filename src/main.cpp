#include "footswitch_rom.h"
#include <Arduino.h>

extern "C" {
#include <uxn.h>
}

#define DEBUG
//#define DEBUG(args...) Serial.printf(args)

Uxn *u;
static Device *devconsole, *devtime;

void error(const char *msg, const char *err) {
  DEBUG("Error %s: %s\n", msg, err);
  for (;;)
    ;
}

static void system_talk(Device *d, Uint8 b0, Uint8 w) {
  if (!w) {
    d->dat[0x2] = d->u->wst.ptr;
    d->dat[0x3] = d->u->rst.ptr;
  } /*else {
          putcolors(&ppu, &d->dat[0x8]);
          reqdraw = 1;
  }*/
  (void)b0;
}

static void pin_talk(Device *d, Uint8 b0, Uint8 w) {
  if (b0 == 3)
    pinMode(d->dat[0x2], d->dat[0x3]);
  else if (b0 == 5 && w)
    digitalWrite(d->dat[0x2], d->dat[0x5]);
  else if (b0 == 4 && !w)
    d->dat[0x4] = digitalRead(d->dat[0x2]);
  (void)b0;
}

static void clock_talk(Device *d, Uint8 b0, Uint8 w) {
  if (!w && b0 == 2)
    mempoke16(d->dat, 0x02, (Uint16)millis());
}

static void console_talk(Device *d, Uint8 b0, Uint8 w) {
  if (w && b0 == 8)
    Serial.write(d->dat[8]);
}

static void nil_talk(Device *d, Uint8 b0, Uint8 w) {
  (void)d;
  (void)b0;
  (void)w;
}

int loaduxn_from_flash(Uxn *u, void *rom, size_t size) {
  FILE *f;
  if (rom == nullptr)
    error("Rom", "Invalid source address.");
  memcpy(u->ram.dat + PAGE_PROGRAM, rom, size);
  DEBUG("Uxn loaded[0x%x].\n", (unsigned int)rom);
  return 1;
}

void setup() {
  Serial.begin(31250);
  if ((u = (Uxn *)malloc(sizeof(Uxn))) == nullptr)
    error("Memory", "not enough");
  if (!bootuxn(u))
    error("Boot", "Failed");
  if (!loaduxn_from_flash(u, footswitch_rom, sizeof(footswitch_rom)))
    return error("Load", "Failed");

  portuxn(u, 0x0, (char *)"system", system_talk);
  devconsole = portuxn(u, 0x1, (char *)"console", console_talk);
  portuxn(u, 0x2, (char *)"---", nil_talk);
  portuxn(u, 0x3, (char *)"---", nil_talk);
  portuxn(u, 0x4, (char *)"---", nil_talk);
  portuxn(u, 0x5, (char *)"---", nil_talk);
  portuxn(u, 0x6, (char *)"---", nil_talk);
  portuxn(u, 0x7, (char *)"---", nil_talk);
  portuxn(u, 0x8, (char *)"---", nil_talk);
  portuxn(u, 0x9, (char *)"---", nil_talk);
  portuxn(u, 0xa, (char *)"---", nil_talk);
  portuxn(u, 0xb, (char *)"---", nil_talk);
  portuxn(u, 0xc, (char *)"pin", pin_talk);
  devtime = portuxn(u, 0xd, (char *)"clock", clock_talk);
  portuxn(u, 0xe, (char *)"---", nil_talk);
  portuxn(u, 0xf, (char *)"---", nil_talk);
  evaluxn(u, 0x0100);
}

void loop() {
	if(!evaluxn(u, mempeek16(devtime->dat, 0)))
		error("Eval", "error");
  
}