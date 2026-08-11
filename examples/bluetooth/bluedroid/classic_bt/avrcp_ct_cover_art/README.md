| Supported Targets | ESP32 | ESP32-S31 |
| ----------------- | ----- | --------- |

AVRCP-CT-COVER-ART EXAMPLE
======================

This is an example demonstrating the API for implementing Audio/Video Remote Control Profile to get and display cover art image.

## Required components

- [bt_app_core_utils](../common/bt_app_core_utils)
- [bredr_app_common_utils](../common/bredr_app_common_utils)
- [a2dp_sink_common_utils](../common/a2dp_utils/a2dp_sink_common_utils)
- [a2dp_sink_int_codec_utils](../common/a2dp_utils/a2dp_sink_int_codec_utils)
- [a2dp_sink_ext_codec_utils](../common/a2dp_utils/a2dp_sink_ext_codec_utils)
- [avrcp_common_utils](../common/avrcp_utils/avrcp_common_utils)
- [avrcp_metadata_utils](../common/avrcp_utils/avrcp_metadata_utils)
- [avrcp_cover_art_utils](../common/avrcp_utils/avrcp_cover_art_utils)

```
+---------------------------------------------------+---------------------+
|                avrcp_cover_art_utils              |                     |
+---------------------------------------------------+                     |
|                avrcp_metadata_utils               |                     |
+---------------------------------------------------+                     |
|                 avrcp_common_utils                |                     |
+-------------------------+-------------------------+  bt_app_core_utils  |
|a2dp_sink_int_codec_utils|a2dp_sink_ext_codec_utils|                     |
+-------------------------+-------------------------+                     |
|               a2dp_sink_common_utils              |                     |
+---------------------------------------------------+                     |
|               bredr_app_common_utils              |                     |
+---------------------------------------------------+---------------------+
```

Detailed information can be viewed through the [../common/README.md](../common/README.md).

## How to use this example

### Hardware Required

* An ESP32 or ESP32-S31 development board
* A SPI-interfaced LCD
* A USB cable for power supply and programming

### Hardware Connection

The connection between development board and the LCD is as follows:

```
   Development Board                       LCD Screen
      +---------+              +---------------------------------+
      |         |              |                                 |
      |     3V3 +--------------+ VCC   +----------------------+  |
      |         |              |       |                      |  |
      |     GND +--------------+ GND   |                      |  |
      |         |              |       |                      |  |
      |   DATA0 +--------------+ MOSI  |                      |  |
      |         |              |       |                      |  |
      |    PCLK +--------------+ SCK   |                      |  |
      |         |              |       |                      |  |
      |      CS +--------------+ CS    |                      |  |
      |         |              |       |                      |  |
      |     D/C +--------------+ D/C   |                      |  |
      |         |              |       |                      |  |
      |     RST +--------------+ RST   |                      |  |
      |         |              |       |                      |  |
      |BK_LIGHT +--------------+ BCKL  +----------------------+  |
      |         |              |                                 |
      +---------+              +---------------------------------+
```

The GPIO number used by this example can be changed in [avrcp_cover_art_service.c](../common/avrcp_utils/avrcp_cover_art_utils/avrcp_cover_art_service.c), where:

| GPIO number              | LCD pin |
| ------------------------ | ------- |
| EXAMPLE_PIN_NUM_PCLK     | SCK     |
| EXAMPLE_PIN_NUM_CS       | CS      |
| EXAMPLE_PIN_NUM_DC       | DC      |
| EXAMPLE_PIN_NUM_RST      | RST     |
| EXAMPLE_PIN_NUM_DATA0    | MOSI    |
| EXAMPLE_PIN_NUM_BK_LIGHT | BCKL    |

Note that the level used to turn on the LCD backlight may vary: some LCD modules need a low level to turn it on, while others require a high level. You can change the backlight level macro `EXAMPLE_LCD_BK_LIGHT_ON_LEVEL` in [avrcp_cover_art_service.c](../common/avrcp_utils/avrcp_cover_art_utils/avrcp_cover_art_service.c).

### Configure the project

```
idf.py menuconfig
```

* The AVRCP CT Cover Art feature is enabled by default. We can disable it by unselecting the menuconfig option `Component config --> Bluetooth --> Bluedroid Options --> Classic Bluetooth --> AVRCP Features --> AVRCP CT Cover Art`. This example will try to use the AVRCP CT Cover Art feature to get the cover art image, count the image size, and display it if the peer device supports it.
* **Memory Configuration**: To ensure that the A2DP sink stream and the display of AVRCP cover art on the LCD can run simultaneously, the following configurations are required and have been set in `sdkconfig.defaults`:
  * `CONFIG_SPIRAM=y`: Enables external SPI RAM (PSRAM) support. This provides additional memory space needed for buffering audio data during A2DP streaming while simultaneously handling cover art image decoding and display operations. Without this, the application may run out of internal RAM when processing both audio streams and image data.
  * `CONFIG_PARTITION_TABLE_SINGLE_APP_LARGE=y`: Selects the single large app partition table scheme, which allocates more space for the application binary. This is necessary because the example application includes multiple Bluetooth profiles (A2DP, AVRCP), image decoding libraries, and LCD display drivers, requiring a larger application partition than the default partition table provides.

### Build and Flash

Build the project and flash it to the board, then run monitor tool to view serial output.

```
idf.py -p PORT flash monitor
```

(To exit the serial monitor, type ``Ctrl-]``.)

## Example Output

The output when receiving a cover art image:

```
I (31579) RC_CT: AVRC metadata rsp: attribute id 0x80, 1000526
I (32039) RC_CA_SRV: Cover Art Client final data event, image size: 12315 bytes
I (32119) RC_CA_SRV: JPEG image decoded! Size of the decoded image is: 200px x 200px.
```

And then, the LCD screen will display the cover art image.

## Troubleshooting

For any technical queries, please open an [issue](https://github.com/espressif/esp-idf/issues) on GitHub. We will get back to you soon.

## v5.1.4 移植说明（2026-08-12，DeepSeek-V4-Flash）

本目录从 esp-idf v6.1.0 移植（avrcp_ct_cover_art 例程 + common 组件），
配合 v5.1.4 框架的 AVRCP Cover Art 移植（见 components/bt 的 coverart 改动）。

**已知差异（v5.1.4 适配）**：
- 例程的 A2DP 流部分依赖 v6.1.0 组件架构（esp_driver_i2s/esp_driver_dac），
  v5.1.4 无此组件拆分；如需完整编译请按 v5.1.4 a2dp_sink 例程适配
- cover art 核心逻辑（avrcp_cover_art_utils：JPEG 接收/解码/显示）不依赖
  A2DP 流，可独立使用；显示层需适配实际屏幕（例程为 ST7789 LCD）
- 依赖 esp_jpeg 组件（idf.py add-dependency esp_jpeg 拉取）
- 需要 v5.1.4 框架开启 CONFIG_BT_AVRCP_CT_COVER_ART_ENABLED（默认 y）
