/*
Copyright (C) 2012  Till Theato <root@ttill.de>
This file is part of Kdenlive. See www.kdenlive.org.

This program is free software: you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation, either version 3 of the License, or
(at your option) any later version.
*/

#include "generictimelineclip.h"
#include "genericprojectclip.h"
#include "core/project/producerwrapper.h"
#include "core/project/timelinetrack.h"

#include <KDebug>


GenericTimelineClip::GenericTimelineClip(GenericProjectClip* projectClip, TimelineTrack* parent, ProducerWrapper* producer) :
    AbstractTimelineClip(projectClip, parent, producer)
{
    kDebug() << "generic timelineclip created";
}

GenericTimelineClip::~GenericTimelineClip()
{

}

#include "generictimelineclip.moc"