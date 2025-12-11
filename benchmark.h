#pragma once
#include <chrono>
#include <optional>
#include <type_traits>
#include <utility>

namespace ucpp {
	namespace detail {
		template <typename T>
		struct is_duration : std::false_type {};

		template <typename Rep, typename Period>
		struct is_duration<std::chrono::duration<Rep, Period>> : std::true_type {};

		template <typename T>
		constexpr inline bool is_duration_v = is_duration<T>::value;
	}

	class benchmark {
	public:
		using clock_t = std::chrono::steady_clock;

		enum class state_t {
			idle, halted, running, ended
		};

		enum class start_mode_t {
			automatic, manual
		};

		template <class T>
		struct function_result_t {
			T ret;
			benchmark bm;
		};

		explicit benchmark(start_mode_t Start = start_mode_t::manual) noexcept
			: m_Start{}
			, m_End{}
			, m_HaltStart{}
			, m_HaltTime{ clock_t::duration::zero() }
			, m_State{ state_t::idle } {
			if (Start == start_mode_t::automatic) {
				start();
			}
		}

		// Starts the benchmark, fails if it wasn't in idle state
		bool start() noexcept {
			if (!has_started()) {
				m_Start = clock_t::now();
				m_State = state_t::running;
				return true;
			}
			return false;
		}

		void reset(start_mode_t Start = start_mode_t::manual) noexcept {
			m_Start = {};
			m_End = {};
			m_HaltStart = {};
			m_HaltTime = clock_t::duration::zero();
			m_State = state_t::idle;
			if (Start == start_mode_t::automatic) {
				start();
			}
		}

		// Ends the benchmark, fails if it wasn't in running or halted state
		bool end() noexcept {
			if (is_running() || is_halted()) {
				resume();
				m_End = clock_t::now();
				m_State = state_t::ended;
				return true;
			}
			return false;
		}

		// Ends the benchmark, fails if it wasn't in running state
		bool halt() noexcept {
			if (is_running()) {
				m_HaltStart = clock_t::now();
				m_State = state_t::halted;
				return true;
			}
			return false;
		}

		// Resumes the benchmark, fails if it wasn't in halted state
		bool resume() noexcept {
			if (is_halted()) {
				m_HaltTime += clock_t::now() - m_HaltStart;
				m_State = state_t::running;
				return true;
			}
			return false;
		}

		template <class D = clock_t::duration>
			requires detail::is_duration_v<D>
		[[nodiscard]] D runtime() const noexcept {
			D result{};
			switch (m_State) {
				case state_t::halted: {
					result = std::chrono::duration_cast<D>((m_HaltStart - m_Start) - m_HaltTime);
					break;
				}
				case state_t::running: {
					result = std::chrono::duration_cast<D>((clock_t::now() - m_Start) - m_HaltTime);
					break;
				}
				case state_t::ended: {
					result = std::chrono::duration_cast<D>((m_End - m_Start) - m_HaltTime);
					break;
				}
				case state_t::idle:
				default: {
					return D(0); 
				}
			}
			return result > D::zero() ? result : D::zero();
		}

		[[nodiscard]] std::chrono::microseconds runtime_us() const noexcept {
			return runtime<std::chrono::microseconds>();
		};

		[[nodiscard]] std::chrono::milliseconds runtime_ms() const noexcept {
			return runtime<std::chrono::milliseconds>();
		};


		template <class D = clock_t::duration>
			requires detail::is_duration_v<D>
		[[nodiscard]] D halt_time() const noexcept {
			if (is_halted()) {
				return std::chrono::duration_cast<D>(m_HaltTime + (clock_t::now() - m_HaltStart));
			}
			else {
				return std::chrono::duration_cast<D>(m_HaltTime);
			}
		}

		[[nodiscard]] std::optional<clock_t::time_point> start_timestamp() const noexcept {
			if (has_started()) {
				return m_Start;
			}
			else {
				return std::nullopt;
			}
		}

		[[nodiscard]] std::optional<clock_t::time_point> end_timestamp() const noexcept {
			if (has_ended()) {
				return m_End;
			}
			else {
				return std::nullopt;
			}
		}

		[[nodiscard]] std::optional<clock_t::time_point> halt_start_timestamp() const noexcept {
			if (is_halted()) {
				return m_HaltStart;
			}
			else {
				return std::nullopt;
			}
		}

		[[nodiscard]] state_t state() const noexcept {
			return m_State;
		}

		[[nodiscard]] bool is_halted() const noexcept {
			return m_State == state_t::halted;
		}

		[[nodiscard]] bool is_running() const noexcept {
			return m_State == state_t::running;
		}

		[[nodiscard]] bool has_started() const noexcept {
			return m_State != state_t::idle;
		}

		[[nodiscard]] bool has_ended() const noexcept {
			return m_State == state_t::ended;
		}

		template <class F, class... A>
		[[nodiscard]] static auto run(F&& Function, A&&... Args) -> std::conditional_t<std::is_void_v<std::invoke_result_t<F, A...>>, benchmark, function_result_t<std::invoke_result_t<F, A...>>> {
			benchmark bm{ benchmark::start_mode_t::automatic };
			if constexpr (std::is_void_v<std::invoke_result_t<F, A...>>) {
				std::invoke(std::forward<F>(Function), std::forward<A>(Args)...);
				bm.end();
				return std::move(bm);
			}
			else {
				auto result = std::invoke(std::forward<F>(Function), std::forward<A>(Args)...);
				bm.end();
				return function_result_t<std::invoke_result_t<F, A...>>{ std::move(result), std::move(bm) };
			}
		}

	private:
		clock_t::time_point m_Start;
		clock_t::time_point m_End;
		clock_t::time_point m_HaltStart;
		clock_t::duration	m_HaltTime;
		state_t				m_State;
	};

	constexpr inline auto auto_start = ucpp::benchmark::start_mode_t::automatic;
	constexpr inline auto manual_start = ucpp::benchmark::start_mode_t::manual;

	template <class F>
	class scoped_benchmark {
	public:
		scoped_benchmark(F&& Callback)
			: m_Benchmark{ ucpp::benchmark::start_mode_t::automatic }
			, m_Callback{ std::forward<F>(Callback) } {
		}

		~scoped_benchmark() {
			m_Benchmark.end();
			m_Callback(std::move(m_Benchmark));
		}

	private:
		benchmark m_Benchmark;
		std::decay_t<F> m_Callback;
	};

	class scoped_halt {
	public:
		scoped_halt(benchmark& Benchmark)
			: m_Benchmark{ Benchmark } {
			m_Benchmark.halt();
		}

		~scoped_halt() {
			m_Benchmark.resume();
		}

	private:
		benchmark& m_Benchmark;
	};
}