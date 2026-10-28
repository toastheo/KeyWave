#include <atomic>
#include <catch2/catch_test_macros.hpp>
#include <chrono>
#include <thread>

#include "platform/ChildProcess.hpp"

TEST_CASE("Child process preserves argument boundaries without shell interpretation", "[process]")
{
  std::atomic_bool cancelled{false};
  ChildProcess process(KEYWAVE_PROCESS_FIXTURE,
                       {"echo", "", "space and & $()", "a\"b", "trailing\\", "two\\\"quotes"});
  process.closeInput();
  REQUIRE(process.wait(cancelled) == 0);
  auto output = process.diagnostics();
  std::erase(output, '\r'); // The Windows CRT translates the fixture's newlines.
  CHECK(output == "0:\n15:space and & $()\n3:a\"b\n9:trailing\\\n11:two\\\"quotes\n");
}

TEST_CASE("Child process drains excessive diagnostics and retains the failure tail", "[process]")
{
  std::atomic_bool cancelled{false};
  ChildProcess process(KEYWAVE_PROCESS_FIXTURE, {"flood"});
  process.closeInput();
  CHECK(process.wait(cancelled) == 7);
  CHECK(process.diagnostics().size() <= ChildProcess::diagnosticLimit);
  CHECK(process.diagnostics().ends_with("diagnostic-end"));
}

TEST_CASE("Windows child process does not open a console", "[process]")
{
  std::atomic_bool cancelled{false};
  ChildProcess process(KEYWAVE_PROCESS_FIXTURE, {"console"});
  process.closeInput();
  CHECK(process.wait(cancelled) == 0);
}

TEST_CASE("Blocked child input times out and can be cancelled", "[process]")
{
  std::atomic_bool cancelled{false};
  ChildProcess process(KEYWAVE_PROCESS_FIXTURE, {"block"});
  std::vector<std::uint8_t> frame(1024 * 1024);
  const auto before = std::chrono::steady_clock::now();
  SECTION("timeout")
  {
    CHECK_THROWS(process.writeInput(frame, cancelled, std::chrono::milliseconds(150)));
  }
  SECTION("cancel")
  {
    std::jthread cancelThread([&] {
      std::this_thread::sleep_for(std::chrono::milliseconds(100));
      cancelled = true;
    });
    CHECK_THROWS(process.writeInput(frame, cancelled));
  }
  CHECK(std::chrono::steady_clock::now() - before < std::chrono::seconds(2));
}
