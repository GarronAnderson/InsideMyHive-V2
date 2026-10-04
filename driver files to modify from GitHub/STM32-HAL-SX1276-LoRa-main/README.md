# STM32-HAL-SX1276-LoRa

A bare-metal LoRa driver for the Semtech SX1276 written in C using the STM32 HAL library. Written from scratch against the SX1276 datasheet with no third-party dependencies beyond STM32 HAL.

> **Note:** Core TX and RX are functional, and the utility functions (sleep, standby, packet RSSI, packet SNR, version) are now implemented.

---

## Hardware

| Item | Detail |
|------|--------|
| Target MCU | STM32F405RGT6 |
| Radio Module | RFM95 (SX1276) |
| Interface | SPI (SPI2) |
| HAL | STM32 HAL |

---

## Compatibility

| Chip | Support |
|------|---------|
| SX1276 | Full |
| SX1279 | Full |
| SX1277 | SF6 to SF9 only, SF10/11/12 not supported on this variant |
| SX1278 | Frequencies below 525 MHz only |

---

## Features

- LoRa mode only, no FSK/OOK
- Single packet TX with TxDone polling
- Single packet RX with RxDone polling and CRC check
- Configurable spreading factor, bandwidth, coding rate, TX power, sync word, preamble length
- PA_BOOST and RFO pin support, including +20 dBm mode
- Hardware reset sequence on init
- SPI version register sanity check on init (reads REG_VERSION, expects 0x12)
- Utility helpers: sleep, standby, packet RSSI, packet SNR, version read

---

## File Structure

```
STM32-HAL-SX1276-LoRa/
├── LoRa.h        # Register defines, bitmasks, config struct, function prototypes
├── LoRa.c        # Driver implementation
├── examples/     # Example usage showing packet structure and TX/RX
└── README.md
```

---

## Examples

The `examples/` folder contains working code that demonstrates how to structure a packet and send/receive it with this driver.

> **Important:** The examples are written for a specific use case. They show how to build a packet structure and how to send and receive with guards in place, but the packet format is specific to that application. You will need to define your own packet structure to match your use case, or you can send and receive raw bytes with no guards. Use the examples as a structural reference, not as a drop-in.

---

## SPI Configuration (CubeMX)

Configure your SPI peripheral with these settings before using the driver:

| Setting | Value |
|---------|-------|
| Mode | Full-Duplex Master |
| CPOL | Low (0) |
| CPHA | 1 Edge (0), SPI Mode 0 |
| Data Size | 8 bits |
| First Bit | MSB First |
| NSS | Software |
| Baud Rate | <= 10 MHz |

NSS must be software-controlled and passed to the driver handle via the NSS GPIO port and pin; do not use hardware NSS.

---

## Usage

### 1. Define your hardware handle and config

```c
SX1276_HandleTypedef lora_handle = {
    .hspi     = &hspi2,
    .nss_port = NSS_GPIO_Port,
    .nss_pin  = NSS_Pin,
    .rst_port = RST_GPIO_Port,
    .rst_pin  = RST_Pin
};

LORA_CONFIG_TYPEDEF lora_config = {
    .frequency       = 915000000UL,  // 915 MHz
    .spreading_Factor = 7,
    .bandwidth       = 125000,
    .coding_Rate     = 5,            // 4/5
    .tx_Power        = 17,
    .use_Pa_Boost    = 1,
    .preamble_Length = 8,
    .sync_Word       = 0x12,         // private LoRa network
    .enable_CRC      = 1,
    .implicit_header = 0             // explicit header mode
};
```

### 2. Initialise

```c
if (lora_init(&lora_handle, &lora_config) != HAL_OK) {
    // handle error, SPI not working or chip not found
}
```

### 3. Transmit

```c
uint8_t payload[] = {0x01, 0x02, 0x03, 0x04};

if (lora_TX(payload, sizeof(payload), 5000) != HAL_OK) {
    // handle error or timeout
}
```

### 4. Receive

```c
uint8_t rx_buf[64];
uint8_t rx_len = 0;

HAL_StatusTypeDef result = lora_RX(rx_buf, &rx_len, sizeof(rx_buf), 5000);

if (result == HAL_OK) {
    // rx_buf contains rx_len bytes of received data
} else if (result == HAL_TIMEOUT) {
    // no packet received within 5000 ms
} else {
    // SPI error or CRC failure
}
```

### 5. Utility functions

All utility functions return `HAL_StatusTypeDef` and write their result through an output pointer.

```c
lora_sleep();      // enter sleep mode
lora_standby();    // enter standby mode

int16_t rssi = 0;
lora_packet_rssi(&rssi);   // RSSI of last packet in dBm

float snr = 0.0f;
lora_packet_snr(&snr);     // SNR of last packet in dB

uint8_t ver = 0;
lora_version(&ver);        // REG_VERSION, expect 0x12
```

---

## Init Sequence

The driver follows the sequence from the SX1276 datasheet section 4.1.3:

1. Hardware reset via NRESET pin
2. Read REG_VERSION (0x42), must return 0x12
3. Set Sleep mode
4. Enable LoRa mode (can only be set in Sleep)
5. Set FIFO base addresses
6. Set RF frequency
7. Set LNA gain
8. Configure modem (BW, SF, CR, CRC)
9. Set preamble length
10. Set sync word
11. Set TX power
12. Enter Standby

---

## Known Limitations

- TX and RX use polling, DIO0 interrupt support is planned
- Continuous RX mode (`lora_receive_cont`) is not yet implemented
- SF6 requires extra register writes and is not currently handled
- No frequency hopping support

---

## Dependencies

- STM32 HAL (SPI and GPIO)
- Tested on STM32F405RGT6, should work on any STM32 with HAL SPI

---

## Reference

Based on: **SX1276/77/78/79 Datasheet Rev.7, Semtech Corporation, May 2020**

---

## License

MIT License, free to use, modify and distribute with attribution.
