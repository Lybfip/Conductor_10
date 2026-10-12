#include "keymap.h"

namespace {

constexpr uint8_t kFirstMidiNote = 48;
constexpr uint8_t kLastMidiNote = 84;
constexpr uint8_t kLeftShiftModifier = 0x02;

constexpr MidiNoteHidKey ascii_to_hid_key(char key) {
	if (key >= 'a' && key <= 'z') {
		return {static_cast<uint8_t>(0x04 + key - 'a'), 0};
	}
	if (key >= '1' && key <= '9') {
		return {static_cast<uint8_t>(0x1E + key - '1'), 0};
	}
	if (key == '0') {
		return {0x27, 0};
	}
	if (key == '-') {
		return {0x2D, 0};
	}
	if (key == '[') {
		return {0x30, 0};
	}
	if (key == '^') {
		return {0x2E, 0};
	}
	if (key == ';') {
		return {0x33, 0};
	}
	if (key == '\\') {
		return {0x87, 0};
	}
	if (key == ':') {
		return {0x34, 0};
	}
	if (key == ']') {
		return {0x31, 0};
	}
	return {0, 0};
}

// C3 (48) to C6 (84): a-z, 1-0, - on a Japanese keyboard layout.
constexpr MidiNoteHidKey kMidiNoteToHidKey[] = {
	ascii_to_hid_key('z'), ascii_to_hid_key('s'),   // C3 (48) and C#3 (49)
    ascii_to_hid_key('x'), ascii_to_hid_key('d'),   // D3 (50) and D#3 (51)
    ascii_to_hid_key('c'),                          // E3 (52)                         
    ascii_to_hid_key('v'), ascii_to_hid_key('g'),   // F3 (53) and F#3 (54)
    ascii_to_hid_key('b'), ascii_to_hid_key('h'),   // G3 (55) and G#3 (56)
	ascii_to_hid_key('n'), ascii_to_hid_key('j'),   // A3 (57) and A#3 (58)
    ascii_to_hid_key('m'),                          // B3 (59)
	ascii_to_hid_key('q'), ascii_to_hid_key('2'),   // C4 (60) and C#4 (61)
    ascii_to_hid_key('w'), ascii_to_hid_key('3'),   // D4 (62) and D#4 (63)
    ascii_to_hid_key('e'),                          // E4 (64)
    ascii_to_hid_key('r'), ascii_to_hid_key('5'),   // F4 (65) and F#4 (66)
    ascii_to_hid_key('t'), ascii_to_hid_key('6'),   // G4 (67) and G#4 (68)
	ascii_to_hid_key('y'), ascii_to_hid_key('7'),   // A4 (69) and A#4 (70)
    ascii_to_hid_key('u'),                          // B4 (71)
	ascii_to_hid_key('i'), ascii_to_hid_key('9'),   // C5 (72) and C#5 (73)
    ascii_to_hid_key('o'), ascii_to_hid_key('0'),   // D5 (74) and D#5 (75)
    ascii_to_hid_key('p'),                          // E5 (76)
    ascii_to_hid_key('k'), ascii_to_hid_key('-'),   // F5 (77) and F#5 (78)
    ascii_to_hid_key('l'), ascii_to_hid_key('^'),   // G5 (79) and G#5 (80)
	ascii_to_hid_key(';'), ascii_to_hid_key('\\'),  // A5 (81) and A#5 (82)
    ascii_to_hid_key(':'),                          // B5 (83)
	ascii_to_hid_key(']'),                          // C6 (84) 
};

static_assert(sizeof(kMidiNoteToHidKey) / sizeof(kMidiNoteToHidKey[0]) ==
				  kLastMidiNote - kFirstMidiNote + 1,
			  "MIDI note map must cover notes 48 through 84");

}  // namespace

MidiNoteHidKey midi_note_to_hid_key(uint8_t note) {
	constexpr int keyCount = sizeof(kMidiNoteToHidKey) / sizeof(kMidiNoteToHidKey[0]);
	int index = (static_cast<int>(note) - kFirstMidiNote) % keyCount;
	if (index < 0) {
		index += keyCount;
	}

	return kMidiNoteToHidKey[index];
}
