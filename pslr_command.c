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
#include <unistd.h>

#include "pslr_scsi_command.h"
#include "pslr_command.h"
#include "pslr.h"

/**************** Parse Functions ****************/

static const char *bool_string(bool value) { return value ? "true" : "false"; }

static uint32_t get_uint32(pslr_handle_t h, uint8_t *data) {
    ipslr_handle_t *p = (ipslr_handle_t *)h;

    //  use the right function based on the endian.
    if (p->model->is_little_endian) {
        return get_uint32_le(data);
    } else {
        return get_uint32_be(data);
    }
}

static int parse_camera_model(pslr_handle_t h, pslr_data_t *data) {
    ipslr_handle_t *p = (ipslr_handle_t *)h;
    if (data == NULL || data->data == NULL || data->length < 8) {
        return PSLR_READ_ERROR;
    } else if (data->data[0] == 0) {
        p->id = get_uint32_be(data->data);
    } else {
        p->id = get_uint32_le(data->data);
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

static int parse_buffer_segment_info(pslr_handle_t h, pslr_data_t *data, pslr_buffer_segment_info_t *info) {
    if (info == NULL) {
        return PSLR_OK;
    } else if (data == NULL || data->data == NULL || data->length < 16) {
        return PSLR_READ_ERROR;
    } else {
        info->a = get_uint32(h, &data->data[0]);
        info->b = get_uint32(h, &data->data[4]);
        info->addr = get_uint32(h, &data->data[8]);
        info->length = get_uint32(h, &data->data[12]);
        return PSLR_OK;
    }
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
    command_load_from_data(&command, NULL, DATA_USAGE_READ_RESULT);
    generic_command(&command);
    int ret = parse_camera_model(h, &command.data);
    command_save_to_data(&command, NULL);
    return ret;
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

int pslr_get_full_status(pslr_handle_t h) {
    DPRINT("[C]\t\tpslr_get_full_status()\n");
    pslr_command_t command;
    command_init(&command, h, 0x00, 0x08);
    command_load_from_data(&command, NULL, DATA_USAGE_READ_RESULT);
    int ret = generic_command(&command);
    parse_full_status(h, &command.data);
    command_save_to_data(&command, NULL);
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

/**************** Command Group 0x02 ****************/

int pslr_get_buffer_status(pslr_handle_t h, pslr_data_t *data) {
    DPRINT("[C]\t\tpslr_get_buffer_status()\n");
    pslr_command_t command;
    command_init(&command, h, 0x02, 0x00);
    command_load_from_data(&command, data, DATA_USAGE_READ_RESULT);
    int ret = generic_command(&command);
    command_save_to_data(&command, data);
    return ret;
}

int pslr_select_buffer(pslr_handle_t h, uint32_t buffer_number, pslr_buffer_type_t buffer_type,
                       uint32_t buffer_resolution) {
    DPRINT("[C]\t\tpslr_select_buffer(no: %d, type: %d, res: %d)\n", buffer_number, buffer_type, buffer_resolution);
    pslr_command_t command;
    command_init(&command, h, 0x02, 0x01);
    command_add_arg(&command, buffer_number);
    command_add_arg(&command, buffer_type);
    command_add_arg(&command, buffer_resolution);
    ipslr_handle_t *p = (ipslr_handle_t *)h;
    if (!p->model->old_scsi_command) {
        command_add_arg(&command, 0);
    }
    return generic_command(&command);
}

int pslr_delete_buffer(pslr_handle_t h, uint32_t buffer_number) {
    DPRINT("[C]\t\tpslr_delete_buffer(%d)\n", buffer_number);
    //  TODO: 9? or 32? It's set for K10D, but later cameras may have more buffers.
    if (buffer_number > 9) {
        return PSLR_PARAM;
    }

    pslr_command_t command;
    command_init(&command, h, 0x02, 0x03);
    command_add_arg(&command, buffer_number);
    return generic_command(&command);
}

/**************** Command Group 0x04 ****************/

int pslr_get_buffer_segment_info(pslr_handle_t h, pslr_buffer_segment_info_t *info) {
    DPRINT("[C]\t\tpslr_get_buffer_segment_info()\n");
    if (info == NULL) {
        return PSLR_PARAM;
    }

    pslr_command_t command;
    command_init(&command, h, 0x04, 0x00);

    int retry = 20;
    int ret = PSLR_OK;
    info->b = 0;

    while (info->b == 0 && --retry > 0) {
        command_load_from_data(&command, NULL, DATA_USAGE_READ_RESULT);
        ret = generic_command(&command);
        parse_buffer_segment_info(h, &command.data, info);
        command_save_to_data(&command, NULL);

        if (info->b == 0) {
            DPRINT("\t\t\tWaiting for segment info addr: 0x%x len: %d B=%d\n", info->addr, info->length, info->b);
            usleep(POLL_INTERVAL);
        }
    }
    return ret;
}

int pslr_next_buffer_segment(pslr_handle_t h) {
    DPRINT("[C]\t\tpslr_next_buffer_segment()\n");
    pslr_command_t command;
    command_init(&command, h, 0x04, 0x01);
    command_add_arg(&command, 0);
    //  TODO: I don't think this sleep is needed, as `scsi_get_status()` will not return until it's finished,
    //  In another word, there is usleep() inside the function.
    // usleep(100000); // needed !! 100 too short, 1000 not short enough for PEF
    return generic_command(&command);
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

// fullpress: take picture
// halfpress: autofocus
int pslr_shutter(pslr_handle_t h, bool fullpress) {
    DPRINT("[C]\t\tpslr_shutter(%s)\n", bool_string(fullpress));
    pslr_get_full_status(h);
    pslr_command_t command;
    command_init(&command, h, 0x10, 0x05);
    command_add_arg(&command, (fullpress ? 2 : 1));
    return generic_command(&command);
}

int pslr_ae_lock(pslr_handle_t h, bool lock) {
    DPRINT("[C]\t\tpslr_ae_lock()\n");
    pslr_command_t command;
    command_init(&command, h, 0x10, (lock ? 0x06 : 0x08));
    return generic_command(&command);
}

int pslr_green_button(pslr_handle_t h) {
    DPRINT("[C]\t\tpslr_green()\n");
    pslr_command_t command;
    command_init(&command, h, 0x10, 0x07);
    return generic_command(&command);
}

int pslr_do_connect(pslr_handle_t h, bool connect) {
    DPRINT("[C]\t\tpslr_set_connect_mode(%s)\n", bool_string(connect));
    pslr_command_t command;
    command_init(&command, h, 0x10, 0x0a);
    command_add_arg(&command, (connect ? 1 : 0));
    return generic_command(&command);
}

int pslr_continuous(pslr_handle_t h) {
    DPRINT("[C]\t\tpslr_continuous()\n");
    pslr_command_t command;
    command_init(&command, h, 0x10, 0x0c);
    return generic_command(&command);
}

int pslr_bulb(pslr_handle_t h, bool start) {
    DPRINT("[C]\t\tpslr_bulb()\n");
    pslr_command_t command;
    command_init(&command, h, 0x10, 0x0d);
    command_add_arg(&command, (start ? 1 : 0));
    return generic_command(&command);
}

int pslr_dust_removal(pslr_handle_t h) {
    DPRINT("[C]\t\tpslr_dust_removal()\n");
    pslr_command_t command;
    command_init(&command, h, 0x10, 0x11);
    return generic_command(&command);
}

int pslr_button_test(pslr_handle_t h, uint8_t button_number, uint32_t arg) {
    DPRINT("[C]\tpslr_button_test(%X, %X)\n", button_number, arg);
    pslr_command_t command;
    command_init(&command, h, 0x10, button_number);
    command_add_arg(&command, arg);
    int ret = generic_command(&command);
    DPRINT("\tbutton result code: 0x%x\n", ret);
    return ret;
}

/**************** Command Group 0x18 ****************/
static int pslr_command_x18(pslr_command_t *command, bool wrap) {
    if (wrap) {
        pslr_dsp_task_wu_req(command->handle, 1);
    }
    int ret = generic_command(command);
    if (wrap) {
        pslr_dsp_task_wu_req(command->handle, 2);
    }
    return ret;
}

int pslr_set_exposure_mode(pslr_handle_t h, pslr_exposure_mode_t exposure_mode) {
    DPRINT("[C]\t\tpslr_set_exposure_mode(%d)\n", exposure_mode);

    if (exposure_mode >= PSLR_EXPOSURE_MODE_MAX) {
        return PSLR_PARAM;
    }

    pslr_command_t command;
    command_init(&command, h, 0x18, 0x01);
    command_add_arg(&command, 1);
    command_add_arg(&command, exposure_mode);
    return pslr_command_x18(&command, true);
}

int pslr_set_ae_metering_mode(pslr_handle_t h, pslr_ae_metering_t ae_metering_mode) {
    DPRINT("[C]\t\tpslr_set_ae_metering_mode(%X)\n", ae_metering_mode);
    pslr_command_t command;
    command_init(&command, h, 0x18, 0x03);
    command_add_arg(&command, ae_metering_mode);
    return pslr_command_x18(&command, true);
}

int pslr_set_flash_mode(pslr_handle_t h, pslr_flash_mode_t flash_mode) {
    DPRINT("[C]\t\tpslr_set_flash_mode(%X)\n", flash_mode);
    pslr_command_t command;
    command_init(&command, h, 0x18, 0x04);
    command_add_arg(&command, flash_mode);
    return pslr_command_x18(&command, true);
}

int pslr_set_af_mode(pslr_handle_t h, pslr_af_mode_t af_mode) {
    DPRINT("[C]\t\tpslr_set_af_mode(%X)\n", af_mode);
    pslr_command_t command;
    command_init(&command, h, 0x18, 0x05);
    command_add_arg(&command, af_mode);
    return pslr_command_x18(&command, true);
}

int pslr_set_af_point_sel(pslr_handle_t h, pslr_af_point_sel_t af_point_sel) {
    DPRINT("[C]\t\tpslr_set_af_point_sel(%X)\n", af_point_sel);
    pslr_command_t command;
    command_init(&command, h, 0x18, 0x06);
    command_add_arg(&command, af_point_sel);
    return pslr_command_x18(&command, true);
}

int pslr_select_af_point(pslr_handle_t h, pslr_af_point_t af_point) {
    DPRINT("[C]\t\tpslr_select_af_point(0x%x)\n", af_point);
    pslr_command_t command;
    command_init(&command, h, 0x18, 0x07);
    command_add_arg(&command, af_point);
    return pslr_command_x18(&command, true);
}

int pslr_set_white_balance(pslr_handle_t h, pslr_white_balance_mode_t white_balance_mode) {
    DPRINT("[C]\t\tpslr_set_white_balance(0x%X)\n", white_balance_mode);
    pslr_command_t command;
    command_init(&command, h, 0x18, 0x10);
    command_add_arg(&command, white_balance_mode);
    return pslr_command_x18(&command, true);
}

int pslr_set_white_balance_adjustment(pslr_handle_t h, pslr_white_balance_mode_t white_balance_mode,
                                      uint32_t white_balance_tint, uint32_t white_balance_temperature) {
    DPRINT("[C]\t\tpslr_set_white_balance_adjustment(mode=0x%X, tint=0x%X, temp=0x%X)\n", white_balance_mode,
           white_balance_tint, white_balance_temperature);
    pslr_command_t command;
    command_init(&command, h, 0x18, 0x11);
    command_add_arg(&command, white_balance_mode);
    command_add_arg(&command, white_balance_tint);
    command_add_arg(&command, white_balance_temperature);
    return pslr_command_x18(&command, true);
}

int pslr_set_image_format(pslr_handle_t h, pslr_image_format_t format) {
    DPRINT("[C]\t\tpslr_set_image_format(%X)\n", format);

    if (format > PSLR_IMAGE_FORMAT_MAX) {
        return PSLR_PARAM;
    }

    pslr_command_t command;
    command_init(&command, h, 0x18, 0x12);
    command_add_arg(&command, 1);
    command_add_arg(&command, format);
    return pslr_command_x18(&command, true);
}

int pslr_set_jpeg_stars(pslr_handle_t h, int jpeg_stars) {
    DPRINT("[C]\t\tpslr_set_jpeg_stars(%X)\n", jpeg_stars);

    ipslr_handle_t *p = (ipslr_handle_t *)h;
    if (jpeg_stars > p->model->max_jpeg_stars) {
        return PSLR_PARAM;
    }

    int hwqual = get_hw_jpeg_quality(p->model, jpeg_stars);
    pslr_command_t command;
    command_init(&command, h, 0x18, 0x13);
    command_add_arg(&command, 1);
    command_add_arg(&command, hwqual);
    return pslr_command_x18(&command, true);
}

int pslr_set_jpeg_resolution(pslr_handle_t h, int megapixel) {
    DPRINT("[C]\t\tpslr_set_jpeg_resolution(%X)\n", megapixel);
    ipslr_handle_t *p = (ipslr_handle_t *)h;
    int hwres = get_hw_jpeg_resolution(p->model, megapixel);
    pslr_command_t command;
    command_init(&command, h, 0x18, 0x14);
    command_add_arg(&command, 1);
    command_add_arg(&command, hwres);
    return pslr_command_x18(&command, true);
}

int pslr_set_iso(pslr_handle_t h, uint32_t value, uint32_t auto_min_value, uint32_t auto_max_value) {
    DPRINT("[C]\t\tpslr_set_iso(0x%X, auto_min=%X, auto_max=%X)\n", value, auto_min_value, auto_max_value);
    pslr_command_t command;
    command_init(&command, h, 0x18, 0x15);
    command_add_arg(&command, value);
    command_add_arg(&command, auto_min_value);
    command_add_arg(&command, auto_max_value);
    return pslr_command_x18(&command, true);
}

int pslr_set_shutter_speed(pslr_handle_t h, pslr_rational_t value) {
    DPRINT("[C]\t\tpslr_set_shutter(%x)\n", value);
    pslr_command_t command;
    command_init(&command, h, 0x18, 0x16);
    command_add_arg(&command, value.nom);
    command_add_arg(&command, value.denom);
    return pslr_command_x18(&command, true);
}

int pslr_set_aperture(pslr_handle_t h, pslr_rational_t value) {
    DPRINT("[C]\t\tpslr_set_aperture(%x)\n", value);
    pslr_command_t command;
    command_init(&command, h, 0x18, 0x17);
    command_add_arg(&command, value.nom);
    command_add_arg(&command, value.denom);
    command_add_arg(&command, 0);
    return pslr_command_x18(&command, false);
}

int pslr_set_exposure_compensation(pslr_handle_t h, pslr_rational_t value) {
    DPRINT("[C]\t\tpslr_set_exposure_compensation(0x%X)\n", value);
    pslr_command_t command;
    command_init(&command, h, 0x18, 0x18);
    command_add_arg(&command, value.nom);
    command_add_arg(&command, value.denom);
    return pslr_command_x18(&command, true);
}

int pslr_set_flash_exposure_compensation(pslr_handle_t h, pslr_rational_t value) {
    DPRINT("[C]\t\tpslr_set_flash_exposure_compensation(%X)\n", value);
    pslr_command_t command;
    command_init(&command, h, 0x18, 0x1A);
    command_add_arg(&command, value.nom);
    command_add_arg(&command, value.denom);
    return pslr_command_x18(&command, true);
}

int pslr_set_jpeg_image_tone(pslr_handle_t h, pslr_jpeg_image_tone_t image_tone) {
    DPRINT("[C]\t\tpslr_set_jpeg_image_tone(%X)\n", image_tone);

    if (image_tone < 0 || image_tone > PSLR_JPEG_IMAGE_TONE_MAX) {
        return PSLR_PARAM;
    }

    pslr_command_t command;
    command_init(&command, h, 0x18, 0x1B);
    command_add_arg(&command, image_tone);
    return pslr_command_x18(&command, true);
}

int pslr_set_drive_mode(pslr_handle_t h, pslr_drive_mode_t drive_mode) {
    DPRINT("[C]\t\tpslr_set_drive_mode(%X)\n", drive_mode);
    pslr_command_t command;
    command_init(&command, h, 0x18, 0x1C);
    command_add_arg(&command, drive_mode);
    return pslr_command_x18(&command, true);
}

int pslr_set_raw_format(pslr_handle_t h, pslr_raw_format_t format) {
    DPRINT("[C]\t\tpslr_set_raw_format(%X)\n", format);
    if (format > PSLR_RAW_FORMAT_MAX) {
        return PSLR_PARAM;
    }
    pslr_command_t command;
    command_init(&command, h, 0x18, 0x1F);
    command_add_arg(&command, 1);
    command_add_arg(&command, format);
    return pslr_command_x18(&command, true);
}

int pslr_set_jpeg_saturation(pslr_handle_t h, int32_t saturation) {
    DPRINT("[C]\t\tpslr_set_jpeg_saturation(%X)\n", saturation);
    ipslr_handle_t *p = (ipslr_handle_t *)h;
    int hw_saturation = saturation + (pslr_get_model_jpeg_property_levels(h) - 1) / 2;
    if (hw_saturation < 0 || hw_saturation >= p->model->jpeg_property_levels) {
        return PSLR_PARAM;
    }

    pslr_command_t command;
    command_init(&command, h, 0x18, 0x20);
    command_add_arg(&command, 0);
    command_add_arg(&command, hw_saturation);
    return pslr_command_x18(&command, false);
}

int pslr_set_jpeg_sharpness(pslr_handle_t h, int32_t sharpness) {
    DPRINT("[C]\t\tpslr_set_jpeg_sharpness(%X)\n", sharpness);
    ipslr_handle_t *p = (ipslr_handle_t *)h;
    int hw_sharpness = sharpness + (pslr_get_model_jpeg_property_levels(h) - 1) / 2;
    if (hw_sharpness < 0 || hw_sharpness >= p->model->jpeg_property_levels) {
        return PSLR_PARAM;
    }
    pslr_command_t command;
    command_init(&command, h, 0x18, 0x21);
    command_add_arg(&command, 0);
    command_add_arg(&command, hw_sharpness);
    return pslr_command_x18(&command, false);
}

int pslr_set_jpeg_contrast(pslr_handle_t h, int32_t contrast) {
    DPRINT("[C]\t\tpslr_set_jpeg_contrast(%X)\n", contrast);
    ipslr_handle_t *p = (ipslr_handle_t *)h;
    int hw_contrast = contrast + (pslr_get_model_jpeg_property_levels(h) - 1) / 2;
    if (hw_contrast < 0 || hw_contrast >= p->model->jpeg_property_levels) {
        return PSLR_PARAM;
    }
    pslr_command_t command;
    command_init(&command, h, 0x18, 0x22);
    command_add_arg(&command, 0);
    command_add_arg(&command, hw_contrast);
    return pslr_command_x18(&command, false);
}

int pslr_set_color_space(pslr_handle_t h, pslr_color_space_t color_space) {
    DPRINT("[C]\t\tpslr_set_raw_format(%X)\n", color_space);
    if (color_space > PSLR_COLOR_SPACE_MAX) {
        return PSLR_PARAM;
    }
    pslr_command_t command;
    command_init(&command, h, 0x18, 0x23);
    command_add_arg(&command, color_space);
    return pslr_command_x18(&command, true);
}

int pslr_set_jpeg_hue(pslr_handle_t h, int32_t hue) {
    DPRINT("[C]\t\tpslr_set_jpeg_hue(%X)\n", hue);
    ipslr_handle_t *p = (ipslr_handle_t *)h;
    int hw_hue = hue + (pslr_get_model_jpeg_property_levels(h) - 1) / 2;
    DPRINT("hw_hue: %d\n", hw_hue);
    if (hw_hue < 0 || hw_hue >= p->model->jpeg_property_levels) {
        return PSLR_PARAM;
    }
    pslr_command_t command;
    command_init(&command, h, 0x18, 0x25);
    command_add_arg(&command, 0);
    command_add_arg(&command, hw_hue);
    return pslr_command_x18(&command, false);
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
    DPRINT("[C]\t\tpslr_set_adj_data(%d, %s)\n", mode, bool_string(debug_mode));
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
