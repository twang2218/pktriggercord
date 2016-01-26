/*
    pkTriggerCord
    Copyright (C) 2011-2016 Andras Salamon <andras.salamon@melda.info>
    Remote control of Pentax DSLR cameras.

    based on:

    PK-Remote
    Remote control of Pentax DSLR cameras.
    Copyright (C) 2008 Pontus Lidman <pontus@lysator.liu.se>

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
#include "pslr_scsi.h"
#define PREFIX "\t\t\t\t\t"

static void debug_print_data(const char *prefix, const char *tag, uint8_t *data, uint32_t data_length) {
    //  Print first 32 bytes of the data.
    DPRINT("[S]%s %s [", prefix, tag);
    int i;
    for (i = 0; i < data_length && i < 32; ++i) {
        if (i > 0) {
            if (i % 16 == 0) {
                DPRINT("\n%s       ", prefix);
            } else if ((i % 4) == 0) {
                DPRINT(" ");
            }
            DPRINT(" ");
        }
        DPRINT("%02X", data[i]);
    }
    if (data_length > 32) {
        DPRINT(" ... (%d bytes more)", (data_length - 32));
    }
    DPRINT("]\n");
}

#ifdef WIN32
#include "pslr_scsi_win.c"
#else
#include "pslr_scsi_linux.c"
#endif
