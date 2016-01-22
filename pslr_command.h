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

#include <stdint.h>
#include "pslr.h"

int pslr_request_download(pslr_handle_t h, uint32_t address, int32_t length);
int pslr_do_download(pslr_handle_t h, uint8_t* buf, int32_t length);
int pslr_request_upload(pslr_handle_t h, uint32_t address, int32_t length);
int pslr_do_upload(pslr_handle_t h, uint8_t* buf, int32_t length);
int pslr_get_transfer_status(pslr_handle_t h);

/*  ------------------------------------------------------------    */

int pslr_download(pslr_handle_t h, uint32_t address, uint32_t length, uint8_t *buf);
int pslr_upload(pslr_handle_t h, uint32_t address, uint32_t length, uint8_t *buf);

#endif // PSLR_COMMAND_H
