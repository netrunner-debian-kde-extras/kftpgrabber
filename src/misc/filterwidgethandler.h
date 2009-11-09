/*
 * This file is part of the KFTPGrabber project
 *
 * Copyright (C) 2003-2006 by the KFTPGrabber developers
 * Copyright (C) 2003-2006 Jernej Kos <kostko@jweb-network.net>
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
#ifndef KFTPCORE_FILTERFILTERWIDGETHANDLER_H
#define KFTPCORE_FILTERFILTERWIDGETHANDLER_H

#include "filter.h"

#include <QMap>
#include <QStackedWidget>

namespace KFTPCore {

namespace Filter {

class WidgetHandlerManagerPrivate;

/**
 * This is an interface that any condition widget handlers must implement
 * in order to be registred with the manager. The handler handles several
 * widgets at once so it should not keep any member variables.
 *
 * @author Jernej Kos
 */
class ConditionWidgetHandler {
public:
    /**
     * Class destructor.
     */
    virtual ~ConditionWidgetHandler() {}
    
    /**
     * This method has to return a new widget used for displaying condition
     * types. It is usually a combobox with several options. The returned
     * widget is added to the value widget stack.
     *
     * @param parent Widget to be used as parent for the newly created widget
     * @param receiver Object that receives type change notifications
     * @return A valid QWidget instance
     */
    virtual QWidget *createTypeWidget(QWidget *parent, const QObject *receiver) const = 0;
    
    /**
     * This method has to create one or several input widgets for different
     * condition types. Each widget has to be added to the widget stack. An
     * identifying widget name is recommended for each widget, so you can easily
     * access it from other methods which get the value stack widget passed as
     * an argument.
     *
     * @param stack Value widget stack
     * @param receiver Object that receives value change notifications
     */
    virtual void createValueWidgets(QStackedWidget *stack, const QObject *receiver) const = 0;
    
    /**
     * Update the status of all widgets.
     *
     * @param field The field to display
     * @param types Type widget stack to use
     * @param values Value widget stack to use
     */
    virtual void update(int field, QStackedWidget *types, QStackedWidget *values) const = 0;
    
    /**
     * Extract data from internal condition representation and show it to the
     * user using the created widgets.
     *
     * @param types Type widget stack to use
     * @param values Value widget stack to use
     * @param condition The condition representation
     */
    virtual void setCondition(QStackedWidget *types, QStackedWidget *values, const Condition *condition) = 0;
    
    /**
     * This method should return the currently selected condition type.
     *
     * @param widget The type widget previously created
     * @return A valid Condition::Type
     */
    virtual Condition::Type getConditionType(QWidget *widget) const = 0;
    
    /**
     * This method should return the current condition value.
     *
     * @param values Value widget stack to use
     * @return A valid condition value
     */
    virtual QVariant getConditionValue(QStackedWidget *values) const = 0;
};

/**
 * This is an interface that any action widget handlers must implement
 * in order to be registred with the manager. The handler handles several
 * widgets at once so it should not keep any member variables.
 *
 * @author Jernej Kos
 */
class ActionWidgetHandler {
public:
    /**
     * Class destructor.
     */
    virtual ~ActionWidgetHandler() {}
    
    /**
     * This method has to return a new widget used for displaying the action
     * value. It is usually a line edit or a similar input widget. The returned
     * widget is added to the value widget stack.
     *
     * @param parent Widget to be used as parent for the newly created widget
     * @param receiver Object that receives type change notifications
     * @return A valid QWidget instance
     */
    virtual QWidget *createWidget(QWidget *parent, const QObject *receiver) const = 0;
    
    /**
     * Extract data from internal action representation and show it to the
     * user using the created widgets.
     *
     * @param stack Value widget stack to use
     * @param action The action representation
     */
    virtual void setAction(QStackedWidget *stack, const Action *action) const = 0;
    
    /**
     * This method should return the current action value.
     *
     * @param values Value widget stack to use
     * @return A valid action value
     */
    virtual QVariant getActionValue(QWidget *widget) const = 0;
};

/**
 * This class keeps a list of all registred condition and action widget
 * handlers. It is a singleton.
 *
 * @author Jernej Kos
 */
class WidgetHandlerManager
{
friend class WidgetHandlerManagerPrivate;
public:
    /**
     * Get the global class instance.
     */
    static WidgetHandlerManager *self();
    
    /**
     * Create widgets for all currently registred condition handlers.
     *
     * @param types Type widget stack to use
     * @param value Value widget stack to use
     * @param receiver Object that receives change notifications
     */
    void createConditionWidgets(QStackedWidget *types, QStackedWidget *values, const QObject *receiver);
    
    /**
     * Create widgets for all currently registred action handlers.
     *
     * @param stack Value widget stack to use
     * @param receiver Object that receives change notifications
     */
    void createActionWidgets(QStackedWidget *stack, const QObject *receiver);
    
    /**
     * Update the specified condition widget handler.
     *
     * @param field New condition field
     * @param types Type widget stack to use
     * @param values Value widget stack to use
     */
    void update(Field field, QStackedWidget *types, QStackedWidget *values);
    
    /**
     * Extract data from internal condition representation and show it to the
     * user using the created widgets.
     *
     * @param types Type widget stack to use
     * @param values Value widget stack to use
     * @param condition The condition representation
     */
    void setCondition(QStackedWidget *types, QStackedWidget *values, const Condition *condition);
    
    /**
     * Get the currently selected condition type.
     *
     * @param field Condition field
     * @param types Type widget stack to use
     * @return A valid Condition::Type
     */
    Condition::Type getConditionType(Field field, QStackedWidget *types);
    
    /**
     * Get the current condition value.
     *
     * @param field Condition field
     * @param values Value widget stack to use
     * @return A valid condition value
     */
    QVariant getConditionValue(Field field, QStackedWidget *values);
    
    /**
     * Extract data from internal action representation and show it to the
     * user using the created widgets.
     *
     * @param stack Value widget stack to use
     * @param action The action representation
     */
    void setAction(QStackedWidget *stack, const Action *action);
    
    /**
     * Get the current action value.
     *
     * @param stack Value widget stack to use
     * @return A valid action value
     */
    QVariant getActionValue(QStackedWidget *stack);
    
    /**
     * Register a new condition handler with the manager.
     *
     * @param field Field that this handler handles
     * @param handler The actual handler instance
     */
    void registerConditionHandler(Field field, ConditionWidgetHandler *handler);
    
    /**
     * Register a new action handler with the manager.
     *
     * @param type Action type that this handler handles
     * @param handler The actual handler instance
     */
    void registerActionHandler(Action::Type type, ActionWidgetHandler *handler); 
protected:
    /**
     * Class constructor.
     */
    WidgetHandlerManager();
    
    /**
     * Class destructor.
     */
    ~WidgetHandlerManager();
private:
    typedef QMap<Field, ConditionWidgetHandler*> ConditionHandlerMap;
    ConditionHandlerMap m_conditionHandlers;
    
    typedef QMap<Action::Type, ActionWidgetHandler*> ActionHandlerMap;
    ActionHandlerMap m_actionHandlers;
};

}

}

#endif
