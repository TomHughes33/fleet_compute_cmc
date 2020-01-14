/*
 * Copyright (C) 2018 Daniel Nicoletti <dantti12@gmail.com>
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU Affero General Public License as
 * published by the Free Software Foundation, either version 3 of the
 * License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 * GNU Affero General Public License for more details.
 *
 * You should have received a copy of the GNU Affero General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */
#ifndef NETWORKS_H
#define NETWORKS_H

#include <Cutelyst/Controller>

using namespace Cutelyst;

class Virtlyst;
class Networks : public Controller
{
    Q_OBJECT
public:
    explicit Networks(Virtlyst *parent = nullptr);

    C_ATTR(index, :Path :AutoArgs)
    void index(Context *c, const QString &hostId);

    C_ATTR(network, :Path :AutoArgs)
    void network(Context *c, const QString &hostId, const QString &netName);

    static bool defaultNetwork;

private:
    Virtlyst *m_virtlyst;
};

#endif // NETWORKS_H
