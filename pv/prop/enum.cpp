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

#include <assert.h>

#include <QComboBox>

#include "enum.h"

using std::pair;
using std::vector;

namespace pv {
namespace prop {

Enum::Enum(QString name,
	vector<pair<GVariant*, QString> > values,
	Getter getter, Setter setter) :
	Property(name, getter, setter),
	_values(values),
	_selector(NULL)
{
}

Enum::~Enum()
{
	for (unsigned int i = 0; i < _values.size(); i++)
		g_variant_unref(_values[i].first);
}

QWidget* Enum::get_widget(QWidget *parent, bool auto_commit)
{
	if (_selector)
		return _selector;

	GVariant *const value = _getter ? _getter() : NULL;

	_selector = new QComboBox(parent);
	for (unsigned int i = 0; i < _values.size(); i++) {
		const pair<GVariant*, QString> &v = _values[i];
		_selector->addItem(v.second, qVariantFromValue((void*)v.first));
		if (value && g_variant_equal(v.first, value))
			_selector->setCurrentIndex(i);
	}

	g_variant_unref(value);

	if (auto_commit)
		connect(_selector, SIGNAL(currentIndexChanged(int)),
			this, SLOT(on_current_item_changed(int)));

	return _selector;
}

void Enum::commit()
{
	assert(_setter);

	if (!_selector)
		return;

	const int index = _selector->currentIndex();
	if (index < 0)
		return;

	_setter((GVariant*)_selector->itemData(index).value<void*>());
}

void Enum::on_current_item_changed(int)
{
	commit();
}

} // prop
} // pv
