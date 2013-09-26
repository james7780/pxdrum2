#define MIDI_FMT_SINGLETRACK           0
#define MIDI_FMT_MULTIPLETRACKS_SYNCH  1
#define MIDI_FMT_MULTIPLETRACKS_ASYNCH 2

/*
#define MIDI_CHANNEL_0   0
#define MIDI_CHANNEL_1   1
#define MIDI_CHANNEL_2   2
#define MIDI_CHANNEL_3   3
#define MIDI_CHANNEL_4   4
#define MIDI_CHANNEL_5   5
#define MIDI_CHANNEL_6   6
#define MIDI_CHANNEL_7   7
#define MIDI_CHANNEL_8   8
#define MIDI_CHANNEL_9   9
#define MIDI_CHANNEL_10 10
#define MIDI_CHANNEL_11 11
#define MIDI_CHANNEL_12 12
#define MIDI_CHANNEL_13 13
#define MIDI_CHANNEL_14 14
#define MIDI_CHANNEL_15 15

#define MIDI_OCTAVE_0   0
#define MIDI_OCTAVE_1   1
#define MIDI_OCTAVE_2   2
#define MIDI_OCTAVE_3   3
#define MIDI_OCTAVE_4   4
#define MIDI_OCTAVE_5   5
#define MIDI_OCTAVE_6   6
#define MIDI_OCTAVE_7   7
#define MIDI_OCTAVE_8   8
#define MIDI_OCTAVE_9   9
#define MIDI_OCTAVE_10 10

#define MIDI_NOTE_C     0
#define MIDI_NOTE_C_    1
#define MIDI_NOTE_D     2
#define MIDI_NOTE_D_    3
#define MIDI_NOTE_E     4
#define MIDI_NOTE_F     5
#define MIDI_NOTE_F_    6
#define MIDI_NOTE_G     7
#define MIDI_NOTE_G_    8
#define MIDI_NOTE_A     9
#define MIDI_NOTE_A_   10
#define MIDI_NOTE_B    11

#define MIDI_MUSIC_MN   0 // Normale Musik
#define MIDI_MUSIC_ML   1 // Musik Legato
#define MIDI_MUSIC_MS   2 // Musik Staccato
*/

// MIDI EVENT COMMANDS
#define MIDI_EVENT_NOTEOFF						0x80
#define MIDI_EVENT_NOTEON							0x90
#define MIDI_EVENT_AFTERTOUCH					0xA0
#define MIDI_EVENT_CONTROLCHANGE			0xB0
#define MIDI_EVENT_PROGRAMCHANGE			0xC0
#define MIDI_EVENT_CHANNELAFTERTOUCH	0xD0
#define MIDI_EVENT_BEND								0xE0
#define MIDI_META_EVENT								0xFF

// META EVENT COMMANDS
#define MIDI_META_SETTRACKSEQ					0x00						// track seqenuce number
#define MIDI_META_TEXT								0x01
#define MIDI_META_COPYRIGHT						0x02
#define MIDI_META_TRACKNAME						0x03
#define MIDI_META_TRACKINSTRNAME			0x04						// track instrument name
#define MIDI_META_LYRICS							0x05
#define MIDI_META_MARKER							0x06
#define MIDI_META_CUEPOINT						0x07
#define MIDI_META_TRACKEND						0x2F						// track end marker
#define MIDI_META_TEMPO								0x51						// set tempo in microsecs/quarter note
#define MIDI_META_TIMESIG							0x58						// time signature
#define MIDI_META_KEYSIG							0x59						// key signature
#define MIDI_META_USER								0x7F						// sequencer-specific info

// SYSTEM MESSAGES
#define MIDI_SYSTEM_TICK							0xF8						// sync/timing clock/pulse
#define MIDI_SYSTEM_STARTSEQ					0xFA						// start current sequence
#define MIDI_SYSTEM_CONTINUESEQ				0xFB						// continue seq from current pos
#define MIDI_SYSTEM_STOPSEQ						0xFC						// stop sequence

// Header chunk
class MidiHeaderChunk
{
	//char chunkId[4] = { 'M', 'T', 'h', 'd' };							// always "MThd"
	unsigned int headerSize;			// always 6
	unsigned short format;				// 0 = single-track, 1 = mutiltrack,sync, 2 = multitrack,async 
	unsigned short numTracks;			// number of tracks in this file
	unsigned short ticksPerQNote;	// ticks per quarter-note

	MidiHeaderChunk()
		{
		headerSize = 6;
		format = MIDI_FMT_SINGLETRACK;
		numTracks = 1;
		ticksPerQNote = 96;
		}

	void Write(FILE* pfile);
};

// track chunk
class MidiTrackChunk
{
	//char chunkId[4] = { 'M', 'T', 'r', 'k' };
	unsigned int size;												// number of bytes (not incl header)

	MidiTrackChunk()
		{
		size = 0;
		}

	void Write(FILE* pfile);
};


// Class to manage a MIDI track
class MidiTrack
{
	MidiTrack(char* name, char* instrname);
	void AddEvent(long delta, unsigned char command, unsigned char* data);
	bool Read(FILE* pfile);
	void Write(FILE* pfile);

	char m_name[200];
	char m_instrumentName[200];
	// array of bytes
	unsigned char m_data[10000];
	unsigned short m_dataLength;
};

// Clas to read/write a MIDI file
class MidiFile
{
	MidiFile(char* filename);

	bool Read();
	bool Write();

private:
	FILE* m_pfile;
};


/*
//////////////////////////////////////////////////////////////////
// MidiEventCommand
//////////////////////////////////////////////////////////////////
class CMidiEventCommand : public CObject
{
public:
	CMidiEventCommand(BYTE channel = 0, long deltatime = 0);
	virtual ~CMidiEventCommand();
	virtual void WriteToFile(ostream& os);
	virtual DWORD GetLength()         {return 0;}; // should be overridden

	BYTE Channel;
	long DeltaTime;
};

class CMidiEventCommandNoteOff : public CMidiEventCommand
{
public:
	CMidiEventCommandNoteOff(BYTE channel, long deltatime, BYTE octave, BYTE notenumber, BYTE velocity);
	virtual void WriteToFile(ostream& os);
	virtual DWORD GetLength()         {return 3;};

	BYTE Octave;
	BYTE NoteNumber;
	BYTE Velocity;
};

class CMidiEventCommandNoteOn : public CMidiEventCommand
{
public:
	CMidiEventCommandNoteOn(BYTE channel, long deltatime, BYTE octave, BYTE notenumber, BYTE velocity);
	virtual void WriteToFile(ostream& os);
	virtual DWORD GetLength()         {return 3;};

	BYTE Octave;
	BYTE NoteNumber;
	BYTE Velocity;
};

class CMidiEventCommandKeyAfterTouch : public CMidiEventCommand
{
public:
	CMidiEventCommandKeyAfterTouch(BYTE channel, long deltatime, BYTE octave, BYTE notenumber, BYTE velocity);
	virtual void WriteToFile(ostream& os);
	virtual DWORD GetLength()         {return 3;};

	BYTE Octave;
	BYTE NoteNumber;
	BYTE Velocity;
};

class CMidiEventCommandControlChange : public CMidiEventCommand
{
public:
	CMidiEventCommandControlChange(BYTE channel, long deltatime, BYTE controllernumber, BYTE newvalue);
	virtual void WriteToFile(ostream& os);
	virtual DWORD GetLength()         {return 3;};

	BYTE ControllerNumber;
	BYTE NewValue;
};

class CMidiEventCommandProgramChange : public CMidiEventCommand
{
public:
	CMidiEventCommandProgramChange(BYTE channel, long deltatime, BYTE newprogramnumber);
	virtual void WriteToFile(ostream& os);
	virtual DWORD GetLength()         {return 2;};

	BYTE NewProgramNumber;
};

class CMidiMetaEventCommand : public CMidiEventCommand
{
public:
	CMidiMetaEventCommand(BYTE command, BYTE datalen = 0, BYTE* data = NULL);
	virtual ~CMidiMetaEventCommand();
	virtual void WriteToFile(ostream& os);
	virtual DWORD GetLength();

	BYTE Command;
	long DataLen;
	BYTE* Data;
};

class CMidiMetaEventCommandEndOfTrack : public CMidiMetaEventCommand
{
public:
	CMidiMetaEventCommandEndOfTrack();
};

class CMidiMetaEventCommandTextEvent : public CMidiMetaEventCommand
{
public:
	CMidiMetaEventCommandTextEvent(const char* text);
};

class CMidiMetaEventCommandCopyrightNotice : public CMidiMetaEventCommand
{
public:
	CMidiMetaEventCommandCopyrightNotice(const char* copyrightnotice);
};

class CMidiMetaEventCommandTrackName : public CMidiMetaEventCommand
{
public:
	CMidiMetaEventCommandTrackName(const char* trackname);
};

class CMidiMetaEventCommandInstrumentName : public CMidiMetaEventCommand
{
public:
	CMidiMetaEventCommandInstrumentName(const char* instrumentname);
};

class CMidiMetaEventCommandLyric : public CMidiMetaEventCommand
{
public:
	CMidiMetaEventCommandLyric(const char* lyric);
};

class CMidiMetaEventCommandMarker : public CMidiMetaEventCommand
{
public:
	CMidiMetaEventCommandMarker(const char* marker);
};

class CMidiMetaEventCommandCuePoint : public CMidiMetaEventCommand
{
public:
	CMidiMetaEventCommandCuePoint(const char* cuepoint);
};

class CMidiMetaEventCommandMIDIChannelPrefix : public CMidiMetaEventCommand
{
public:
	CMidiMetaEventCommandMIDIChannelPrefix(BYTE cc);
};

class CMidiMetaEventCommandSetTempo : public CMidiMetaEventCommand
{
public:
	CMidiMetaEventCommandSetTempo(long tempo);
};

class CMidiMetaEventCommandTimeSignature : public CMidiMetaEventCommand
{
public:
	CMidiMetaEventCommandTimeSignature(BYTE nn, BYTE dd, BYTE cc, BYTE bb);
};

//////////////////////////////////////////////////////////////////
// MidiTrack
//////////////////////////////////////////////////////////////////
class CMidiTrack : public CCollection<CMidiEventCommand>
{
public:
	CMidiTrack();
	virtual void WriteToFile(ostream& os);

	// "Shortcuts"
	virtual void Copyright(const char* copyright);
	virtual void TimeSignature(BYTE nn, BYTE dd, BYTE cc, BYTE bb);
	virtual void Tempo(long tempo);
	virtual void NoteOn(BYTE channel, long deltatime, BYTE octave, BYTE notenumber, BYTE velocity);
	virtual void NoteOff(BYTE channel, long deltatime, BYTE octave, BYTE notenumber, BYTE velocity);
	virtual void Note(BYTE channel, long deltatime, BYTE octave, BYTE notenumber, BYTE musicart = MIDI_MUSIC_MN, BYTE velocity = 96, BYTE vel_off = 64);
	virtual void Pause(BYTE channel, long deltatime);
	virtual void PlayString(BYTE channel, const char* playstring, BYTE velocity = 96, BYTE vel_off = 64);
	virtual void EndOfTrack();

	CMidiTrackHeader Header;
};

//////////////////////////////////////////////////////////////////
// MidiFile
//////////////////////////////////////////////////////////////////
class CMidiFile : public CCollection<CMidiTrack>
{
public:
	CMidiFile(WORD deltatimeticks, WORD fileformat = MIDI_SINGLETRACK, WORD trackcount = 1);
	virtual void WriteToFile(const char* filename);
	virtual void WriteToFile(ostream& os);

	CMidiFileHeader Header;
};

//////////////////////////////////////////////////////////////////
// SampleCode
//////////////////////////////////////////////////////////////////
void SampleCode();
*/
