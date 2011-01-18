/*
    pkTriggerCord
    Copyright (C) 2011 Andras Salamon <andras.salamon@melda.info>
    Remote control of Pentax DSLR cameras.

    based on:

    PK-Remote
    Remote control of Pentax DSLR cameras.
    Copyright (C) 2008 Pontus Lidman <pontus@lysator.liu.se>

    This program is free software: you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation, either version 3 of the License, or
    (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */
#include <stdint.h>
#include <sys/ioctl.h>
#include <string.h>
#include <stdio.h>
#include <linux/../scsi/sg.h>

#include "pslr_scsi.h"

void print_scsi_error(sg_io_hdr_t *pIo, uint8_t *sense_buffer) {
    int k;

    if (pIo->sb_len_wr > 0) {
        DPRINT("SCSI error: sense data: ");
        for (k = 0; k < pIo->sb_len_wr; ++k) {
            if ((k > 0) && (0 == (k % 10)))
                DPRINT("\n  ");
            DPRINT("0x%02x ", sense_buffer[k]);
        }
        DPRINT("\n");
    }
    if (pIo->masked_status)
        DPRINT("SCSI status=0x%x\n", pIo->status);
    if (pIo->host_status)
        DPRINT("host_status=0x%x\n", pIo->host_status);
    if (pIo->driver_status)
        DPRINT("driver_status=0x%x\n", pIo->driver_status);
}

int scsi_read(int sg_fd, uint8_t *cmd, uint32_t cmdLen,
        uint8_t *buf, uint32_t bufLen) {
    sg_io_hdr_t io;
    uint8_t sense[32];
    int r;

    memset(&io, 0, sizeof (io));

    io.interface_id = 'S';
    io.cmd_len = cmdLen;
    /* io.iovec_count = 0; */ /* memset takes care of this */
    io.mx_sb_len = sizeof (sense);
    io.dxfer_direction = SG_DXFER_FROM_DEV;
    io.dxfer_len = bufLen;
    io.dxferp = buf;
    io.cmdp = cmd;
    io.sbp = sense;
    io.timeout = 20000; /* 20000 millisecs == 20 seconds */
    /* io.flags = 0; */ /* take defaults: indirect IO, etc */
    /* io.pack_id = 0; */
    /* io.usr_ptr = NULL; */

    r = ioctl(sg_fd, SG_IO, &io);
    if (r == -1) {
        perror("ioctl");
        return -PSLR_DEVICE_ERROR;
    }

    if ((io.info & SG_INFO_OK_MASK) != SG_INFO_OK) {
        print_scsi_error(&io, sense);
        return -PSLR_SCSI_ERROR;
    } else {
        /* Older Pentax DSLR will report all bytes remaining, so make
         * a special case for this (treat it as all bytes read). */
        if (io.resid == bufLen)
            return bufLen;
        else
            return bufLen - io.resid;
    }
}

int scsi_write(int sg_fd, uint8_t *cmd, uint32_t cmdLen,
        uint8_t *buf, uint32_t bufLen) {

    sg_io_hdr_t io;
    uint8_t sense[32];
    int r;

    memset(&io, 0, sizeof (io));

    io.interface_id = 'S';
    io.cmd_len = cmdLen;
    /* io.iovec_count = 0; */ /* memset takes care of this */
    io.mx_sb_len = sizeof (sense);
    io.dxfer_direction = SG_DXFER_TO_DEV;
    io.dxfer_len = bufLen;
    io.dxferp = buf;
    io.cmdp = cmd;
    io.sbp = sense;
    io.timeout = 20000; /* 20000 millisecs == 20 seconds */
    /* io.flags = 0; */ /* take defaults: indirect IO, etc */
    /* io.pack_id = 0; */
    /* io.usr_ptr = NULL; */

    r = ioctl(sg_fd, SG_IO, &io);

    if (r == -1) {
        perror("ioctl");
        return PSLR_DEVICE_ERROR;
    }

    if ((io.info & SG_INFO_OK_MASK) != SG_INFO_OK) {
        print_scsi_error(&io, sense);
        return PSLR_SCSI_ERROR;
    } else {
        return PSLR_OK;
    }
}