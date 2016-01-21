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

#include <stdint.h>
#include <stdbool.h>
#include "pslr_model.h"

#define MAX_ARGUMENTS 8

typedef enum { SCSI_WRITE, SCSI_READ } scsi_direction_t;

typedef struct {
    ipslr_handle_t *handle;
    uint8_t c0;
    uint8_t c1;
    uint8_t args_count;
    uint32_t args[MAX_ARGUMENTS];
    bool read_result;
    uint8_t *data;
    int32_t data_length;
    scsi_direction_t direction;
} pslr_command_t;

typedef struct {
    uint32_t length;
    uint8_t status[2];
} pslr_status_t;

int scsi_send_argument(pslr_command_t *command);
int scsi_send_command(pslr_command_t *command);
int scsi_get_status(pslr_command_t *command, pslr_status_t *status);
int scsi_read_result(pslr_command_t *command);
int generic_command(pslr_command_t *command);
void command_init(pslr_command_t *command);
void command_free(pslr_command_t *command);

#endif // PSLR_SCSI_COMMAND_H
