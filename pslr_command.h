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
#ifndef PSLR_COMMAND_H
#define PSLR_COMMAND_H

#include "pslr_common.h"
#include "pslr_enum.h"

/*  ------------------------------------------------------------    */

//  Group 0x00
int pslr_set_mode(pslr_handle_t h, uint32_t mode);
int pslr_get_short_status(pslr_handle_t h, pslr_data_t *data);
int pslr_identify(pslr_handle_t h);
int pslr_connect_legacy(pslr_handle_t h);
int pslr_get_full_status(pslr_handle_t h, pslr_data_t *data);
int pslr_dsp_task_wu_req(pslr_handle_t h, uint32_t mode);
//  Group 0x06
int pslr_request_download(pslr_handle_t h, uint32_t address, int32_t length);
int pslr_do_download(pslr_handle_t h, pslr_data_t *data);
int pslr_request_upload(pslr_handle_t h, uint32_t address, int32_t length);
int pslr_do_upload(pslr_handle_t h, pslr_data_t *data);
int pslr_get_transfer_status(pslr_handle_t h, pslr_data_t *data);
//  Group 0x10
int pslr_shutter(pslr_handle_t h, bool fullpress);
int pslr_ae_lock(pslr_handle_t h, bool lock);
int pslr_green_button(pslr_handle_t h);
int pslr_do_connect(pslr_handle_t h, bool connect);
int pslr_continuous(pslr_handle_t h);
int pslr_bulb(pslr_handle_t h, bool start);
int pslr_dust_removal(pslr_handle_t h);
//  Group 0x18
int pslr_set_exposure_mode(pslr_handle_t h, pslr_exposure_mode_t exposure_mode);
int pslr_set_ae_metering_mode(pslr_handle_t h, pslr_ae_metering_t ae_metering_mode);
int pslr_set_flash_mode(pslr_handle_t h, pslr_flash_mode_t flash_mode);
int pslr_set_af_mode(pslr_handle_t h, pslr_af_mode_t af_mode);
int pslr_set_af_point_sel(pslr_handle_t h, pslr_af_point_sel_t af_point_sel);
int pslr_select_af_point(pslr_handle_t h, pslr_af_point_t af_point);
int pslr_set_white_balance(pslr_handle_t h, pslr_white_balance_mode_t white_balance_mode);
int pslr_set_white_balance_adjustment(pslr_handle_t h, pslr_white_balance_mode_t white_balance_mode,
                                      uint32_t white_balance_tint, uint32_t white_balance_temperature);
int pslr_set_image_format(pslr_handle_t h, pslr_image_format_t format);
int pslr_set_jpeg_stars(pslr_handle_t h, int jpeg_stars);
int pslr_set_jpeg_resolution(pslr_handle_t h, int megapixel);
int pslr_set_iso(pslr_handle_t h, uint32_t value, uint32_t auto_min_value, uint32_t auto_max_value);
int pslr_set_shutter_speed(pslr_handle_t h, pslr_rational_t value);
int pslr_set_aperture(pslr_handle_t h, pslr_rational_t value);
int pslr_set_exposure_compensation(pslr_handle_t h, pslr_rational_t value);
int pslr_set_flash_exposure_compensation(pslr_handle_t h, pslr_rational_t value);
int pslr_set_jpeg_image_tone(pslr_handle_t h, pslr_jpeg_image_tone_t image_tone);
int pslr_set_drive_mode(pslr_handle_t h, pslr_drive_mode_t drive_mode);
int pslr_set_raw_format(pslr_handle_t h, pslr_raw_format_t format);
int pslr_set_jpeg_saturation(pslr_handle_t h, int32_t saturation);
int pslr_set_jpeg_sharpness(pslr_handle_t h, int32_t sharpness);
int pslr_set_jpeg_contrast(pslr_handle_t h, int32_t contrast);
int pslr_set_color_space(pslr_handle_t h, pslr_color_space_t color_space);
int pslr_set_jpeg_hue(pslr_handle_t h, int32_t hue);
//  Group 0x23
int pslr_write_adj_data(pslr_handle_t h, uint32_t value);
int pslr_set_adj_mode_flag(pslr_handle_t h, uint32_t mode, uint32_t value);
int pslr_get_adj_mode_flag(pslr_handle_t h, uint32_t mode, pslr_data_t *data);
int pslr_set_adj_data(pslr_handle_t h, uint32_t mode, bool debug_mode);
int pslr_get_adj_data(pslr_handle_t h, uint32_t mode, pslr_data_t *data);

#endif // PSLR_COMMAND_H
