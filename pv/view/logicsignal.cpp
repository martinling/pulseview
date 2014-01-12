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

#include <extdef.h>

#include <math.h>

#include <QFormLayout>
#include <QToolBar>

#include "logicsignal.h"
#include "view.h"

#include "pv/sigsession.h"
#include "pv/data/logic.h"
#include "pv/data/logicsnapshot.h"
#include "pv/view/view.h"

using boost::shared_ptr;
using std::deque;
using std::max;
using std::min;
using std::pair;
using std::vector;

namespace pv {
namespace view {

const float LogicSignal::Oversampling = 2.0f;

const QColor LogicSignal::EdgeColour(0x80, 0x80, 0x80);
const QColor LogicSignal::HighColour(0x00, 0xC0, 0x00);
const QColor LogicSignal::LowColour(0xC0, 0x00, 0x00);

const QColor LogicSignal::SignalColours[10] = {
	QColor(0x16, 0x19, 0x1A),	// Black
	QColor(0x8F, 0x52, 0x02),	// Brown
	QColor(0xCC, 0x00, 0x00),	// Red
	QColor(0xF5, 0x79, 0x00),	// Orange
	QColor(0xED, 0xD4, 0x00),	// Yellow
	QColor(0x73, 0xD2, 0x16),	// Green
	QColor(0x34, 0x65, 0xA4),	// Blue
	QColor(0x75, 0x50, 0x7B),	// Violet
	QColor(0x88, 0x8A, 0x85),	// Grey
	QColor(0xEE, 0xEE, 0xEC),	// White
};

LogicSignal::LogicSignal(pv::SigSession &session, sr_probe *const probe,
	shared_ptr<data::Logic> data) :
	Signal(session, probe),
	_data(data),
	_trigger_none(NULL),
	_trigger_rising(NULL),
	_trigger_high(NULL),
	_trigger_falling(NULL),
	_trigger_low(NULL),
	_trigger_change(NULL)
{
	_colour = SignalColours[probe->index % countof(SignalColours)];
}

LogicSignal::~LogicSignal()
{
}

boost::shared_ptr<pv::data::SignalData> LogicSignal::data() const
{
	return _data;
}

boost::shared_ptr<pv::data::Logic> LogicSignal::logic_data() const
{
	return _data;
}

void LogicSignal::paint_back(QPainter &p, int left, int right)
{
	if (_probe->enabled)
		paint_axis(p, get_y(), left, right);
}

void LogicSignal::paint_mid(QPainter &p, int left, int right)
{
	using pv::view::View;

	QLineF *line;

	vector< pair<int64_t, bool> > edges;

	assert(_probe);
	assert(_data);
	assert(right >= left);

	assert(_view);
	const int y = _v_offset - _view->v_offset();
	
	const double scale = _view->scale();
	assert(scale > 0);
	
	const double offset = _view->offset();

	if (!_probe->enabled)
		return;

	const float high_offset = y - View::SignalHeight + 0.5f;
	const float low_offset = y + 0.5f;

	const deque< shared_ptr<pv::data::LogicSnapshot> > &snapshots =
		_data->get_snapshots();
	if (snapshots.empty())
		return;

	const shared_ptr<pv::data::LogicSnapshot> &snapshot =
		snapshots.front();

	double samplerate = _data->samplerate();

	// Show sample rate as 1Hz when it is unknown
	if (samplerate == 0.0)
		samplerate = 1.0;

	const double pixels_offset = offset / scale;
	const double start_time = _data->get_start_time();
	const int64_t last_sample = snapshot->get_sample_count() - 1;
	const double samples_per_pixel = samplerate * scale;
	const double start = samplerate * (offset - start_time);
	const double end = start + samples_per_pixel * (right - left);

	snapshot->get_subsampled_edges(edges,
		min(max((int64_t)floor(start), (int64_t)0), last_sample),
		min(max((int64_t)ceil(end), (int64_t)0), last_sample),
		samples_per_pixel / Oversampling, _probe->index);
	assert(edges.size() >= 2);

	// Paint the edges
	const unsigned int edge_count = edges.size() - 2;
	QLineF *const edge_lines = new QLineF[edge_count];
	line = edge_lines;

	for (vector<pv::data::LogicSnapshot::EdgePair>::const_iterator i =
			edges.begin() + 1;
		i != edges.end() - 1; i++) {
		const float x = ((*i).first / samples_per_pixel -
			pixels_offset) + left;
		*line++ = QLineF(x, high_offset, x, low_offset);
	}

	p.setPen(EdgeColour);
	p.drawLines(edge_lines, edge_count);
	delete[] edge_lines;

	// Paint the caps
	const unsigned int max_cap_line_count = (edges.size() - 1);
	QLineF *const cap_lines = new QLineF[max_cap_line_count];

	p.setPen(HighColour);
	paint_caps(p, cap_lines, edges, true, samples_per_pixel,
		pixels_offset, left, high_offset);
	p.setPen(LowColour);
	paint_caps(p, cap_lines, edges, false, samples_per_pixel,
		pixels_offset, left, low_offset);

	delete[] cap_lines;
}

void LogicSignal::paint_caps(QPainter &p, QLineF *const lines,
	vector< pair<int64_t, bool> > &edges, bool level,
	double samples_per_pixel, double pixels_offset, float x_offset,
	float y_offset)
{
	QLineF *line = lines;

	for (vector<pv::data::LogicSnapshot::EdgePair>::const_iterator i =
		edges.begin(); i != (edges.end() - 1); i++)
		if ((*i).second == level) {
			*line++ = QLineF(
				((*i).first / samples_per_pixel -
					pixels_offset) + x_offset, y_offset,
				((*(i+1)).first / samples_per_pixel -
					pixels_offset) + x_offset, y_offset);
		}

	p.drawLines(lines, line - lines);
}

void LogicSignal::init_trigger_actions(QWidget *parent)
{
	_trigger_none = new QAction(QIcon(":/icons/trigger-none.svg"),
		tr("No trigger"), parent);
	_trigger_none->setCheckable(true);
	connect(_trigger_none, SIGNAL(triggered()),
		this, SLOT(on_trigger_none()));

	_trigger_rising = new QAction(QIcon(":/icons/trigger-rising.svg"),
		tr("Trigger on rising edge"), parent);
	_trigger_rising->setCheckable(true);
	connect(_trigger_rising, SIGNAL(triggered()),
		this, SLOT(on_trigger_rising()));

	_trigger_high = new QAction(QIcon(":/icons/trigger-high.svg"),
		tr("Trigger on high level"), parent);
	_trigger_high->setCheckable(true);
	connect(_trigger_high, SIGNAL(triggered()),
		this, SLOT(on_trigger_high()));

	_trigger_falling = new QAction(QIcon(":/icons/trigger-falling.svg"),
		tr("Trigger on falling edge"), parent);
	_trigger_falling->setCheckable(true);
	connect(_trigger_falling, SIGNAL(triggered()),
		this, SLOT(on_trigger_falling()));

	_trigger_low = new QAction(QIcon(":/icons/trigger-low.svg"),
		tr("Trigger on low level"), parent);
	_trigger_low->setCheckable(true);
	connect(_trigger_low, SIGNAL(triggered()),
		this, SLOT(on_trigger_low()));

	_trigger_change = new QAction(QIcon(":/icons/trigger-change.svg"),
		tr("Trigger on rising or falling edge"), parent);
	_trigger_change->setCheckable(true);
	connect(_trigger_change, SIGNAL(triggered()),
		this, SLOT(on_trigger_change()));
}

void LogicSignal::populate_popup_form(QWidget *parent, QFormLayout *form)
{
	GVariant *gvar;

	Signal::populate_popup_form(parent, form);

	// Add the trigger actions
	const sr_dev_inst *const sdi = _session.get_device();
	if (sr_config_list(sdi->driver, sdi, NULL, SR_CONF_TRIGGER_TYPE,
		&gvar) == SR_OK)
	{
		const char *const trig_types =
			g_variant_get_string(gvar, NULL);

		if (trig_types && trig_types[0] != '\0')
		{
			_trigger_bar = new QToolBar(parent);

			init_trigger_actions(_trigger_bar);
			_trigger_bar->addAction(_trigger_none);
			add_trigger_action(trig_types, 'r', _trigger_rising);
			add_trigger_action(trig_types, '1', _trigger_high);
			add_trigger_action(trig_types, 'f', _trigger_falling);
			add_trigger_action(trig_types, '0', _trigger_low);
			add_trigger_action(trig_types, 'c', _trigger_change);
		
			update_trigger_actions();

			form->addRow(tr("Trigger"), _trigger_bar);
		}

		g_variant_unref(gvar);
	}
}

void LogicSignal::add_trigger_action(const char *trig_types, char type,
	QAction *action)
{
	while(*trig_types)
		if(*trig_types++ == type) {
			_trigger_bar->addAction(action);
			break;
		}
}

void LogicSignal::update_trigger_actions()
{
	const char cur_trigger = _probe->trigger ?
		_probe->trigger[0] : '\0';
	_trigger_none->setChecked(cur_trigger == '\0');
	_trigger_rising->setChecked(cur_trigger == 'r');
	_trigger_high->setChecked(cur_trigger == '1');
	_trigger_falling->setChecked(cur_trigger == 'f');
	_trigger_low->setChecked(cur_trigger == '0');
	_trigger_change->setChecked(cur_trigger == 'c');
}

void LogicSignal::set_trigger(char type)
{
	const char trigger_type_string[2] = {type, 0};
	const char *const trigger_string =
		(type != 0) ? trigger_type_string : NULL;

	const sr_dev_inst *const sdi = _session.get_device();
	const int probe_count = g_slist_length(sdi->probes);
	assert(probe_count > 0);

	assert(_probe && _probe->index < probe_count);

	for (int i = 0; i < probe_count; i++) {
		sr_dev_trigger_set(sdi, i, (i == _probe->index) ?
			trigger_string : NULL);
	}

	update_trigger_actions();
}

void LogicSignal::on_trigger_none()
{
	set_trigger('\0');	
}

void LogicSignal::on_trigger_rising()
{
	set_trigger('r');	
}

void LogicSignal::on_trigger_high()
{
	set_trigger('1');	
}

void LogicSignal::on_trigger_falling()
{
	set_trigger('f');	
}

void LogicSignal::on_trigger_low()
{
	set_trigger('0');	
}

void LogicSignal::on_trigger_change()
{
	set_trigger('c');	
}

} // namespace view
} // namespace pv
