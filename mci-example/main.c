#include <stdio.h>
#include <diskio.h>
#include <math.h>
#include <stdbool.h>

void sprint_double(char *buffer, double x, int precision) {
    long integral_part = (long) x;
    long exponent = (long) pow(10, precision);
    long decimal_part = (long) ((x - integral_part) * exponent);
    sprintf(buffer, "%ld.%ld", integral_part, decimal_part);
}

int main(void)
{
    printf("You are running RIOT on a(n) %s board.\n", RIOT_BOARD);
    printf("This board features a(n) %s MCU.\n", RIOT_MCU);

    /*
     * Initialize MCI and gather information on memory card
     */

    DSTATUS status = MCI_initialize();
    bool read_only = false;
    printf("MCI status: 0x%hhx\n", status);

    if(status == STA_NOINIT) {
        printf("Could not initialize MCI interface :(\n");
        return 1;
    } else if(status == STA_NODISK) {
        printf("NO SDCard detected. Aborting\n");
        return 1;
    } else if(status == STA_PROTECT) {
        printf("SDCard is in read-only mode\n");
        read_only = true;
    }

    unsigned long sector_count = 0;
    MCI_ioctl(GET_SECTOR_COUNT, &sector_count);
    printf("sector_count: %lu\n", sector_count);

    unsigned short sector_size = 0;
    MCI_ioctl(GET_SECTOR_SIZE, &sector_size);
    printf("sector_size: %hu\n", sector_size);

    unsigned long block_size = 0;
    MCI_ioctl(GET_BLOCK_SIZE, &block_size);
    printf("block_size: %lu\n", block_size);

    double capacity = (1.0 * sector_size * sector_count) / (1024 * 1024 * 1024);
    char capacity_buffer[8];
    sprint_double(capacity_buffer, capacity, 2);
    printf("SDcard capacity: %sGiB\n", capacity_buffer);

    if(read_only) {
        printf("SDCard in read-only mode. Not performing read/write test.");
        return 0;
    }

    /*
     * Perform a read-write check on the SDCard
     */

    #define BUFFER_SIZE 512
    unsigned char write_buffer[BUFFER_SIZE + 1]; // For the \0
    unsigned char read_buffer[BUFFER_SIZE + 1];

    if(sector_size > BUFFER_SIZE) {
        printf("Does not support a sector size larger than %d, write test aborted.\n", BUFFER_SIZE);
        return 1;
    }

    for(int i=0; i<sector_size; i++) {
        sprintf(write_buffer + i, "%d", i%10);
    }

    unsigned long op_start_sector = 0; // We'll start writing at this sector
    unsigned char op_sector_count = 1; // We'll write a single sector

    printf("About to write the following:\n%s\n", write_buffer);
    status = MCI_write(write_buffer, op_start_sector, op_sector_count);
    printf("MCI write status: 0x%hhx\n", status);

    printf("About to read from SDCard\n");
    status = MCI_read(read_buffer, op_start_sector, op_sector_count);
    printf("MCI read status: 0x%hhx\n", status);
    printf("Read the following:\n%s\n", read_buffer);

    return 0;
}
