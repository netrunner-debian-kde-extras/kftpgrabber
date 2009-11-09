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
#include "filter.h"

#include <QRegExp>

#include <kapplication.h>
#include <klocale.h>
#include <kconfig.h>
#include <kglobal.h>
#include <kconfiggroup.h>

namespace KFTPCore {

namespace Filter {

Condition::Condition(Field field, Type type, const QVariant &value)
  : m_field(field),
    m_type(type),
    m_value(value)
{
}

bool Condition::matches(const KFTPEngine::DirectoryEntry &entry) const
{
  bool result = false;
  QString check;
  
  switch (m_field) {
    default:
    case Filename: check = entry.filename(); break;
    case EntryType: check = entry.type(); break;
    case Size: check = QString::number(entry.size()); break;
  }
  
  switch (m_type) {
    case None: result = false; break;
    
    case Contains: result = (check.contains(m_value.toString()) > 0); break;
    case ContainsNot: result = (check.contains(m_value.toString()) == 0); break;
    
    case Is: result = (check == m_value.toString()); break;
    case IsNot: result = (check != m_value.toString()); break;
    
    case Matches: {
      QRegExp r(m_value.toString());
      result = (r.indexIn(check) > -1);
      break;
    }
    case MatchesNot: {
      QRegExp r(m_value.toString());
      result = (r.indexIn(check) == -1);
      break;
    }
    
    case Greater: result = (check.toULongLong() > m_value.toULongLong()); break;
    case Smaller: result = (check.toULongLong() < m_value.toULongLong()); break;
  }
  
  return result;
}

ConditionChain::ConditionChain()
  : QList<Condition*>(),
    m_type(All)
{
}

ConditionChain::ConditionChain(Type type)
  : QList<Condition*>(),
    m_type(type)
{
}

ConditionChain::~ConditionChain()
{
  qDeleteAll(*this);
}

bool ConditionChain::matches(const KFTPEngine::DirectoryEntry &entry) const
{
  if (isEmpty())
    return false;
  
  ConditionChain::ConstIterator le = end();
  for (ConditionChain::ConstIterator i = begin(); i != le; ++i) {
    bool match = (*i)->matches(entry);
    
    if (match && m_type == Any)
      return true;
    else if (!match && m_type == All)
      return false;
  }
  
  if (m_type == Any)
    return false;
  
  return true;
}

Action::Action()
  : m_valid(false)
{
}

Action::Action(Type type, const QVariant &value)
  : m_valid(true),
    m_type(type),
    m_value(value)
{
}

ActionChain::ActionChain()
  : QList<Action*>()
{
}

ActionChain::~ActionChain()
{
  qDeleteAll(*this);
}

const Action *ActionChain::getAction(Action::Type type) const
{
  ActionChain::ConstIterator le = end();
  for (ActionChain::ConstIterator i = begin(); i != le; ++i)
    if ((*i)->type() == type)
      return (*i);
  
  return 0;
}

Rule::Rule()
  : m_name(QString::null),
    m_enabled(false)
{
}

Rule::Rule(const Rule *rule)
  : m_name(rule->name()),
    m_enabled(rule->isEnabled())
{
  // Copy conditions
  const ConditionChain *conditionList = rule->conditions();
  m_conditionChain.setType(conditionList->type());
  
  ConditionChain::ConstIterator cle = conditionList->end();
  for (ConditionChain::ConstIterator i = conditionList->begin(); i != cle; ++i) {
    const Condition *c = (*i);
    
    m_conditionChain.append(new Condition(c->field(), c->type(), c->value()));
  }
  
  // Copy actions
  const ActionChain *actionList = rule->actions();
  
  ActionChain::ConstIterator ale = actionList->end();
  for (ActionChain::ConstIterator i = actionList->begin(); i != ale; ++i) {
    const Action *a = (*i);
    
    m_actionChain.append(new Action(a->type(), a->value()));
  }
}

Rule::Rule(const QString &name)
  : m_name(name),
    m_enabled(true)
{
  // Add a simple condition and a simple action
  m_conditionChain.append(new Condition(Filename, Condition::Contains, QVariant("")));
  m_actionChain.append(new Action(Action::None, QVariant()));
}

Filters *Filters::m_self = 0;

Filters *Filters::self()
{
  if (!m_self)
    m_self = new Filters();
  
  return m_self;
}

Filters::Filters()
  : QList<Rule*>(),
    m_enabled(true)
{
  // Generate human readable strings
  m_fieldNames << i18n("Filename");
  m_fieldNames << i18n("Entry Type");
  m_fieldNames << i18n("Size");
  
  m_actionNames << " ";
  m_actionNames << i18n("Change priority");
  m_actionNames << i18n("Skip when queuing");
  m_actionNames << i18n("Colorize in list view");
  m_actionNames << i18n("Hide from list view");
  m_actionNames << i18n("Lowercase destination");
  
  // Load the filters
  load();
}

Filters::~Filters()
{
  qDeleteAll(*this);
}

void Filters::close()
{
  m_self = 0;
  delete this;
}

void Filters::save()
{
  int num = 0;
  KSharedConfigPtr config = KGlobal::config();
  
  KConfigGroup group = config->group("Filters");
  group.writeEntry("count", count());
  
  // Remove any existing sections
  for (int i = 0; ; i++) {
    QString groupName = QString("Filter #%1").arg(i);
    
    if (config->hasGroup(groupName))
      config->deleteGroup(groupName);
    else
      break;
  }
  
  Filters::ConstIterator le = constEnd();
  for (Filters::ConstIterator i = constBegin(); i != le; ++i, num++) {
    const Rule *rule = (*i);
    
    group.changeGroup(QString("Filter #%1").arg(num));
    group.writeEntry("name", rule->name());
    group.writeEntry("enabled", rule->isEnabled());
    
    // Write conditions
    int cnum = 0;
    const ConditionChain *conditions = rule->conditions();
    group.writeEntry("conditions", conditions->count());
    group.writeEntry("conditions-type", (int) conditions->type());
    
    ConditionChain::ConstIterator cle = conditions->end();
    for (ConditionChain::ConstIterator j = conditions->begin(); j != cle; ++j, cnum++) {
      const Condition *c = (*j);
      QString prefix = QString("condition%1-").arg(cnum);
      
      group.writeEntry(prefix + "field", (int) c->field());
      group.writeEntry(prefix + "type", (int) c->type());
      group.writeEntry(prefix + "value", c->value());
      group.writeEntry(prefix + "valueType", (int) c->value().type());
    }
    
    // Write actions
    int anum = 0;
    const ActionChain *actions = rule->actions();
    group.writeEntry("actions", actions->count());
    
    ActionChain::ConstIterator ale = actions->end();
    for (ActionChain::ConstIterator j = actions->begin(); j != ale; ++j, anum++) {
      const Action *a = (*j);
      QString prefix = QString("action%1-").arg(anum);
      
      group.writeEntry(prefix + "type", (int) a->type());
      group.writeEntry(prefix + "value", a->value());
    }
  }
}

void Filters::load()
{
  int num = 0;
  KSharedConfigPtr config = KGlobal::config();
  
  KConfigGroup group = config->group("Filters");
  num = group.readEntry<int>("count", 0);
  
  for (int i = 0; i < num; i++) {
    Rule *rule = new Rule();
    
    group.changeGroup(QString("Filter #%1").arg(i));
    rule->setName(group.readEntry("name", i18n("Unnamed Rule")));
    rule->setEnabled(group.readEntry<bool>("enabled", true));
    
    // Read conditions
    ConditionChain *conditions = const_cast<ConditionChain*>(rule->conditions());
    int cnum = group.readEntry<int>("conditions", 0);
    conditions->setType((ConditionChain::Type) group.readEntry<int>("conditions-type", 0));
    
    for (int j = 0; j < cnum; j++) {
      QString prefix = QString("condition%1-").arg(j);
      
      conditions->append(new Condition((Field) group.readEntry<int>(prefix + "field", 0),
                                       (Condition::Type) group.readEntry<int>(prefix + "type", 0),
                                       group.readEntry(prefix + "value", QVariant((QVariant::Type) group.readEntry<int>(prefix + "valueType", 0)))));
    }
    
    // Read actions
    ActionChain *actions = const_cast<ActionChain*>(rule->actions());
    int anum = group.readEntry<int>("actions", 0);
    
    for (int j = 0; j < anum; j++) {
      QString prefix = QString("action%1-").arg(j);
      
      actions->append(new Action((Action::Type) group.readEntry<int>(prefix + "type", 0),
                                 group.readEntry(prefix + "value", QVariant())));
    }
    
    append(rule);
  }
}

const ActionChain *Filters::process(const KFTPEngine::DirectoryEntry &entry) const
{
  if (m_enabled) {
    Filters::ConstIterator le = end();
    for (Filters::ConstIterator i = begin(); i != le; ++i) {
      const Rule *rule = (*i);
      
      if (rule->isEnabled() && rule->conditions()->matches(entry))
        return rule->actions();
    }
  }
  
  // Nothing has matched
  return 0;
}

const Action *Filters::process(const KFTPEngine::DirectoryEntry &entry, QList<Action::Type> types) const
{
  const ActionChain *chain = process(entry);
  
  if (!chain || chain->isEmpty())
    return 0;
  
  // Find an action that matches the filter
  ActionChain::ConstIterator le = chain->end();
  for (ActionChain::ConstIterator i = chain->begin(); i != le; ++i) {
    if (types.contains((*i)->type()))
      return (*i);
  }
  
  return 0;
}

const Action *Filters::process(const KFTPEngine::DirectoryEntry &entry, Action::Type filter) const
{
  const ActionChain *chain = process(entry);
  
  if (!chain || chain->isEmpty())
    return 0;
  
  // Find an action that matches the filter
  ActionChain::ConstIterator le = chain->end();
  for (ActionChain::ConstIterator i = chain->begin(); i != le; ++i) {
    if ((*i)->type() == filter)
      return (*i);
  }
  
  return 0;
}

const Action *Filters::process(const KUrl &url, filesize_t size, bool directory, Action::Type filter) const
{
  KFTPEngine::DirectoryEntry entry;
  entry.setFilename(url.fileName());
  entry.setSize(size);
  entry.setType(directory ? 'd' : 'f');
  
  return process(entry, filter);
}

const ActionChain *Filters::process(const KUrl &url, filesize_t size, bool directory) const
{
  KFTPEngine::DirectoryEntry entry;
  entry.setFilename(url.fileName());
  entry.setSize(size);
  entry.setType(directory ? 'd' : 'f');
  
  return process(entry);
}

const Action *Filters::process(const KUrl &url, Action::Type filter) const
{
  return process(url, 0, false, filter);
}

}

}
