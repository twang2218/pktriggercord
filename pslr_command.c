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

/**************** Simple Commands ****************/

/**************** Command Group 00 ****************/

int pslr_get_short_status(pslr_handle_t h, pslr_data_t *data) {
    DPRINT("[C]\t\tpslr_get_short_status()\n");
    pslr_command_t command;
    command_init(&command, h, 0x00, 0x01);
    command_load_from_data(&command, data);
    int ret = generic_command(&command);
    command_save_to_data(&command, data);
    return ret;
}

int pslr_get_full_status(pslr_handle_t h, pslr_data_t *data) {
    DPRINT("[C]\t\tpslr_get_full_status()\n");
    pslr_command_t command;
    command_init(&command, h, 0x00, 0x08);
    command_load_from_data(&command, data);
    int ret = generic_command(&command);
    command_save_to_data(&command, data);
    //  TODO: parse the status data later;
    return ret;
}

int pslr_dsp_task_wu_req(pslr_handle_t h, uint32_t mode) {
    pslr_command_t command;
    command_init(&command, h, 0x00, 0x09);
    command_add_arg(&command, mode);
    return generic_command(&command);
}

/**************** Command Group 06 ****************/

int pslr_request_download(pslr_handle_t h, uint32_t address, int32_t length) {
    DPRINT("[C]\t\tpslr_request_download(0x%08x, 0x%x)\n", address, length);
    pslr_command_t command;
    command_init(&command, h, 0x06, 0x00);
    command_add_arg(&command, address);
    command_add_arg(&command, length);
    return generic_command(&command);
}

int pslr_do_download(pslr_handle_t h, pslr_data_t *data) {
    DPRINT("[C]\t\tpslr_do_download(0x%x)\n", data->length);
    pslr_command_t command;
    command_init(&command, h, 0x06, 0x02);
    command_load_from_data(&command, data);
    //  the data will be returned from SCSI_READ, rather separate commands.
    command.read_result = false;
    command.direction = SCSI_READ;
    int ret = generic_command(&command);
    command_save_to_data(&command, data);
    return ret;
}

int pslr_request_upload(pslr_handle_t h, uint32_t address, int32_t length) {
    DPRINT("[C]\t\tpslr_request_upload(0x%08x, 0x%x)\n", address, length);
    pslr_command_t command;
    command_init(&command, h, 0x06, 0x01);
    command_add_arg(&command, address);
    command_add_arg(&command, length);
    return generic_command(&command);
}

int pslr_do_upload(pslr_handle_t h, pslr_data_t *data) {
    DPRINT("[C]\t\tpslr_do_upload(0x%x)\n", data->length);
    pslr_command_t command;
    command_init(&command, h, 0x06, 0x03);
    command_load_from_data(&command, data);
    //  the buffer is used for sending data, not receiving.
    command.read_result = false;
    int ret = generic_command(&command);
    command_save_to_data(&command, data);
    return ret;
}

int pslr_get_transfer_status(pslr_handle_t h, pslr_data_t *data) {
    DPRINT("[C]\t\tplsr_get_transfer_status()\n");
    pslr_command_t command;
    command_init(&command, h, 0x06, 0x04);
    command_load_from_data(&command, data);
    int ret = generic_command(&command);
    command_save_to_data(&command, data);
    return ret;
}

/**************** Command Group 23 ****************/

int pslr_write_adj_data(pslr_handle_t h, uint32_t value) {
    DPRINT("[C]\t\tpslr_write_adj_data(%d)\n", value);
    pslr_command_t command;
    command_init(&command, h, 0x23, 0x00);
    command_add_arg(&command, value);
    return generic_command(&command);
}

int pslr_set_adj_mode_flag(pslr_handle_t h, uint32_t mode, uint32_t value) {
    DPRINT("[C]\t\tpslr_set_adj_mode_flag(%d, %d)\n", mode, value);
    pslr_command_t command;
    command_init(&command, h, 0x23, 0x04);
    command_add_arg(&command, mode);
    command_add_arg(&command, value);
    return generic_command(&command);
}

int pslr_get_adj_mode_flag(pslr_handle_t h, uint32_t mode, pslr_data_t *data) {
    DPRINT("[C]\t\tpslr_get_adj_mode_flag(%d)\n", mode);
    pslr_command_t command;
    command_init(&command, h, 0x23, 0x05);
    command_add_arg(&command, mode);
    command_load_from_data(&command, data);
    int ret = generic_command(&command);
    command_save_to_data(&command, data);
    return ret;
}

int pslr_set_adj_data(pslr_handle_t h, uint32_t mode, bool debug_mode) {
    DPRINT("[C]\t\tpslr_set_adj_data(%d, %d)\n", mode, debug_mode);
    pslr_command_t command;
    command_init(&command, h, 0x23, 0x06);
    command_add_arg(&command, mode);
    if (debug_mode) {
        command_add_arg(&command, 1);
        command_add_arg(&command, 1);
    } else {
        command_add_arg(&command, 0);
        command_add_arg(&command, 0);
    }
    command_add_arg(&command, 0);
    command_add_arg(&command, 0);

    return generic_command(&command);
}

int pslr_get_adj_data(pslr_handle_t h, uint32_t mode, pslr_data_t *data) {
    DPRINT("[C]\t\tpslr_get_adj_data(%d)\n", mode);
    pslr_command_t command;
    command_init(&command, h, 0x23, 0x07);
    command_add_arg(&command, mode);
    command_load_from_data(&command, data);
    int ret = generic_command(&command);
    command_save_to_data(&command, data);
    return ret;
}

/**************** Composite Commands ****************/

int pslr_download(pslr_handle_t h, uint32_t address, pslr_data_t *data) {
    DPRINT("[C]\t\tpslr_download(address = 0x%X, length = %d)\n", address, data->length);
    // uint32_t length_start = length;

    int retry = 0;
    int offset = 0;
    pslr_data_t tempdata;
    while ((tempdata.length = data->length - offset) > 0) {
        tempdata.data = data->data + offset;
        uint32_t block = (tempdata.length > BLOCK_SIZE) ? BLOCK_SIZE : tempdata.length;
        pslr_request_download(h, address + offset, block);
        pslr_do_download(h, &tempdata);
        if (tempdata.length < 0) {
            if (retry < BLOCK_RETRY) {
                retry++;
                continue;
            }
            return PSLR_READ_ERROR;
        }
        offset += tempdata.length;
        retry = 0;

        //  callback
        // if (progress_callback) {
        //     progress_callback(length_start - length, length_start);
        // }
    }

    return PSLR_OK;
}

int pslr_upload(pslr_handle_t h, uint32_t address, pslr_data_t *data) {
    DPRINT("[C]\t\tpslr_upload(address = 0x%X, length = %d)\n", address, data->length);
    // uint32_t length_start = length;

    int retry = 0;
    int offset = 0;
    pslr_data_t tempdata;
    while ((tempdata.length = data->length - offset) > 0) {
        tempdata.data = data->data + offset;
        uint32_t block = (tempdata.length > BLOCK_SIZE) ? BLOCK_SIZE : tempdata.length;
        pslr_request_upload(h, address + offset, block);
        pslr_do_upload(h, &tempdata);
        if (tempdata.length < 0) {
            if (retry < BLOCK_RETRY) {
                retry++;
                continue;
            }
            return PSLR_READ_ERROR;
        }
        offset += tempdata.length;
        retry = 0;

        //  callback
        // if (progress_callback) {
        //     progress_callback(length_start - length, length_start);
        // }
    }

    return PSLR_OK;
}

int pslr_set_debug_mode(pslr_handle_t h, bool debug_mode) {
    DPRINT("[C]\t\tpslr_debug_mode(%d)\n", debug_mode);

    pslr_dsp_task_wu_req(h, 1);
    pslr_get_adj_data(h, 3, NULL);
    pslr_get_adj_mode_flag(h, 3, NULL);
    pslr_get_short_status(h, NULL);
    pslr_set_adj_data(h, 3, debug_mode);
    pslr_get_short_status(h, NULL);
    pslr_set_adj_mode_flag(h, 3, 1);
    pslr_write_adj_data(h, 0);
    pslr_dsp_task_wu_req(h, 2);
    pslr_get_short_status(h, NULL);

    return PSLR_OK;
}
