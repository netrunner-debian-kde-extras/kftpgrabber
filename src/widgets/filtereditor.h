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
#ifndef KFTPWIDGETSFILTEREDITOR_H
#define KFTPWIDGETSFILTEREDITOR_H

#include "widgetlister.h"
#include "misc/filter.h"

#include <QWidget>
#include <QGroupBox>
#include <QListWidget>
#include <QRadioButton>
#include <QComboBox>
#include <QStackedWidget>
#include <QCheckBox>

#include <KPushButton>

namespace KFTPWidgets {

/**
 * A visual representation of a condition.
 *
 * @author Jernej Kos
 */
class FilterConditionWidget : public QWidget {
Q_OBJECT
public:
    /**
     * Class constructor.
     *
     * @param parent Parent widget
     */
    FilterConditionWidget(QWidget *parent);
    
    /**
     * Associate a condition with this widget.
     *
     * @param condition The internal condition representation
     */
    void setCondition(const KFTPCore::Filter::Condition *condition);
    
    /**
     * Return the condition associated with this widget.
     *
     * @return A valid condition or 0 if no condition has been associated
     */
    KFTPCore::Filter::Condition *condition() const { return m_condition; }
private slots:
    void slotFieldChanged(int field);
    void slotTypeChanged();
    void slotValueChanged();
private:
    QComboBox *m_fieldCombo;
    QStackedWidget *m_typeStack;
    QStackedWidget *m_valueStack;
    
    KFTPCore::Filter::Condition *m_condition;
};

/**
 * A container for condition representation widgets.
 *
 * @author Jernej Kos
 */
class FilterConditionWidgetLister : public WidgetLister {
Q_OBJECT
public:
    /**
     * Class constructor.
     *
     * @param parent Parent widget
     */
    FilterConditionWidgetLister(QWidget *parent);
    
    /**
     * Load the conditions from the specified rule.
     *
     * @param rule The rule instance
     */ 
    void loadConditions(KFTPCore::Filter::Rule *rule);
protected:
    QWidget *createWidget(QWidget *parent);
protected slots:
    void slotMore();
    void slotFewer();
    void slotClear();
private:
    KFTPCore::Filter::Rule *m_rule;
};

/**
 * A list of conditions together with all/any configuration.
 *
 * @author Jernej Kos
 */
class FilterConditionsList : public QGroupBox {
Q_OBJECT
public:
    /**
     * Class constructor.
     *
     * @param parent Parent widget
     */
    FilterConditionsList(QWidget *parent);
public slots:
    /**
     * Reset the condition list and disable it.
     */
    void reset();
    
    /**
     * Load the conditions from the specified rule.
     *
     * @param rule The rule instance
     */
    void loadRule(KFTPCore::Filter::Rule *rule);
private slots:
    void slotMatchTypeChanged(int type);
private:
    QRadioButton *m_buttonAll;
    QRadioButton *m_buttonAny;
    
    FilterConditionWidgetLister *m_lister;
    KFTPCore::Filter::Rule *m_rule;
};

/**
 * A visual representation of an action.
 *
 * @author Jernej Kos
 */
class FilterActionWidget : public QWidget {
Q_OBJECT
public:
    /**
     * Class constructor.
     *
     * @param parent Parent widget
     */
    FilterActionWidget(QWidget *parent);
    
    /**
     * Associate an action with this widget.
     *
     * @param action The internal action representation
     */
    void setAction(const KFTPCore::Filter::Action *action);
    
    /**
     * Return the action associated with this widget.
     *
     * @return A valid action or 0 if no action has been associated
     */
    KFTPCore::Filter::Action *action() const { return m_action; }
private slots:
    void slotActionChanged(int field);
    void slotValueChanged();
private:
    QComboBox *m_actionCombo;
    QStackedWidget *m_valueStack;
    
    KFTPCore::Filter::Action *m_action;
};

/**
 * A container for action representation widgets.
 *
 * @author Jernej Kos
 */
class FilterActionWidgetLister : public WidgetLister {
Q_OBJECT
public:
    /**
     * Class constructor.
     *
     * @param parent Parent widget
     */
    FilterActionWidgetLister(QWidget *parent);
    
    /**
     * Load the actions from the specified rule.
     *
     * @param rule The rule instance
     */ 
    void loadActions(KFTPCore::Filter::Rule *rule);
protected:
    QWidget *createWidget(QWidget *parent);
protected slots:
    void slotMore();
    void slotFewer();
    void slotClear();
private:
    KFTPCore::Filter::Rule *m_rule;
};

/**
 * A list of actions.
 *
 * @author Jernej Kos
 */
class FilterActionsList : public QGroupBox {
Q_OBJECT
public:
    /**
     * Class constructor.
     *
     * @param parent Parent widget
     */
    FilterActionsList(QWidget *parent);
public slots:
    /**
     * Reset the action list and disable it.
     */
    void reset();
    
    /**
     * Load the actions from the specified rule.
     *
     * @param rule The rule instance
     */
    void loadRule(KFTPCore::Filter::Rule *rule);
private:
    FilterActionWidgetLister *m_lister;
    KFTPCore::Filter::Rule *m_rule;
};

/**
 * A widget that displays the list of currently loaded filter rules.
 *
 * @author Jernej Kos
 */
class FilterListWidget : public QGroupBox {
Q_OBJECT
public:
    /**
     * Class constructor.
     *
     * @param parent Parent widget
     */
    FilterListWidget(QWidget *parent);
    
    /**
     * Reset the filter editor and reload all the rules.
     */
    void reset();
private slots:
    void slotItemActivated(QListWidgetItem *item);
    
    void slotNewRule();
    void slotDeleteRule();
    void slotRenameRule();
    void slotCopyRule();
    
    void slotUp();
    void slotDown();
private:
    QListWidget *m_listWidget;
    
    KPushButton *m_buttonUp;
    KPushButton *m_buttonDown;
    
    QPushButton *m_buttonNew;
    QPushButton *m_buttonCopy;
    QPushButton *m_buttonDelete;
    QPushButton *m_buttonRename;
signals:
    /**
     * This signal gets emitted when a new rule should be displayed by
     * other widgets.
     *
     * @param rule The rule to display
     */
    void ruleChanged(KFTPCore::Filter::Rule *rule);
    
    /**
     * This signal gets emitted when a rule is removed.
     */
    void ruleRemoved();
};

/**
 * This widget is a global filter editor and enables the user to add,
 * remove or modify existing filters.
 *
 * @author Jernej Kos
 */
class FilterEditor : public QWidget {
Q_OBJECT
public:
    /**
     * Class constructor.
     */
    FilterEditor(QWidget *parent);
    
    /**
     * Reset the filter editor and reload all the rules.
     */
    void reset();
private slots:
    void slotRuleChanged(KFTPCore::Filter::Rule *rule);
    void slotRuleRemoved();
    
    void slotEnabledChanged();
private:
    KFTPCore::Filter::Rule *m_rule;
    
    QCheckBox *m_enabledCheck;
    FilterListWidget *m_listWidget;
    FilterConditionsList *m_conditionsList;
    FilterActionsList *m_actionsList;
};

}

#endif
