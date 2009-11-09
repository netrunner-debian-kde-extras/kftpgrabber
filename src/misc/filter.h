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
#ifndef KFTPCORE_FILTERFILTERS_H
#define KFTPCORE_FILTERFILTERS_H

#include <QVariant>
#include <QList>
#include <QStringList>

#include "engine/directorylisting.h"

namespace KFTPCore {

namespace Filter {

/**
 * Possible filter fields.
 */
enum Field {
  Filename = 0,
  EntryType = 1,
  Size = 2
};

/**
 * A filter condition class of different types.
 *
 * @author Jernej Kos
 */
class Condition {
public:
    /**
     * Condition type.
     *
     * The following types are valid:
     * - None: this rule never validates
     * - Contains: field contains a given substring
     * - ContainsNot: field does not contain a given substring
     * - Is: field is equal to the given value
     * - IsNot: field is not equal to the given value
     * - Matches: field matches a given regular expression
     * - MatchesNot: field doesn't match a given regular expression
     * - Greater: field's integer value is greater than the given value
     * - Smaller: field's integer value is smaller than the given value
     */
    enum Type {
      None = -1,
      Contains = 0,
      ContainsNot = 1,
      Is = 2,
      IsNot = 3,
      Matches = 4,
      MatchesNot = 5,
      Greater = 6,
      Smaller = 7
    };
    
    /**
     * Class constructor. The constructed condition is invalid.
     */
    Condition() {}
    
    /**
     * Class constructor.
     *
     * @param field Field to check
     * @param type Condition type
     * @param value Value to check against
     */
    Condition(Field field, Type type, const QVariant &value);
    
    /**
     * Returns the field this condition operates on.
     */
    Field field() const { return m_field; }
    
    /**
     * Set condition field.
     *
     * @param field A valid condition field
     */
    void setField(Field field) { m_field = field; }
    
    /**
     * Returns the type of this condition.
     */
    Type type() const { return m_type; }
    
    /**
     * Set condition type.
     *
     * @param type A valid condition type
     */
    void setType(Type type) { m_type = type; }
    
    /**
     * Returns the value this condition validates the field with.
     */
    QVariant value() const { return m_value; }
    
    /**
     * Set condition validation value.
     *
     * @param value A valid validation value
     */
    void setValue(const QVariant &value) { m_value = value; }
    
    /**
     * Does the specified entry match this condition ?
     *
     * @param entry Directory entry to compare
     * @return True if the entry matches this condition, false otherwise
     */
    bool matches(const KFTPEngine::DirectoryEntry &entry) const;
private:
    Field m_field;
    Type m_type;
    QVariant m_value;
};

/**
 * This class represents a chain of filter conditions.
 *
 * @author Jernej Kos
 */
class ConditionChain : public QList<Condition*> {
public:
    /**
     * Chain type.
     *
     * The following types are valid:
     * - All: all conditions must match
     * - Any: any condition can match
     */
    enum Type {
      All = 0,
      Any = 1
    };
    
    /**
     * Class constructor.
     */
    ConditionChain();
    
    /**
     * Class constructor.
     */
    ConditionChain(Type type);
    
    /**
     * Class destructor.
     */
    ~ConditionChain();
    
    /**
     * Returns condition chain match type.
     */
    Type type() const { return m_type; }
    
    /**
     * Set condition chain match type.
     *
     * @param type A valid type
     */
    void setType(Type type) { m_type = type; }
    
    /**
     * Does the specified entry match this condition chain ? The actual
     * matching depends on chain type.
     *
     * @param entry Directory entry to compare
     * @return True if the entry matches this chain, false otherwise
     */
    bool matches(const KFTPEngine::DirectoryEntry &entry) const;
private:
    Type m_type;
};

/**
 * This class represents a single action to take.
 */
class Action {
public:
    /**
     * Action type.
     *
     * These are the valid types:
     * - Priority: when queuing files, their priority should be changed
     * - Skip: do not queue such files
     * - Colorize: change font color in the list view
     * - Hide: do not display such files in the list view
     * - Lowercase: lowercase the destination filename when queuing
     */
    enum Type {
      None = 0,
      Priority = 1,
      Skip = 2,
      Colorize = 3,
      Hide = 4,
      Lowercase = 5
    };
    
    /**
     * Class constructor. The resulting action is invalid.
     */
    Action();
    
    /**
     * Class constructor.
     *
     * @param type Action type
     * @param value Action parameters
     */
    Action(Type type, const QVariant &value);
    
    /**
     * Returns true if the action is valid.
     */
    bool isValid() const { return m_valid; }
    
    /**
     * Get action's type.
     */
    Type type() const { return m_type; }
    
    /**
     * Set the action type.
     *
     * @param type A valid action type
     */
    void setType(Type type) { m_type = type; }
    
    /**
     * Get action's parameters.
     */
    QVariant value() const { return m_value; }
    
    /**
     * Set action parameter.
     *
     * @param value Parameter value
     */
    void setValue(const QVariant &value) { m_value = value; }
private:
    bool m_valid;
    Type m_type;
    QVariant m_value;
};

/**
 * This class represents a chain of filter actions.
 *
 * @author Jernej Kos
 */
class ActionChain : public QList<Action*> {
public:
    /**
     * Class constructor.
     */
    ActionChain();
    
    /**
     * Class destructor.
     */
    ~ActionChain();
    
    /**
     * Get an action of the specified type.
     *
     * @param type Action type to search for
     * @return A valid Action or null if there is none
     */
    const Action *getAction(Action::Type type) const;
};

/**
 * This class represents a single filter rule consiting of a condition chain
 * and an action chain.
 *
 * @author Jernej Kos
 */
class Rule {
public:
    /**
     * Class constructor.
     */
    Rule();
    
    /**
     * Class copy constructor. Creates a duplicate deep copy of the provided
     * rule.
     *
     * @param rule The rule to copy
     */
    Rule(const Rule *rule);
    
    /**
     * Class constructor.
     *
     * @param name Human readable rule name
     */
    Rule(const QString &name);
    
    /**
     * Get rule's name.
     */
    QString name() const { return m_name; }
    
    /**
     * Set rule's name.
     */
    void setName(const QString &name) { m_name = name; }
    
    /**
     * Is this rule enabled or not ?
     *
     * @return True if the rule is enabled, false otherwise
     */
    bool isEnabled() const { return m_enabled; }
    
    /**
     * Enable or disable this rule.
     *
     * @param value True if the rule is enabled, false otherwise
     */
    void setEnabled(bool value) { m_enabled = value; }
    
    /**
     * Get the condition chain reference.
     */
    const ConditionChain *conditions() const { return &m_conditionChain; }
    
    /**
     * Get the action chain reference.
     */
    const ActionChain *actions() const { return &m_actionChain; }
private:
    QString m_name;
    bool m_enabled;
    ConditionChain m_conditionChain;
    ActionChain m_actionChain;
};

/**
 * This class contains all the currently loaded rules.
 *
 * @author Jernej Kos
 */
class Filters : public QList<Rule*> {
public:
    /**
     * Get the global rule chain.
     */
    static Filters *self();
    
    /**
     * Class destructor.
     */
    ~Filters();
    
    /**
     * Load the rules from a file.
     */
    void load();
    
    /**
     * Serialize the rules and save them to a file.
     */
    void save();
    
    /**
     * Destroys the global Filters instance. Do not call self() after calling
     * this method otherwise the filters will be reinitialized!
     */
    void close();
    
    /**
     * Is filtering enabled or not ?
     *
     * @return True if filtering is enabled, false otherwise
     */
    bool isEnabled() const { return m_enabled; }
    
    /**
     * Enable or disable filtering.
     *
     * @param value True if filtering is enabled, false otherwise
     */
    void setEnabled(bool value) { m_enabled = value; }
    
    /**
     * Process the specified entry and return an action chain that matched
     * first.
     *
     * @param entry The entry to process
     * @return An ActionChain reference (might be empty if nothing matched)
     */ 
    const ActionChain *process(const KFTPEngine::DirectoryEntry &entry) const;
    
    /**
     * Process the specified entry and return an action to use. This will
     * go trough all loaded rules and attempt to process each one by one.
     * The first one that succeeds is returned.
     *
     * @param entry The entry to process
     * @param types Only return the action of this type
     * @return An Action reference (might be invalid if nothing matched)
     */
    const Action *process(const KFTPEngine::DirectoryEntry &entry, QList<Action::Type> types) const;
    
    /**
     * Process the specified entry and return an action to use. This will
     * go trough all loaded rules and attempt to process each one by one.
     * The first one that succeeds is returned.
     *
     * @param entry The entry to process
     * @param filter Only return the action of this type
     * @return An Action reference (might be invalid if nothing matched)
     */
    const Action *process(const KFTPEngine::DirectoryEntry &entry, Action::Type filter) const;
    
    /**
     * This method is provided for convienience. It behaves just like the
     * above method.
     *
     * @param url File's URL
     * @param size File's size
     * @param directory True if the entry is a directory
     * @param filter Only return the action of this type
     * @return An Action reference (might be invalid if nothing matched)
     */
    const Action *process(const KUrl &url, filesize_t size, bool directory, Action::Type filter) const;
    
    /**
     * Process the specified entry and return an action chain that matched
     * first.
     *
     * @param url File's URL
     * @param size File's size
     * @param directory True if the entry is a directory
     * @return An ActionChain reference (might be invalid if nothing matched)
     */
    const ActionChain *process(const KUrl &url, filesize_t size, bool directory) const;
    
    /**
     * This method is provided for convienience. It behaves just like the
     * above method.
     *
     * Note that 0 will be used for filesize and this may affect the filter
     * process!
     *
     * @param url File's URL
     * @param filter Only return the action of this type
     * @return An Action reference (might be invalid if nothing matched)
     */
    const Action *process(const KUrl &url, Action::Type filter) const;
    
    /**
     * Get a human readable list of possible field names.
     *
     * @return A QStringList representing the field names
     */
    const QStringList &getFieldNames() { return m_fieldNames; }
    
    /**
     * Get a human readable list of possible action names
     *
     * @return A QStringList representing the action names
     */
    const QStringList &getActionNames() { return m_actionNames; }
protected:
    /**
     * Class constructor.
     */
    Filters();
private:
    static Filters *m_self;
    
    bool m_enabled;
    ActionChain m_emptyActionChain;
    
    QStringList m_fieldNames;
    QStringList m_actionNames;
};

}

}

Q_DECLARE_METATYPE(KFTPCore::Filter::Rule*)

#endif
