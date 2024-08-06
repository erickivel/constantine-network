#ifndef CRC8_H
#define CRC8_H

#include <stdint.h>
#include <stddef.h>

/*
 *  crc8() -
 *
 *  This function computes the CRC-8 checksum for the data provided in the
 *  buffer and stores the result in the location pointed to by `crc`. The CRC-8
 *  is a cyclic redundancy check that provides a simple way to check the
 *  integrity of data.
 *
 *  @buf: Pointer to the data buffer for which the CRC-8 is to be calculated.
 *  @n  : The number of bytes in the data buffer.
 *  @crc: Pointer to the location where the calculated CRC-8 checksum will be
 *        stored.
 */
extern void crc8(const uint8_t* buf, size_t n, uint8_t* crc);

#endif  /* CRC8_H */
