#pragma once
#include <QHash>
#include "ScheduledTask.h"

class GlobalScheduledTaskManager
{
    QHash<QString, std::shared_ptr<ScheduledTask>> tasks;
public:
    static GlobalScheduledTaskManager& instance() {
        static GlobalScheduledTaskManager instance;
        return instance;
    }
    void addTask(QString taskId, std::shared_ptr<ScheduledTask> task) {
        tasks.insert(taskId, task);
    }
    void removeTask(const QString& taskId) {
        if (tasks.contains(taskId))
            tasks.remove(taskId);
    }
    void clearTasks() {
        tasks.clear();
    }
    const QHash<QString, std::shared_ptr<ScheduledTask>>& getAllTasks() const {
        return tasks;
    }
};

