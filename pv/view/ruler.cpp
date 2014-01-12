/*
 * This file is part of the PulseView project.
 *
 * Copyright (C) 2012 Joel Holdsworth <joel@airwebreathe.org.uk>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA  02110-1301 USA
 */

#include "ruler.h"

#include "cursor.h"
#include "view.h"
#include "viewport.h"

#include <extdef.h>

#include <assert.h>
#include <math.h>
#include <limits.h>

#include <QApplication>
#include <QMouseEvent>
#include <QPainter>
#include <QTextStream>

#include <pv/widgets/popup.h>

using namespace Qt;
using boost::shared_ptr;

namespace pv {
namespace view {

const int Ruler::RulerHeight = 30;
const int Ruler::MinorTickSubdivision = 4;
const int Ruler::ScaleUnits[3] = {1, 2, 5};

const QString Ruler::SIPrefixes[9] =
	{"f", "p", "n", QChar(0x03BC), "m", "", "k", "M", "G"};
const int Ruler::FirstSIPrefixPower = -15;

const int Ruler::HoverArrowSize = 5;

Ruler::Ruler(View &parent) :
	MarginWidget(parent),
	_dragging(false)
{
	setMouseTracking(true);

	connect(&_view, SIGNAL(hover_point_changed()),
		this, SLOT(hover_point_changed()));
}

void Ruler::clear_selection()
{
	CursorPair &cursors = _view.cursors();
	cursors.first()->select(false);
	cursors.second()->select(false);
	update();
}

QString Ruler::format_time(double t, unsigned int prefix,
	unsigned int precision)
{
	const double multiplier = pow(10.0,
		(int)- prefix * 3 - FirstSIPrefixPower);

	QString s;
	QTextStream ts(&s);
	ts.setRealNumberPrecision(precision);
	ts << fixed << forcesign << (t  * multiplier) <<
		SIPrefixes[prefix] << "s";
	return s;
}

QSize Ruler::sizeHint() const
{
	return QSize(0, RulerHeight);
}

void Ruler::paintEvent(QPaintEvent*)
{

	const double SpacingIncrement = 32.0f;
	const double MinValueSpacing = 32.0f;
	const int ValueMargin = 3;

	QPainter p(this);
	p.setRenderHint(QPainter::Antialiasing);

	double min_width = SpacingIncrement, typical_width;
	double tick_period;
	unsigned int prefix;

	// Find tick spacing, and number formatting that does not cause
	// value to collide.
	do
	{
		const double min_period = _view.scale() * min_width;

		const int order = (int)floorf(log10f(min_period));
		const double order_decimal = pow(10.0, order);

		unsigned int unit = 0;

		do
		{
			tick_period = order_decimal * ScaleUnits[unit++];
		} while (tick_period < min_period && unit < countof(ScaleUnits));

		prefix = (order - FirstSIPrefixPower) / 3;
		assert(prefix < countof(SIPrefixes));


		typical_width = p.boundingRect(0, 0, INT_MAX, INT_MAX,
			AlignLeft | AlignTop, format_time(_view.offset(),
			prefix)).width() + MinValueSpacing;

		min_width += SpacingIncrement;

	} while(typical_width > tick_period / _view.scale());

	const int text_height = p.boundingRect(0, 0, INT_MAX, INT_MAX,
		AlignLeft | AlignTop, "8").height();

	// Draw the tick marks
	p.setPen(palette().color(foregroundRole()));

	const double minor_tick_period = tick_period / MinorTickSubdivision;
	const double first_major_division =
		floor(_view.offset() / tick_period);
	const double first_minor_division =
		ceil(_view.offset() / minor_tick_period);
	const double t0 = first_major_division * tick_period;

	int division = (int)round(first_minor_division -
		first_major_division * MinorTickSubdivision) - 1;

	const int major_tick_y1 = text_height + ValueMargin * 2;
	const int tick_y2 = height();
	const int minor_tick_y1 = (major_tick_y1 + tick_y2) / 2;

	double x;

	do {
		const double t = t0 + division * minor_tick_period;
		x = (t - _view.offset()) / _view.scale();

		if (division % MinorTickSubdivision == 0)
		{
			// Draw a major tick
			p.drawText(x, ValueMargin, 0, text_height,
				AlignCenter | AlignTop | TextDontClip,
				format_time(t, prefix));
			p.drawLine(QPointF(x, major_tick_y1),
				QPointF(x, tick_y2));
		}
		else
		{
			// Draw a minor tick
			p.drawLine(QPointF(x, minor_tick_y1),
				QPointF(x, tick_y2));
		}

		division++;

	} while (x < width());

	// Draw the cursors
	if (_view.cursors_shown())
		_view.cursors().draw_markers(p, rect(), prefix);

	// Draw the hover mark
	draw_hover_mark(p);

	p.end();
}

void Ruler::mouseMoveEvent(QMouseEvent *e)
{
	if (!(e->buttons() & Qt::LeftButton))
		return;
	
	if ((e->pos() - _mouse_down_point).manhattanLength() <
		QApplication::startDragDistance())
		return;

	_dragging = true;

	if (shared_ptr<TimeMarker> m = _grabbed_marker.lock())
		m->set_time(_view.offset() +
			((double)e->x() + 0.5) * _view.scale());
}

void Ruler::mousePressEvent(QMouseEvent *e)
{
	if (e->buttons() & Qt::LeftButton)
	{
		_mouse_down_point = e->pos();

		_grabbed_marker.reset();

		clear_selection();

		if (_view.cursors_shown()) {
			CursorPair &cursors = _view.cursors();
			if (cursors.first()->get_label_rect(
				rect()).contains(e->pos()))
				_grabbed_marker = cursors.first();
			else if (cursors.second()->get_label_rect(
				rect()).contains(e->pos()))
				_grabbed_marker = cursors.second();
		}

		if (shared_ptr<TimeMarker> m = _grabbed_marker.lock())
			m->select();

		selection_changed();
	}
}

void Ruler::mouseReleaseEvent(QMouseEvent *)
{
	using pv::widgets::Popup;

	if (!_dragging)
		if (shared_ptr<TimeMarker> m = _grabbed_marker.lock()) {
			Popup *const p = m->create_popup(&_view);
			p->set_position(mapToGlobal(QPoint(m->get_x(),
				height())), Popup::Bottom);
			p->show();
		}

	_dragging = false;
	_grabbed_marker.reset();
}

void Ruler::draw_hover_mark(QPainter &p)
{
	const int x = _view.hover_point().x();

	if (x == -1 || _dragging)
		return;

	p.setPen(QPen(Qt::NoPen));
	p.setBrush(QBrush(palette().color(foregroundRole())));

	const int b = height() - 1;
	const QPointF points[] = {
		QPointF(x, b),
		QPointF(x - HoverArrowSize, b - HoverArrowSize),
		QPointF(x + HoverArrowSize, b - HoverArrowSize)
	};
	p.drawPolygon(points, countof(points));
}

void Ruler::hover_point_changed()
{
	update();
}

} // namespace view
} // namespace pv
