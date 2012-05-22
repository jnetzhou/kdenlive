/*
Copyright (C) 2012  Till Theato <root@ttill.de>
This file is part of kdenlive. See www.kdenlive.org.

This program is free software: you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation, either version 3 of the License, or
(at your option) any later version.
*/

#ifndef IMAGEPROJECTCLIP_H
#define IMAGEPROJECTCLIP_H

#include "core/project/abstractprojectclip.h"

class ImageTimelineClip;
class QPixmap;


class ImageProjectClip : public AbstractProjectClip
{
    Q_OBJECT

public:
    ImageProjectClip(const KUrl& url, QObject* parent = 0);
    ImageProjectClip(ProducerWrapper* producer, QObject* parent = 0);
    ImageProjectClip(const QDomElement &description, QObject *parent = 0);
    ~ImageProjectClip();

    AbstractTimelineClip *addInstance(ProducerWrapper *producer, TimelineTrack *parent);

    QPixmap *thumbnail();

private:
    void init();

    QList<ImageTimelineClip *> m_instances;
    QPixmap *m_thumbnail;
};

#endif
