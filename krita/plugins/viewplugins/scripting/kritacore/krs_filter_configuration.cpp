/*
 *  Copyright (c) 2006 Cyrille Berger <cberger@cberger.net>
 *
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU Library General Public License as published by
 *  the Free Software Foundation; either version 2 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU Library General Public License for more details.
 *
 *  You should have received a copy of the GNU Library General Public License
 *  along with this program; if not, write to the Free Software
 *  Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301, USA.
 */

#include "krs_filter_configuration.h"

#include <api/variant.h>

#include <kis_filter_configuration.h>

namespace Kross {
namespace KritaCore {

FilterConfiguration::FilterConfiguration(const QString name, Q_INT32 version)
    : Kross::Api::Class<FilterConfiguration>("KritaFilterConfiguration"), m_fConfig(new KisFilterConfiguration(name, version))
{
    addFunction("setProperty", &FilterConfiguration::setProperty, Kross::Api::ArgumentList() << Kross::Api::Argument("Kross::Api::Variant::String") << Kross::Api::Argument("Kross::Api::Variant") );
    addFunction("getProperty", &FilterConfiguration::getProperty, Kross::Api::ArgumentList() << Kross::Api::Argument("Kross::Api::Variant::String") );
}

FilterConfiguration::~FilterConfiguration()
{
}

const QString FilterConfiguration::getClassName() const {
    return "Kross::KritaCore::FilterConfiguration";
}


Kross::Api::Object::Ptr FilterConfiguration::setProperty(Kross::Api::List::Ptr args)
{
    QString name = Kross::Api::Variant::toString(args->item(0));
    QVariant value = Kross::Api::Variant::toVariant(args->item(1));
    m_fConfig->setProperty(name, value);
    return 0;
}
Kross::Api::Object::Ptr FilterConfiguration::getProperty(Kross::Api::List::Ptr args)
{
    QString name = Kross::Api::Variant::toString(args->item(0));
    QVariant value;
    if(m_fConfig->getProperty( name, value))
    {
        return new Kross::Api::Variant(value);
    } else {
        return 0;
    }
}

}
}
