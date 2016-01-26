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

#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <stdlib.h>
#include <signal.h>
#include <unistd.h>
#include <stdbool.h>
#include <stdarg.h>
#include <dirent.h>
#include <math.h>

#include "pslr.h"
#include "pslr_scsi.h"
#include "pslr_lens.h"

#define BLKSZ 65536  /* Block size for downloads; if too big, we get
                     * memory allocation error from sg driver */
#define BLOCK_SIZE 0x10000 /* TODO: only 0x200 in download random range */
#define BLOCK_RETRY 3 /* Number of retries, since we can occasionally
                       * get SCSI errors when downloading data */

#define CHECK(x) do {                           \
        int __r;                                \
        __r = (x);                                                      \
        if (__r != PSLR_OK) {                                           \
            fprintf(stderr, "%s:%d:%s failed: %d\n", __FILE__, __LINE__, #x, __r); \
            return __r;                                                 \
        }                                                               \
    } while (0)

void sleep_sec(double sec) {
    int i;
    for(i=0; i<floor(sec); ++i) {
	usleep(999999); // 1000000 is not working for Windows
    }
    usleep(1000000*(sec-floor(sec)));
}

ipslr_handle_t pslr;

void hexdump(uint8_t *buf, uint32_t bufLen);

static pslr_progress_callback_t progress_callback = NULL;

user_file_format_t file_formats[3] = {
    { USER_FILE_FORMAT_PEF, "PEF", "pef"},
    { USER_FILE_FORMAT_DNG, "DNG", "dng"},
    { USER_FILE_FORMAT_JPEG, "JPEG", "jpg"},
};


const char* valid_vendors[3] = {"PENTAX", "SAMSUNG", "RICOHIMG"};
const char* valid_models[3] = {"DIGITAL_CAMERA", "DSC", "Digital Camera"};

user_file_format_t *get_file_format_t( user_file_format uff ) {
    int i;
    for (i = 0; i<sizeof(file_formats) / sizeof(file_formats[0]); i++) {
        if (file_formats[i].uff == uff) {
            return &file_formats[i];
        }
    }
    return NULL;
}

int pslr_set_user_file_format(pslr_handle_t h, user_file_format uff) {
    switch( uff ) {
    case USER_FILE_FORMAT_PEF:
	pslr_set_image_format(h, PSLR_IMAGE_FORMAT_RAW);
	pslr_set_raw_format(h, PSLR_RAW_FORMAT_PEF);
	break;
    case USER_FILE_FORMAT_DNG:
	pslr_set_image_format(h, PSLR_IMAGE_FORMAT_RAW);
	pslr_set_raw_format(h, PSLR_RAW_FORMAT_DNG);
	break;
    case USER_FILE_FORMAT_JPEG:
	pslr_set_image_format(h, PSLR_IMAGE_FORMAT_JPEG);
	break;
    case USER_FILE_FORMAT_MAX:
	return PSLR_PARAM;
    }
    return PSLR_OK;
}

user_file_format get_user_file_format( pslr_status *st ) {
    int rawfmt = st->raw_format;
    int imgfmt = st->image_format;
    if (imgfmt == PSLR_IMAGE_FORMAT_JPEG) {
        return USER_FILE_FORMAT_JPEG;
    } else {
        if (rawfmt == PSLR_RAW_FORMAT_PEF) {
            return USER_FILE_FORMAT_PEF;
        } else {
            return USER_FILE_FORMAT_DNG;
	}
    }
}

pslr_handle_t pslr_init( char *model, char *device ) {
    int fd;
    char vendorId[20];
    char productId[20];
    int driveNum;
    char **drives;
    const char *camera_name;

    DPRINT("[C]\tplsr_init()\n");

    if( device == NULL ) {
	drives = get_drives(&driveNum);
    } else {
	driveNum = 1;
	drives = malloc( driveNum * sizeof(char*) );
        drives[0] = malloc( strlen( device )+1 );
	strncpy( drives[0], device, strlen( device ) );
	drives[0][strlen(device)]='\0';
    }
    int i;
    for( i=0; i<driveNum; ++i ) {
	pslr_result result = get_drive_info( drives[i], &fd, vendorId, sizeof(vendorId), productId, sizeof(productId));

	DPRINT("\tChecking drive:  %s %s %s\n", drives[i], vendorId, productId);
	if( find_in_array( valid_vendors, sizeof(valid_vendors)/sizeof(valid_vendors[0]),vendorId) != -1
	    && find_in_array( valid_models, sizeof(valid_models)/sizeof(valid_models[0]), productId) != -1 ) {
	    if( result == PSLR_OK ) {
		DPRINT("\tFound camera %s %s\n", vendorId, productId);
		pslr.fd = fd;
		if( model != NULL ) {
		    // user specified the camera model
		    camera_name = pslr_camera_name( &pslr );
		    DPRINT("\tName of the camera: %s\n", camera_name);
		    if( str_comparison_i( camera_name, model, strlen( camera_name) ) == 0 ) {
			return &pslr;
		    } else {
			DPRINT("\tIgnoring camera %s %s\n", vendorId, productId);
			pslr_shutdown ( &pslr );
			pslr.id = 0;
			pslr.model = NULL;
		    }
		} else {
		    return &pslr;
		}
	    } else {
		DPRINT("\tCannot get drive info of Pentax camera. Please do not forget to install the program using 'make install'\n");
		// found the camera but communication is not possible
		close( fd );
		continue;
	    }
	} else {
	    close_drive( &fd );
	    continue;
	}
    }
    DPRINT("\tcamera not found\n");
    return NULL;
}

int pslr_shutdown(pslr_handle_t h) {
    DPRINT("[C]\tpslr_shutdown()\n");
    ipslr_handle_t *p = (ipslr_handle_t *) h;
    close_drive(&p->fd);
    return PSLR_OK;
}

int pslr_get_status(pslr_handle_t h, pslr_status *ps) {
    DPRINT("[C]\tpslr_get_status()\n");
    ipslr_handle_t *p = (ipslr_handle_t *) h;
    memset( ps, 0, sizeof( pslr_status ));
    CHECK(pslr_get_full_status(p));
    memcpy(ps, &p->status, sizeof (pslr_status));
    return PSLR_OK;
}

char *format_rational( pslr_rational_t rational, char * fmt ) {
    char *ret = malloc(32);
    if( rational.denom == 0 ) {
	snprintf( ret, 32, "unknown" );
    } else {
	snprintf( ret, 32, fmt, 1.0 * rational.nom / rational.denom );
    }
    return ret;
}

char *get_white_balance_single_adjust_str( uint32_t adjust, char negativeChar, char positiveChar ) {
    char *ret = malloc(4);
    if( adjust < 7 ) {
	snprintf( ret, 4, "%c%d", negativeChar, 7-adjust);
    } else if( adjust > 7 ) {
	snprintf( ret, 4, "%c%d", positiveChar, adjust-7);
    } else {
	ret = "";
    }
    return ret;
}

char *get_white_balance_adjust_str( uint32_t adjust_mg, uint32_t adjust_ba ) {
    char *ret = malloc(8);
    if( adjust_mg != 7 || adjust_ba != 7 ) {
	snprintf(ret, 8, "%s%s", get_white_balance_single_adjust_str(adjust_mg, 'M', 'G'),get_white_balance_single_adjust_str(adjust_ba, 'B', 'A')); } else {
	ret = "0";
    }
    return ret;
}

char *collect_status_info( pslr_handle_t h, pslr_status status ) {
    char *strbuffer = malloc(8192);
    sprintf(strbuffer,"%-32s: %d\n", "current iso", status.current_iso);
    sprintf(strbuffer+strlen(strbuffer),"%-32s: %d/%d\n", "current shutter speed", status.current_shutter_speed.nom, status.current_shutter_speed.denom);
    sprintf(strbuffer+strlen(strbuffer),"%-32s: %d/%d\n", "camera max shutter speed", status.max_shutter_speed.nom, status.max_shutter_speed.denom);
    sprintf(strbuffer+strlen(strbuffer),"%-32s: %s\n", "current aperture", format_rational( status.current_aperture, "%.1f"));
    sprintf(strbuffer+strlen(strbuffer),"%-32s: %s\n", "lens max aperture", format_rational( status.lens_max_aperture, "%.1f"));
    sprintf(strbuffer+strlen(strbuffer),"%-32s: %s\n", "lens min aperture", format_rational( status.lens_min_aperture, "%.1f"));
    sprintf(strbuffer+strlen(strbuffer),"%-32s: %d/%d\n", "set shutter speed", status.set_shutter_speed.nom, status.set_shutter_speed.denom);
    sprintf(strbuffer+strlen(strbuffer),"%-32s: %s\n", "set aperture", format_rational( status.set_aperture, "%.1f"));
    sprintf(strbuffer+strlen(strbuffer),"%-32s: %d\n", "fixed iso", status.fixed_iso);
    sprintf(strbuffer+strlen(strbuffer),"%-32s: %d-%d\n", "auto iso", status.auto_iso_min,status.auto_iso_max);
    sprintf(strbuffer+strlen(strbuffer),"%-32s: %d\n", "jpeg quality", status.jpeg_quality);
    sprintf(strbuffer+strlen(strbuffer),"%-32s: %dM\n", "jpeg resolution", pslr_get_jpeg_resolution( h, status.jpeg_resolution));
    sprintf(strbuffer+strlen(strbuffer),"%-32s: %s\n", "jpeg image tone", get_pslr_jpeg_image_tone_str(status.jpeg_image_tone));
    sprintf(strbuffer+strlen(strbuffer),"%-32s: %d\n", "jpeg saturation", status.jpeg_saturation);
    sprintf(strbuffer+strlen(strbuffer),"%-32s: %d\n", "jpeg contrast", status.jpeg_contrast);
    sprintf(strbuffer+strlen(strbuffer),"%-32s: %d\n", "jpeg sharpness", status.jpeg_sharpness);
    sprintf(strbuffer+strlen(strbuffer),"%-32s: %d\n", "jpeg hue", status.jpeg_hue);
    sprintf(strbuffer+strlen(strbuffer),"%-32s: %s mm\n", "zoom", format_rational(status.zoom, "%.2f"));
    sprintf(strbuffer+strlen(strbuffer),"%-32s: %d\n", "focus", status.focus);
    sprintf(strbuffer+strlen(strbuffer),"%-32s: %s\n", "color space", get_pslr_color_space_str(status.color_space));
    sprintf(strbuffer+strlen(strbuffer),"%-32s: %d\n", "image format", status.image_format);
    sprintf(strbuffer+strlen(strbuffer),"%-32s: %s\n", "raw format", get_pslr_raw_format_str(status.raw_format));
    sprintf(strbuffer+strlen(strbuffer),"%-32s: %d\n", "light meter flags", status.light_meter_flags);
    sprintf(strbuffer+strlen(strbuffer),"%-32s: %s\n", "ec", format_rational( status.ec, "%.2f" ) );
    sprintf(strbuffer+strlen(strbuffer),"%-32s: %s\n", "custom ev steps", get_pslr_custom_ev_steps_str(status.custom_ev_steps));
    sprintf(strbuffer+strlen(strbuffer),"%-32s: %d\n", "custom sensitivity steps", status.custom_sensitivity_steps);
    sprintf(strbuffer+strlen(strbuffer),"%-32s: %d (%s)\n", "exposure mode", status.exposure_mode, get_pslr_exposure_submode_str(status.exposure_submode));
    sprintf(strbuffer+strlen(strbuffer),"%-32s: %d\n", "user mode flag", status.user_mode_flag);
    sprintf(strbuffer+strlen(strbuffer),"%-32s: %s\n", "ae metering mode", get_pslr_ae_metering_str(status.ae_metering_mode));
    sprintf(strbuffer+strlen(strbuffer),"%-32s: %s\n", "af mode", get_pslr_af_mode_str(status.af_mode));
    sprintf(strbuffer+strlen(strbuffer),"%-32s: %s\n", "af point select", get_pslr_af_point_sel_str(status.af_point_select));
    sprintf(strbuffer+strlen(strbuffer),"%-32s: %d\n", "selected af point", status.selected_af_point);
    sprintf(strbuffer+strlen(strbuffer),"%-32s: %d\n", "focused af point", status.focused_af_point);
    sprintf(strbuffer+strlen(strbuffer),"%-32s: %s\n", "drive mode", get_pslr_drive_mode_str(status.drive_mode));
    sprintf(strbuffer+strlen(strbuffer),"%-32s: %s\n", "auto bracket mode", status.auto_bracket_mode > 0 ? "on" : "off");
    sprintf(strbuffer+strlen(strbuffer),"%-32s: %d\n", "auto bracket picture count", status.auto_bracket_picture_count);
    sprintf(strbuffer+strlen(strbuffer),"%-32s: %s\n", "auto bracket ev", format_rational(status.auto_bracket_ev, "%.2f"));
    sprintf(strbuffer+strlen(strbuffer),"%-32s: %s\n", "shake reduction", status.shake_reduction > 0 ? "on" : "off");
    sprintf(strbuffer+strlen(strbuffer),"%-32s: %s\n", "white balance mode", get_pslr_white_balance_mode_str(status.white_balance_mode));
    sprintf(strbuffer+strlen(strbuffer),"%-32s: %s\n", "white balance adjust", get_white_balance_adjust_str(status.white_balance_adjust_mg, status.white_balance_adjust_ba));
    sprintf(strbuffer+strlen(strbuffer),"%-32s: %s\n", "flash mode", get_pslr_flash_mode_str(status.flash_mode));
    sprintf(strbuffer+strlen(strbuffer),"%-32s: %.2f\n", "flash exposure compensation", (1.0 * status.flash_exposure_compensation/256));
    sprintf(strbuffer+strlen(strbuffer),"%-32s: %.2f\n", "manual mode ev", (1.0 * status.manual_mode_ev / 10));
    sprintf(strbuffer+strlen(strbuffer),"%-32s: %s\n", "lens", get_lens_name(status.lens_id1, status.lens_id2));
    sprintf(strbuffer+strlen(strbuffer),"%-32s: %.2fV %.2fV %.2fV %.2fV\n", "battery", 0.01 * status.battery_1, 0.01 * status.battery_2, 0.01 * status.battery_3, 0.01 * status.battery_4);
    return strbuffer;
}

int pslr_get_status_buffer(pslr_handle_t h, uint8_t *st_buf) {
    DPRINT("[C]\tpslr_get_status_buffer()\n");
    ipslr_handle_t *p = (ipslr_handle_t *) h;
    memset( st_buf, 0, MAX_STATUS_BUF_SIZE);
//    CHECK(ipslr_status_full(p, &p->status));
    pslr_get_full_status(p);
    memcpy(st_buf, p->status_buffer, MAX_STATUS_BUF_SIZE);
    return PSLR_OK;
}

int pslr_get_buffer(pslr_handle_t h, int bufno, pslr_buffer_type_t type, int resolution,
        uint8_t **ppData, uint32_t *pLen) {
    DPRINT("[C]\tpslr_get_buffer()\n");
    uint8_t *buf = 0;
    int ret;
    ret = pslr_buffer_open(h, bufno, type, resolution);
    if( ret != PSLR_OK ) {
	return ret;
    }

    uint32_t size = pslr_buffer_get_size(h);
    buf = malloc(size);
    if (!buf) {
	return PSLR_NO_MEMORY;
    }

    int bytes = pslr_buffer_read(h, buf, size);

    if( bytes != size ) {
	return PSLR_READ_ERROR;
    }
    pslr_buffer_close(h);
    if (ppData) {
	*ppData = buf;
    }
    if (pLen) {
	*pLen = size;
    }

    return PSLR_OK;
}

int pslr_set_progress_callback(pslr_handle_t h, pslr_progress_callback_t cb, uintptr_t user_data) {
    progress_callback = cb;
    return PSLR_OK;
}

// int pslr_test( pslr_handle_t h, bool cmd9_wrap, int subcommand, int argnum,  int arg1, int arg2, int arg3, int arg4) {
//   DPRINT("[C]\tpslr_test(wrap=%d, subcommand=0x%x, %x, %x, %x, %x)\n", cmd9_wrap, subcommand, arg1, arg2, arg3, arg4);
//     ipslr_handle_t *p = (ipslr_handle_t *) h;
//     return ipslr_handle_command_x18( p, cmd9_wrap, subcommand, argnum, arg1, arg2, arg3, arg4);
// }


int _get_user_jpeg_resolution( ipslr_model_info_t *model, int hwres ) {
    return model->jpeg_resolutions[hwres];
}

int pslr_get_jpeg_resolution(pslr_handle_t h, int hwres) {
    ipslr_handle_t *p = (ipslr_handle_t *) h;
    return _get_user_jpeg_resolution( p->model, hwres );
}

int pslr_buffer_open(pslr_handle_t h, int bufno, pslr_buffer_type_t buftype, int bufres) {
    DPRINT("[C]\tpslr_buffer_open(#%X, type=%X, res=%X)\n", bufno, buftype, bufres);
    pslr_buffer_segment_info_t info;
    uint16_t bufs;
    uint32_t buf_total = 0;
    int i, j;
    int ret;
    int retry = 0;
    int retry2 = 0;

    ipslr_handle_t *p = (ipslr_handle_t *) h;

    memset(&info, 0, sizeof (info));

    CHECK(pslr_get_full_status(p));
    bufs = p->status.bufmask;
    DPRINT("\tp->status.bufmask = %x\n", p->status.bufmask);

    if( p->model->parser_function && (bufs & (1 << bufno)) == 0) {
	// do not check this for limited support cameras
        DPRINT("\tNo buffer data (%d)\n", bufno);
        return PSLR_READ_ERROR;
    }

    while (retry < 3) {
        /* If we get response 0x82 from the camera, there is a
         * desynch. We can recover by stepping through segment infos
         * until we get the last one (b = 2). Retry up to 3 times. */
        ret = pslr_select_buffer(h, bufno, buftype, bufres);
        if (ret == PSLR_OK)
            break;

        retry++;
        retry2 = 0;
        /* Try up to 9 times to reach segment info type 2 (last
         * segment) */
        do {
            CHECK(pslr_get_buffer_segment_info(h, &info));
            CHECK(pslr_next_buffer_segment(h));
            DPRINT("\tRecover: b=%d\n", info.b);
        } while (++retry2 < 10 && info.b != 2);
    }

    if (retry == 3)
        return ret;

    i = 0;
    j = 0;
    do {
        CHECK(pslr_get_buffer_segment_info(h, &info));
        DPRINT("\t%d: Addr: 0x%X Len: %d(0x%08X) B=%d\n", i, info.addr, info.length, info.length, info.b);
        if (info.b == 4)
            p->segments[j].offset = info.length;
        else if (info.b == 3) {
            if (j == MAX_SEGMENTS) {
                DPRINT("\tToo many segments.\n");
                return PSLR_NO_MEMORY;
            }
            p->segments[j].addr = info.addr;
            p->segments[j].length = info.length;
            j++;
        }
        CHECK(pslr_next_buffer_segment(h));
        buf_total += info.length;
        i++;
    } while (i < 9 && info.b != 2);
    p->segment_count = j;
    p->offset = 0;
    return PSLR_OK;
}

uint32_t pslr_buffer_read(pslr_handle_t h, uint8_t *buf, uint32_t size) {
    ipslr_handle_t *p = (ipslr_handle_t *) h;
    int i;
    uint32_t pos = 0;
    uint32_t seg_offs;
    uint32_t addr;
    uint32_t blksz;
    int ret;

    DPRINT("[C]\tpslr_buffer_read(%X, %d)\n", buf, size);

    /* Find current segment */
    for (i = 0; i < p->segment_count; i++) {
        if (p->offset < pos + p->segments[i].length)
            break;
        pos += p->segments[i].length;
    }

    seg_offs = p->offset - pos;
    addr = p->segments[i].addr + seg_offs;

    /* Compute block size */
    blksz = size;
    if (blksz > p->segments[i].length - seg_offs)
        blksz = p->segments[i].length - seg_offs;
    if (blksz > BLKSZ)
        blksz = BLKSZ;

//    DPRINT("File offset %d segment: %d offset %d address 0x%x read size %d\n", p->offset,
//           i, seg_offs, addr, blksz);
    pslr_data_t data;
    data.data = buf;
    data.length = blksz;
    ret = pslr_download(h, addr, &data);
    if (ret != PSLR_OK)
        return 0;
    p->offset += blksz;
    return blksz;
}

uint32_t pslr_buffer_get_size(pslr_handle_t h) {
    ipslr_handle_t *p = (ipslr_handle_t *) h;
    int i;
    uint32_t len = 0;
    for (i = 0; i < p->segment_count; i++) {
        len += p->segments[i].length;
    }
    DPRINT("\tbuffer get size:%d\n",len);
    return len;
}

void pslr_buffer_close(pslr_handle_t h) {
    ipslr_handle_t *p = (ipslr_handle_t *) h;
    memset(&p->segments[0], 0, sizeof (p->segments));
    p->offset = 0;
    p->segment_count = 0;
}

int pslr_get_model_max_jpeg_stars(pslr_handle_t h) {
    ipslr_handle_t *p = (ipslr_handle_t *) h;
    return p->model->max_jpeg_stars;
}

int pslr_get_model_buffer_size(pslr_handle_t h) {
    ipslr_handle_t *p = (ipslr_handle_t *) h;
    return p->model->buffer_size;
}

int pslr_get_model_jpeg_property_levels(pslr_handle_t h) {
    ipslr_handle_t *p = (ipslr_handle_t *) h;
    return p->model->jpeg_property_levels;
}

int *pslr_get_model_jpeg_resolutions(pslr_handle_t h) {
    ipslr_handle_t *p = (ipslr_handle_t *) h;
    return p->model->jpeg_resolutions;
}

bool pslr_get_model_only_limited(pslr_handle_t h) {
    ipslr_handle_t *p = (ipslr_handle_t *) h;
    return p->model->buffer_size == 0 && !p->model->parser_function;
}

bool pslr_get_model_has_jpeg_hue(pslr_handle_t h) {
    ipslr_handle_t *p = (ipslr_handle_t *) h;
    return p->model->has_jpeg_hue;
}

bool pslr_get_model_need_exposure_conversion(pslr_handle_t h) {
    ipslr_handle_t *p = (ipslr_handle_t *) h;
    return p->model->need_exposure_mode_conversion;
}

int pslr_get_model_fastest_shutter_speed(pslr_handle_t h) {
    ipslr_handle_t *p = (ipslr_handle_t *) h;
    return p->model->fastest_shutter_speed;
}

int pslr_get_model_base_iso_min(pslr_handle_t h) {
    ipslr_handle_t *p = (ipslr_handle_t *) h;
    return p->model->base_iso_min;
}

int pslr_get_model_base_iso_max(pslr_handle_t h) {
    ipslr_handle_t *p = (ipslr_handle_t *) h;
    return p->model->base_iso_max;
}

int pslr_get_model_extended_iso_min(pslr_handle_t h) {
    ipslr_handle_t *p = (ipslr_handle_t *) h;
    return p->model->extended_iso_min;
}

int pslr_get_model_extended_iso_max(pslr_handle_t h) {
    ipslr_handle_t *p = (ipslr_handle_t *) h;
    return p->model->extended_iso_max;
}

pslr_jpeg_image_tone_t pslr_get_model_max_supported_image_tone(pslr_handle_t h) {
    ipslr_handle_t *p = (ipslr_handle_t *) h;
    return p->model->max_supported_image_tone;
}

const char *pslr_camera_name(pslr_handle_t h) {
    DPRINT("[C]\tpslr_camera_name()\n");
    ipslr_handle_t *p = (ipslr_handle_t *) h;
    int ret;
    if (p->id == 0) {
        ret = pslr_identify(h);
        if (ret != PSLR_OK)
            return NULL;
    }
    if (p->model)
        return p->model->name;
    else {
        static char unk_name[256];
        snprintf(unk_name, sizeof (unk_name), "ID#%x", p->id);
        unk_name[sizeof (unk_name) - 1] = '\0';
        return unk_name;
    }
}

pslr_buffer_type_t pslr_get_jpeg_buffer_type(pslr_handle_t h, int jpeg_stars) {
    ipslr_handle_t *p = (ipslr_handle_t *) h;
    return 2 + get_hw_jpeg_quality( p->model, jpeg_stars );
}

/* ----------------------------------------------------------------------- */

int pslr_connect(pslr_handle_t h) {
    DPRINT("[C]\t\tpslr_connect()\n");

    pslr_get_short_status(h, NULL);
    pslr_set_mode(h, 1);
    pslr_get_short_status(h, NULL);

    pslr_identify(h);
    pslr_get_full_status(h);

    pslr_dsp_task_wu_req(h, 2);
    pslr_get_full_status(h);

    pslr_do_connect(h, true);
    pslr_connect_legacy(h);
    pslr_get_full_status(h);

    return PSLR_OK;
}

int pslr_disconnect(pslr_handle_t h) {
    DPRINT("[C]\tpslr_disconnect()\n");
    pslr_do_connect(h, false);
    pslr_set_mode(h, 0);
    pslr_get_short_status(h, NULL);
    return PSLR_OK;
}

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

/* Done by reverse engineering the USB communication between PK Tether and */
/* Pentax K-10D camera. The debug on/off should work without breaking the  */
/* camera, but you are invoking this subroutines without any warranties    */
/* Originally written by: Samo Penic, 2014 */
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

/* -----------------------------------------------------------------------
 write_debug
----------------------------------------------------------------------- */
void write_debug( const char* message, ... ){

    // Be sure debug is really on as DPRINT doesn't know
    //
    if( !debug ) return;

    // Write to stderr
    //
    va_list argp;
    va_start(argp, message);
    vfprintf( stderr, message, argp );
    va_end(argp);
    return;
}
