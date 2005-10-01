/* live-f1
 *
 * Copyright © 2005 Scott James Remnant <scott@netsplit.com>.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA  02110-1301 USA
 */

#ifndef LIVE_F1_PACKET_H
#define LIVE_F1_PACKET_H

#include "live-f1.h"


/* Which car the packet is for */
#define PACKET_CAR(_p) ((_p)[0] & 0x1f)

/* Which type of packet it is */
#define PACKET_TYPE(_p) (((_p)[0] >> 5) | (((_p)[1] & 0x01) << 3))

/* Data from a short packet */
#define SHORT_PACKET_DATA(_p) (((_p)[1] & 0x0e) >> 1)

/* Data from a special packet */
#define SPECIAL_PACKET_DATA(_p) ((_p)[1] >> 1)

/* Length of the packet if it's one of the long ones */
#define LONG_PACKET_LEN(_p) (SPECIAL_PACKET_DATA(_p))

/* Flag for a nominally short packet with no following data */
#define SHORT_PACKET_EMPTY(_p) (((_p)[1] & 0xf0) == 0xf0)

/* Length of the packet if it's one of the short ones */
#define SHORT_PACKET_LEN(_p) ((SHORT_PACKET_EMPTY(_p) ? 0 : ((_p)[1] >> 4)))

/* Length of the packet if it's a special one */
#define SPECIAL_PACKET_LEN(_p) 0


/* Types of packets for cars */
typedef enum {
	CAR_POSITION_UPDATE,
	CAR_POSITION,
	CAR_NUMBER,
	CAR_DRIVER,
	/* Everything else is short */
	CAR_POSITION_HISTORY = 15
} CarPacketType;

/* Types of non-car packets */
typedef enum {
	SYS_EVENT_ID = 1,
	SYS_KEY_FRAME,
	SYS_UNKNOWN_SPECIAL_A,
	SYS_UNKNOWN_LONG_A,
	SYS_UNKNOWN_SPECIAL_B,
	SYS_UNKNOWN_LONG_B,
	SYS_STRANGE_A, /* Always two bytes */
	SYS_UNKNOWN_SHORT_A = 9,
	SYS_UNKNOWN_LONG_C,
	SYS_UNKNOWN_SHORT_B,
	SYS_COPYRIGHT
} SystemPacketType;


SJR_BEGIN_EXTERN

void handle_car_packet    (CurrentState *state, const unsigned char *packet);
void handle_system_packet (CurrentState *state, const unsigned char *packet);

SJR_END_EXTERN

#endif /* LIVE_F1_PACKET_H */
