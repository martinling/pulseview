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

#include <stdint.h>

#include <QDebug>

#include "deviceoptions.h"

#include <pv/prop/bool.h>
#include <pv/prop/double.h>
#include <pv/prop/enum.h>
#include <pv/prop/int.h>

#include <libsigrok/libsigrok.hpp>

using boost::optional;
using std::function;
using std::make_pair;
using std::pair;
using std::shared_ptr;
using std::string;
using std::vector;

using sigrok::Configurable;
using sigrok::ConfigKey;
using sigrok::Error;

namespace pv {
namespace prop {
namespace binding {

DeviceOptions::DeviceOptions(shared_ptr<sigrok::Configurable> configurable) :
	_configurable(configurable)
{
	assert(configurable);

	for (auto key : configurable->config_keys(ConfigKey::DEVICE_OPTIONS)) {
		Glib::VariantContainerBase gvar_list;

		try {
			gvar_list = configurable->config_list(key);
		} catch (Error error) {
			qDebug() << "Config key"
				<< QString::fromStdString(key->get_name())
				<< "was listed, but no options were given";
			continue;
		}

		const QString name = QString::fromStdString(key->get_description());

		const Property::Getter get = [&, key]() {
			return _configurable->config_get(key); };
		const Property::Setter set = [&, key](Glib::VariantBase value) {
			_configurable->config_set(key, value); };

		switch (key->get_id())
		{
		case SR_CONF_SAMPLERATE:
			// Sample rate values are not bound because they are shown
			// in the SamplingBar
			break;

		case SR_CONF_CAPTURE_RATIO:
			bind_int(name, "%", pair<int64_t, int64_t>(0, 100),
				get, set);
			break;

		case SR_CONF_PATTERN_MODE:
		case SR_CONF_BUFFERSIZE:
		case SR_CONF_TRIGGER_SOURCE:
		case SR_CONF_TRIGGER_SLOPE:
		case SR_CONF_FILTER:
		case SR_CONF_COUPLING:
		case SR_CONF_CLOCK_EDGE:
			bind_enum(name, gvar_list, get, set);
			break;

		case SR_CONF_EXTERNAL_CLOCK:
		case SR_CONF_RLE:
			bind_bool(name, get, set);
			break;

		case SR_CONF_TIMEBASE:
			bind_enum(name, gvar_list, get, set, print_timebase);
			break;

		case SR_CONF_VDIV:
			bind_enum(name, gvar_list, get, set, print_vdiv);
			break;

		case SR_CONF_VOLTAGE_THRESHOLD:
			bind_enum(name, gvar_list, get, set, print_voltage_threshold);
			break;

		default:
			break;
		}
	}
}

void DeviceOptions::bind_bool(const QString &name,
	Property::Getter getter, Property::Setter setter)
{
	assert(_configurable);
	_properties.push_back(shared_ptr<Property>(new Bool(
		name, getter, setter)));
}

void DeviceOptions::bind_enum(const QString &name,
	Glib::VariantContainerBase gvar_list, Property::Getter getter,
	Property::Setter setter, function<QString (Glib::VariantBase)> printer)
{
	Glib::VariantBase gvar;
	vector< pair<Glib::VariantBase, QString> > values;

	assert(_configurable);

	Glib::VariantIter iter(gvar_list);
	while ((iter.next_value(gvar)))
		values.push_back(make_pair(gvar, printer(gvar)));

	_properties.push_back(shared_ptr<Property>(new Enum(name, values,
		getter, setter)));
}

void DeviceOptions::bind_int(const QString &name, QString suffix,
	optional< std::pair<int64_t, int64_t> > range,
	Property::Getter getter, Property::Setter setter)
{
	assert(_configurable);

	_properties.push_back(shared_ptr<Property>(new Int(name, suffix, range,
		getter, setter)));
}

QString DeviceOptions::print_timebase(Glib::VariantBase gvar)
{
	uint64_t p, q;
	g_variant_get(gvar.gobj(), "(tt)", &p, &q);
	return QString::fromUtf8(sr_period_string(p * q));
}

QString DeviceOptions::print_vdiv(Glib::VariantBase gvar)
{
	uint64_t p, q;
	g_variant_get(gvar.gobj(), "(tt)", &p, &q);
	return QString::fromUtf8(sr_voltage_string(p, q));
}

QString DeviceOptions::print_voltage_threshold(Glib::VariantBase gvar)
{
	gdouble lo, hi;
	g_variant_get(gvar.gobj(), "(dd)", &lo, &hi);
	return QString("L<%1V H>%2V").arg(lo, 0, 'f', 1).arg(hi, 0, 'f', 1);
}

} // binding
} // prop
} // pv

