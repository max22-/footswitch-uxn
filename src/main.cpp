#include "footswitch_rom.h"
#include <Arduino.h>

extern "C" {
#include <uxn.h>
}

Uxn *u;
static Device *devconsole, *devtime;

void error(const char *msg, const char *err) {
  Serial.printf("Error %s: %s\n", msg, err);
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
  //Serial.printf("pin_talk(*d, %d, %d)\n", b0, w);
  if (b0 == 3)
    pinMode(d->dat[0x2], d->dat[0x3]);
  else if (b0 == 5 && w) {
    digitalWrite(d->dat[0x2], d->dat[0x5]);
    //Serial.printf("digitalWrite(%d, %s)\n", d->dat[2], d->dat[5] ? "HIGH" : "LOW");
  }
  else if (b0 == 4 && !w)
	  d->dat[0x4] = digitalRead(d->dat[0x2]);
  (void)b0;
}

static void time_talk(Device *d, Uint8 b0, Uint8 w) {
  if (!w && b0 == 2)
	  mempoke16(d->dat, 0x02, (Uint16)millis());
}

static void console_talk(Device *d, Uint8 b0, Uint8 w) {
  if (w && b0 == 8)
    Serial.write(d->dat[8]);
#warning Signed or unsigned char ???
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
  fprintf(stderr, "Uxn loaded[0x%x].\n", (unsigned int)rom);
  return 1;
}

int evaluxn_debug(Uxn *u, Uint16 vec) {
  u->ram.ptr = vec;
  u->wst.error = 0;
  u->rst.error = 0;
  Serial.printf("*** eval(%x) ***\n", vec);
  while (u->ram.ptr) {
    Serial.println("* step");
    Serial.println("  Stack :");
    for (int i = 0; i < 20; i++)
      Serial.printf("%02x ", u->wst.dat[i]);
    Serial.println();
    for (int i = 0; i < 20; i++)
      if (i == u->wst.ptr)
        Serial.print("^  ");
      else
        Serial.print("   ");
    Serial.println();
    if (!stepuxn(u, u->ram.dat[u->ram.ptr++]))
      return 0;
    delay(1000);
  }
  Serial.printf("*** end eval ***");
  delay(5000);
  for (;;)
    ;
  return 1;
}

void setup() {
  Serial.begin(115200);
  for (int i = 5; i >= 0; i--) {
    Serial.println(i);
    delay(1000);
  }
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
  devtime = portuxn(u, 0xd, (char *)"time", time_talk);
  portuxn(u, 0xe, (char *)"---", nil_talk);
  portuxn(u, 0xf, (char *)"---", nil_talk);
  evaluxn(u, 0x0100);
}

void loop() {
  static uint32_t oldMillis = millis();
  uint32_t newMillis = millis();

  if (newMillis != oldMillis) {
    if (!evaluxn(u, mempeek16(devtime->dat, 0)))
      error("Eval", "error");
    oldMillis = newMillis;
  }
}