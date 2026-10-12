#pragma once

#include <stdint.h>

typedef struct {
	uint8_t key;
	uint8_t modifiers;
} MidiNoteHidKey;

MidiNoteHidKey midi_note_to_hid_key(uint8_t note);
