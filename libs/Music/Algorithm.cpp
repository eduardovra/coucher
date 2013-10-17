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


#include "Algorithm.h"
using namespace Music;

Algorithm::Algorithm(double volume_treshold)
: m_volume_treshold(volume_treshold)
, m_volume_max(0.0)
{
}

Algorithm::~Algorithm()
{
}

Transform::Transform(double volume_treshold, double components_treshold, int size)
: Algorithm(volume_treshold)
, m_components_treshold(components_treshold)
, m_components(size)
, m_first_fond(-1)
, m_is_fondamental(size)
{
}
Transform::Transform(double volume_treshold, double components_treshold)
: Algorithm(volume_treshold)
, m_components_treshold(components_treshold)
, m_components(GetNbSemitones())
, m_first_fond(-1)
, m_is_fondamental(GetNbSemitones())
{
}

