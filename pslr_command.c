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
#include "pslr_command.h"

/* Block size for downloads; if too big, we get
 * memory allocation error from sg driver */
#define BLOCK_SIZE 0x200 /* 0x10000 */

/* Number of retries, since we can occasionally
 * get SCSI errors when downloading data */
#define BLOCK_RETRY 3

/*  ------------------------------------------------------------    */

int pslr_request_download(pslr_handle_t h, uint32_t address, int32_t length) {
    pslr_command_t command;
    command_init(&command);
    command.handle = (ipslr_handle_t*) h;
    command.c0 = 0x06;
    command.c1 = 0x00;
    command.args_count = 2;
    command.args[0] = address;
    command.args[1] = length;
    return generic_command(&command);
}

int pslr_do_download(pslr_handle_t h, uint8_t* buf, int32_t length) {
    pslr_command_t command;
    command_init(&command);
    command.handle = (ipslr_handle_t*) h;
    command.c0 = 0x06;
    command.c1 = 0x02;
    command.direction = SCSI_READ;
    command.data = buf;
    command.data_length = length;
    generic_command(&command);
    return command.data_length;
}

int pslr_request_upload(pslr_handle_t h, uint32_t address, int32_t length) {
    pslr_command_t command;
    command_init(&command);
    command.handle = (ipslr_handle_t*) h;
    command.c0 = 0x06;
    command.c1 = 0x01;
    command.args_count = 2;
    command.args[0] = address;
    command.args[1] = length;
    return generic_command(&command);
}

int pslr_do_upload(pslr_handle_t h, uint8_t* buf, int32_t length) {
    pslr_command_t command;
    command_init(&command);
    command.handle = (ipslr_handle_t*) h;
    command.c0 = 0x06;
    command.c1 = 0x03;
    command.data = buf;
    command.data_length = length;
    generic_command(&command);
    return command.data_length;
}

int pslr_get_transfer_status(pslr_handle_t h) {
    DPRINT("[C]\t\tplsr_get_transfer_status()\n");
    pslr_command_t command;
    command_init(&command);
    command.handle = (ipslr_handle_t*) h;
    command.c0 = 0x06;
    command.c1 = 0x04;
    command.read_result = true;
    return generic_command(&command);
}

/*  ------------------------------------------------------------    */

int pslr_download(pslr_handle_t h, uint32_t address, uint32_t length, uint8_t *buf) {
    DPRINT("[C]\t\tpslr_download(address = 0x%X, length = %d)\n", address, length);
    // uint32_t length_start = length;

    int retry = 0;
    int offset = 0;
    while ((length - offset) > 0) {
        uint32_t block = ((length - offset) > BLOCK_SIZE) ? BLOCK_SIZE : (length - offset);
        pslr_request_download(h, address + offset, block);
        int ret = pslr_do_download(h, buf + offset, block);
        if (ret < 0) {
            if (retry < BLOCK_RETRY) {
                retry++;
                continue;
            }
            return PSLR_READ_ERROR;
        }
        offset += ret;
        retry = 0;

        //  callback
        // if (progress_callback) {
        //     progress_callback(length_start - length, length_start);
        // }
    }

    return PSLR_OK;
}

int pslr_upload(pslr_handle_t h, uint32_t address, uint32_t length, uint8_t *buf) {
    DPRINT("[C]\t\tpslr_upload(address = 0x%X, length = %d)\n", address, length);
    // uint32_t length_start = length;

    int retry = 0;
    int offset = 0;
    while ((length - offset) > 0) {
        uint32_t block = ((length - offset) > BLOCK_SIZE) ? BLOCK_SIZE : (length - offset);
        pslr_request_upload(h, address + offset, block);
        int ret = pslr_do_upload(h, buf + offset, block);
        if (ret < 0) {
            if (retry < BLOCK_RETRY) {
                retry++;
                continue;
            }
            return PSLR_READ_ERROR;
        }
        offset += ret;
        retry = 0;

        //  callback
        // if (progress_callback) {
        //     progress_callback(length_start - length, length_start);
        // }
    }

    return PSLR_OK;
}
