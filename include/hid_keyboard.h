#pragma once

#include <stdint.h>

typedef struct {
	uint8_t modifiers;
	uint8_t reserved;
	uint8_t keys[6];
} HidKeyboardReport;

#ifdef __cplusplus
extern "C" {
#endif

void hid_keyboard_init(void);
void hid_keyboard_send_report(const HidKeyboardReport *report);
void hid_keyboard_task(void);

#ifdef __cplusplus
}
#endif
