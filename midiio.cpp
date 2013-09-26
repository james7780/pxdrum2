// Simple MIDI file read/write class
// James Higgs 2009
//
#include <stdlib.h>
#include "SDL.h"
#include "platform.h"
#include "midiio.h"

// Class to manage a MIDI track
MidiTrack::MidiTrack(char* name, char* instrname)
{

}

// Add a MIDI event to this track
void MidiTrack::AddEvent(long delta, unsigned char command, unsigned char* data)
{

}

// Read MIDI track data from disk
bool MidiTrack::Read(FILE* pfile)
{
	return false;
}

// Write MIDI track data to disk
void MidiTrack::Write(FILE* pfile)
{
	
}

//////////////////////////////////////////////////////////////////////////////
// Clas to read/write a MIDI file
//////////////////////////////////////////////////////////////////////////////
MidiFile::MidiFile(char* filename)
{

}

///  
bool MidiFile::Read()
{
	return false;
}

///
bool MidiFile::Write()
{
	return false;
}
