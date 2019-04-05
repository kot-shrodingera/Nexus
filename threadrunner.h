#pragma once

#include <QObject>
#include <QThread>


namespace ThreadRunner {

  template <class T>
  std::decay_t<T> decay_copy(T&& v) { return std::forward<T>(v); }

  class ThreadWorker : public QObject {
    Q_OBJECT
  public:
    ThreadWorker(QObject* parent = nullptr) : QObject(parent) {}

  public:
    template<class Function>
    void run(Function&& f) {
      std::invoke(decay_copy(std::forward<Function>(f)));
      emit finished();
    }

  signals:
    void finished();
  };



  template<class Function>
  void ThreadRunner(Function&& func, QObject* parent = nullptr) {
    auto thread = new QThread(parent);
    auto worker = new ThreadWorker(parent);
    worker->moveToThread(thread);
    QObject::connect(thread, &QThread::started, worker, [worker, func] {
      worker->run(func);
    });
    QObject::connect(worker, &ThreadWorker::finished, thread, &QThread::quit);
    QObject::connect(worker, &ThreadWorker::finished,
                     worker, &ThreadWorker::deleteLater);
    QObject::connect(thread, &QThread::finished, thread, &QThread::deleteLater);

    thread->start();
  }
}
