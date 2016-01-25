/*
    pkTriggerCord
    Copyright (C) 2011-2016 Andras Salamon <andras.salamon@melda.info>
    Remote control of Pentax DSLR cameras.

    Support for K200D added by Jens Dreyer <jens.dreyer@udo.edu> 04/2011
    Support for K-r added by Vincenc Podobnik <vincenc.podobnik@gmail.com> 06/2011
    Support for K-30 added by Camilo Polymeris <cpolymeris@gmail.com> 09/2012
    Support for K-01 added by Ethan Queen <ethanqueen@gmail.com> 01/2013
    Support for K-3 added by Tao Wang <twang2218@gmail.com> 01/2016

    based on:

    PK-Remote
    Remote control of Pentax DSLR cameras.
    Copyright (C) 2008 Pontus Lidman <pontus@lysator.liu.se>

    PK-Remote for Windows
    Copyright (C) 2010 Tomasz Kos

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

#ifndef PSLR_H
#define PSLR_H

#include "pslr_enum.h"
#include "pslr_scsi.h"
#include "pslr_model.h"
#include "pslr_command.h"

#define PSLR_LIGHT_METER_AE_LOCK 0x8

typedef enum {
    USER_FILE_FORMAT_PEF,
    USER_FILE_FORMAT_DNG,
    USER_FILE_FORMAT_JPEG,
    USER_FILE_FORMAT_MAX
} user_file_format;

typedef struct {
    user_file_format uff;
    const char *file_format_name;
    const char *extension;
} user_file_format_t;

extern user_file_format_t file_formats[3];

user_file_format_t *get_file_format_t( user_file_format uff );

typedef enum {
    PSLR_CUSTOM_SENSITIVITY_STEPS_1EV,
    PSLR_CUSTOM_SENSITIVITY_STEPS_AS_EV,
    PSLR_CUSTOM_SENSITIVITY_STEPS_MAX
} pslr_custom_sensitivity_steps_t;

typedef void *pslr_handle_t;

typedef void (*pslr_progress_callback_t)(uint32_t current, uint32_t total);

void sleep_sec(double sec);

pslr_handle_t pslr_init(char *model, char *device);
int pslr_shutdown(pslr_handle_t h);
const char *pslr_model(uint32_t id);

int pslr_get_status(pslr_handle_t h, pslr_status *sbuf);
int pslr_get_status_buffer(pslr_handle_t h, uint8_t *st_buf);

char *collect_status_info( pslr_handle_t h, pslr_status status );

int pslr_get_buffer(pslr_handle_t h, int bufno, pslr_buffer_type_t type, int resolution,
                    uint8_t **pdata, uint32_t *pdatalen);

int pslr_set_progress_callback(pslr_handle_t h, pslr_progress_callback_t cb,
                               uintptr_t user_data);

int pslr_set_user_file_format(pslr_handle_t h, user_file_format uff);
user_file_format get_user_file_format( pslr_status *st );

int pslr_buffer_open(pslr_handle_t h, int bufno, pslr_buffer_type_t type, int resolution);
uint32_t pslr_buffer_read(pslr_handle_t h, uint8_t *buf, uint32_t size);
void pslr_buffer_close(pslr_handle_t h);
uint32_t pslr_buffer_get_size(pslr_handle_t h);


const char *pslr_camera_name(pslr_handle_t h);
int pslr_get_model_max_jpeg_stars(pslr_handle_t h);
int pslr_get_model_jpeg_property_levels(pslr_handle_t h);
int pslr_get_model_buffer_size(pslr_handle_t h);
int pslr_get_model_fastest_shutter_speed(pslr_handle_t h);
int pslr_get_model_base_iso_min(pslr_handle_t h);
int pslr_get_model_base_iso_max(pslr_handle_t h);
int pslr_get_model_extended_iso_min(pslr_handle_t h);
int pslr_get_model_extended_iso_max(pslr_handle_t h);
int *pslr_get_model_jpeg_resolutions(pslr_handle_t h);
bool pslr_get_model_only_limited(pslr_handle_t h);
bool pslr_get_model_has_jpeg_hue(pslr_handle_t h);
bool pslr_get_model_need_exposure_conversion(pslr_handle_t h);
pslr_jpeg_image_tone_t pslr_get_model_max_supported_image_tone(pslr_handle_t h);

pslr_buffer_type_t pslr_get_jpeg_buffer_type(pslr_handle_t h, int quality);
int pslr_get_jpeg_resolution(pslr_handle_t h, int hwres);

char *format_rational( pslr_rational_t rational, char * fmt );

// int pslr_test( pslr_handle_t h, bool cmd9_wrap, int subcommand, int argnum,  int arg1, int arg2, int arg3, int arg4);

void write_debug( const char* message, ... );

/*  ------------------------------------------------------------    */
int pslr_connect(pslr_handle_t h);
int pslr_disconnect(pslr_handle_t h);

int pslr_download(pslr_handle_t h, uint32_t address, pslr_data_t *data);
int pslr_upload(pslr_handle_t h, uint32_t address, pslr_data_t *data);
int pslr_set_debug_mode(pslr_handle_t h, bool debug_mode);

#endif
