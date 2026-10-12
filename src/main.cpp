#include <stdint.h>

#include "hid_keyboard.h"
#include "keymap.h"
#include "hardware/clocks.h"
#include "pico/stdlib.h"

// USBホストスタックまたはHIDキーボード実装によって提供される可能性のある、弱結合（weakly linked）関数。
namespace {

constexpr size_t kUsbMidiPacketSize = 4;		// USB MIDIパケットのサイズ
constexpr size_t kHidKeyCount = 6;				// HIDキーボードレポート内の最大キー数
constexpr size_t kMidiNoteCount = 128;

// MIDIパケットを読み取る関数（弱結合）
extern "C" bool midi_host_read_packet(uint8_t *packet)
	__attribute__((weak));

// USBホストタスク（弱結合）
extern "C" void usb_host_task(void) __attribute__((weak));

// USBホスト初期化（弱結合）
extern "C" bool usb_host_init(void) __attribute__((weak));

// HIDキーボードレポートに指定されたキーが含まれているかを確認する関数
bool report_contains_key(const HidKeyboardReport &report, uint8_t key) {
	for (uint8_t current : report.keys) {
		if (current == key) {
			return true;
		}
	}
	return false;
}

// HIDキーボードレポートにキーを追加する関数
void add_key(HidKeyboardReport &report, uint8_t key) {
	if (key == 0 || report_contains_key(report, key)) {
		return;
	}

	for (uint8_t &current : report.keys) {
		if (current == 0) {
			current = key;
			return;
		}
	}
}

void rebuild_report(const bool (&active_notes)[kMidiNoteCount], HidKeyboardReport &report) {
	report = {};
	for (size_t note = 0; note < kMidiNoteCount; ++note) {
		if (!active_notes[note]) {
			continue;
		}

		const MidiNoteHidKey mapping = midi_note_to_hid_key(static_cast<uint8_t>(note));
		report.modifiers |= mapping.modifiers;
		add_key(report, mapping.key);
	}
}

// MIDIパケットを処理し、HIDキーボードレポートを更新する関数
void process_midi_packet(const uint8_t *packet, bool (&active_notes)[kMidiNoteCount],
						 HidKeyboardReport &report) {
	const uint8_t status = packet[1];
	const uint8_t data1 = packet[2];
	const uint8_t data2 = packet[3];
	const uint8_t message = status & 0xF0;

	if (message == 0x90 && data2 != 0) {
		active_notes[data1] = true;
	} else if (message == 0x80 || (message == 0x90 && data2 == 0)) {
		active_notes[data1] = false;
	} else {
		return;
	}

	rebuild_report(active_notes, report);
	hid_keyboard_send_report(&report);
}

}  // namespace

// メイン関数
int main() {
	// PIO USBのフルスピード動作には12MHzの倍数のシステムクロックが必要。
	set_sys_clock_khz(120000, true);

	stdio_init_all();		// 標準入出力の初期化
	hid_keyboard_init();

	HidKeyboardReport report{};		// HIDキーボードレポートの初期化
	bool active_notes[kMidiNoteCount]{};
	uint8_t packet[kUsbMidiPacketSize]{};		// MIDIパケットの初期化

	if (usb_host_init != nullptr) {
		usb_host_init();
	}

	// メインループ
	while (true) {
		if (usb_host_task != nullptr) {
			usb_host_task();
		}

		// MIDIパケットを読み取り、処理する
		if (midi_host_read_packet != nullptr && midi_host_read_packet(packet)) {
			process_midi_packet(packet, active_notes, report);
		}


		// HIDキーボードタスクを実行する
		hid_keyboard_task();

		// ループの内容を最適化するために、tight_loop_contents()を呼び出す
		tight_loop_contents();
	}
}
