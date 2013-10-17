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


#include "omidistream.h"

#include <iostream>
using namespace std;

namespace Music
{
	queue<unsigned char> note::s_free_tags;
	map<unsigned char, unsigned char> note::s_tags;

	unsigned char note::getFreeTag()
	{
		if(s_free_tags.empty())
			for(unsigned char c=1; c<255; c++)
				s_free_tags.push(c);

		unsigned char tag = s_free_tags.front();
		s_free_tags.pop();

		return tag;
	}
	note::note(unsigned char n, double t, double duration)
	{
		snd_seq_ev_clear(&m_ev);

		m_ev.type = SND_SEQ_EVENT_NOTE;
		m_ev.flags = SND_SEQ_TIME_STAMP_REAL|SND_SEQ_TIME_MODE_ABS|SND_SEQ_EVENT_LENGTH_FIXED|SND_SEQ_PRIORITY_NORMAL;
		m_ev.time.time.tv_sec = (unsigned int)t;
		m_ev.time.time.tv_nsec = (unsigned int)((t-(unsigned int)t)*1000000000);
		m_ev.queue = ~SND_SEQ_QUEUE_DIRECT;
		m_ev.tag = getFreeTag();
		m_ev.data.note.note = n;
		m_ev.data.note.velocity = 64;
		m_ev.data.note.off_velocity = 64;
		m_ev.data.note.duration = (unsigned int)(duration*1000);
	}

	note_on::note_on()
	{
		snd_seq_ev_clear(&m_ev);
	}
	note_on::note_on(unsigned char n, double t)
	{
		snd_seq_ev_clear(&m_ev);

		m_ev.type = SND_SEQ_EVENT_NOTEON;
		m_ev.flags = SND_SEQ_EVENT_LENGTH_FIXED|SND_SEQ_PRIORITY_NORMAL;
		if(t==MIDI_TIME_DIRECT)	m_ev.queue = SND_SEQ_QUEUE_DIRECT;
		else
		{
			m_ev.flags |= SND_SEQ_TIME_STAMP_REAL|SND_SEQ_TIME_MODE_ABS;
			m_ev.queue = ~SND_SEQ_QUEUE_DIRECT;
			m_ev.time.time.tv_sec = (unsigned int)t;
			m_ev.time.time.tv_nsec = (unsigned int)((t-(unsigned int)t)*1000000000);
		}
		m_ev.tag = getFreeTag();
		m_ev.data.note.note = n;
		m_ev.data.note.velocity = 64;

		addNote(m_ev.tag, n);

//		cerr << "note_off::note_on tag=" << int(m_ev.tag) << " note=" << int(m_ev.data.note.note) << " time=" << t << endl;
	}
	note_off::note_off(const note_on& n_on, double t)
	{
		snd_seq_ev_clear(&m_ev);

		m_ev.type = SND_SEQ_EVENT_NOTEOFF;
		m_ev.flags = SND_SEQ_EVENT_LENGTH_FIXED|SND_SEQ_PRIORITY_NORMAL;
		if(t==MIDI_TIME_DIRECT)	m_ev.queue = SND_SEQ_QUEUE_DIRECT;
		else
		{
			m_ev.flags |= SND_SEQ_TIME_STAMP_REAL|SND_SEQ_TIME_MODE_ABS;
			m_ev.queue = ~SND_SEQ_QUEUE_DIRECT;
			m_ev.time.time.tv_sec = (unsigned int)t;
			m_ev.time.time.tv_nsec = (unsigned int)((t-(unsigned int)t)*1000000000);
		}
		m_ev.tag = n_on.getTag();
		m_ev.data.note.note = n_on.m_ev.data.note.note;
		m_ev.data.note.velocity = 64;
	}

	omidistream::omidistream(string name, double tempo, unsigned char channel)
	: m_seq(NULL)
	, m_queue(0)
	, m_port(0)
	, m_channel(channel)
	, m_A3Index(69)
	{
		int err = snd_seq_open(&m_seq, "default", SND_SEQ_OPEN_OUTPUT, 0);
		if (err < 0)	throw string("snd_seq_open: ")+snd_strerror(err);

		snd_seq_set_client_name(m_seq, name.c_str());

		m_port = snd_seq_create_simple_port(m_seq, (name+" port").c_str(),
				SND_SEQ_PORT_CAP_READ|SND_SEQ_PORT_CAP_SUBS_READ, SND_SEQ_PORT_TYPE_MIDI_GENERIC);

		// 		cout << "client built: " << snd_seq_client_id(m_seq) << ":" << m_port << endl;

		m_queue = snd_seq_alloc_named_queue(m_seq, (name+" queue").c_str());
		snd_seq_queue_tempo_t* queue_tempo;
		snd_seq_queue_tempo_alloca(&queue_tempo);
		snd_seq_queue_tempo_set_tempo(queue_tempo, (unsigned int)(60*1000000/tempo)); // 60 BPM
		snd_seq_queue_tempo_set_ppq(queue_tempo, 48); // 48 PPQ
		snd_seq_set_queue_tempo(m_seq, m_queue, queue_tempo);
		// 		cout << "Queue built and temperized: " << m_queue << endl;
		snd_seq_start_queue(m_seq, m_queue, NULL);
	}

	omidistream::~omidistream()
	{
		snd_seq_stop_queue(m_seq, m_queue, NULL);
	}

	omidistream& omidistream::operator<<(note n)
	{
		//		n.ev.source.client = snd_seq_client_id(m_seq);
		snd_seq_ev_set_source(&(n.m_ev), m_port);
		snd_seq_ev_set_subs(&(n.m_ev));
		n.m_ev.data.note.channel = m_channel;
		if(n.m_ev.queue!=SND_SEQ_QUEUE_DIRECT)
			n.m_ev.queue = m_queue;

		if(n.m_ev.type!=SND_SEQ_EVENT_NOTEON)
		{
			note::s_free_tags.push(n.m_ev.tag);
			note::s_tags.erase(n.m_ev.tag);
		}

		int err = snd_seq_event_output(m_seq, &n.m_ev);
		if(err<0)	throw string("operator<<(note) snd_seq_event_output: ")+string(snd_strerror(err));

		return *this;
	}

	omidistream& omidistream::drain()
	{
		int err = snd_seq_drain_output(m_seq);
		if(err<0)	throw string("::omidistream::drain() snd_seq_drain_output: ")+string(snd_strerror(err));

		return *this;
	}

	omidistream& drain(omidistream& str)
	{
		return str.drain();
	}
};

