// Copyright 2004 "Gilles Degottex"

// This file is part of "Music"

// "Music" is free software; you can redistribute it and/or modify
// it under the terms of the GNU Lesser General Public License as published by
// the Free Software Foundation; either version 2.1 of the License, or
// (at your option) any later version.
// 
// "Music" is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
// GNU Lesser General Public License for more details.
// 
// You should have received a copy of the GNU Lesser General Public License
// along with this program; if not, write to the Free Software
// Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA


#ifndef _omidistream_h_
#define _omidistream_h_

#include <string>
#include <queue>
#include <map>
using namespace std;
#include <alsa/asoundlib.h>

namespace Music
{
	class omidistream;

#define MIDI_TIME_DIRECT -1.0

	//! a simple note (and an abstract one)
	class note
	{
		friend class omidistream;
	  public:	
		static queue<unsigned char> s_free_tags;
		static map<unsigned char, unsigned char> s_tags;

	  protected:
		static unsigned char getFreeTag();
		static void addNote(unsigned char tag, unsigned char note)	{s_tags.insert(make_pair(tag, note));}
		static unsigned char getNote(unsigned char tag)				{return (*(s_tags.find(tag))).second;}

		snd_seq_event_t m_ev;

		note(){}

	  public:
		//! unique ctor
		/*! no direct send can be used ! TODO do it ...
		 *
		 * \param n the pitch [0,127]
		 * \param duration duration of the note {seconds}
		 * \param t absolute time when the note must be played {seconds}
		 */
		note(unsigned char n, double duration=1.0, double t=MIDI_TIME_DIRECT);

		//! return the internal tag of the note
		unsigned char getTag() const {return m_ev.tag;}
	};

	//! a starting note
	class note_on : public note
	{
		friend class omidistream;
		friend class note_off;

	  public:
		//! build a note on event
		/*!
		 * \param n the pitch [0,127]
		 * \param t absolute time when the note must be played {seconds}
		 * \param pTag pTage the returning unique identifier of the note
		 */
		note_on(unsigned char n, double t=MIDI_TIME_DIRECT);

		//! build an empty note on event
		note_on();
	};

	//! a ending note
	class note_off : public note
	{
		friend class omidistream;

	  public:
		//! build a note off event from the on event
		/*!
		 * \param n_on the starting event
		 * \param t absolute time when the note must be played {seconds}
		 */
		note_off(const note_on& n_on, double t=MIDI_TIME_DIRECT);
	};

	//! a midi stream for outputing notes in the midi alsa sequencer
	/*! sample:\n\n
	 *		omidistream str("test lib", 60);\n
	 *		str << note(64, 0) << note(64, 0.5) << note(64, 1) << note(66, 1.5) << note(68, 2, 2) << note(66, 3, 2) << note(64, 4) << note(68, 4.5) << note(66, 5) << note(66, 5.5) << note(64, 6, 2) << drain;\n
	 *     sleep(10);\n
	 */
	class omidistream
	{
		friend class note;

		snd_seq_t* m_seq;
		int m_queue;
		int m_port;
		unsigned char m_channel;
		int m_A3Index;

	  public:

		//! unique ctor
		/*!
		 * \param name a name for the stream (shown in aconnect)
		 * \param tempo the starting tempo of the whole stream {BPM (beats per minutes)}
		 * \param channel midi channel where the note must be thrown [1,?]
		 */
		omidistream(std::string name, double tempo, unsigned char channel=1);

		//! return the unique identifier of the alsa midi client
		int getAlsaMidiID()				{return snd_seq_client_id(m_seq);}
		
		//! return the index of A3 in MIDI standard (69 by default)
		int getA3Index()				{return m_A3Index;}

		//! return the alsa midi port associated to the output sequencer
		int getPort()					{return m_port;}

		//! manualy flush the stream
		omidistream& drain();

		//! output a note in the stream
		omidistream& operator<<(note n);
		inline omidistream& operator<<(note_on n){return operator<<((note)n);}
		inline omidistream& operator<<(note_off n){return operator<<((note)n);}

		//! stream operator for modifiers
		template<class Fn> omidistream& operator<<(Fn fn){fn(*this);return *this;}

		~omidistream();
	};

	//! stream flushing modifier
	omidistream& drain(omidistream& str);
}

#endif // _omidistream_h_

