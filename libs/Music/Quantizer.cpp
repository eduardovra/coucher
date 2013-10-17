// Copyright 2005 "Gilles Degottex"

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


#include "Quantizer.h"

#include <iostream>
using namespace std;
#include <Music/Music.h>
using namespace Music;
#include <CppAddons/Random.h>

Quantizer::Quantizer(float tolerance, float min_density)
{
	m_min_ht = -48;
	m_channels.resize(97);

	m_tolerance = tolerance;
	m_min_density = min_density;

	m_time.start();
}

void Quantizer::quantize(const vector<bool> hts, int min_ht)
{
	double current_time = m_time.elapsed();

	m_min_stored_recon = 1000000;
	for(size_t ht=0; ht<hts.size(); ht++)
	{
		int rht = ht+min_ht-m_min_ht;

		// add the new one
		m_channels[rht].old_states.push_front(State(current_time, hts[ht]));

		m_min_stored_recon = min(m_min_stored_recon, int(m_channels[rht].old_states.size()));

		// update channel
		update(rht);

		// drop unused recognitions
		while(!m_channels[rht].old_states.empty() && (current_time-m_channels[rht].old_states.back().time>m_tolerance))
			m_channels[rht].old_states.pop_back();
	}
}

void Quantizer::update(int rht)
{
	Channel& channel = m_channels[rht];

	if(!channel.old_states.empty())
	{
		// compute density
		int dens = 0;
		for(size_t i=0; i<channel.old_states.size(); i++)
			if(channel.old_states[i].play)
				dens++;

		channel.reliability = float(dens)/channel.old_states.size();

		// if a density is strong enough (depend of parameter dens_required)
		if(channel.reliability>m_min_density)
		{
			if(channel.state==Channel::QC_NOTHING)
			{
				channel.state = Channel::QC_STARTING;
				channel.duration.start();
				channel.lag.start();
			}

			if(channel.state==Channel::QC_STARTING)
			{
				if(channel.lag.elapsed()>m_tolerance)
				{
					channel.state = Channel::QC_PLAYING;
					channel.last_tag = Random::s_random.nextInt();
					MFireEvent(noteStarted(channel.last_tag, rht+m_min_ht, -channel.lag.elapsed()));
				}
			}

			if(channel.state==Channel::QC_PLAYING)
				channel.lag.start();
		}
		else
		{
			if(channel.state==Channel::QC_STARTING)
				channel.state = Channel::QC_NOTHING;
			else if(channel.state==Channel::QC_PLAYING && channel.lag.elapsed()>m_tolerance)
			{
				channel.state = Channel::QC_NOTHING;
				MFireEvent(noteFinished(channel.last_tag, rht+m_min_ht, -channel.lag.elapsed()));
				MFireEvent(notePlayed(rht+m_min_ht, channel.duration.elapsed()-channel.lag.elapsed(), -channel.lag.elapsed()-channel.duration.elapsed()));
			}
		}
	}
}

void Quantizer::cutAll()
{
	m_min_stored_recon = 0;
	for(size_t ht=0; ht<m_channels.size(); ht++)
	{
		m_channels[ht].old_states.clear();
		update(ht+m_min_ht);
	}
}

