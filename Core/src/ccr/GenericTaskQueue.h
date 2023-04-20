#pragma once 
#include <deque> 
#include <future> 
#include <functional> 

namespace chil::ccr
{
	class GenericTaskQueue
	{
		using Task = std::move_only_function<void()>;
	public:
		template<class F>
		auto Push(F&& function)
		{
			using T = std::invoke_result_t<F>;
			std::packaged_task<T()> pkg{ std::forward<F>(function) };
			auto future = pkg.get_future();
			PushWrappedTask_([pkg = std::move(pkg)]() mutable { pkg(); });
			return future;
		}
		void PopExecute();
	private:
		// functions 
		void PushWrappedTask_(Task task);
		// data 
		std::mutex mtx_;
		std::deque<Task> tasks_;
	};
}