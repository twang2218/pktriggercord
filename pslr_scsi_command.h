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

#ifndef PSLR_SCSI_COMMAND_H
#define PSLR_SCSI_COMMAND_H

#include "pslr_common.h"
#include "pslr_model.h"

#define MAX_ARGUMENTS 8

typedef enum { SCSI_WRITE, SCSI_READ } scsi_direction_t;
typedef enum { DATA_USAGE_NONE, DATA_USAGE_SCSI_WRITE, DATA_USAGE_SCSI_READ, DATA_USAGE_READ_RESULT } scsi_data_usage_t;

typedef struct {
    ipslr_handle_t *handle;
    uint8_t c0;
    uint8_t c1;
    uint8_t args_count;
    uint32_t args[MAX_ARGUMENTS];
    pslr_data_t data;
    scsi_data_usage_t data_usage;
} pslr_command_t;

typedef struct {
    uint32_t length;
    uint8_t status[2];
} pslr_command_status_t;

//  SCSI Commands
int scsi_send_argument(pslr_command_t *command);
int scsi_send_command(pslr_command_t *command);
int scsi_get_status(pslr_command_t *command, pslr_command_status_t *status);
int scsi_read_result(pslr_command_t *command);
int generic_command(pslr_command_t *command);

//  Command Helper
void command_init(pslr_command_t *command, pslr_handle_t h, uint8_t c0, uint8_t c1);
void command_add_arg(pslr_command_t *command, uint32_t value);
void command_load_from_data(pslr_command_t *command, pslr_data_t *data, scsi_data_usage_t usage);
void command_save_to_data(pslr_command_t *command, pslr_data_t *data);
void command_free_data(pslr_command_t *command);

#endif // PSLR_SCSI_COMMAND_H
