#include "GenericTaskQueue.h" 

namespace chil::ccr
{
	void GenericTaskQueue::PopExecute()
	{
		Task task;
		{
			std::lock_guard lck{ mtx_ };
			task = std::move(tasks_.front());
			tasks_.pop_front();
		}
		task();
	}
	void GenericTaskQueue::PushWrappedTask_(Task task)
	{
		std::lock_guard lck{ mtx_ };
		tasks_.push_back(std::move(task));
	}
}