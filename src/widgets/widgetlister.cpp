/*
 * This file is part of the KFTPGrabber project
 *
 * Copyright (C) 2003-2007 by the KFTPGrabber developers
 * Copyright (C) 2003-2007 Jernej Kos <kostko@jweb-network.net>
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License as
 * published by the Free Software Foundation; either version 2 of the
 * License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful, but
 * is provided AS IS, WITHOUT ANY WARRANTY; without even the implied
 * warranty of MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE, and
 * NON-INFRINGEMENT.  See the GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston,
 * MA 02110-1301, USA.
 *
 * In addition, as a special exception, the copyright holders give
 * permission to link the code of portions of this program with the
 * OpenSSL library under certain conditions as described in each
 * individual source file, and distribute linked combinations
 * including the two.
 *
 * You must obey the GNU General Public License in all respects
 * for all of the code used other than OpenSSL.  If you modify
 * file(s) with this exception, you may extend this exception to your
 * version of the file(s), but you are not obligated to do so.  If you
 * do not wish to do so, delete this exception statement from your
 * version.  If you delete this exception statement from all source
 * files in the program, then also delete it here.
 */
#include "widgetlister.h"

#include <KLocale>
#include <KPushButton>
#include <KDialog>
#include <KHBox>

namespace KFTPWidgets {

WidgetLister::WidgetLister(QWidget *parent, int minWidgets, int maxWidgets)
  : QWidget(parent)
{
  m_minWidgets = qMax(minWidgets, 0);
  m_maxWidgets = qMax(maxWidgets, m_minWidgets + 1);

  // The button box
  m_layout = new QVBoxLayout(this);
  m_layout->setMargin(0);
  
  m_buttonBox = new KHBox(this);
  m_buttonBox->setSpacing(KDialog::spacingHint());
  m_layout->addWidget(m_buttonBox);

  m_buttonMore = new KPushButton(KIcon("list-add"), i18n("More"), m_buttonBox);
  m_buttonBox->setStretchFactor(m_buttonMore, 0);

  m_buttonFewer = new KPushButton(KIcon("list-remove"), i18n("Fewer"), m_buttonBox);
  m_buttonBox->setStretchFactor(m_buttonFewer, 0);

  QWidget *spacer = new QWidget(m_buttonBox);
  m_buttonBox->setStretchFactor(spacer, 1);

  m_buttonClear = new KPushButton(KIcon("edit-clear"), i18n("Clear"), m_buttonBox);
  m_buttonBox->setStretchFactor(m_buttonClear, 0);

  // Connect signals
  connect(m_buttonMore, SIGNAL(clicked()), this, SLOT(slotMore()));
  connect(m_buttonFewer, SIGNAL(clicked()), this, SLOT(slotFewer()));
  connect(m_buttonClear, SIGNAL(clicked()), this, SLOT(slotClear()));

  enableControls();
}

WidgetLister::~WidgetLister()
{
  qDeleteAll(m_widgetList);
}

void WidgetLister::slotMore()
{
  addWidget();
  enableControls();
}

void WidgetLister::slotFewer()
{
  removeWidget();
  enableControls();
}

void WidgetLister::clear()
{
  setNumberShown(m_minWidgets);

  // Clear remaining widgets
  foreach (QWidget *widget, m_widgetList) {
    clearWidget(widget);
  }

  enableControls();
  emit clearWidgets();
}

void WidgetLister::slotClear()
{
  clear();
}

void WidgetLister::addWidget(QWidget *widget)
{
  if (!widget)
    widget = createWidget(this);

  m_layout->insertWidget(m_layout->indexOf(m_buttonBox), widget);
  m_widgetList.append(widget);
  widget->show();
  
  enableControls();
  emit widgetAdded(widget);
}

void WidgetLister::removeWidget()
{
  QWidget *widget = m_widgetList.takeLast();
  delete widget;
  
  enableControls();
  emit widgetRemoved();
}

void WidgetLister::clearWidget(QWidget *widget)
{
  Q_UNUSED(widget)
}

QWidget *WidgetLister::createWidget(QWidget* parent)
{
  return new QWidget(parent);
}

void WidgetLister::setNumberShown(int number)
{
  int superfluousWidgets = qMax((int) m_widgetList.count() - number, 0);
  int missingWidgets = qMax(number - (int) m_widgetList.count(), 0);

  // Remove superfluous widgets
  for (; superfluousWidgets; superfluousWidgets--)
    removeWidget();

  // Add missing widgets
  for (; missingWidgets; missingWidgets--)
    addWidget();
}

void WidgetLister::enableControls()
{
  int count = m_widgetList.count();
  bool isMaxWidgets = (count >= m_maxWidgets);
  bool isMinWidgets = (count <= m_minWidgets);

  m_buttonMore->setEnabled(!isMaxWidgets);
  m_buttonFewer->setEnabled(!isMinWidgets);
}

}
