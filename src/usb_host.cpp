#include <cstdio>
#include <cstdint>

#include "pico/stdlib.h"
#include "pio_usb.h"
#include "tusb.h"

namespace {

constexpr size_t kMidiPacketSize = 4;
constexpr uint8_t kAudioClass = 0x01;
constexpr uint8_t kMidiStreamingSubclass = 0x03;
constexpr uint8_t kBulkTransfer = 2;
constexpr uint8_t kEndpointDirectionIn = 0x80;
constexpr uint8_t kMidiStatusNoteOn = 0x90;
constexpr uint8_t kMidiStatusNoteOff = 0x80;
constexpr uint8_t kMidiCinNoteOff = 0x08;
constexpr uint8_t kMidiCinNoteOn = 0x09;

struct MidiPacketRingBuffer {
    static constexpr size_t kCapacity = 32;
    uint8_t data[kCapacity][kMidiPacketSize]{};
    size_t head = 0;
    size_t tail = 0;

    bool push(const uint8_t packet[kMidiPacketSize]) {
        const size_t next = (head + 1) % kCapacity;
        if (next == tail) {
            return false;
        }

        for (size_t i = 0; i < kMidiPacketSize; ++i) {
            data[head][i] = packet[i];
        }
        head = next;
        return true;
    }

    bool pop(uint8_t packet[kMidiPacketSize]) {
        if (head == tail) {
            return false;
        }

        for (size_t i = 0; i < kMidiPacketSize; ++i) {
            packet[i] = data[tail][i];
        }
        tail = (tail + 1) % kCapacity;
        return true;
    }
};

MidiPacketRingBuffer g_midi_ring;
uint8_t g_midi_bulk_in_ep = 0;
uint8_t g_midi_daddr = 0;
uint8_t g_midi_rx_buf[64]{};
uint8_t g_config_desc_buf[512]{};
tuh_xfer_t g_midi_xfer{};

void dump_hex_bytes(const char *label, const uint8_t *data, size_t len) {
    std::printf("%s", label);
    for (size_t i = 0; i < len; ++i) {
        std::printf(" %02X", data[i]);
    }
    std::printf("\n");
}

bool is_midi_streaming_interface(const tusb_desc_interface_t &itf) {
    return (itf.bInterfaceClass == kAudioClass) &&
           (itf.bInterfaceSubClass == kMidiStreamingSubclass);
}

bool is_bulk_in_endpoint(const tusb_desc_endpoint_t &ep) {
    const uint8_t endpoint_address = ep.bEndpointAddress;
    const uint8_t transfer_type = ep.bmAttributes.xfer;
    return ((endpoint_address & kEndpointDirectionIn) != 0) &&
           (transfer_type == kBulkTransfer);
}

void parse_midi_packet(const uint8_t packet[kMidiPacketSize]) {
    const uint8_t cin = packet[0] & 0x0F;
    const uint8_t status = packet[1];
    const uint8_t data1 = packet[2];
    const uint8_t data2 = packet[3];

    if ((cin == kMidiCinNoteOn || cin == kMidiCinNoteOff) &&
        ((status & 0xF0) == kMidiStatusNoteOn || (status & 0xF0) == kMidiStatusNoteOff)) {
        std::printf("USB MIDI: note %s ch=%u note=%u vel=%u\n",
                    ((status & 0xF0) == kMidiStatusNoteOn) ? "ON" : "OFF",
                    static_cast<unsigned>(status & 0x0F),
                    static_cast<unsigned>(data1),
                    static_cast<unsigned>(data2));
    } else {
        std::printf("USB MIDI: status=0x%02X cin=%u data1=%u data2=%u\n",
                    status,
                    static_cast<unsigned>(cin),
                    static_cast<unsigned>(data1),
                    static_cast<unsigned>(data2));
    }

    dump_hex_bytes("USB MIDI raw:", packet, kMidiPacketSize);
}

void midi_bulk_in_cb(tuh_xfer_t *xfer);

bool arm_midi_bulk_in_transfer(uint8_t daddr, uint8_t endpoint_address) {
    g_midi_xfer = {};
    g_midi_xfer.daddr = daddr;
    g_midi_xfer.ep_addr = endpoint_address;
    g_midi_xfer.buffer = g_midi_rx_buf;
    g_midi_xfer.buflen = sizeof(g_midi_rx_buf);
    g_midi_xfer.complete_cb = midi_bulk_in_cb;
    return tuh_edpt_xfer(&g_midi_xfer);
}

void midi_bulk_in_cb(tuh_xfer_t *xfer) {
    if (xfer == nullptr) {
        std::printf("USB MIDI: null transfer callback\n");
        return;
    }

    if (xfer->result != XFER_RESULT_SUCCESS) {
        std::printf("USB MIDI: RX error daddr=%u result=%d\n", xfer->daddr, static_cast<int>(xfer->result));
        return;
    }

    if (xfer->actual_len > 0) {
        const size_t packet_count = xfer->actual_len / kMidiPacketSize;
        bool has_non_realtime_packet = false;
        for (size_t packet_index = 0; packet_index < packet_count; ++packet_index) {
            const uint8_t *packet = &g_midi_rx_buf[packet_index * kMidiPacketSize];
            if (packet[0] == 0 && packet[1] == 0 && packet[2] == 0 && packet[3] == 0) {
                continue;
            }

            const uint8_t cin = packet[0] & 0x0F;
            if (cin == 0x0F && packet[1] >= 0xF8) {
                continue;
            }

            has_non_realtime_packet = true;
            if (g_midi_ring.push(packet)) {
                parse_midi_packet(packet);
            } else {
                std::printf("USB MIDI: packet queue full, dropping event\n");
            }
        }

        if (has_non_realtime_packet) {
            std::printf("USB MIDI: received %u bytes\n", static_cast<unsigned>(xfer->actual_len));
            dump_hex_bytes("USB MIDI transfer:", g_midi_rx_buf, xfer->actual_len);
        }

        if ((xfer->actual_len % kMidiPacketSize) != 0) {
            std::printf("USB MIDI: ignoring %u trailing bytes\n",
                        static_cast<unsigned>(xfer->actual_len % kMidiPacketSize));
        }
    }

    if (g_midi_bulk_in_ep != 0) {
        if (!arm_midi_bulk_in_transfer(xfer->daddr, xfer->ep_addr)) {
            std::printf("USB host: failed to re-arm MIDI bulk IN transfer\n");
        }
    }
}

void open_midi_bulk_in_endpoint(uint8_t daddr, const tusb_desc_endpoint_t &ep_desc) {
    g_midi_daddr = daddr;
    g_midi_bulk_in_ep = ep_desc.bEndpointAddress;

    if (!tuh_edpt_open(daddr, &ep_desc)) {
        std::printf("USB host: failed to open MIDI bulk IN endpoint 0x%02X\n", ep_desc.bEndpointAddress);
        return;
    }

    std::printf("USB host: opened MIDI bulk IN endpoint 0x%02X for daddr=%u\n",
                ep_desc.bEndpointAddress, daddr);

    if (!arm_midi_bulk_in_transfer(daddr, ep_desc.bEndpointAddress)) {
        std::printf("USB host: failed to start MIDI bulk IN transfer\n");
    }
}

void scan_configuration_for_midi_endpoints(uint8_t daddr, const uint8_t *config_desc, uint16_t total_len) {
    const uint8_t *cursor = config_desc;
    const uint8_t *end = config_desc + total_len;
    bool in_midi_interface = false;
    uint8_t midi_interface_number = 0;

    while (cursor + 1 < end) {
        const uint8_t descriptor_length = cursor[0];
        const uint8_t descriptor_type = cursor[1];

        if (descriptor_length == 0 || cursor + descriptor_length > end) {
            break;
        }

        if (descriptor_type == TUSB_DESC_INTERFACE) {
            const auto *itf = reinterpret_cast<const tusb_desc_interface_t *>(cursor);
            in_midi_interface = is_midi_streaming_interface(*itf);
            if (in_midi_interface) {
                midi_interface_number = itf->bInterfaceNumber;
                std::printf("USB host: found MIDI Streaming interface #%u (class=0x%02X, subclass=0x%02X)\n",
                            midi_interface_number,
                            itf->bInterfaceClass,
                            itf->bInterfaceSubClass);
            }
        }
        else if (descriptor_type == TUSB_DESC_ENDPOINT && in_midi_interface) {
            const auto *ep = reinterpret_cast<const tusb_desc_endpoint_t *>(cursor);
            std::printf("USB host: interface #%u endpoint 0x%02X transfer=%u maxpkt=%u\n",
                        midi_interface_number,
                        ep->bEndpointAddress,
                        static_cast<unsigned>(ep->bmAttributes.xfer),
                        static_cast<unsigned>(ep->wMaxPacketSize));

            if (is_bulk_in_endpoint(*ep)) {
                std::printf("USB host: candidate MIDI bulk IN endpoint found on daddr=%u\n", daddr);
                open_midi_bulk_in_endpoint(daddr, *ep);
                break;
            }
        }

        cursor += descriptor_length;
    }
}

void midi_configuration_callback(tuh_xfer_t *xfer) {
    if (xfer == nullptr) {
        std::printf("USB host: null config descriptor callback\n");
        return;
    }

    if (xfer->result != XFER_RESULT_SUCCESS) {
        std::printf("USB host: config descriptor fetch failed for daddr=%u, result=%d\n",
                    xfer->daddr,
                    static_cast<int>(xfer->result));
        return;
    }

    std::printf("USB host: config descriptor length=%lu for daddr=%u\n",
                static_cast<unsigned long>(xfer->actual_len),
                xfer->daddr);

    scan_configuration_for_midi_endpoints(xfer->daddr,
                                          g_config_desc_buf,
                                          xfer->actual_len);
}

}  // namespace

extern "C" bool midi_host_read_packet(uint8_t *packet) {
    return g_midi_ring.pop(packet);
}

extern "C" void tuh_mount_cb(uint8_t daddr) {
    std::printf("USB host: device mounted, daddr=%u\n", daddr);

    if (!tuh_descriptor_get_configuration(daddr,
                                          0,
                                          g_config_desc_buf,
                                          sizeof(g_config_desc_buf),
                                          midi_configuration_callback,
                                          0)) {
        std::printf("USB host: failed to request config descriptor for daddr=%u\n", daddr);
    }
}

extern "C" void tuh_umount_cb(uint8_t daddr) {
    std::printf("USB host: device unmounted, daddr=%u\n", daddr);
    g_midi_bulk_in_ep = 0;
    g_midi_daddr = 0;
}

extern "C" void usb_host_task(void) {
    tuh_task();
}

extern "C" bool usb_host_init(void) {
    std::printf("USB host: init start\n");

    pio_usb_configuration_t pio_cfg = PIO_USB_DEFAULT_CONFIG;
    pio_cfg.pin_dp = 0;
    pio_cfg.pinout = PIO_USB_PINOUT_DPDM;

    tuh_configure(1, TUH_CFGID_RPI_PIO_USB_CONFIGURATION, &pio_cfg);
    return tuh_init(1);
}

extern "C" void usb_host_poll(void) {
    tuh_task();
}

extern "C" void usb_midi_debug_log(const uint8_t packet[kMidiPacketSize]) {
    dump_hex_bytes("USB MIDI debug:", packet, kMidiPacketSize);
}
