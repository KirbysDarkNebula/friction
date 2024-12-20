/*
#
# Friction - https://friction.graphics
#
# Copyright (c) Ole-André Rodlie and contributors
#
# This program is free software: you can redistribute it and/or modify
# it under the terms of the GNU General Public License as published by
# the Free Software Foundation, either version 3 of the License, or
# (at your option) any later version.
#
# This program is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
# GNU General Public License for more details.
#
# You should have received a copy of the GNU General Public License
# along with this program.  If not, see <http://www.gnu.org/licenses/>.
#
# See 'README.md' for more information.
#
*/

#ifndef FRICTION_MARKER_EDITOR_H
#define FRICTION_MARKER_EDITOR_H


#include "ui_global.h"

#include <QWidget>
#include <QTreeWidget>

#include "canvas.h"

namespace Friction
{
    namespace Ui
    {
        class UI_EXPORT MarkerEditor : public QWidget
        {
            Q_OBJECT
        public:
            explicit MarkerEditor(Canvas *scene = nullptr,
                                  QWidget *parent = nullptr);
            void addMarker();
            void removeMarker();
            void clearMarkers(); 

        private:
            Canvas *mScene;
            QTreeWidget *mTree;

            void setup();
            void populate();
            bool duplicate(QTreeWidgetItem *item,
                           const int &frame);
        };
    }
}

#endif // FRICTION_MARKER_EDITOR_H
