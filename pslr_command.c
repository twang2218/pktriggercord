/*
    pkTriggerCord
    Copyright (C) 2011-2016 Andras Salamon <andras.salamon@melda.info>
    Remote control of Pentax DSLR cameras.

    This program is free software: you can redistribute it and/or modify
    it under the terms of the GNU Lesser General Public License as published by
    the Free Software Foundation, either version 3 of the License, or
    (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU Lesser General Public License for more details.

    You should have received a copy of the GNU General Public License
    and GNU Lesser General Public License
    along with this program.  If not, see <http://www.gnu.org/licenses/>.
*/

#include <string.h>
#include <stdbool.h>

#include "pslr_scsi_command.h"

/* Block size for downloads; if too big, we get
 * memory allocation error from sg driver */
#define BLOCK_SIZE 0x200 /* 0x10000 */

/* Number of retries, since we can occasionally
 * get SCSI errors when downloading data */
#define BLOCK_RETRY 3

/*  ------------------------------------------------------------    */

int ipslr_request_download(ipslr_handle_t *p, uint32_t address, int32_t length) {
    pslr_command_t command;
    command_init(&command);
    command.handle = p;
    command.c0 = 0x06;
    command.c1 = 0x00;
    command.args_count = 2;
    command.args[0] = address;
    command.args[1] = length;
    return generic_command(&command);
}

int ipslr_do_download(ipslr_handle_t *p, uint8_t* buf, int32_t length) {
    pslr_command_t command;
    command_init(&command);
    command.handle = p;
    command.c0 = 0x06;
    command.c1 = 0x02;
    command.direction = SCSI_READ;
    command.data = buf;
    command.data_length = length;
    generic_command(&command);
    return command.data_length;
}

int ipslr_request_upload(ipslr_handle_t *p, uint32_t address, int32_t length) {
    pslr_command_t command;
    command_init(&command);
    command.handle = p;
    command.c0 = 0x06;
    command.c1 = 0x01;
    command.args_count = 2;
    command.args[0] = address;
    command.args[1] = length;
    return generic_command(&command);
}

int ipslr_do_upload(ipslr_handle_t *p, uint8_t* buf, int32_t length) {
    pslr_command_t command;
    command_init(&command);
    command.handle = p;
    command.c0 = 0x06;
    command.c1 = 0x03;
    command.data = buf;
    command.data_length = length;
    generic_command(&command);
    return command.data_length;
}

int ipslr_get_transfer_status(ipslr_handle_t *p) {
    pslr_command_t command;
    command_init(&command);
    command.handle = p;
    command.c0 = 0x06;
    command.c1 = 0x04;
    command.read_result = true;
    return generic_command(&command);
}

/*  ------------------------------------------------------------    */

int pslr_download(ipslr_handle_t *p, uint32_t address, uint32_t length, uint8_t *buf) {
    DPRINT("[C]\t\tpslr_download(address = 0x%X, length = %d)\n", address, length);
    // uint32_t length_start = length;

    int retry = 0;
    while (length > 0) {
        uint32_t block = (length > BLOCK_SIZE) ? BLOCK_SIZE : length;
        ipslr_request_download(p, address, block);
        int ret = ipslr_do_download(p, buf, block);
        if (ret < 0) {
            if (retry < BLOCK_RETRY) {
                retry++;
                continue;
            }
            return PSLR_READ_ERROR;
        }

        //  shift pointers
        buf += ret;
        length -= ret;
        address += ret;
        //  clear retry counter
        retry = 0;

        //  callback
        // if (progress_callback) {
        //     progress_callback(length_start - length, length_start);
        // }
    }

    return PSLR_OK;
}

int pslr_upload(ipslr_handle_t *p, uint32_t address, uint32_t length, uint8_t *buf) {
    DPRINT("[C]\t\tpslr_upload(address = 0x%X, length = %d)\n", address, length);
    // uint32_t length_start = length;

    int retry = 0;
    while (length > 0) {
        uint32_t block = (length > BLOCK_SIZE) ? BLOCK_SIZE : length;
        ipslr_request_upload(p, address, block);
        int ret = ipslr_do_upload(p, buf, block);
        if (ret < 0) {
            if (retry < BLOCK_RETRY) {
                retry++;
                continue;
            }
            return PSLR_READ_ERROR;
        }

        //  shift pointers
        buf += ret;
        length -= ret;
        address += ret;

        //  clear retry counter
        retry = 0;

        //  callback
        // if (progress_callback) {
        //     progress_callback(length_start - length, length_start);
        // }
    }

    return PSLR_OK;
}
