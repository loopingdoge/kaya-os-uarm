 /*******************************************************************************
  * Copyright 2014, Devid Farinelli, Erik Minarini, Alberto Nicoletti           *
  * This file is part of kaya2014.                                              *
  *                                                                             *
  * kaya2014 is free software: you can redistribute it and/or modify            *
  * it under the terms of the GNU General Public License as published by        *
  * the Free Software Foundation, either version 3 of the License, org          *
  * (at your option) any later version.                                         *
  *                                                                             *
  * kaya2014 is distributed in the hope that it will be useful,                 *
  * but WITHOUT ANY WARRANTY; without even the implied warranty of              *
  * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the               *
  * GNU General Public License for more details.                                *
  *                                                                             *
  * You should have received a copy of the GNU General Public licenses          *
  * along with kaya2014.  If not, see <http://www.gnu.org/licenses/>.           *
  ******************************************************************************/

#ifndef CONST_H
#define CONST_H

/* Kaya costanti specifiche */

#define ENABLE_VM 0x00000001

/* Devices */
#define COMMAND_REG_OFFSET 	0x00000004
#define TERM_STATUS_READ 	0x00000000
#define TERM_COMMAND_READ 	0x00000004
#define TERM_STATUS_WRITE 	0x00000008
#define TERM_COMMAND_WRITE 	0x0000000C

/* costanti di uso generale */
#define EXTERN extern
#define	FALSE	0
#define	TRUE 1

#define NULL ((void *)0)

#endif
