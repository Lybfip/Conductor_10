#include <cstring>

#include "hid_keyboard.h"
#include "tusb.h"

namespace {

constexpr uint8_t kKeyboardInterface = 0;
constexpr uint8_t kKeyboardEndpoint = 0x81;

enum : uint8_t {
	kStringLanguage = 0,
	kStringManufacturer,
	kStringProduct,
	kStringSerial,
};

tusb_desc_device_t const kDeviceDescriptor = {
	.bLength = sizeof(tusb_desc_device_t),
	.bDescriptorType = TUSB_DESC_DEVICE,
	.bcdUSB = 0x0200,
	.bDeviceClass = 0,
	.bDeviceSubClass = 0,
	.bDeviceProtocol = 0,
	.bMaxPacketSize0 = CFG_TUD_ENDPOINT0_SIZE,
	.idVendor = 0xCafe,
	.idProduct = 0x4001,
	.bcdDevice = 0x0100,
	.iManufacturer = kStringManufacturer,
	.iProduct = kStringProduct,
	.iSerialNumber = kStringSerial,
	.bNumConfigurations = 1,
};

uint8_t const kKeyboardReportDescriptor[] = {
	TUD_HID_REPORT_DESC_KEYBOARD()
};

constexpr uint16_t kConfigurationLength = TUD_CONFIG_DESC_LEN + TUD_HID_DESC_LEN;
uint8_t const kConfigurationDescriptor[] = {
	TUD_CONFIG_DESCRIPTOR(1, 1, 0, kConfigurationLength, TUSB_DESC_CONFIG_ATT_REMOTE_WAKEUP, 100),
	TUD_HID_DESCRIPTOR(kKeyboardInterface, 0, HID_ITF_PROTOCOL_KEYBOARD,
					   sizeof(kKeyboardReportDescriptor), kKeyboardEndpoint,
					   CFG_TUD_HID_EP_BUFSIZE, 10),
};

char const *const kStringDescriptors[] = {
	nullptr,
	"Raspberry Pi",
	"MIDI to HID Keyboard",
	"0001",
};

uint16_t g_string_descriptor[32];
HidKeyboardReport g_pending_report{};
bool g_report_pending = false;

}  // namespace

extern "C" void hid_keyboard_init(void) {
	tud_init(0);
}

extern "C" void hid_keyboard_send_report(const HidKeyboardReport *report) {
	if (report == nullptr) {
		return;
	}

	g_pending_report = *report;
	g_report_pending = true;
}

extern "C" void hid_keyboard_task(void) {
	tud_task();

	if (g_report_pending && tud_hid_ready()) {
		if (tud_hid_report(0, &g_pending_report, sizeof(g_pending_report))) {
			g_report_pending = false;
		}
	}
}

extern "C" uint8_t const *tud_descriptor_device_cb(void) {
	return reinterpret_cast<uint8_t const *>(&kDeviceDescriptor);
}

extern "C" uint8_t const *tud_hid_descriptor_report_cb(uint8_t instance) {
	(void) instance;
	return kKeyboardReportDescriptor;
}

extern "C" uint8_t const *tud_descriptor_configuration_cb(uint8_t index) {
	(void) index;
	return kConfigurationDescriptor;
}

extern "C" uint16_t const *tud_descriptor_string_cb(uint8_t index, uint16_t langid) {
	(void) langid;

	if (index == kStringLanguage) {
		g_string_descriptor[1] = 0x0409;
		g_string_descriptor[0] = (TUSB_DESC_STRING << 8) | 4;
		return g_string_descriptor;
	}

	if (index >= TU_ARRAY_SIZE(kStringDescriptors)) {
		return nullptr;
	}

	char const *const string = kStringDescriptors[index];
	const size_t length = std::strlen(string);
	const size_t descriptor_length = (length < 31) ? length : 31;

	for (size_t offset = 0; offset < descriptor_length; ++offset) {
		g_string_descriptor[offset + 1] = string[offset];
	}
	g_string_descriptor[0] = (TUSB_DESC_STRING << 8) | (descriptor_length * 2 + 2);
	return g_string_descriptor;
}

extern "C" uint16_t tud_hid_get_report_cb(uint8_t instance, uint8_t report_id,
											 hid_report_type_t report_type, uint8_t *buffer,
											 uint16_t request_length) {
	(void) instance;
	(void) report_id;
	(void) report_type;
	(void) buffer;
	(void) request_length;
	return 0;
}

extern "C" void tud_hid_set_report_cb(uint8_t instance, uint8_t report_id,
										hid_report_type_t report_type, uint8_t const *buffer,
										uint16_t buffer_size) {
	(void) instance;
	(void) report_id;
	(void) report_type;
	(void) buffer;
	(void) buffer_size;
}
