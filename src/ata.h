#ifndef ATA_H
#define ATA_H

#include <stdint.h>

/* Citește un sector (512 bytes) de la adresa LBA specificată */
void ata_read_sector(uint32_t lba, uint8_t *buf);

/* Scrie un sector (512 bytes) la adresa LBA specificată */
void ata_write_sector(uint32_t lba, uint8_t *buf);

#endif
