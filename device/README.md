# 声网 BK7258 Demo工程

*简体中文| [English](README.en.md)*

## 例程简介

本例程演示了如何通过 BK7358 AI Robotic Kid 开发板，集成声网RTSA Lite SDK，实现音视频通话功能演示。

### 文件结构
```
├── ai_dashboard
│   ├── CMakeLists.txt
│   ├── config
│   │   ├── bk7258
│   │   │   ├── bk7258_partitions.csv
│   │   │   ├── config
│   │   │   ├── configuration.json
│   │   │   ├── partitions.csv
│   │   │   └── usr_gpio_cfg.h
│   │   ├── bk7258_cp1
│   │   │   ├── config
│   │   │   └── usr_gpio_cfg.h
│   │   └── bk7258_cp2
│   │       └── config
│   ├── main
│   │   ├── app_main.c
│   │   ├── app_main.h
│   │   ├── audio_para.c
│   │   ├── CMakeLists.txt
│   │   ├── include
│   │   │   ├── FifoBuffer.h
│   │   │   ├── fpscc.h
│   │   │   └── ota_display.h
│   │   ├── Kconfig.projbuild
│   │   ├── vendor_flash.c
│   │   └── vendor_flash_partition.h
│   └── pj_config.mk
├── beken_genie
│   ├── CMakeLists.txt
│   ├── config
│   │   ├── bk7258
│   │   │   ├── bk7258_partitions.csv
│   │   │   ├── config
│   │   │   ├── configuration.json
│   │   │   ├── partitions.csv
│   │   │   └── usr_gpio_cfg.h
│   │   ├── bk7258_cp1
│   │   │   ├── config
│   │   │   └── usr_gpio_cfg.h
│   │   └── bk7258_cp2
│   │       └── config
│   ├── main
│   │   ├── app_main.c
│   │   ├── app_main.h
│   │   ├── audio_para.c
│   │   ├── CMakeLists.txt
│   │   ├── include
│   │   │   ├── FifoBuffer.h
│   │   │   ├── fpscc.h
│   │   │   └── ota_display.h
│   │   ├── Kconfig.projbuild
│   │   ├── vendor_flash.c
│   │   └── vendor_flash_partition.h
│   └── pj_config.mk
├── beken_genie_ab
│   ├── CMakeLists.txt
│   ├── config
│   │   ├── bk7258
│   │   │   ├── ab_position_independent.csv
│   │   │   ├── bk7258_partitions.csv
│   │   │   ├── config
│   │   │   ├── configurationab.json
│   │   │   ├── configuration.json
│   │   │   ├── partitions.csv
│   │   │   └── usr_gpio_cfg.h
│   │   ├── bk7258_cp1
│   │   │   ├── config
│   │   │   └── usr_gpio_cfg.h
│   │   ├── bk7258_cp2
│   │   │   └── config
│   │   └── ota_rbl.config
│   ├── main
│   │   ├── CMakeLists.txt
│   │   ├── Kconfig.projbuild
│   │   ├── vendor_flash.c
│   │   └── vendor_flash_partition.h
│   └── pj_config.mk
├── beken_wss
│   ├── CMakeLists.txt
│   ├── config
│   │   ├── bk7258
│   │   │   ├── bk7258_partitions.csv
│   │   │   ├── config
│   │   │   ├── configuration.json
│   │   │   ├── partitions.csv
│   │   │   └── usr_gpio_cfg.h
│   │   ├── bk7258_cp1
│   │   │   ├── config
│   │   │   └── usr_gpio_cfg.h
│   │   └── bk7258_cp2
│   │       └── config
│   ├── main
│   │   ├── app_main.c
│   │   ├── app_main.h
│   │   ├── audio_para.c
│   │   ├── CMakeLists.txt
│   │   ├── include
│   │   │   ├── FifoBuffer.h
│   │   │   ├── fpscc.h
│   │   │   └── ota_display.h
│   │   ├── Kconfig.projbuild
│   │   ├── vendor_flash.c
│   │   └── vendor_flash_partition.h
│   └── pj_config.mk
├── beken_wss_nopsram
│   ├── CMakeLists.txt
│   ├── config
│   │   ├── bk7257
│   │   │   ├── bk7257_partitions.csv
│   │   │   ├── config
│   │   │   ├── configuration.json
│   │   │   ├── partitions.csv
│   │   │   ├── usr_gpio_cfg.h
│   │   │   └── usr_key_cfg.h
│   │   ├── bk7257_cp1
│   │   │   ├── config
│   │   │   └── usr_gpio_cfg.h
│   │   ├── bk7258
│   │   │   ├── bk7258_partitions.csv
│   │   │   ├── config
│   │   │   ├── configuration.json
│   │   │   ├── partitions.csv
│   │   │   ├── usr_gpio_cfg.h
│   │   │   └── usr_key_cfg.h
│   │   └── bk7258_cp1
│   │       ├── config
│   │       └── usr_gpio_cfg.h
│   ├── main
│   │   ├── app_main.c
│   │   ├── app_main.h
│   │   ├── audio_para.c
│   │   ├── CMakeLists.txt
│   │   ├── include
│   │   │   ├── FifoBuffer.h
│   │   │   ├── fpscc.h
│   │   │   └── ota_display.h
│   │   ├── Kconfig.projbuild
│   │   ├── vendor_flash.c
│   │   └── vendor_flash_partition.h
│   └── pj_config.mk
├── common_components
│   ├── asr
│   │   ├── armino_asr.c
│   │   ├── armino_asr.h
│   │   ├── CMakeLists.txt
│   │   └── Kconfig
│   ├── audio_engine
│   │   ├── audio_config.h
│   │   ├── audio_dump_data.c
│   │   ├── audio_dump_data.h
│   │   ├── audio_engine.c
│   │   ├── audio_engine.h
│   │   ├── audio_log.h
│   │   ├── audio_transfer.c
│   │   ├── audio_transfer.h
│   │   ├── CMakeLists.txt
│   │   └── Kconfig
│   ├── bk_app_event
│   │   ├── app_event.c
│   │   ├── app_event.h
│   │   ├── CMakeLists.txt
│   │   ├── Kconfig
│   │   └── prompt_tone.h
│   ├── bk_boarding_service
│   │   ├── bk_genie_comm.h
│   │   ├── boarding_core.c
│   │   ├── boarding_service.c
│   │   ├── boarding_service.h
│   │   ├── CMakeLists.txt
│   │   ├── Kconfig
│   │   ├── wifi_boarding_internal.h
│   │   ├── wifi_boarding_utils.c
│   │   └── wifi_boarding_utils.h
│   ├── bk_bt
│   │   ├── a2dp_sink
│   │   │   ├── a2dp_sink_demo.c
│   │   │   ├── a2dp_sink_demo.h
│   │   │   ├── mpeg4_get_bits.h
│   │   │   ├── mpeg4_latm_dec.c
│   │   │   ├── mpeg4_latm_dec.h
│   │   │   ├── ring_buffer_node.c
│   │   │   └── ring_buffer_node.h
│   │   ├── a2dp_sink_demo_cli.c
│   │   ├── bt_manager.c
│   │   ├── bt_manager.h
│   │   ├── CMakeLists.txt
│   │   ├── headset_user_config.h
│   │   ├── hfp_hf
│   │   │   ├── hfp_hf_demo.c
│   │   │   ├── hfp_hf_demo.h
│   │   │   ├── ring_buffer_particle.c
│   │   │   └── ring_buffer_particle.h
│   │   ├── Kconfig
│   │   ├── pan
│   │   │   ├── bt_comm_list.c
│   │   │   ├── bt_comm_list.h
│   │   │   ├── bt_manager.c
│   │   │   ├── bt_manager.h
│   │   │   ├── hidd_service.c
│   │   │   ├── hidd_service.h
│   │   │   ├── pan_demo_cli.c
│   │   │   ├── pan_service.c
│   │   │   ├── pan_service.h
│   │   │   └── pan_user_config.h
│   │   └── storage
│   │       ├── bluetooth_storage.c
│   │       └── bluetooth_storage.h
│   ├── bk_countdown
│   │   ├── CMakeLists.txt
│   │   ├── countdown_app.c
│   │   ├── countdown_app.h
│   │   ├── countdown.c
│   │   ├── countdown.h
│   │   └── Kconfig
│   ├── bk_key_app
│   │   ├── CMakeLists.txt
│   │   ├── Kconfig
│   │   ├── key_app_config.h
│   │   ├── key_app_service.c
│   │   └── key_app_service.h
│   ├── bk_led_blink
│   │   ├── CMakeLists.txt
│   │   ├── Kconfig
│   │   ├── led_app.c
│   │   ├── led_app.h
│   │   ├── led_blink.c
│   │   └── led_blink.h
│   ├── bk_motor
│   │   ├── CMakeLists.txt
│   │   ├── Kconfig
│   │   ├── motor.c
│   │   └── motor.h
│   ├── bk_smart_config
│   │   ├── CMakeLists.txt
│   │   ├── include
│   │   │   ├── bk_smart_config_agora_adapter.h
│   │   │   ├── bk_smart_config.h
│   │   │   ├── bk_smart_config_lingxin_adapter.h
│   │   │   ├── bk_smart_config_volc_adapter.h
│   │   │   └── bk_smart_config_wss_adapter.h
│   │   ├── Kconfig
│   │   └── src
│   │       ├── adapter
│   │       │   ├── agora
│   │       │   │   └── bk_smart_config_agora_adapter.c
│   │       │   ├── lingxin
│   │       │   │   └── bk_smart_config_lingxin_adapter.c
│   │       │   ├── volc
│   │       │   │   └── bk_smart_config_volc_adapter.c
│   │       │   └── wss
│   │       │       └── bk_smart_config_wss_adapter.c
│   │       └── core
│   │           └── bk_smart_config_core.c
│   ├── dual_screen_avi_play
│   │   ├── CMakeLists.txt
│   │   ├── Kconfig
│   │   ├── lv_font.c
│   │   ├── lvgl_app.c
│   │   ├── lvgl_ui.c
│   │   ├── lvgl_ui.h
│   │   └── ota_display.c
│   ├── network_transfer
│   │   ├── agora_rtc
│   │   │   ├── agora_config.h
│   │   │   ├── agora_convoai_iot.c
│   │   │   ├── agora_convoai_iot.h
│   │   │   ├── agora_debug.c
│   │   │   ├── agora_rtc.c
│   │   │   ├── agora_rtc.h
│   │   │   └── agora_rtc_main.c
│   │   ├── bk_wss
│   │   │   ├── bk_wss.c
│   │   │   ├── bk_wss_config.h
│   │   │   ├── bk_wss_debug.c
│   │   │   ├── bk_wss_debug.h
│   │   │   ├── bk_wss.h
│   │   │   ├── bk_wss_main.c
│   │   │   └── bk_wss_private.h
│   │   ├── CMakeLists.txt
│   │   ├── Kconfig
│   │   ├── lingxin_wss
│   │   │   └── lingxin_wss_main.c
│   │   ├── network_transfer.c
│   │   ├── network_transfer.h
│   │   └── volc_rtc
│   │       ├── RtcBotUtils.c
│   │       ├── RtcBotUtils.h
│   │       ├── RtcHttpUtils.c
│   │       ├── RtcHttpUtils.h
│   │       ├── volc_config.h
│   │       ├── volc_rtc.c
│   │       ├── volc_rtc.h
│   │       └── volc_rtc_main.c
│   ├── resource
│   │   ├── agent_joined_16k_mono_16bit_en.mp3
│   │   ├── agent_joined_16k_mono_16bit_en.wav
│   │   ├── agent_offline_16k_mono_16bit_en.mp3
│   │   ├── agent_offline_16k_mono_16bit_en.wav
│   │   ├── agent_start_fail_16k_mono_16bit_en.mp3
│   │   ├── agent_start_fail_16k_mono_16bit_en.wav
│   │   ├── angry.avi
│   │   ├── asr_standby_16k_mono_16bit_en.mp3
│   │   ├── asr_standby_16k_mono_16bit_en.wav
│   │   ├── asr_wakeup_16k_mono_16bit_en.mp3
│   │   ├── asr_wakeup_16k_mono_16bit_en.wav
│   │   ├── curious.avi
│   │   ├── genie_eye.avi
│   │   ├── happy.avi
│   │   ├── love.avi
│   │   ├── low_voltage_16k_mono_16bit_en.mp3
│   │   ├── low_voltage_16k_mono_16bit_en.wav
│   │   ├── network_provision_16k_mono_16bit_en.mp3
│   │   ├── network_provision_16k_mono_16bit_en.wav
│   │   ├── network_provision_fail_16k_mono_16bit_en.mp3
│   │   ├── network_provision_fail_16k_mono_16bit_en.wav
│   │   ├── network_provision_success_16k_mono_16bit_en.mp3
│   │   ├── network_provision_success_16k_mono_16bit_en.wav
│   │   ├── neutral.avi
│   │   ├── ota_image.jpg
│   │   ├── ota_update_fail_16k_mono_16bit_en.mp3
│   │   ├── ota_update_fail_16k_mono_16bit_en.wav
│   │   ├── ota_update_start_16k_mono_16bit_en.wav
│   │   ├── ota_update_success_16k_mono_16bit_en.mp3
│   │   ├── ota_update_success_16k_mono_16bit_en.wav
│   │   ├── reconnect_network_16k_mono_16bit_en.mp3
│   │   ├── reconnect_network_16k_mono_16bit_en.wav
│   │   ├── reconnect_network_fail_16k_mono_16bit_en.mp3
│   │   ├── reconnect_network_fail_16k_mono_16bit_en.wav
│   │   ├── reconnect_network_success_16k_mono_16bit_en.mp3
│   │   ├── reconnect_network_success_16k_mono_16bit_en.wav
│   │   ├── rtc_connection_lost_16k_mono_16bit_en.mp3
│   │   ├── rtc_connection_lost_16k_mono_16bit_en.wav
│   │   ├── sad.avi
│   │   ├── sleepy.avi
│   │   ├── surprise.avi
│   │   └── thinking.avi
│   ├── single_screen_avi_play
│   │   ├── CMakeLists.txt
│   │   ├── Kconfig
│   │   ├── lvgl_app.c
│   │   ├── lvgl_ui.c
│   │   └── ota_display.c
│   ├── single_screen_font_display
│   │   ├── CMakeLists.txt
│   │   ├── Kconfig
│   │   ├── lv_comm_list.c
│   │   ├── lv_comm_list.h
│   │   ├── lvgl_app.c
│   │   ├── lvgl_ui.c
│   │   └── ota_display.c
│   └── video_engine
│       ├── CMakeLists.txt
│       ├── Kconfig
│       ├── video_config.h
│       ├── video_dump_data.c
│       ├── video_dump_data.h
│       ├── video_engine.c
│       ├── video_engine.h
│       └── video_log.h
├── lingxin
│   ├── CMakeLists.txt
│   ├── config
│   │   ├── bk7258
│   │   │   ├── bk7258_partitions.csv
│   │   │   ├── config
│   │   │   ├── configuration.json
│   │   │   ├── partitions.csv
│   │   │   └── usr_gpio_cfg.h
│   │   ├── bk7258_cp1
│   │   │   ├── config
│   │   │   └── usr_gpio_cfg.h
│   │   └── bk7258_cp2
│   │       └── config
│   ├── main
│   │   ├── app_main.c
│   │   ├── app_main.h
│   │   ├── audio_para.c
│   │   ├── CMakeLists.txt
│   │   ├── include
│   │   │   ├── FifoBuffer.h
│   │   │   ├── fpscc.h
│   │   │   └── ota_display.h
│   │   ├── Kconfig.projbuild
│   │   ├── vendor_flash.c
│   │   └── vendor_flash_partition.h
│   └── pj_config.mk
├── readme.txt
├── rock_paper_scissors
│   ├── CMakeLists.txt
│   ├── config
│   │   ├── bk7258
│   │   │   ├── bk7258_partitions.csv
│   │   │   ├── config
│   │   │   ├── configuration.json
│   │   │   └── partitions.csv
│   │   ├── bk7258_cp1
│   │   │   └── config
│   │   └── bk7258_cp2
│   │       └── config
│   ├── main
│   │   ├── app_cpu0_main.c
│   │   ├── app_cpu1_main.c
│   │   ├── app_main_cpu1.cc
│   │   ├── CMakeLists.txt
│   │   ├── display
│   │   │   ├── lv_custom_font.c
│   │   │   ├── lvgl_app.c
│   │   │   ├── lv_paper.c
│   │   │   ├── lv_rock.c
│   │   │   └── lv_scissors.c
│   │   ├── Kconfig.projbuild
│   │   ├── main.c
│   │   ├── media
│   │   │   ├── media_audio.c
│   │   │   ├── media_audio.h
│   │   │   ├── media_main.c
│   │   │   ├── media_main.h
│   │   │   ├── media_ts.c
│   │   │   └── resource
│   │   │       ├── cant_det.c
│   │   │       ├── cant_det.pcm
│   │   │       ├── detecting.c
│   │   │       ├── detecting.pcm
│   │   │       ├── draw.c
│   │   │       ├── get_ready.c
│   │   │       ├── get_ready.pcm
│   │   │       ├── my_show.c
│   │   │       ├── my_show.pcm
│   │   │       ├── paper.c
│   │   │       ├── paper.pcm
│   │   │       ├── pls_stop_hand.c
│   │   │       ├── pls_stop_hand.pcm
│   │   │       ├── resource.h
│   │   │       ├── rock.c
│   │   │       ├── rock.pcm
│   │   │       ├── scissors.c
│   │   │       ├── scissors.pcm
│   │   │       ├── you_loss.c
│   │   │       ├── you_loss.pcm
│   │   │       ├── your_show.c
│   │   │       ├── your_show.pcm
│   │   │       ├── you_win.c
│   │   │       └── you_win.pcm
│   │   ├── tflite
│   │   │   ├── detection_responder.cc
│   │   │   ├── detection_responder.h
│   │   │   ├── gesture_detection_model_data.cc
│   │   │   ├── gesture_detection_model_data.h
│   │   │   ├── image_provider.cc
│   │   │   ├── image_provider.h
│   │   │   ├── main_functions.cc
│   │   │   ├── main_functions.h
│   │   │   ├── model_settings.cc
│   │   │   └── model_settings.h
│   │   ├── vendor_flash.c
│   │   └── vendor_flash_partition.h
│   ├── Makefile
│   └── pj_config.mk
├── soundhub
│   ├── CMakeLists.txt
│   ├── config
│   │   ├── bk7258
│   │   │   ├── bk7258_partitions.csv
│   │   │   ├── config
│   │   │   ├── configuration.json
│   │   │   ├── partitions.csv
│   │   │   └── usr_gpio_cfg.h
│   │   ├── bk7258_cp1
│   │   │   ├── config
│   │   │   └── usr_gpio_cfg.h
│   │   └── bk7258_cp2
│   │       └── config
│   ├── main
│   │   ├── app_audio_arbiter.c
│   │   ├── app_audio_arbiter.h
│   │   ├── app_evt_process.c
│   │   ├── app_evt_process.h
│   │   ├── app_main.c
│   │   ├── app_main.h
│   │   ├── audio_para.c
│   │   ├── CMakeLists.txt
│   │   ├── include
│   │   │   ├── FifoBuffer.h
│   │   │   └── fpscc.h
│   │   ├── Kconfig.projbuild
│   │   ├── vendor_flash.c
│   │   └── vendor_flash_partition.h
│   └── pj_config.mk
├── soundhub_wss
│   ├── CMakeLists.txt
│   ├── config
│   │   ├── bk7258
│   │   │   ├── bk7258_partitions.csv
│   │   │   ├── config
│   │   │   ├── configuration.json
│   │   │   ├── partitions.csv
│   │   │   └── usr_gpio_cfg.h
│   │   ├── bk7258_cp1
│   │   │   ├── config
│   │   │   └── usr_gpio_cfg.h
│   │   └── bk7258_cp2
│   │       └── config
│   ├── main
│   │   ├── app_audio_arbiter.c
│   │   ├── app_audio_arbiter.h
│   │   ├── app_evt_process.c
│   │   ├── app_evt_process.h
│   │   ├── app_main.c
│   │   ├── app_main.h
│   │   ├── audio_para.c
│   │   ├── CMakeLists.txt
│   │   ├── include
│   │   │   ├── FifoBuffer.h
│   │   │   ├── fpscc.h
│   │   │   └── ota_display.h
│   │   ├── Kconfig.projbuild
│   │   ├── vendor_flash.c
│   │   └── vendor_flash_partition.h
│   └── pj_config.mk
├── tflite_micro
│   ├── gesture_detection
│   │   ├── CMakeLists.txt
│   │   ├── config
│   │   │   ├── bk7258
│   │   │   │   ├── bk7258_partitions.csv
│   │   │   │   ├── config
│   │   │   │   ├── configuration.json
│   │   │   │   └── partitions.csv
│   │   │   ├── bk7258_cp1
│   │   │   │   └── config
│   │   │   └── bk7258_cp2
│   │   │       └── config
│   │   ├── main
│   │   │   ├── app_cpu0_main.c
│   │   │   ├── app_cpu1_main.c
│   │   │   ├── app_main.c
│   │   │   ├── app_main_cpu1.cc
│   │   │   ├── CMakeLists.txt
│   │   │   ├── main.c
│   │   │   ├── tflite
│   │   │   │   ├── detection_responder.cc
│   │   │   │   ├── detection_responder.h
│   │   │   │   ├── gesture_detection_model_data.cc
│   │   │   │   ├── gesture_detection_model_data.h
│   │   │   │   ├── image_provider.cc
│   │   │   │   ├── image_provider.h
│   │   │   │   ├── main_functions.cc
│   │   │   │   ├── main_functions.h
│   │   │   │   ├── model_settings.cc
│   │   │   │   └── model_settings.h
│   │   │   ├── vendor_flash.c
│   │   │   └── vendor_flash_partition.h
│   │   ├── Makefile
│   │   └── pj_config.mk
│   └── micro_speech
│       ├── CMakeLists.txt
│       ├── config
│       │   ├── bk7258
│       │   │   ├── bk7258_partitions.csv
│       │   │   ├── config
│       │   │   ├── configuration.json
│       │   │   └── partitions.csv
│       │   ├── bk7258_cp1
│       │   │   └── config
│       │   └── bk7258_cp2
│       │       └── config
│       ├── main
│       │   ├── app_cpu0_main.c
│       │   ├── app_cpu1_main.c
│       │   ├── app_main.c
│       │   ├── app_main_cpu1.cc
│       │   ├── CMakeLists.txt
│       │   ├── main.c
│       │   ├── tflite
│       │   │   ├── audio_preprocessor_int8_model_data.cc
│       │   │   ├── audio_preprocessor_int8_model_data.h
│       │   │   ├── main_functions.cc
│       │   │   ├── main_functions.h
│       │   │   ├── micro_model_settings.h
│       │   │   ├── micro_speech_quantized_model_data.cc
│       │   │   ├── micro_speech_quantized_model_data.h
│       │   │   ├── micro_speech_test_cc
│       │   │   ├── no_1000ms_audio_data.cc
│       │   │   ├── no_1000ms_audio_data.h
│       │   │   ├── no_30ms_audio_data.cc
│       │   │   ├── no_30ms_audio_data.h
│       │   │   ├── noise_1000ms_audio_data.cc
│       │   │   ├── noise_1000ms_audio_data.h
│       │   │   ├── silence_1000ms_audio_data.cc
│       │   │   ├── silence_1000ms_audio_data.h
│       │   │   ├── yes_1000ms_audio_data.cc
│       │   │   ├── yes_1000ms_audio_data.h
│       │   │   ├── yes_30ms_audio_data.cc
│       │   │   └── yes_30ms_audio_data.h
│       │   ├── vendor_flash.c
│       │   └── vendor_flash_partition.h
│       ├── Makefile
│       └── pj_config.mk
└── volc_rtc
    ├── CMakeLists.txt
    ├── config
    │   ├── bk7258
    │   │   ├── bk7258_partitions.csv
    │   │   ├── config
    │   │   ├── configuration.json
    │   │   ├── partitions.csv
    │   │   └── usr_gpio_cfg.h
    │   ├── bk7258_cp1
    │   │   ├── config
    │   │   └── usr_gpio_cfg.h
    │   └── bk7258_cp2
    │       └── config
    ├── main
    │   ├── app_main.c
    │   ├── app_main.h
    │   ├── audio_para.c
    │   ├── CMakeLists.txt
    │   ├── include
    │   │   ├── FifoBuffer.h
    │   │   ├── fpscc.h
    │   │   └── ota_display.h
    │   ├── Kconfig.projbuild
    │   ├── vendor_flash.c
    │   └── vendor_flash_partition.h
    ├── pj_config.mk
    └── README_CN.md

```

## 环境配置

### 硬件要求

本例程目前仅支持`BK7258 AI Robotic Kid`开发板。

## 编译和下载

### Linux 操作系统

#### 获取 bk_aidk 框架工程

本例程支持 bk_aidk branch ai_release/v[2.0.1] 及以后的，例程默认使用 tag ai_release/v[2.0.1.8] (commit id: 14f49e17332828700ff51e95d89d3090f37e70f1)。

开发环境搭建请参考BK官方说明：https://docs.bekencorp.com/arminodoc/bk_idk/bk7258/zh_CN/v2.0.1/get-started/index.html

请在确认你已经从BK官方获得相关工程下载权限后，从`github`获取`bk_aidk`工程，如下所示：

```bash
$ git clone --recurse-submodules https://github.com/bekencorp/bk_aidk.git -b ai_release/v2.0.1
$ git checkout ai_release/v2.0.1.8
$ git submodule update --recursive
```

#### 修改 bk_aidk 工程

1. 需要将`projects`目录，复制到`bk_aidk`工程，完全替换`projects`目录（建议先删除原始BK官方`projects`目录后复制）：
```bash
$ rm -rf ${bk_aidk路径}/projects
$ cp -r ./projects ${bk_aidk路径}/projects
```
2. 修改`projects/common_components/network_transfer/agora_rtc/agora_config.h`配置文件，设置server URL为你自己部署的服务器，比如：
```
#define CONFIG_AGENT_SERVER_URL         "http://192.168.1.100:5001"
```

#### 编译固件

在bk_aidk工程目录下，构建demo固件：
```bash
$ cd ${bk_aidk路径}
$ make bk7258 PROJECT=beken_genie
```
注：构建生成的bin文件存放于`build/beken_genie/bk7258/all-app.bin`，对应OTA升级文件存放于`build/beken_genie/bk7258/encrypt/app_pack.rbl`。

#### 烧录固件
固件构建完毕后，参考BK官方说明，使用烧录程序下载固件：https://docs.bekencorp.com/arminodoc/bk_idk/bk7258/zh_CN/v2.0.1/developer-guide/config_tools/bk_tool_bkfil.html

## 如何使用例程

### 五分钟快速体验

注意：

1. 请使用Type-C数据线接入开发板`USB TO UART`接口，并与电脑连接。
2. 该接口同时用于电池充电。
3. 请注意`RST`按键位置，当烧录工具无法自动重启开发板时，可以手动重启恢复烧录能力。

### 开始注册自己的声网账号

接下来我们带您创建属于您自己的声网账号。
参考文档：https://doc.shengwang.cn/doc/convoai/restful/get-started/enable-service

#### Demo运行

1. 开发板插入电池或数据线后会自动启动。
2. 如果还未给开发板设置wifi账号及密码，请先长按`S1`5秒键进入配网模式。
3. 使用配套的APP（本示例只提供Android工程，请自行构建并安装APK）
4. 配网成功后，你可以说`hi,阿米诺`语音唤醒设备，启动和AI Agent的通话。
5. 对话完毕后，你可以说`bye bye,阿米诺`退出和AI Agent的通话。
6. 开发板在静默状态3分钟后自动进入深度休眠状态，你可以通过`RST`键重启运行。
