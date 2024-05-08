#include <catch2/catch_template_test_macros.hpp>
#include <Tools/scope.hpp>

#include <memory>
#include <mutex>

namespace {

std::mutex CleanMutex;
bool DidClean = false;
void clean() noexcept { DidClean = true; }

struct Cleaner {
	bool cleaned = false;
	void clean() noexcept { cleaned = true; }
	void run(bool release) {
		{
			auto cleanup = ctp::ScopeExit{[this]() noexcept { clean(); }};
			CHECK_FALSE(cleaned);
			if (release)
				cleanup.release();
		}
		CHECK(cleaned != release);
	}
};

} // namespace

using namespace ctp;

TEST_CASE("ScopeExit cleanup.", "[ScopeExit]") {
	GIVEN("An empty ScopeExit.") {
		THEN("Leaving scope does not cause an error.") {
			const auto cleanup = ScopeExit{[]() noexcept {}};
		}
		THEN("Leaving scope via exception does not cause an error.") {
			try {
				const auto cleanup = ScopeExit{[]() noexcept {}};
				throw std::exception{};
			} catch (std::exception) {}
		}
	}

	GIVEN("A ScopeExit that sets a bool.") {
		bool b = false;

		THEN("Leaving scope sets the bool.") {
			{
				const auto cleanup = ScopeExit{[&b]() noexcept { b = true; }};
				CHECK_FALSE(b);
			}

			CHECK(b);
		}
		THEN("Leaving scope via exception sets the bool.") {
			try {
				const auto cleanup = ScopeFail{[&b]() noexcept { b = true; }};
				CHECK_FALSE(b);
				throw std::exception{};
			} catch (std::exception) {}

			CHECK(b);
		}
		THEN("Releasing it stops to bool from being set.") {
			{
				auto cleanup = ScopeExit{[&b]() noexcept { b = true; }};
				CHECK_FALSE(b);
				cleanup.release();
			}

			CHECK_FALSE(b);
		}
	}

	GIVEN("A ScopeExit that calls a free function.") {
		const std::scoped_lock lock{CleanMutex};
		DidClean = false;
		THEN("Leaving scope calls it.") {
			{
				const auto cleanup = ScopeExit{clean};
				CHECK_FALSE(DidClean);
			}

			CHECK(DidClean);
		}
		THEN("Leaving scope via exception calls it.") {
			try {
				const auto cleanup = ScopeExit{clean};
				CHECK_FALSE(DidClean);
				throw std::exception{};
			} catch (std::exception) {}

			CHECK(DidClean);
		}
	}

	GIVEN("A ScopeExit that calls a member function.") {
		Cleaner cleaner;
		THEN("Leaving scope calls it.") {
			cleaner.run(false);
		}
		THEN("Cancelling does not call it.") {
			cleaner.run(true);
		}
	}

	GIVEN("A ScopeExit that cleans up a unique_ptr.") {
		auto ptr = std::make_unique<int>(42);
		THEN("Leaving scope calls it.") {
			{
				const auto cleanup = ScopeExit{[&ptr]() noexcept { ptr.reset(); }};
				CHECK(ptr);
				CHECK(*ptr == 42);
			}

			CHECK_FALSE(ptr);
		}
	}
}

TEST_CASE("ScopeFail cleanup.", "[ScopeFail]") {
	GIVEN("An empty ScopeFail.") {
		THEN("Leaving scope does not cause an error.") {
			const auto cleanup = ScopeFail{[]() noexcept {}};
		}
		THEN("Leaving scope via exception does not cause an error.") {
			try {
				const auto cleanup = ScopeFail{[]() noexcept {}};
				throw std::exception{};
			} catch (std::exception) {}
		}
	}

	GIVEN("A ScopeFail that sets a bool.") {
		bool b = false;

		THEN("Leaving scope without exception doesn't set the bool.") {
			{
				const auto cleanup = ScopeFail{[&b]() noexcept { b = true; }};
				CHECK_FALSE(b);
			}

			CHECK_FALSE(b);
		}
		THEN("Leaving scope via exception sets the bool.") {
			try {
				const auto cleanup = ScopeFail{[&b]() noexcept { b = true; }};
				CHECK_FALSE(b);
				throw std::exception{};
			} catch (std::exception) {}

			CHECK(b);
		}
		THEN("Releasing it has no effect when no exception is thrown.") {
			{
				auto cleanup = ScopeFail{[&b]() noexcept { b = true; }};
				CHECK_FALSE(b);
				cleanup.release();
			}

			CHECK_FALSE(b);
		}
		THEN("Releasing it stops the bool from being set when an exception is thrown.") {
			try {
				auto cleanup = ScopeFail{[&b]() noexcept { b = true; }};
				CHECK_FALSE(b);
				cleanup.release();
				throw std::exception{};
			} catch (std::exception) {}

			CHECK_FALSE(b);
		}
	}

	GIVEN("A ScopeFail that calls a free function.") {
		const std::scoped_lock lock{CleanMutex};
		DidClean = false;
		THEN("Leaving scope without an exception doesn't call it.") {
			{
				const auto cleanup = ScopeFail{clean};
				CHECK_FALSE(DidClean);
			}

			CHECK_FALSE(DidClean);
		}
		THEN("Leaving scope via exception calls it.") {
			try {
				const auto cleanup = ScopeFail{clean};
				CHECK_FALSE(DidClean);
				throw std::exception{};
			} catch (std::exception) {}

			CHECK(DidClean);
		}
	}
}

TEST_CASE("ScopeSuccess cleanup.", "[ScopeSuccess]") {
	GIVEN("An empty ScopeSuccess.") {
		THEN("Leaving scope does not cause an error.") {
			const auto cleanup = ScopeSuccess{[]() noexcept {}};
		}
		THEN("Leaving scope via exception does not cause an error.") {
			try {
				const auto cleanup = ScopeSuccess{[]() noexcept {}};
				throw std::exception{};
			} catch (std::exception) {}
		}
	}

	GIVEN("A ScopeSuccess that sets a bool.") {
		bool b = false;

		THEN("Leaving scope without exception sets the bool.") {
			{
				const auto cleanup = ScopeSuccess{[&b]() noexcept { b = true; }};
				CHECK_FALSE(b);
			}

			CHECK(b);
		}
		THEN("Leaving scope via exception doesn't set the bool.") {
			try {
				const auto cleanup = ScopeSuccess{[&b]() noexcept { b = true; }};
				CHECK_FALSE(b);
				throw std::exception{};
			} catch (std::exception) {}

			CHECK_FALSE(b);
		}
		THEN("Releasing it stops the bool from being set when no exception is thrown.") {
			{
				auto cleanup = ScopeSuccess{[&b]() noexcept { b = true; }};
				CHECK_FALSE(b);
				cleanup.release();
			}

			CHECK_FALSE(b);
		}
		THEN("Releasing it has no effect when an exception is thrown.") {
			try {
				auto cleanup = ScopeSuccess{[&b]() noexcept { b = true; }};
				CHECK_FALSE(b);
				cleanup.release();
				throw std::exception{};
			} catch (std::exception) {}

			CHECK_FALSE(b);
		}
	}

	GIVEN("A ScopeSuccess that calls a free function.") {
		const std::scoped_lock lock{CleanMutex};
		DidClean = false;
		THEN("Leaving scope without an exception calls it.") {
			{
				const auto cleanup = ScopeSuccess{clean};
				CHECK_FALSE(DidClean);
			}

			CHECK(DidClean);
		}
		THEN("Leaving scope via exception doesn't call it.") {
			try {
				const auto cleanup = ScopeSuccess{clean};
				CHECK_FALSE(DidClean);
				throw std::exception{};
			} catch (std::exception) {}

			CHECK_FALSE(DidClean);
		}
	}
}
