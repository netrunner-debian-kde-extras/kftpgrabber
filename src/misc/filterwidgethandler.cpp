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
#include "filterwidgethandler.h"

#include <QComboBox>
#include <QLabel>

#include <KLocale>
#include <KGlobal>
#include <KLineEdit>
#include <knuminput.h>
#include <kcolorbutton.h>

namespace KFTPCore {

namespace Filter {

namespace {

static const struct {
  const Condition::Type type;
  const char *name;
} TextTypes[] = {
  { Condition::Contains,      I18N_NOOP("contains") },
  { Condition::ContainsNot,   I18N_NOOP("does not contain") },
  { Condition::Is,            I18N_NOOP("equals") },
  { Condition::IsNot,         I18N_NOOP("does not equal") },
  { Condition::Matches,       I18N_NOOP("matches regexp") },
  { Condition::MatchesNot,    I18N_NOOP("does not match regexp") }
};

static const int TextTypeCount = sizeof(TextTypes) / sizeof(*TextTypes);

class TextWidgetHandler : public ConditionWidgetHandler
{
public:
    TextWidgetHandler()
      : ConditionWidgetHandler()
    {
    }
    
    QWidget *createTypeWidget(QWidget *parent, const QObject *receiver) const
    {
      QComboBox *combo = new QComboBox(parent);
      
      for (int i = 0; i < TextTypeCount; i++) {
        combo->insertItem(i, i18n(TextTypes[i].name));
      }
      
      combo->adjustSize();
      
      // Connect the signal
      QObject::connect(combo, SIGNAL(activated(int)), receiver, SLOT(slotTypeChanged()));
      
      return combo;
    }
    
    Condition::Type getConditionType(QWidget *widget) const
    {
      QComboBox *combo = static_cast<QComboBox*>(widget);
      return TextTypes[combo->currentIndex()].type;
    }
    
    void createValueWidgets(QStackedWidget *stack, const QObject *receiver) const
    {
      KLineEdit *lineEdit = new KLineEdit(stack);
      lineEdit->setObjectName("textWidgetHandler_LineEdit");
      QObject::connect(lineEdit, SIGNAL(textChanged(const QString&)), receiver, SLOT(slotValueChanged()));
      stack->addWidget(lineEdit);
    }
    
    QVariant getConditionValue(QStackedWidget *values) const
    {
      KLineEdit *lineEdit = values->findChild<KLineEdit*>("textWidgetHandler_LineEdit");
      return QVariant(lineEdit->text());
    }
    
    void update(int field, QStackedWidget *types, QStackedWidget *values) const
    {
      types->setCurrentIndex(field);
      values->setCurrentWidget(values->findChild<QWidget*>("textWidgetHandler_LineEdit"));
    }
    
    void setCondition(QStackedWidget *types, QStackedWidget *values, const Condition *condition)
    {
      // Set condition type
      const Condition::Type type = condition->type();
      int typeIndex = 0;
      
      for (; typeIndex < TextTypeCount; typeIndex++)
        if (type == TextTypes[typeIndex].type)
          break;
      
      QComboBox *combo = static_cast<QComboBox*>(types->widget(((int) condition->field())));
      combo->blockSignals(true);
      combo->setCurrentIndex(typeIndex);
      combo->blockSignals(false);
      types->setCurrentWidget(combo);
      
      // Set condition value
      KLineEdit *lineEdit = values->findChild<KLineEdit*>("textWidgetHandler_LineEdit");
      lineEdit->blockSignals(true);
      lineEdit->setText(condition->value().toString());
      lineEdit->blockSignals(false);
      values->setCurrentWidget(lineEdit);
    }
};

}

namespace {

static const struct {
  const Condition::Type type;
  const char *name;
} EntryTypes[] = {
  { Condition::Is,            I18N_NOOP("is") },
  { Condition::IsNot,         I18N_NOOP("is not") }
};

static const int EntryTypeCount = sizeof(EntryTypes) / sizeof(*EntryTypes);

class EntryWidgetHandler : public ConditionWidgetHandler
{
public:
    EntryWidgetHandler()
      : ConditionWidgetHandler()
    {
    }
    
    QWidget *createTypeWidget(QWidget *parent, const QObject *receiver) const
    {
      QComboBox *combo = new QComboBox(parent);
      
      for (int i = 0; i < EntryTypeCount; i++) {
        combo->addItem(i18n(EntryTypes[i].name));
      }
      
      combo->adjustSize();
      
      // Connect the signal
      QObject::connect(combo, SIGNAL(activated(int)), receiver, SLOT(slotTypeChanged()));
      
      return combo;
    }
    
    Condition::Type getConditionType(QWidget *widget) const
    {
      QComboBox *combo = static_cast<QComboBox*>(widget);
      return EntryTypes[combo->currentIndex()].type;
    }
    
    void createValueWidgets(QStackedWidget *stack, const QObject *receiver) const
    {
      QComboBox *combo = new QComboBox(stack);
      combo->setObjectName("entryWidgetHandler_Combo");
      combo->insertItem(0, i18n("File"));
      combo->insertItem(1, i18n("Directory"));
      
      QObject::connect(combo, SIGNAL(activated(int)), receiver, SLOT(slotValueChanged()));
      stack->addWidget(combo);
    }
    
    QVariant getConditionValue(QStackedWidget *values) const
    {
      QComboBox *combo = values->findChild<QComboBox*>("entryWidgetHandler_Combo");
      QVariant value;
      
      if (combo->currentIndex() == 0)
        value = QVariant(QString("f"));
      else
        value = QVariant(QString("d"));
      
      return value;
    }
    
    void update(int field, QStackedWidget *types, QStackedWidget *values) const
    {
      types->setCurrentIndex(field);
      values->setCurrentWidget(values->findChild<QWidget*>("entryWidgetHandler_Combo"));
    }
    
    void setCondition(QStackedWidget *types, QStackedWidget *values, const Condition *condition)
    {
      // Set condition type
      const Condition::Type type = condition->type();
      int typeIndex = 0;
      
      for (; typeIndex < EntryTypeCount; typeIndex++)
        if (type == EntryTypes[typeIndex].type)
          break;
      
      QComboBox *combo = static_cast<QComboBox*>(types->widget(((int) condition->field())));
      combo->blockSignals(true);
      combo->setCurrentIndex(typeIndex);
      combo->blockSignals(false);
      types->setCurrentWidget(combo);
      
      // Set condition value
      combo = values->findChild<QComboBox*>("entryWidgetHandler_Combo");
      combo->blockSignals(true);
      combo->setCurrentIndex(condition->value().toString() == "f" ? 0 : 1);
      combo->blockSignals(false);
      values->setCurrentWidget(combo);
    }
};

}

namespace {

static const struct {
  const Condition::Type type;
  const char *name;
} SizeTypes[] = {
  { Condition::Is,            I18N_NOOP("equals") },
  { Condition::IsNot,         I18N_NOOP("does not equal") },
  { Condition::Greater,       I18N_NOOP("is greater than") },
  { Condition::Smaller,       I18N_NOOP("is smaller than") }
};

static const int SizeTypeCount = sizeof(SizeTypes) / sizeof(*SizeTypes);

class SizeWidgetHandler : public ConditionWidgetHandler
{
public:
    SizeWidgetHandler()
      : ConditionWidgetHandler()
    {
    }
    
    QWidget *createTypeWidget(QWidget *parent, const QObject *receiver) const
    {
      QComboBox *combo = new QComboBox(parent);
      
      for (int i = 0; i < SizeTypeCount; i++) {
        combo->insertItem(i, i18n(SizeTypes[i].name));
      }
      
      combo->adjustSize();
      
      // Connect the signal
      QObject::connect(combo, SIGNAL(activated(int)), receiver, SLOT(slotTypeChanged()));
      
      return combo;
    }
    
    Condition::Type getConditionType(QWidget *widget) const
    {
      QComboBox *combo = static_cast<QComboBox*>(widget);
      return SizeTypes[combo->currentIndex()].type;
    }
    
    void createValueWidgets(QStackedWidget *stack, const QObject *receiver) const
    {
      KIntNumInput *numInput = new KIntNumInput(stack);
      numInput->setObjectName("sizeWidgetHandler_NumInput");
      numInput->setMinimum(0); 
      numInput->setSuffix(" " + i18n("bytes"));
      
      QObject::connect(numInput, SIGNAL(valueChanged(int)), receiver, SLOT(slotValueChanged()));
      stack->addWidget(numInput);
    }
    
    QVariant getConditionValue(QStackedWidget *values) const
    {
      KIntNumInput *numInput = values->findChild<KIntNumInput*>("sizeWidgetHandler_NumInput");
      return QVariant(numInput->value());
    }
    
    void update(int field, QStackedWidget *types, QStackedWidget *values) const
    {
      types->setCurrentIndex(field);
      values->setCurrentWidget(values->findChild<QWidget*>("sizeWidgetHandler_NumInput"));
    }
    
    void setCondition(QStackedWidget *types, QStackedWidget *values, const Condition *condition)
    {
      // Set condition type
      const Condition::Type type = condition->type();
      int typeIndex = 0;
      
      for (; typeIndex < SizeTypeCount; typeIndex++)
        if (type == SizeTypes[typeIndex].type)
          break;
      
      QComboBox *combo = static_cast<QComboBox*>(types->widget(((int) condition->field())));
      combo->blockSignals(true);
      combo->setCurrentIndex(typeIndex);
      combo->blockSignals(false);
      types->setCurrentWidget(combo);
      
      // Set condition value
      KIntNumInput *numInput = values->findChild<KIntNumInput*>("sizeWidgetHandler_NumInput");
      numInput->blockSignals(true);
      numInput->setValue(condition->value().toInt());
      numInput->blockSignals(false);
      values->setCurrentWidget(numInput);
    }
};

}

class EmptyActionWidgetHandler : public ActionWidgetHandler
{
public:
    EmptyActionWidgetHandler()
      : ActionWidgetHandler()
    {
    }
    
    virtual QWidget *createWidget(QWidget *parent, const QObject *receiver) const
    {
      Q_UNUSED(receiver);

      return new QWidget(parent);
    }
    
    QVariant getActionValue(QWidget *widget) const
    {
      Q_UNUSED(widget);
      
      return QVariant(QString());
    }
    
    void setAction(QStackedWidget *stack, const Action *action) const
    {
      stack->setCurrentIndex((int) action->type());
    }
};

class NoneActionWidgetHandler : public EmptyActionWidgetHandler
{
public:
    NoneActionWidgetHandler()
      : EmptyActionWidgetHandler()
    {
    }
    
    QWidget *createWidget(QWidget *parent, const QObject *receiver) const
    {
      Q_UNUSED(receiver);

      return new QLabel(i18n("Please select an action."), parent);
    }
};

class PriorityActionWidgetHandler : public ActionWidgetHandler
{
public:
    PriorityActionWidgetHandler()
      : ActionWidgetHandler()
    {
    }
    
    QWidget *createWidget(QWidget *parent, const QObject *receiver) const
    {
      KIntNumInput *numInput = new KIntNumInput(parent);
      numInput->setPrefix(i18n("Priority:") + " ");
      
      QObject::connect(numInput, SIGNAL(valueChanged(int)), receiver, SLOT(slotValueChanged()));
      return numInput;
    }
    
    QVariant getActionValue(QWidget *widget) const
    {
      KIntNumInput *numInput = static_cast<KIntNumInput*>(widget);
      return QVariant(numInput->value());
    }
    
    void setAction(QStackedWidget *stack, const Action *action) const
    {
      stack->setCurrentIndex((int) action->type());
      KIntNumInput *numInput = static_cast<KIntNumInput*>(stack->currentWidget());
      numInput->setValue(action->value().toInt());
    }
};

class ColorizeActionWidgetHandler : public ActionWidgetHandler
{
public:
    ColorizeActionWidgetHandler()
      : ActionWidgetHandler()
    {
    }
    
    QWidget *createWidget(QWidget *parent, const QObject *receiver) const
    {
      KColorButton *colorButton = new KColorButton(parent);
      
      QObject::connect(colorButton, SIGNAL(changed(const QColor&)), receiver, SLOT(slotValueChanged()));
      return colorButton;
    }
    
    QVariant getActionValue(QWidget *widget) const
    {
      KColorButton *colorButton = static_cast<KColorButton*>(widget);
      return QVariant(colorButton->color());
    }
    
    void setAction(QStackedWidget *stack, const Action *action) const
    {
      stack->setCurrentIndex((int) action->type());
      KColorButton *colorButton = static_cast<KColorButton*>(stack->currentWidget());
      colorButton->setColor(action->value().value<QColor>());
    }
};

class WidgetHandlerManagerPrivate {
public:
    WidgetHandlerManager instance;
};

K_GLOBAL_STATIC(WidgetHandlerManagerPrivate, widgetHandlerManagerPrivate)

WidgetHandlerManager *WidgetHandlerManager::self()
{
  return &widgetHandlerManagerPrivate->instance;
}

WidgetHandlerManager::WidgetHandlerManager()
{
  // Register condition handlers
  registerConditionHandler(Filename, new TextWidgetHandler());
  registerConditionHandler(EntryType, new EntryWidgetHandler());
  registerConditionHandler(Size, new SizeWidgetHandler());
  
  // Register action handlers
  registerActionHandler(Action::None, new NoneActionWidgetHandler());
  registerActionHandler(Action::Priority, new PriorityActionWidgetHandler());
  registerActionHandler(Action::Skip, new EmptyActionWidgetHandler());
  registerActionHandler(Action::Colorize, new ColorizeActionWidgetHandler());
  registerActionHandler(Action::Hide, new EmptyActionWidgetHandler());
  registerActionHandler(Action::Lowercase, new EmptyActionWidgetHandler());
}

WidgetHandlerManager::~WidgetHandlerManager()
{
}

void WidgetHandlerManager::registerConditionHandler(Field field, ConditionWidgetHandler *handler)
{
  m_conditionHandlers[field] = handler;
}

void WidgetHandlerManager::registerActionHandler(Action::Type type, ActionWidgetHandler *handler)
{
  m_actionHandlers[type] = handler;
}

void WidgetHandlerManager::createConditionWidgets(QStackedWidget *types, QStackedWidget *values, const QObject *receiver)
{
  ConditionHandlerMap::ConstIterator le = m_conditionHandlers.constEnd();
  for (ConditionHandlerMap::ConstIterator i = m_conditionHandlers.constBegin(); i != le; ++i) {
    Field field = i.key();
    const ConditionWidgetHandler *handler = i.value();
    
    types->insertWidget((int) field, handler->createTypeWidget(types, receiver));
    handler->createValueWidgets(values, receiver);
  }
}

void WidgetHandlerManager::createActionWidgets(QStackedWidget *stack, const QObject *receiver)
{
  ActionHandlerMap::ConstIterator le = m_actionHandlers.constEnd();
  for (ActionHandlerMap::ConstIterator i = m_actionHandlers.constBegin(); i != le; ++i) {
    Action::Type type = i.key();
    const ActionWidgetHandler *handler = i.value();
    
    stack->insertWidget((int) type, handler->createWidget(stack, receiver));
  }
}

void WidgetHandlerManager::update(Field field, QStackedWidget *types, QStackedWidget *values)
{
  m_conditionHandlers[field]->update((int) field, types, values);
}

void WidgetHandlerManager::setCondition(QStackedWidget *types, QStackedWidget *values, const Condition *condition)
{
  m_conditionHandlers[condition->field()]->setCondition(types, values, condition);
}

Condition::Type WidgetHandlerManager::getConditionType(Field field, QStackedWidget *types)
{
  return m_conditionHandlers[field]->getConditionType(types->widget((int) field));
}

QVariant WidgetHandlerManager::getConditionValue(Field field, QStackedWidget *values)
{
  return m_conditionHandlers[field]->getConditionValue(values);
}

void WidgetHandlerManager::setAction(QStackedWidget *stack, const Action *action)
{
  m_actionHandlers[action->type()]->setAction(stack, action);
}

QVariant WidgetHandlerManager::getActionValue(QStackedWidget *stack)
{
  QWidget *widget = stack->currentWidget();
  return m_actionHandlers[(Action::Type) stack->indexOf(widget)]->getActionValue(widget);
}

}

}
