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


#include "Music.h"

#include <algorithm>
#include <iostream>
using namespace std;

Music::NotesName Music::s_notes_name = Music::LOCAL_ANGLO;
int Music::s_tonality = 0;
int Music::s_sampling_rate = -1;
double Music::s_AFreq = 440.0;
const int Music::UNDEFINED_SEMITONE = -1000;
int Music::s_semitone_min = -48;
int Music::s_semitone_max = +48;

Music::SettingsListener::SettingsListener()
{
	s_settings_listeners.push_back(this);
}
Music::SettingsListener::~SettingsListener()
{
	s_settings_listeners.remove(this);
}

list<Music::SettingsListener*>	Music::s_settings_listeners;
void Music::AddSettingsListener(SettingsListener* l)
{
	if(find(s_settings_listeners.begin(), s_settings_listeners.end(), l)==s_settings_listeners.end())
		s_settings_listeners.push_back(l);
}
void Music::RemoveSettingsListener(SettingsListener* l)
{
	s_settings_listeners.remove(l);
}

void Music::SetSamplingRate(int sampling_rate)
{
	s_sampling_rate = sampling_rate;
	for(list<Music::SettingsListener*>::iterator it=s_settings_listeners.begin(); it!=s_settings_listeners.end(); ++it)
		(*it)->samplingRateChanged();
}

void Music::SetAFreq(double AFreq)
{
	s_AFreq = AFreq;
	for(list<Music::SettingsListener*>::iterator it=s_settings_listeners.begin(); it!=s_settings_listeners.end(); ++it)
		(*it)->AFreqChanged();
}

void Music::SetSemitoneBounds(int semitone_min, int semitone_max)
{
	s_semitone_min = semitone_min;
	s_semitone_max = semitone_max;

	for(list<Music::SettingsListener*>::iterator it=s_settings_listeners.begin(); it!=s_settings_listeners.end(); ++it)
		(*it)->semitoneBoundsChanged();
}

