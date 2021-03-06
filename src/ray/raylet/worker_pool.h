#ifndef RAY_RAYLET_WORKER_POOL_H
#define RAY_RAYLET_WORKER_POOL_H

#include <inttypes.h>
#include <list>
#include <unordered_map>
#include <unordered_set>

#include "ray/common/client_connection.h"
#include "ray/raylet/worker.h"

namespace ray {

namespace raylet {

class Worker;

/// \class WorkerPool
///
/// The WorkerPool is responsible for managing a pool of Workers. Each Worker
/// is a container for a unit of work.
class WorkerPool {
 public:
  /// Create a pool and asynchronously start the specified number of workers.
  /// Once each worker process has registered with an external server, the
  /// process should create and register a new Worker, then add itself to the
  /// pool.
  ///
  /// \param num_workers The number of workers to start.
  /// \param worker_command The command used to start the worker process.
  WorkerPool(int num_workers, const std::vector<std::string> &worker_command);

  /// Create a pool with zero workers.
  ///
  /// \param num_workers The number of workers to start.
  /// \param worker_command The command used to start the worker process.
  WorkerPool(const std::vector<std::string> &worker_command);

  /// Destructor responsible for freeing a set of workers owned by this class.
  virtual ~WorkerPool();

  /// Asynchronously start a new worker process. Once the worker process has
  /// registered with an external server, the process should create and
  /// register a new Worker, then add itself to the pool. Failure to start
  /// the worker process is a fatal error.
  ///
  /// \param force_start Controls whether to force starting a worker regardless of any
  /// workers that have already been started but not yet registered.
  void StartWorker(bool force_start = false);

  /// Register a new worker. The Worker should be added by the caller to the
  /// pool after it becomes idle (e.g., requests a work assignment).
  ///
  /// \param The Worker to be registered.
  void RegisterWorker(std::shared_ptr<Worker> worker);

  /// Get the client connection's registered worker.
  ///
  /// \param The client connection owned by a registered worker.
  /// \return The Worker that owns the given client connection. Returns nullptr
  /// if the client has not registered a worker yet.
  std::shared_ptr<Worker> GetRegisteredWorker(
      std::shared_ptr<LocalClientConnection> connection) const;

  /// Disconnect a registered worker.
  ///
  /// \param The worker to disconnect. The worker must be registered.
  /// \return Whether the given worker was in the pool of idle workers.
  bool DisconnectWorker(std::shared_ptr<Worker> worker);

  /// Add an idle worker to the pool.
  ///
  /// \param The idle worker to add.
  void PushWorker(std::shared_ptr<Worker> worker);

  /// Pop an idle worker from the pool. The caller is responsible for pushing
  /// the worker back onto the pool once the worker has completed its work.
  ///
  /// \param actor_id The returned worker must have this actor ID.
  /// \return An idle worker with the requested actor ID. Returns nullptr if no
  /// such worker exists.
  std::shared_ptr<Worker> PopWorker(const ActorID &actor_id);

  /// Return the current size of the worker pool. Counts only the workers that registered
  /// and requested a task.
  ///
  /// \return The total count of all workers (actor and non-actor) in the pool.
  uint32_t Size() const;

 protected:
  /// Add started worker PID to the internal list of started workers (for testing).
  ///
  /// \param pid A process identifier for the worker being started.
  void AddStartedWorker(pid_t pid);

  /// Return a number of workers currently started but not registered.
  ///
  /// \return The number of worker PIDs stored for started workers.
  uint32_t NumStartedWorkers() const;

 private:
  std::vector<std::string> worker_command_;
  /// The pool of idle workers.
  std::list<std::shared_ptr<Worker>> pool_;
  /// The pool of idle actor workers.
  std::unordered_map<ActorID, std::shared_ptr<Worker>, UniqueIDHasher> actor_pool_;
  /// All workers that have registered and are still connected, including both
  /// idle and executing.
  // TODO(swang): Make this a map to make GetRegisteredWorker faster.
  std::list<std::shared_ptr<Worker>> registered_workers_;
  std::unordered_set<pid_t> started_worker_pids_;
};

}  // namespace raylet

}  // namespace ray

#endif  // RAY_RAYLET_WORKER_POOL_H
