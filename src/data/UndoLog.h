#pragma once

#include <QSqlDatabase>
#include <QStringList>
#include <vector>

#include "Repositories.h"

// Undo/redo for every change made through the connection, without per-operation commands.
//
// Temporary triggers record, for each insert, update and delete on the data tables, the
// SQL statement that reverses it (deletes of cascaded children included). A step is the
// list of those statements; undoing runs them backwards, and the triggers record the
// reverse of that run, which becomes the redo step. Ids are preserved.
class UndoLog : public IUndoHistory
{
public:
    explicit UndoLog(QSqlDatabase db);

    void commit(const QString &label) override;
    bool canUndo() const override { return !m_undo.empty(); }
    bool canRedo() const override { return !m_redo.empty(); }
    QString undoLabel() const override { return m_undo.empty() ? QString() : m_undo.back().label; }
    QString redoLabel() const override { return m_redo.empty() ? QString() : m_redo.back().label; }
    void undo() override;
    void redo() override;
    // Forgets all history, including changes not committed yet.
    void clear();

private:
    struct Step {
        QString label;
        QStringList statements; // In the order the changes happened.
    };

    void install();
    QStringList drain();
    // Runs the step's statements backwards; returns the statements that reverse that run.
    QStringList replay(const Step &step);

    static constexpr size_t kMaxSteps = 200;

    QSqlDatabase m_db;
    std::vector<Step> m_undo;
    std::vector<Step> m_redo;
};
