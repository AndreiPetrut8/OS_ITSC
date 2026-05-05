#include "ata.h"
#include "io.h"

#define ATA_PRIMARY_DATA         0x1F0
#define ATA_PRIMARY_ERR          0x1F1
#define ATA_PRIMARY_SECCOUNT     0x1F2
#define ATA_PRIMARY_LBA_LOW      0x1F3
#define ATA_PRIMARY_LBA_MID      0x1F4
#define ATA_PRIMARY_LBA_HIGH     0x1F5
#define ATA_PRIMARY_DRIVE_SEL    0x1F6
#define ATA_PRIMARY_COMMAND      0x1F7
#define ATA_PRIMARY_STATUS       0x1F7

static void ata_wait_bsy() {
    while (inb(ATA_PRIMARY_STATUS) & 0x80);
}

static void ata_wait_drq() {
    while (!(inb(ATA_PRIMARY_STATUS) & 0x08));
}

void ata_read_sector(uint32_t lba, uint8_t *buf) {
    ata_wait_bsy();
    outb(ATA_PRIMARY_DRIVE_SEL, 0xE0 | ((lba >> 24) & 0x0F));
    outb(ATA_PRIMARY_SECCOUNT, 1);
    outb(ATA_PRIMARY_LBA_LOW, (uint8_t)lba);
    outb(ATA_PRIMARY_LBA_MID, (uint8_t)(lba >> 8));
    outb(ATA_PRIMARY_LBA_HIGH, (uint8_t)(lba >> 16));
    outb(ATA_PRIMARY_COMMAND, 0x20); // Command 0x20 = Read Sectors

    ata_wait_bsy();
    ata_wait_drq();

    uint16_t *ptr = (uint16_t *)buf;
    for (int i = 0; i < 256; i++) {
        ptr[i] = inw(ATA_PRIMARY_DATA);
    }
}

void ata_write_sector(uint32_t lba, uint8_t *buf) {
    ata_wait_bsy();
    outb(ATA_PRIMARY_DRIVE_SEL, 0xE0 | ((lba >> 24) & 0x0F));
    outb(ATA_PRIMARY_SECCOUNT, 1);
    outb(ATA_PRIMARY_LBA_LOW, (uint8_t)lba);
    outb(ATA_PRIMARY_LBA_MID, (uint8_t)(lba >> 8));
    outb(ATA_PRIMARY_LBA_HIGH, (uint8_t)(lba >> 16));
    outb(ATA_PRIMARY_COMMAND, 0x30); // Command 0x30 = Write Sectors

    ata_wait_bsy();
    ata_wait_drq();

    uint16_t *ptr = (uint16_t *)buf;
    for (int i = 0; i < 256; i++) {
        outw(ATA_PRIMARY_DATA, ptr[i]);
    }
}
