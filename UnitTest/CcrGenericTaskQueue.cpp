#include "ChilCppUnitTest.h"
#include <Core/src/ccr/GenericTaskQueue.h>

using namespace Microsoft::VisualStudio::CppUnitTestFramework;

using namespace chil;
using namespace ccr;
using namespace std::string_literals;

namespace Ccr
{
	TEST_CLASS(CcrGenericTaskQueue)
	{
	public:
		TEST_METHOD(PushAndPopExecute)
		{
			GenericTaskQueue taskQueue;

			// Push a task to add 1 to x
			int x = 0;
			auto future1 = taskQueue.Push([&x]() { return x += 1; });

			// Push a task to add 2 to x
			auto future2 = taskQueue.Push([&x]() { return x += 2; });

			// Pop and execute tasks
			taskQueue.PopExecute();
			taskQueue.PopExecute();

			// Check result of futures
			Assert::AreEqual(1, future1.get());
			Assert::AreEqual(3, future2.get());
			Assert::AreEqual(3, x);
		}

		TEST_METHOD(MultipleThreadsPushAndPopExecute)
		{
			GenericTaskQueue taskQueue;

			// Push tasks from multiple threads
			int x = 0;
			std::vector<std::future<void>> futures;
			for (int i = 0; i < 10; i++)
			{
				auto future = std::async(std::launch::async, [&taskQueue, &x]() {
					taskQueue.Push([&x]() { x += 1; });
				});
				futures.push_back(std::move(future));
			}

			// Pop and execute tasks from multiple threads
			std::vector<std::future<void>> popFutures;
			for (int i = 0; i < 10; i++)
			{
				auto popFuture = std::async(std::launch::async, [&taskQueue]() {
					taskQueue.PopExecute();
				});
				popFutures.push_back(std::move(popFuture));
			}

			// Wait for all futures to complete
			for (auto& future : futures)
			{
				future.wait();
			}
			for (auto& popFuture : popFutures)
			{
				popFuture.wait();
			}

			// Check result of x
			Assert::AreEqual(10, x);
		}
	};
}