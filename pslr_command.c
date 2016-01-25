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
#include "pslr.h"

/* Block size for downloads; if too big, we get
 * memory allocation error from sg driver */
#define BLOCK_SIZE 0x200 /* 0x10000 */

/* Number of retries, since we can occasionally
 * get SCSI errors when downloading data */
#define BLOCK_RETRY 3

/**************** Parse Functions ****************/

static int parse_camera_model(pslr_handle_t h, uint8_t *data) {
    ipslr_handle_t *p = (ipslr_handle_t *)h;
    if (data[0] == 0) {
        p->id = get_uint32_be(data);
    } else {
        p->id = get_uint32_le(data);
    }
    DPRINT("\t\t\tid of the camera: %x\n", p->id);
    p->model = find_model_by_id(p->id);

    if (!p->model) {
        DPRINT("\nUnknown Pentax camera.\n");
        return -1;
    } else {
        return PSLR_OK;
    }
}

static int parse_full_status(pslr_handle_t h, pslr_data_t *data) {
    ipslr_handle_t *p = (ipslr_handle_t *)h;
    if (p->model == NULL) {
        DPRINT("\t\t\tp->model is null.\n");
    } else {
        if (data->length > 0 && p->model->buffer_size != data->length) {
            DPRINT("\t\t\tWaiting for %d bytes but got %d\n", p->model->buffer_size, data->length);
            return PSLR_READ_ERROR;
        }

        memcpy(p->status_buffer, data->data, (data->length > MAX_STATUS_BUF_SIZE) ? MAX_STATUS_BUF_SIZE : data->length);

        (*p->model->parser_function)(p, &p->status);
        if (p->model->need_exposure_mode_conversion) {
            p->status.exposure_mode = exposure_mode_conversion(p->status.exposure_mode);
        }
        DPRINT("\t\t\tbufmask=0x%x\n", p->status.bufmask);
    }

    return PSLR_OK;
}

/**************** Simple Commands ****************/

/**************** Command Group 0x00 ****************/
int pslr_set_mode(pslr_handle_t h, uint32_t mode) {
    DPRINT("[C]\t\tpslr_set_mode(%d)\n", mode);
    pslr_command_t command;
    command_init(&command, h, 0x00, 0x00);
    command_add_arg(&command, mode);
    return generic_command(&command);
}

int pslr_get_short_status(pslr_handle_t h, pslr_data_t *data) {
    DPRINT("[C]\t\tpslr_get_short_status()\n");
    pslr_command_t command;
    command_init(&command, h, 0x00, 0x01);
    command_load_from_data(&command, data, DATA_USAGE_READ_RESULT);
    int ret = generic_command(&command);
    command_save_to_data(&command, data);
    return ret;
}

int pslr_identify(pslr_handle_t h) {
    DPRINT("[C]\t\tpslr_identify()\n");
    pslr_command_t command;
    command_init(&command, h, 0x00, 0x04);
    generic_command(&command);
    return parse_camera_model(h, command.data);
}

int pslr_connect_legacy(pslr_handle_t h) {
    ipslr_handle_t *p = (ipslr_handle_t *)h;
    if (p->model->old_scsi_command) {
        //  only for legacy cameras
        DPRINT("[C]\t\tpslr_connect_legacy()\n");
        pslr_command_t command;
        command_init(&command, h, 0x00, 0x05);
        return generic_command(&command);
    } else {
        return PSLR_OK;
    }
}

int pslr_get_full_status(pslr_handle_t h, pslr_data_t *data) {
    DPRINT("[C]\t\tpslr_get_full_status()\n");
    pslr_command_t command;
    command_init(&command, h, 0x00, 0x08);
    command_load_from_data(&command, data, DATA_USAGE_READ_RESULT);
    int ret = generic_command(&command);
    command_save_to_data(&command, data);
    parse_full_status(h, data);
    return ret;
}

int pslr_dsp_task_wu_req(pslr_handle_t h, uint32_t mode) {
    ipslr_handle_t *p = (ipslr_handle_t *)h;
    if (!p->model->old_scsi_command) {
        DPRINT("[C]\t\tpslr_dsp_task_wu_req(%d)\n", mode);
        pslr_command_t command;
        command_init(&command, h, 0x00, 0x09);
        command_add_arg(&command, mode);
        return generic_command(&command);
    } else {
        //  not for legacy cameras
        return PSLR_OK;
    }
}

/**************** Command Group 0x06 ****************/

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
    command_load_from_data(&command, data, DATA_USAGE_SCSI_READ);
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
    command_load_from_data(&command, data, DATA_USAGE_SCSI_WRITE);
    int ret = generic_command(&command);
    command_save_to_data(&command, data);
    return ret;
}

int pslr_get_transfer_status(pslr_handle_t h, pslr_data_t *data) {
    DPRINT("[C]\t\tplsr_get_transfer_status()\n");
    pslr_command_t command;
    command_init(&command, h, 0x06, 0x04);
    command_load_from_data(&command, data, DATA_USAGE_READ_RESULT);
    int ret = generic_command(&command);
    command_save_to_data(&command, data);
    return ret;
}

/**************** Command Group 0x10 ****************/

int pslr_do_connect(pslr_handle_t h, bool connect) {
    DPRINT("[C]\t\tpslr_set_connect_mode(%d)\n", connect);
    pslr_command_t command;
    command_init(&command, h, 0x10, 0x0a);
    command_add_arg(&command, (connect ? 1 : 0));
    return generic_command(&command);
}

/**************** Command Group 0x23 ****************/

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
    command_load_from_data(&command, data, DATA_USAGE_READ_RESULT);
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
    command_load_from_data(&command, data, DATA_USAGE_READ_RESULT);
    int ret = generic_command(&command);
    command_save_to_data(&command, data);
    return ret;
}
