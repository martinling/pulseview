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

#ifndef PULSEVIEW_PV_DEVICEMANAGER_H
#define PULSEVIEW_PV_DEVICEMANAGER_H

#include <list>
#include <map>
#include <memory>
#include <string>

namespace Glib {
	class VariantBase;
}

namespace sigrok {
	class ConfigKey;
	class Context;
	class Driver;
	class HardwareDevice;
}

namespace pv {

class SigSession;

class DeviceManager
{
public:
	DeviceManager(std::shared_ptr<sigrok::Context> context);

	~DeviceManager();

	std::shared_ptr<sigrok::Context> context();

	const std::list< std::shared_ptr<sigrok::HardwareDevice> >&
		devices() const;

	std::list< std::shared_ptr<sigrok::HardwareDevice> > driver_scan(
		std::shared_ptr<sigrok::Driver> driver,
		std::map<const sigrok::ConfigKey *, Glib::VariantBase> drvopts);

private:
	static bool compare_devices(std::shared_ptr<sigrok::HardwareDevice> a,
		std::shared_ptr<sigrok::HardwareDevice> b);

protected:
	std::shared_ptr<sigrok::Context> _context;
	std::list< std::shared_ptr<sigrok::HardwareDevice> > _devices;
};

} // namespace pv

#endif // PULSEVIEW_PV_DEVICEMANAGER_H
