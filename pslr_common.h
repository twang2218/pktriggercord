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
#ifndef PSLR_COMMON_H
#define PSLR_COMMON_H

#include <stdint.h>
#include <stdbool.h>

typedef struct {
    uint8_t *data;
    int32_t length;
} pslr_data_t;

typedef struct {
    int32_t nom;
    int32_t denom;
} pslr_rational_t;

typedef void *pslr_handle_t;

#endif // PSLR_COMMON_H
