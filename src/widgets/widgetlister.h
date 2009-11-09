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
#ifndef KFTPWIDGETSWIDGETLISTER_H
#define KFTPWIDGETSWIDGETLISTER_H

#include <QWidget>
#include <QList>
#include <QVBoxLayout>

class QPushButton;
class QVBoxLayout;
class KHBox;

namespace KFTPWidgets {

/**
 * This class has been adopted from KDEPIM with slight functional and style
 * changes.
 *
 * @author Jernej Kos
 * @author Marc Mutz <marc@mutz.com>
 */
class WidgetLister : public QWidget
{
Q_OBJECT
public:
  /**
   * Class constructor.
   *
   * @param parent Parent widget
   * @param minWidgets Minimum number of widgets in the list
   * @param maxWidgets Maximum number of widgets in the list
   */
  WidgetLister(QWidget* parent, int minWidgets, int maxWidgets);
  
  /**
   * Class destructor.
   */
  virtual ~WidgetLister();
  
  /**
   * Clears all widgets.
   */
  void clear();
  
  /**
   * Sets the number of currently shown widgets on screen.
   *
   * @param number The number of widgets that should be visible
   */
  virtual void setNumberShown(int number);
protected slots:
  /**
   * Called whenever the user clicks on the 'more' button. Reimplementations
   * should call this method, because this implementation does all the dirty
   * work with adding the widgets  to the layout (through @ref addWidget)
   * and enabling/disabling the control buttons.
   */
  virtual void slotMore();
  
  /**
   * Called whenever the user clicks on the 'fewer' button. Reimplementations
   * should call this method, because this implementation does all the dirty
   * work with removing the widgets from the layout (through @ref removeWidget)
   * and enabling/disabling the control buttons.
   */
  virtual void slotFewer();
  
  /**
   * Called whenever the user clicks on the 'clear' button. Reimplementations
   * should call this method, because this implementation does all the dirty
   * work with removing all but @ref m_minWidgets widgets from the layout and
   * enabling/disabling the control buttons.
   */
  virtual void slotClear();
protected:
  /**
   * Adds a single widget. Doesn't care if there are already @ref m_maxWidgets
   * on screen and whether it should enable/disable any controls. It simply does
   * what it is asked to do.  You want to reimplement this method if you want to
   * initialize the the widget when showing it on screen. Make sure you call this
   * implementaion, though, since you cannot put the widget on screen from derived
   * classes (@p m_layout is private). Make sure the parent of the QWidget to add is
   * this WidgetLister.
   *
   * @param widget The widget that should be added
   */
  virtual void addWidget(QWidget *widget = 0);
  
  /**
   * Removes a single (always the last) widget. Doesn't care if there are still only
   * @ref m_minWidgets left on screen and whether it should enable/disable any controls.
   * It simply does what it is asked to do. You want to reimplement this method if you
   * want to save the the widget's state before removing it from screen. Make sure you
   * call this implementaion, though, since you should not remove the widget from
   * screen from derived classes.
   */
  virtual void removeWidget();
  
  /**
   * Called to clear a given widget. The default implementation does nothing.
   *
   * @param widget The widget that should be cleared
   */
  virtual void clearWidget(QWidget *widget);
  
  /**
   * This method should return a new widget to add to the widget list.
   *
   * @param parent The parent widget
   * @return A valid QWidget
   */
  virtual QWidget *createWidget(QWidget *parent);
protected:
  QList<QWidget*> m_widgetList;
  int m_minWidgets;
  int m_maxWidgets;
signals:
  /**
   * This signal is emitted whenever a widget gets added.
   */
  void widgetAdded(QWidget *widget);
  
  /**
   * This signal is emitted whenever a widget gets removed.
   */
  void widgetRemoved();
  
  /**
   * This signal is emitted whenever the clear button is clicked.
   */
  void clearWidgets();
private:
  void enableControls();

  QPushButton *m_buttonMore;
  QPushButton *m_buttonFewer;
  QPushButton *m_buttonClear;
  QVBoxLayout *m_layout;
  KHBox *m_buttonBox;
};

}

#endif
