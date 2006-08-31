/***************************************************************************
                          trackviewnamedecorator  -  description
                             -------------------
    begin                : Fri Nov 28 2003
    copyright            : (C) 2003 by Jason Wood
    email                : jasonwood@blueyonder.co.uk
 ***************************************************************************/

/***************************************************************************
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 ***************************************************************************/
#include "trackviewnamedecorator.h"

#include <qnamespace.h>
#include  <qpainter.h>
#include  <kdebug.h>

#include "docclipref.h"
#include "gentime.h"
#include "kdenlivedoc.h"
#include "ktimeline.h"

namespace Gui {

    TrackViewNameDecorator::TrackViewNameDecorator(KTimeLine * timeline,
	KdenliveDoc * doc):DocTrackDecorator(timeline, doc) {
    } TrackViewNameDecorator::~TrackViewNameDecorator() {
    }

// virtual
    void TrackViewNameDecorator::paintClip(double startX, double endX,
	QPainter & painter, DocClipRef * clip, QRect & rect,
	bool selected) {
		int sx = (int)startX;
	    /*(int) timeline()->mapValueToLocal(clip->trackStart().
	    frames(document()->framesPerSecond()));*/
		int ex = (int)endX;
	    /*(int) timeline()->mapValueToLocal(clip->trackEnd().
	    frames(document()->framesPerSecond()));*/
	int clipWidth = ex - sx;
	int tx = ex;

	if (sx < rect.x()) {
	    sx = rect.x();
	}
	if (ex > rect.x() + rect.width()) {
	    ex = rect.x() + rect.width();
	}
	//ex -= sx;

	//painter.setClipRect(sx, rect.y(), clipWidth, rect.height());
	QString txt = clip->name();
	if (clip->speed() != 1.0) txt.append(" (" + QString::number((int)(clip->speed() * 100)) + "%)");
	// draw video name text
	QRect textBound =
	    painter.boundingRect(0, 0, clipWidth, rect.height(), Qt::AlignLeft, txt);

	double border = 150.0;
	int nameRepeat =
	    (int) std::floor((double) clipWidth /
	    ((double) textBound.width() + border));
	if (nameRepeat < 1)
	    nameRepeat = 1;
	int textWidth = clipWidth / nameRepeat;


	if (textWidth > 0) {
	    int start =
		(int) timeline()->mapValueToLocal(clip->trackStart().
		frames(document()->framesPerSecond()));

	    start = sx - ((sx - start) % textWidth);
	    int count = start;

	    while (count < sx + ex) {
		if (count + textWidth <= tx) {
		    painter.setPen(selected ? Qt::white : Qt::black);
		    painter.drawText(count, rect.y() + 12, textWidth,
			rect.height() - 12, Qt::AlignTop | Qt::AlignHCenter,
			txt);
		}
		count += textWidth;
	    }

	// black line on top if clip has effects
	if (clip->hasEffect()) {
	    QString txt = clip->selectedEffect()->name().upper();
	    QFont orig = painter.font();
	    QFont ft = orig;
	    ft.setPixelSize(7);
	    painter.setFont(ft);
	    int textWidth = painter.fontMetrics().width( txt );
	    painter.fillRect(startX + 1, rect.y() + 1, textWidth + 4, 9, QBrush(Qt::black));
	    painter.setPen(Qt::white);
	    painter.drawText(startX + 2, rect.y() + 1, textWidth + 2, 8, Qt::AlignTop | Qt::AlignLeft, txt);
	    painter.setFont(orig);
	}
	    painter.setPen(Qt::black);
	}

	//painter.setClipping(false);
        
        QPen pen = painter.pen();
        if (selected) {
            pen.setColor(Qt::red);
            //pen.setStyle(Qt::DotLine);
            painter.setPen(pen);
        }
		  painter.drawRect((int)startX, rect.y(), (int)(endX - startX), rect.height());
        if (selected) {
            pen.setColor(Qt::black);
            //pen.setStyle(Qt::SolidLine);
            painter.setPen(pen);
        }
    }

}				// namespace Gui
