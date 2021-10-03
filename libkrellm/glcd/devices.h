/* libkrellm/glcd
|
|  Copyright (C) 2013-2015 Bill Wilson   billw@gkrellm.net
|
|  libkrellm/glcd is free software: you can redistribute it and/or modify
|  it under the terms of the GNU General Public License as published by
|  the Free Software Foundation, either version 3 of the License, or
|  (at your option) any later version.
|
|  libkrellm/glcd is distributed in the hope that it will be useful,
|  but WITHOUT ANY WARRANTY; without even the implied warranty of
|  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
|  GNU General Public License for more details.
|
|  You should have received a copy of the GNU General Public License
|  along with the libkrellm.  If not, see <http://www.gnu.org/licenses/>.
|
*/
#ifndef DEVICES_H
#define DEVICES_H


typedef struct CommandParams
	{
	uint8	command;
	uint8	n_params;
	uint16	*params;
	}
	CommandData;

#define	DEVICE_DELAY	0x7ffe

#include "SSD1289.h"
#include "ILI9327.h"
#include "ILI9325.h"

	
#endif
