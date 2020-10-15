/* This file is part of the Calligra libraries
   Copyright (C) 2001 Werner Trobin <trobin@kde.org>

This library is free software; you can redistribute it and/or
modify it under the terms of the GNU Library General Public
License as published by the Free Software Foundation; either
version 2 of the License, or (at your option) any later version.

This library is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
Library General Public License for more details.

You should have received a copy of the GNU Library General Public License
along with this library; see the file COPYING.LIB.  If not, write to
the Free Software Foundation, Inc., 51 Franklin Street, Fifth Floor,
Boston, MA 02110-1301, USA.
*/
// clazy:excludeall=qstring-arg
#include "KoFilterVertex.h"
#include <limits.h> // UINT_MAX
#include "KoFilterEdge.h"

namespace CalligraFilter {

Vertex::Vertex(const QByteArray& mimeType)
        : m_predecessor(nullptr)
        , m_mimeType(mimeType)
        , m_weight(UINT_MAX)
        , m_index(-1)
        , d(nullptr)
{
}

Vertex::~Vertex()
{
    qDeleteAll(m_edges);
}

bool Vertex::setKey(unsigned int key)
{
    if (m_weight > key) {
        m_weight = key;
        return true;
    }
    return false;
}

void Vertex::reset()
{
    m_weight = UINT_MAX;
    m_predecessor = nullptr;
}

void Vertex::addEdge(Edge* edge)
{
    if (!edge || edge->weight() == 0)
        return;
    m_edges.append(edge);
}

const Edge* Vertex::findEdge(const Vertex* vertex) const
{
    if (!vertex)
        return nullptr;
    const Edge* edge = nullptr;
    for (Edge* e : qAsConst(m_edges)) {
        if (e->vertex() == vertex &&
            (!edge || e->weight() < edge->weight())) {
            edge = e;
        }
    }
    return edge;
}

void Vertex::relaxVertices(PriorityQueue<Vertex>& queue)
{
    for (Edge* e : qAsConst(m_edges)) {
        e->relax(this, queue);
    }
}

void Vertex::dump(const QByteArray& indent) const
{
#ifdef NDEBUG
    Q_UNUSED(indent)
#else
    debugFilter << indent << "Vertex:" << m_mimeType << " (" << m_weight << "):";
    const QByteArray i(indent + "   ");
    for (Edge* edge : qAsConst(m_edges)) {
        edge->dump(i);
    }
#endif
}

}
