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
#include "filtereditor.h"
#include "listview.h"
#include "misc/filterwidgethandler.h"

#include <QButtonGroup>

#include <KLocale>
#include <KDialog>
#include <KInputDialog>
#include <KHBox>

using namespace KFTPCore::Filter;

namespace KFTPWidgets {

FilterEditor::FilterEditor(QWidget *parent)
  : QWidget(parent),
    m_rule(0)
{
  QHBoxLayout *mainLayout = new QHBoxLayout(this);
  mainLayout->setSpacing(KDialog::spacingHint());
  
  m_listWidget = new FilterListWidget(this);
  mainLayout->addWidget(m_listWidget, 1);
  
  QVBoxLayout *rightLayout = new QVBoxLayout();
  rightLayout->setParent(mainLayout);
  mainLayout->addItem(rightLayout);
  mainLayout->setStretchFactor(rightLayout, KDialog::spacingHint());
  
  m_enabledCheck = new QCheckBox(i18n("Filter &enabled"), this);
  m_enabledCheck->setEnabled(false);
  rightLayout->addWidget(m_enabledCheck);
  
  m_conditionsList = new FilterConditionsList(this);
  rightLayout->addWidget(m_conditionsList/*, 0, Qt::AlignTop*/);
  
  m_actionsList = new FilterActionsList(this);
  rightLayout->addWidget(m_actionsList/*, 0, Qt::AlignTop*/);
  
  rightLayout->addStretch(1);
  
  // Connect some signals
  connect(m_enabledCheck, SIGNAL(clicked()), this, SLOT(slotEnabledChanged()));
  
  connect(m_listWidget, SIGNAL(ruleChanged(KFTPCore::Filter::Rule*)), this, SLOT(slotRuleChanged(KFTPCore::Filter::Rule*)));
  connect(m_listWidget, SIGNAL(ruleRemoved()), this, SLOT(slotRuleRemoved()));
  
  connect(m_listWidget, SIGNAL(ruleChanged(KFTPCore::Filter::Rule*)), m_conditionsList, SLOT(loadRule(KFTPCore::Filter::Rule*)));
  connect(m_listWidget, SIGNAL(ruleRemoved()), m_conditionsList, SLOT(reset()));
  
  connect(m_listWidget, SIGNAL(ruleChanged(KFTPCore::Filter::Rule*)), m_actionsList, SLOT(loadRule(KFTPCore::Filter::Rule*)));
  connect(m_listWidget, SIGNAL(ruleRemoved()), m_actionsList, SLOT(reset())); 
  
  // Reset the view to load all the current rules
  m_listWidget->reset();
}

void FilterEditor::slotRuleChanged(KFTPCore::Filter::Rule *rule)
{
  m_enabledCheck->setEnabled(true);
  m_enabledCheck->setChecked(rule->isEnabled());
  
  m_rule = rule;
}

void FilterEditor::slotRuleRemoved()
{
  m_enabledCheck->setChecked(false);
  m_enabledCheck->setEnabled(false);
}

void FilterEditor::slotEnabledChanged()
{
  if (m_rule)
    m_rule->setEnabled(m_enabledCheck->isChecked());
}

void FilterEditor::reset()
{
  m_enabledCheck->setChecked(false);
  m_enabledCheck->setEnabled(false);
  
  m_conditionsList->reset();
  m_actionsList->reset();
  m_listWidget->reset();
}

FilterListWidget::FilterListWidget(QWidget *parent)
  : QGroupBox(i18n("Filters"), parent)
{
  QVBoxLayout *layout = new QVBoxLayout(this);
  m_listWidget = new QListWidget(this);
  layout->addWidget(m_listWidget);
  
  KHBox *hb = new KHBox(this);
  hb->setSpacing(4);
  layout->addWidget(hb);
  
  // Up/down buttons
  m_buttonUp = new KPushButton(QString::null, hb);
  m_buttonUp->setIcon(KIcon("go-up"));
  m_buttonUp->setMinimumSize(m_buttonUp->sizeHint() * 1.2);
  
  m_buttonDown = new KPushButton(QString::null, hb);
  m_buttonDown->setIcon(KIcon("go-down"));
  m_buttonDown->setMinimumSize(m_buttonDown->sizeHint() * 1.2);
  
  m_buttonUp->setToolTip(i18n("Up"));
  m_buttonDown->setToolTip(i18n("Down"));
  
  // New, copy, delete buttons
  hb = new KHBox(this);
  hb->setSpacing(4);
  layout->addWidget(hb);
  
  m_buttonNew = new QPushButton(QString::null, hb);
  m_buttonNew->setIcon(KIcon("list-add"));
  m_buttonNew->setMinimumSize(m_buttonNew->sizeHint() * 1.2);
  
  m_buttonCopy = new QPushButton(QString::null, hb);
  m_buttonCopy->setIcon(KIcon("edit-copy"));
  m_buttonCopy->setMinimumSize(m_buttonCopy->sizeHint() * 1.2);
  
  m_buttonDelete = new QPushButton(QString::null, hb);
  m_buttonDelete->setIcon(KIcon("list-remove"));
  m_buttonDelete->setMinimumSize(m_buttonDelete->sizeHint() * 1.2);
  
  m_buttonRename = new QPushButton(QString::null, hb);
  m_buttonRename->setIcon(KIcon("edit-rename"));
  m_buttonRename->setMinimumSize(m_buttonRename->sizeHint() * 1.2);
  
  m_buttonNew->setToolTip(i18n("New"));
  m_buttonCopy->setToolTip(i18n("Copy"));
  m_buttonDelete->setToolTip(i18n("Delete"));
  m_buttonRename->setToolTip(i18n("Rename"));
  
  // Connect the signals
  connect(m_buttonNew, SIGNAL(clicked()), this, SLOT(slotNewRule()));
  connect(m_buttonDelete, SIGNAL(clicked()), this, SLOT(slotDeleteRule()));
  connect(m_buttonRename, SIGNAL(clicked()), this, SLOT(slotRenameRule()));
  connect(m_buttonCopy, SIGNAL(clicked()), this, SLOT(slotCopyRule()));
  
  connect(m_buttonUp, SIGNAL(clicked()), this, SLOT(slotUp()));
  connect(m_buttonDown, SIGNAL(clicked()), this, SLOT(slotDown()));
  
  connect(m_listWidget, SIGNAL(currentItemChanged(QListWidgetItem*, QListWidgetItem*)), this, SLOT(slotItemActivated(QListWidgetItem*)));
  
  m_buttonUp->setEnabled(false);
  m_buttonDown->setEnabled(false);
  m_buttonCopy->setEnabled(false);
  m_buttonDelete->setEnabled(false);
  m_buttonRename->setEnabled(false);
}

void FilterListWidget::reset()
{
  m_listWidget->clear();
  
  // Load all existing rules
  Filters *filters = Filters::self();
  Filters::ConstIterator le = filters->constEnd();
  
  for (Filters::ConstIterator i = filters->constBegin(); i != le; ++i) {
    QListWidgetItem *item = new QListWidgetItem();
    item->setText((*i)->name());
    item->setData(Qt::UserRole, QVariant::fromValue(*i));
    m_listWidget->addItem(item);
  }
    
  // Select the first rule
  m_listWidget->setCurrentItem(m_listWidget->item(0));
}

void FilterListWidget::slotItemActivated(QListWidgetItem *item)
{
  const int row = m_listWidget->row(item);
  if (row < 0)
    return;
  
  m_buttonUp->setEnabled(row > 0);
  m_buttonDown->setEnabled(row < m_listWidget->count() - 1);
  m_buttonRename->setEnabled(true);
  m_buttonCopy->setEnabled(true);
  m_buttonDelete->setEnabled(true);
  
  // Signal the rule change
  Rule *rule = item->data(Qt::UserRole).value<Rule*>();
  
  if (rule)
    emit ruleChanged(rule);
}

void FilterListWidget::slotNewRule()
{
  Rule *rule = new Rule(i18n("Unnamed Rule"));
  QListWidgetItem *current = m_listWidget->currentItem();
  QListWidgetItem *item = new QListWidgetItem();
  item->setText(rule->name());
  item->setData(Qt::UserRole, QVariant::fromValue(rule));
  
  if (current) {
    Rule *currentRule = current->data(Qt::UserRole).value<Rule*>();
    Filters::self()->insert(Filters::self()->indexOf(currentRule), rule);
    m_listWidget->insertItem(m_listWidget->row(current), item);
  } else {
    Filters::self()->append(rule);
    m_listWidget->addItem(item);
  }
  
  // Select the newly inserted item
  m_listWidget->setCurrentItem(item);
}

void FilterListWidget::slotDeleteRule()
{
  QListWidgetItem *current = m_listWidget->currentItem();
  
  if (current) {
    const int row = m_listWidget->row(current);
    Rule *rule = current->data(Qt::UserRole).value<Rule*>();
    
    emit ruleRemoved();
    delete current;
    
    Filters::self()->removeAll(rule);
    delete rule;
    
    //m_listWidget->setCurrentRow(row);
  }
  
  if (!m_listWidget->currentItem()) {
    m_buttonUp->setEnabled(false);
    m_buttonDown->setEnabled(false);
    m_buttonRename->setEnabled(false);
    m_buttonCopy->setEnabled(false);
    m_buttonDelete->setEnabled(false);
  }
}

void FilterListWidget::slotRenameRule()
{
  QListWidgetItem *current = m_listWidget->currentItem();
  
  if (current) {
    Rule *rule = current->data(Qt::UserRole).value<Rule*>();
    QString name = KInputDialog::getText(i18n("Rename Rule"), i18n("Rename rule '%1' to:", rule->name()), rule->name());
    
    if (name.trimmed().isEmpty())
      name = i18n("Unnamed Rule");
    
    rule->setName(name);
    current->setText(name);
  }
}

void FilterListWidget::slotCopyRule()
{
  QListWidgetItem *current = m_listWidget->currentItem();
  
  if (current) {
    Rule *currentRule = current->data(Qt::UserRole).value<Rule*>();
    Rule *rule = new Rule(currentRule);
    
    QListWidgetItem *item = new QListWidgetItem();
    item->setText(rule->name());
    item->setData(Qt::UserRole, QVariant::fromValue(rule));
    
    Filters::self()->insert(Filters::self()->indexOf(currentRule) + 1, rule);
    m_listWidget->insertItem(m_listWidget->row(current) + 1, item);
    m_listWidget->setCurrentItem(item);
  }
}

void FilterListWidget::slotUp()
{
  QListWidgetItem *current = m_listWidget->currentItem();
  
  if (current) {
    const int row = m_listWidget->row(current);
    if (!row)
      return;
    
    // Remove the current rule
    Rule *rule = current->data(Qt::UserRole).value<Rule*>();
    const int index = Filters::self()->indexOf(rule);
    Filters::self()->takeAt(index);
    m_listWidget->takeItem(row);
    
    // Reinsert the item one position higher
    Filters::self()->insert(index - 1, rule);
    m_listWidget->insertItem(row - 1, current);
    
    m_buttonUp->setEnabled(row - 1 > 0);
    m_listWidget->setCurrentItem(current);
  }
}

void FilterListWidget::slotDown()
{
  QListWidgetItem *current = m_listWidget->currentItem();
  
  if (current) {
    const int row = m_listWidget->row(current);
    if (row == m_listWidget->count() - 1)
      return;
    
    // Remove the current rule
    Rule *rule = current->data(Qt::UserRole).value<Rule*>();
    const int index = Filters::self()->indexOf(rule);
    Filters::self()->takeAt(index);
    m_listWidget->takeItem(row);
    
    // Reinsert the item one position lower
    Filters::self()->insert(index + 1, rule);
    m_listWidget->insertItem(row + 1, current);
    
    m_buttonDown->setEnabled(row + 1 < m_listWidget->count() - 1);
    m_listWidget->setCurrentItem(current);
  }
}

FilterConditionsList::FilterConditionsList(QWidget *parent)
  : QGroupBox(i18n("Conditions"), parent)
{
  setEnabled(false);
  
  // Create the layout
  QVBoxLayout *layout = new QVBoxLayout(this);
  
  m_buttonAll = new QRadioButton(i18n("Match a&ll of the following"), this);
  m_buttonAny = new QRadioButton(i18n("Match an&y of the following"), this);
  
  layout->addWidget(m_buttonAll);
  layout->addWidget(m_buttonAny);
  
  m_buttonAll->setChecked(true);
  m_buttonAny->setChecked(false);
  
  QButtonGroup *bg = new QButtonGroup(this);
  bg->addButton(m_buttonAll, (int) ConditionChain::All);
  bg->addButton(m_buttonAny, (int) ConditionChain::Any);
  
  // Connect some signals
  connect(bg, SIGNAL(buttonClicked(int)), this, SLOT(slotMatchTypeChanged(int)));
  
  m_lister = new FilterConditionWidgetLister(this);
  layout->addWidget(m_lister);
}

void FilterConditionsList::reset()
{
  m_lister->clear();
  setEnabled(false);
}

void FilterConditionsList::loadRule(Rule *rule)
{
  m_rule = rule;
  
  switch (rule->conditions()->type()) {
    case ConditionChain::All: m_buttonAll->setChecked(true); break;
    case ConditionChain::Any: m_buttonAny->setChecked(true); break;
  }
  
  m_lister->loadConditions(rule);
  setEnabled(true);
}

void FilterConditionsList::slotMatchTypeChanged(int type)
{
  if (m_rule)
    const_cast<ConditionChain*>(m_rule->conditions())->setType((ConditionChain::Type) type);
}

FilterConditionWidgetLister::FilterConditionWidgetLister(QWidget *parent)
  : WidgetLister(parent, 0, 7),
    m_rule(0)
{
  setMinimumWidth(400);
}

void FilterConditionWidgetLister::loadConditions(KFTPCore::Filter::Rule *rule)
{
  const ConditionChain *conditions = rule->conditions();
  
  // Clear the current list
  setNumberShown(qMax(conditions->count(), 0));
  
  ConditionChain::ConstIterator le = conditions->end();
  QList<QWidget*>::Iterator wi = m_widgetList.begin();
  for (ConditionChain::ConstIterator i = conditions->begin(); i != le; ++i, ++wi)
    static_cast<FilterConditionWidget*>((*wi))->setCondition((*i));
  
  m_rule = rule;
}

void FilterConditionWidgetLister::slotMore()
{
  WidgetLister::slotMore();
  
  // Actually add the condition and update the latest widget
  Condition *condition = new Condition(Filename, Condition::Contains, QVariant(""));
  
  const_cast<ConditionChain*>(m_rule->conditions())->append(condition);
  static_cast<FilterConditionWidget*>(m_widgetList.last())->setCondition(condition);
}

void FilterConditionWidgetLister::slotFewer()
{
  // Actually remove the condition
  Condition *condition = static_cast<FilterConditionWidget*>(m_widgetList.last())->condition();
  const_cast<ConditionChain*>(m_rule->conditions())->removeAll(condition);
  
  WidgetLister::slotFewer();
}

void FilterConditionWidgetLister::slotClear()
{
  if (m_rule)
    const_cast<ConditionChain*>(m_rule->conditions())->clear();
  
  WidgetLister::slotClear();
}

QWidget *FilterConditionWidgetLister::createWidget(QWidget *parent)
{
  return new FilterConditionWidget(parent);
}

FilterConditionWidget::FilterConditionWidget(QWidget *parent)
  : QWidget(parent),
    m_condition(0)
{
  QHBoxLayout *layout = new QHBoxLayout(this);
  layout->setSpacing(KDialog::spacingHint());
  layout->setMargin(0);
  
  m_fieldCombo = new QComboBox(this);
  m_fieldCombo->addItems(Filters::self()->getFieldNames());
  layout->addWidget(m_fieldCombo);
  
  m_typeStack = new QStackedWidget(this);
  m_typeStack->setSizePolicy(QSizePolicy::Preferred, QSizePolicy::Fixed);
  layout->addWidget(m_typeStack);
  
  m_valueStack = new QStackedWidget(this);
  m_valueStack->setSizePolicy(QSizePolicy::Preferred, QSizePolicy::Fixed);
  layout->addWidget(m_valueStack);
  layout->setStretchFactor(m_valueStack, 10);
  
  // Initialize widgets
  WidgetHandlerManager::self()->createConditionWidgets(m_typeStack, m_valueStack, this);
  
  // Connect signals
  connect(m_fieldCombo, SIGNAL(activated(int)), this, SLOT(slotFieldChanged(int)));
  
  setFocusProxy(m_fieldCombo);
}

void FilterConditionWidget::setCondition(const Condition *condition)
{
  m_condition = const_cast<Condition*>(condition);
  
  m_fieldCombo->setCurrentIndex((int) condition->field());
  WidgetHandlerManager::self()->setCondition(m_typeStack, m_valueStack, condition);
}

void FilterConditionWidget::slotFieldChanged(int field)
{
  WidgetHandlerManager::self()->update((Field) field, m_typeStack, m_valueStack);
  
  if (m_condition) {
    // Update the current condition
    m_condition->setField((Field) field);
    slotTypeChanged();
  }
}

void FilterConditionWidget::slotTypeChanged()
{
  if (m_condition) {
    // Update the current condition
    m_condition->setType(WidgetHandlerManager::self()->getConditionType(m_condition->field(), m_typeStack));
    slotValueChanged();
  }
}

void FilterConditionWidget::slotValueChanged()
{
  if (m_condition) {
    // Update the current condition
    m_condition->setValue(WidgetHandlerManager::self()->getConditionValue(m_condition->field(), m_valueStack));
  }
}

FilterActionsList::FilterActionsList(QWidget *parent)
  : QGroupBox(i18n("Actions"), parent)
{
  setEnabled(false);
  
  QVBoxLayout *layout = new QVBoxLayout(this);
  
  m_lister = new FilterActionWidgetLister(this);
  layout->addWidget(m_lister);
}

void FilterActionsList::reset()
{
  m_lister->clear();
  setEnabled(false);
}

void FilterActionsList::loadRule(Rule *rule)
{
  m_rule = rule;
  
  m_lister->loadActions(rule);
  setEnabled(true);
}

FilterActionWidgetLister::FilterActionWidgetLister(QWidget *parent)
  : WidgetLister(parent, 0, 7),
    m_rule(0)
{
  setMinimumWidth(400);
}

void FilterActionWidgetLister::loadActions(KFTPCore::Filter::Rule *rule)
{
  const ActionChain *actions = rule->actions();
  
  // Clear the current list
  setNumberShown(qMax(actions->count(), 0));
  
  ActionChain::ConstIterator le = actions->end();
  QList<QWidget*>::Iterator wi = m_widgetList.begin();
  for (ActionChain::ConstIterator i = actions->begin(); i != le; ++i, ++wi)
    static_cast<FilterActionWidget*>((*wi))->setAction((*i));
  
  m_rule = rule;
}

void FilterActionWidgetLister::slotMore()
{
  WidgetLister::slotMore();
  
  // Actually add the action and update the latest widget
  Action *action = new Action(Action::None, QVariant());
  
  const_cast<ActionChain*>(m_rule->actions())->append(action);
  static_cast<FilterActionWidget*>(m_widgetList.last())->setAction(action);
}

void FilterActionWidgetLister::slotFewer()
{
  // Actually remove the action
  Action *action = static_cast<FilterActionWidget*>(m_widgetList.last())->action();
  const_cast<ActionChain*>(m_rule->actions())->removeAll(action);
  
  WidgetLister::slotFewer();
}

void FilterActionWidgetLister::slotClear()
{
  if (m_rule)
    const_cast<ActionChain*>(m_rule->actions())->clear();
  
  WidgetLister::slotClear();
}

QWidget *FilterActionWidgetLister::createWidget(QWidget *parent)
{
  return new FilterActionWidget(parent);
}

FilterActionWidget::FilterActionWidget(QWidget *parent)
  : QWidget(parent),
    m_action(0)
{
  QHBoxLayout *layout = new QHBoxLayout(this);
  layout->setSpacing(KDialog::spacingHint());
  layout->setMargin(0);
  
  m_actionCombo = new QComboBox(this);
  m_actionCombo->addItems(Filters::self()->getActionNames());
  layout->addWidget(m_actionCombo);
  
  m_valueStack = new QStackedWidget(this);
  m_valueStack->setSizePolicy(QSizePolicy::Preferred, QSizePolicy::Fixed);
  layout->addWidget(m_valueStack);
  layout->setStretchFactor(m_valueStack, 10);
  
  // Initialize widgets
  WidgetHandlerManager::self()->createActionWidgets(m_valueStack, this);
  
  // Connect signals
  connect(m_actionCombo, SIGNAL(activated(int)), this, SLOT(slotActionChanged(int)));
  connect(m_actionCombo, SIGNAL(activated(int)), m_valueStack, SLOT(setCurrentIndex(int)));
  
  setFocusProxy(m_actionCombo);
}

void FilterActionWidget::setAction(const Action *action)
{
  m_action = const_cast<Action*>(action);
  
  m_actionCombo->setCurrentIndex((int) action->type());
  WidgetHandlerManager::self()->setAction(m_valueStack, action);
}

void FilterActionWidget::slotActionChanged(int field)
{
  if (m_action) {
    m_action->setType((Action::Type) field);
    slotValueChanged();
  }
}

void FilterActionWidget::slotValueChanged()
{
  if (m_action)
    m_action->setValue(WidgetHandlerManager::self()->getActionValue(m_valueStack));
}

}

#include "filtereditor.moc"

