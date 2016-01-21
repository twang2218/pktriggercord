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

#include <unistd.h>
#include <stdlib.h>

#include "pslr_scsi.h"
#include "pslr_scsi_command.h"

#define POLL_INTERVAL 100000 /* Number of us to wait when polling */

#define CHECK(x)                                                                                                       \
    do {                                                                                                               \
        int __r;                                                                                                       \
        __r = (x);                                                                                                     \
        if (__r != PSLR_OK) {                                                                                          \
            fprintf(stderr, "%s:%d:%s failed: %d\n", __FILE__, __LINE__, #x, __r);                                     \
            return __r;                                                                                                \
        }                                                                                                              \
    } while (0)

static void debug_print_arguments(pslr_command_t *command) {
    DPRINT("(");
    int i;
    for (i = 0; i < command->args_count; ++i) {
        if (i > 0)
            DPRINT(", ");
        DPRINT("%x", command->args[i]);
    }
    DPRINT(")");
}

static void debug_print_data(const char *prefix, uint8_t *data, uint32_t data_length) {
    //  Print first 32 bytes of the data.
    DPRINT("[R]%s => [", prefix);
    int i;
    for (i = 0; i < data_length && i < 32; ++i) {
        if (i > 0) {
            if (i % 16 == 0) {
                DPRINT("\n%s    ", prefix);
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

int scsi_send_argument(pslr_command_t *command) {
    uint8_t cmd[8] = {0xf0, 0x4f, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00};
    int i;

    DPRINT("[C]\t\t\tsend_argument(");
    if (command->args_count > 0) {
        debug_print_arguments(command);
    }
    DPRINT(")\n");

    //  construct argument buffer
    uint8_t args_size = sizeof(uint32_t) * command->args_count;
    uint8_t args[args_size];
    for (i = 0; i < command->args_count; ++i) {
        if (command->handle->model && command->handle->model->is_little_endian) {
            set_uint32_le(command->args[i], &args[sizeof(uint32_t) * i]);
        } else {
            set_uint32_be(command->args[i], &args[sizeof(uint32_t) * i]);
        }
    }

    //  send the argument
    if (command->handle->model && !command->handle->model->old_scsi_command) {
        //  normally, we send the arguments in one go
        cmd[2] = 0;
        cmd[4] = args_size;
        return scsi_write(command->handle->fd, cmd, sizeof(cmd), args, args_size);
    } else {
        //  for pretty old camera, the arguments should be send one-by-one.
        for (i = 0; i < command->args_count; ++i) {
            cmd[2] = i * sizeof(uint32_t);
            cmd[4] = sizeof(uint32_t);
            int ret = scsi_write(command->handle->fd, cmd, sizeof(cmd), &args[i * sizeof(uint32_t)], sizeof(uint32_t));
            if (ret != PSLR_OK) {
                return ret;
            }
        }
    }

    return PSLR_OK;
}

int scsi_send_command(pslr_command_t *command) {
    DPRINT("[C]\t\t\tscsi_send_command(command: [%02x %02x])\n", command->c0, command->c1);

    uint8_t cmd[8] = {0xf0, 0x24, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00};

    cmd[2] = command->c0;
    cmd[3] = command->c1;
    cmd[4] = command->args_count;

    if (command->direction == SCSI_READ && command->data != NULL && command->data_length > 0) {
        int length = scsi_read(command->handle->fd, cmd, sizeof(cmd), command->data, command->data_length);
        if (length < 0) {
            DPRINT("\t\t\t\tscsi_read() error.\n");
        } else if (length < command->data_length) {
            DPRINT("\t\t\t\tOnly got %d bytes, should be %d bytes.\n", length, command->data_length);
        }
        command->data_length = length;
    } else {
        CHECK(scsi_write(command->handle->fd, cmd, sizeof(cmd), command->data, command->data_length));
    }
    return PSLR_OK;
}

int scsi_get_status(pslr_command_t *command, pslr_status_t *status) {
    DPRINT("[C]\t\t\tscsi_get_status()\n");
    uint8_t cmd[8] = {0xf0, 0x26, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00};
    uint8_t statusbuf[8];

    memset((uint8_t *)status, 0, sizeof(status));

    while (1) {
        memset(statusbuf, 0, sizeof(statusbuf));

        int n = scsi_read(command->handle->fd, cmd, sizeof(cmd), statusbuf, sizeof(statusbuf));
        if (n != sizeof(statusbuf)) {
            DPRINT("\tOnly got %d bytes\n", n);
            /* The *ist DS doesn't know to return the correct number of
                read bytes for this command */
        }

        status->length = get_uint32_le(&statusbuf[0]);
        status->status[0] = statusbuf[6];
        status->status[1] = statusbuf[7];

        if (command->read_result && command->direction == SCSI_WRITE) {
            //  this command needs retrieve data later
            if (status->status[0] == 0x01)
                break;
        } else {
            //  make sure the command finished running
            if ((status->status[1] & 0x01) == 0)
                break;
        }
        usleep(POLL_INTERVAL);
    }

    if ((status->status[1] & 0xff) != 0) {
        DPRINT("\tERROR: Code: [0x%02x 0x%02x], Length: 0x%08x\n", status->status[0], status->status[1],
               status->length);
        return -1;
    } else {
        return PSLR_OK;
    }
}

int scsi_read_result(pslr_command_t *command) {
    DPRINT("[C]\t\t\tread_result(0x%x, size=%d)\n", command->handle->fd, command->data_length);
    uint8_t cmd[8] = {0xf0, 0x49, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00};

    set_uint32_le(command->data_length, &cmd[4]);

    int r = scsi_read(command->handle->fd, cmd, sizeof(cmd), command->data, command->data_length);
    if (r != command->data_length) {
        return PSLR_READ_ERROR;
    } else {
        debug_print_data("\t\t\t\t", command->data, command->data_length);
    }
    return PSLR_OK;
}

int generic_command(pslr_command_t *command) {
    //  Print debug information
    DPRINT("[C]\t\t\tgeneric_command([%02x %02x]", command->c0, command->c1);
    if (command->args_count > 0) {
        DPRINT(", ");
        debug_print_arguments(command);
    }
    DPRINT(")\n");

    //  Send Arguments
    if (command->args_count > 0) {
        CHECK(scsi_send_argument(command));
    }

    //  Send Command
    CHECK(scsi_send_command(command));

    //  Check Status
    pslr_status_t status;
    CHECK(scsi_get_status(command, &status));

    //  Read result
    if (command->direction == SCSI_WRITE && command->read_result) {
        //  This means, the result should be read in another scsi command
        if (command->data == NULL) {
            //  In many cases, the data length is only known by this point from
            //  last `scsi_get_status()` call, so it's more convenient to allocate
            //  the memory here, rather than random guessing a size before calling.
            command->data = (uint8_t *)malloc(status.length);
        }
        command->data_length = status.length;
        CHECK(scsi_read_result(command));
    }

    return PSLR_OK;
}

void command_init(pslr_command_t *command) {
    memset(command, 0, sizeof(pslr_command_t));
    command->read_result = false;
    command->direction = SCSI_WRITE;
}

void command_free(pslr_command_t *command) {
    if (command->data != NULL) {
        free(command->data);
        command->data = NULL;
        command->data_length = 0;
    }
}
