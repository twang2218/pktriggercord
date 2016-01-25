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
int pslr_do_connect(pslr_handle_t h, bool connect);
//  Group 0x23
int pslr_write_adj_data(pslr_handle_t h, uint32_t value);
int pslr_set_adj_mode_flag(pslr_handle_t h, uint32_t mode, uint32_t value);
int pslr_get_adj_mode_flag(pslr_handle_t h, uint32_t mode, pslr_data_t *data);
int pslr_set_adj_data(pslr_handle_t h, uint32_t mode, bool debug_mode);
int pslr_get_adj_data(pslr_handle_t h, uint32_t mode, pslr_data_t *data);

#endif // PSLR_COMMAND_H
