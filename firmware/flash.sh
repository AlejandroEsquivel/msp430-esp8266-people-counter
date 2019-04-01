ESPTOOL=$1

FIRMWARE_DIR=$2


sudo python $ESPTOOL -p /dev/ttyUSB0 write_flash -fm qio \
    0x00000 $FIRMWARE_DIR/bin/boot_v1.7.bin \
    0x01000 $FIRMWARE_DIR/bin/at/512+512/user1.1024.new.2.bin \
    0xfc000 $FIRMWARE_DIR/bin/esp_init_data_default_v08.bin \
    0xfe000 $FIRMWARE_DIR/bin/blank.bin


 #   boot_v1.2+.bin              0x00000
 #   user1.1024.new.2.bin        0x01000
 #   esp_init_data_default.bin   0xfc000 (optional)
 #   blank.bin                   0x7e000 & 0xfe000