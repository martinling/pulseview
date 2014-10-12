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

#include <assert.h>

#include <QLineEdit>
#include <QSpinBox>

#include "string.h"

using std::string;

using Glib::ustring;

namespace pv {
namespace prop {

String::String(QString name,
	Getter getter,
	Setter setter) :
	Property(name, getter, setter),
	_line_edit(NULL)
{
}

QWidget* String::get_widget(QWidget *parent, bool auto_commit)
{
	if (_line_edit)
		return _line_edit;

	if (!_getter)
		return NULL;

	Glib::VariantBase variant = _getter();
	if (!variant.gobj())
		return NULL;

	string value = Glib::VariantBase::cast_dynamic<Glib::Variant<ustring>>(
		variant).get();

	_line_edit = new QLineEdit(parent);
	_line_edit->setText(QString::fromStdString(value));

	if (auto_commit)
		connect(_line_edit, SIGNAL(textEdited(const QString&)),
			this, SLOT(on_text_edited(const QString&)));

	return _line_edit;
}

void String::commit()
{
	assert(_setter);

	if (!_line_edit)
		return;

	QByteArray ba = _line_edit->text().toLocal8Bit();
	_setter(Glib::Variant<ustring>::create(ba.data()));
}

void String::on_text_edited(const QString&)
{
	commit();
}

} // prop
} // pv
