/*
 * This file is part of the PulseView project.
 *
 * Copyright (C) 2013 Joel Holdsworth <joel@airwebreathe.org.uk>
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

#include "devicemanager.h"
#include "sigsession.h"

#include <cassert>
#include <stdexcept>
#include <sstream>
#include <string>
#include <vector>

#include <libsigrok/libsigrok.hpp>

using std::list;
using std::map;
using std::ostringstream;
using std::runtime_error;
using std::shared_ptr;
using std::string;
using std::vector;

using Glib::VariantBase;

using sigrok::ConfigKey;
using sigrok::Context;
using sigrok::Driver;
using sigrok::HardwareDevice;

namespace pv {

DeviceManager::DeviceManager(shared_ptr<Context> context) :
	_context(context)
{
	for (auto entry : context->get_drivers())
		driver_scan(entry.second, map<const ConfigKey *, VariantBase>());
}

DeviceManager::~DeviceManager()
{
}

shared_ptr<Context> DeviceManager::context()
{
	return _context;
}

const list< shared_ptr<HardwareDevice> >& DeviceManager::devices() const
{
	return _devices;
}

list< shared_ptr<HardwareDevice> > DeviceManager::driver_scan(
	shared_ptr<Driver> driver, map<const ConfigKey *, VariantBase> drvopts)
{
	list< shared_ptr<HardwareDevice> > driver_devices;

	assert(driver);

	// Remove any device instances from this driver from the device
	// list. They will not be valid after the scan.
	auto i = _devices.begin();
	while (i != _devices.end()) {
		if ((*i)->get_driver() == driver)
			i = _devices.erase(i);
		else
			i++;
	}

	// Do the scan
	auto devices = driver->scan(drvopts);
	driver_devices.insert(driver_devices.end(), devices.begin(), devices.end());
	driver_devices.sort(compare_devices);

	// Add the scanned devices to the main list
	_devices.insert(_devices.end(), driver_devices.begin(),
		driver_devices.end());
	_devices.sort(compare_devices);

	return driver_devices;
}

bool DeviceManager::compare_devices(shared_ptr<HardwareDevice> a,
	shared_ptr<HardwareDevice> b)
{
	assert(a);
	assert(b);

	return a->get_description().compare(b->get_description()) < 0;
}

} // namespace pv
