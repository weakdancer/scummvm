/* ScummVM - Scumm Interpreter
 * Copyright (C) 2001  Ludvig Strigeus
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 2
 * of the License, or (at your option) any later version.

 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.

 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.
 *
 * $Header$
 *
 */

#include "stdafx.h"
#include "scumm.h"

#if defined(USE_IMUSE)
#include "sound.h"
#else
struct SoundEngine {
	byte **_base_sounds;
	int start_sound(int sound) { return -1; }
	int stop_sound(int sound) { return -1; }
	int stop_all_sounds() { return -1; }
	int32 do_command(int a, int b, int c, int d, int e, int f, int g, int h) { return -1; }
	int get_sound_status(int sound) { return -1; }
	int clear_queue() { return -1; }
};
#endif

void Scumm::addSoundToQueue(int sound) {
	_vars[VAR_LAST_SOUND] = sound;
	ensureResourceLoaded(rtSound, sound);
	addSoundToQueue2(sound);
}

void Scumm::addSoundToQueue2(int sound) {
	if (_soundQue2Pos < 10) {
		_soundQue2[_soundQue2Pos++] = sound;
	}
}

void Scumm::processSoundQues() {
	byte d;
	int i,j;
	int num;
	int16 data[16];
	SoundEngine *se;

	processSfxQueues();

	while (_soundQue2Pos){
		d=_soundQue2[--_soundQue2Pos];
		if (d)
			playSound(d);
	}

	for (i=0; i<_soundQuePos; ) {
		num = _soundQue[i++];
		if (i + num > _soundQuePos) {
			warning("processSoundQues: invalid num value");
			break;
		}
		for (j=0; j<16; j++)
			data[j] = 0;
		if (num>0) {
			for (j=0; j<num; j++)
				data[j] = _soundQue[i+j];
			i += num;

			se = (SoundEngine*)_soundDriver;
#if 0
			debug(1,"processSoundQues(%d,%d,%d,%d,%d,%d,%d,%d,%d)", 
				data[0]>>8,
				data[0]&0xFF,
				data[1],
				data[2],
				data[3],
				data[4],
				data[5],
				data[6],
				data[7]
				);
#endif
			if (se) 
				_vars[VAR_SOUNDRESULT] = se->do_command(data[0],data[1],data[2],data[3],data[4],data[5],data[6],data[7]);
		}
	}
	_soundQuePos = 0;
}

void Scumm::playSound(int sound) {
	SoundEngine *se = (SoundEngine*)_soundDriver;
	if (se) {
		getResourceAddress(rtSound, sound);
		se->start_sound(sound);
	}
}

void Scumm::processSfxQueues() {
	Actor *a;
	int act;
	bool b,finished;

	if (_talk_sound_mode != 0) {
		startTalkSound(_talk_sound_a, _talk_sound_b, _talk_sound_mode);
		_talk_sound_mode = 0;
	}
	
	if (_sfxMode==2) {
		act = _vars[VAR_TALK_ACTOR];
		finished = isSfxFinished();

		if (act!=0 && (uint)act<0x80 && !string[0].no_talk_anim) {
			a = derefActorSafe(act, "processSfxQueues");
			if (a->room==_currentRoom && (finished || !_endOfMouthSync)) {
				b = true;
				if (!finished)
					b = isMouthSyncOff(_curSoundPos);
				if (_mouthSyncMode != b) {
					_mouthSyncMode = b;
					startAnimActor(a, b ? a->talkFrame2 : a->talkFrame1, a->facing);
				}
			}
		}
		if (finished && _talkDelay==0) {
			stopTalk();
			_sfxMode = 0;
		}
	} else if (_sfxMode==1) {
		if (isSfxFinished()) {
			_sfxMode = 0;
		}
	}
}

void Scumm::startTalkSound(uint32 offset, uint32 b, int mode) {
	int num, i;
	byte file_byte,file_byte_2;
	uint16 elem;

	if (!_sfxFile) {
		warning("startTalkSound: SFX file is not open");
		return;
	}

	fileSeek((FILE*)_sfxFile, offset + 8, SEEK_SET);
	i = 0;
	if (b>8) {
		num = (b-8)>>1;
		do {
			fileRead((FILE*)_sfxFile, &file_byte, sizeof(file_byte));
			fileRead((FILE*)_sfxFile, &file_byte_2, sizeof(file_byte_2));
			_mouthSyncTimes[i++] = file_byte | (file_byte_2<<8);
		} while (--num);
	}
	_mouthSyncTimes[i] = 0xFFFF;
	_sfxMode = mode;
	_curSoundPos = 0;
	_mouthSyncMode = true;

	startSfxSound(_sfxFile);
}

bool Scumm::isMouthSyncOff(uint pos) {
	uint j;
	bool val = true;
	uint16 *ms = _mouthSyncTimes;

	_endOfMouthSync = false;
	do {
		val ^= 1;
		j = *ms++;
		if (j==0xFFFF) {
			_endOfMouthSync = true;
			break;
		}
	} while (pos > j);
	return val;
}


int Scumm::isSoundRunning(int sound) {
	SoundEngine *se;
	int i;

	i = _soundQue2Pos;
	while (i--) {
		if (_soundQue2[i] == sound)
			return 1;
	}

	if (isSoundInQueue(sound))
		return 1;

	if (!isResourceLoaded(rtSound, sound))
		return 0;

	
	se = (SoundEngine*)_soundDriver;
	if (!se)
		return 0;
	return se->get_sound_status(sound);
}

bool Scumm::isSoundInQueue(int sound) {
	int i = 0,j, num;
	int16 table[16];

	while (i < _soundQuePos) {
		num = _soundQue[i++];

		memset(table, 0, sizeof(table));

		if (num > 0) {
			for (j=0; j<num; j++)
				table[j] = _soundQue[i+j];
			i += num;
			if (table[0] == 0x10F && table[1]==8 && table[2] == sound)
				return 1;
		}
	}
	return 0;
}

void Scumm::stopSound(int a) {
	SoundEngine *se;
	int i;

	se = (SoundEngine*)_soundDriver;
	if (se)
		se->stop_sound(a);

	for (i=0; i<10; i++)
		if (_soundQue2[i] == (byte)a)
			_soundQue2[i] = 0;
}

void Scumm::stopAllSounds() {
	SoundEngine *se = (SoundEngine*)_soundDriver;
	if (se) {
		se->stop_all_sounds();
		se->clear_queue();
	}
	clearSoundQue();
}

void Scumm::clearSoundQue() {
	_soundQue2Pos = 0;
	memset(_soundQue2, 0, sizeof(_soundQue2));
}

void Scumm::soundKludge(int16 *list) {
	int16 *ptr;
	int i;

	if (list[0]==-1) {
		processSoundQues();
		return;
	}
	_soundQue[_soundQuePos++] = 8;

	ptr = _soundQue + _soundQuePos;
	_soundQuePos += 8;

	for (i=0; i<8; i++)
		*ptr++ = list[i];
	if (_soundQuePos > 0x100)
		error("Sound que buffer overflow");
}

void Scumm::talkSound(uint32 a, uint32 b, int mode) {
	_talk_sound_a = a;
	_talk_sound_b = b;
	_talk_sound_mode = mode;
}

static const uint32 sound_tags[] = {
	MKID('ADL ')
};

void Scumm::setupSound() {
	SoundEngine *se = (SoundEngine*)_soundDriver;
	if (se) {
		se->_base_sounds = res.address[rtSound];
	}
	_soundTagTable = (byte*)sound_tags;
	_numSoundTags = 1;
	_sfxFile = openSfxFile();
}

struct VOCHeader {
	byte id[19];
	byte extra[7];
};

static const char VALID_VOC_ID[] = "Creative Voice File";
static const char VALID_VOC_VERSION[] = "";
void Scumm::startSfxSound(void *file) {
	VOCHeader hdr;
	int block_type;
	byte work[8];
	uint size,i;
	int rate,comp;
	byte *data;

	if (fread(&hdr, sizeof(hdr), 1, (FILE*)file) != 1 || 
		memcmp(hdr.id, VALID_VOC_ID, sizeof(hdr.id)) != 0) {
		warning("startSfxSound: invalid header");
		return;
	}

	block_type = getc( (FILE*)file );
	if (block_type != 1) {
		warning("startSfxSound: Expecting block_type == 1, got %d", block_type);
		return;
	}

	fread(work, 3, 1, (FILE*)file);

	size = ( work[0] | ( work[1] << 8 ) | ( work[2] << 16 ) ) - 2;
	rate = getc( (FILE*)file );
	comp = getc( (FILE*)file );

	if (comp != 0) {
		warning("startSfxSound: Unsupported compression type %d", comp);
		return;
	}

	data = (byte*) malloc(size);
	if (data==NULL) {
		error("startSfxSound: out of memory");
		return;
	}
	
	if (fread(data, size, 1, (FILE*)file) != 1) {
		/* no need to free the memory since error will shut down */
		error("startSfxSound: cannot read %d bytes", size);
		return;
	}
	for(i=0;i<size; i++)
		data[i] ^= 0x80;

	playSfxSound(data, size, 1000000 / (256 - rate) );
}

void *Scumm::openSfxFile() {
	return fopen("monster.sou", "rb");
}