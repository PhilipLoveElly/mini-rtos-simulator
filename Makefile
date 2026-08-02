CXX := g++

CPPFLAGS := -Iinclude
CXXFLAGS := -Wall -Wextra -Wpedantic -std=c++17 -O2

CORE_SOURCES := \
	src/rtos.cpp \
	src/scheduler.cpp \
	src/task.cpp

MAIN_TARGET := mini_rtos

BENCHMARK_TARGETS := \
	scheduler_benchmark \
	scheduler_latency_benchmark \
	priority_inheritance_benchmark \
	message_queue_benchmark

.PHONY: all clean benchmarks debug

all: $(MAIN_TARGET)

benchmarks: $(BENCHMARK_TARGETS)

$(MAIN_TARGET): src/main.cpp $(CORE_SOURCES)
	$(CXX) $(CPPFLAGS) $(CXXFLAGS) \
		src/main.cpp \
		$(CORE_SOURCES) \
		-o $@

scheduler_benchmark: benchmarks/scheduler_benchmark.cpp $(CORE_SOURCES)
	$(CXX) $(CPPFLAGS) $(CXXFLAGS) \
		benchmarks/scheduler_benchmark.cpp \
		$(CORE_SOURCES) \
		-o $@

scheduler_latency_benchmark: benchmarks/scheduler_latency_benchmark.cpp $(CORE_SOURCES)
	$(CXX) $(CPPFLAGS) $(CXXFLAGS) \
		benchmarks/scheduler_latency_benchmark.cpp \
		$(CORE_SOURCES) \
		-o $@

priority_inheritance_benchmark: benchmarks/priority_inheritance_benchmark.cpp $(CORE_SOURCES)
	$(CXX) $(CPPFLAGS) $(CXXFLAGS) \
		benchmarks/priority_inheritance_benchmark.cpp \
		$(CORE_SOURCES) \
		-o $@

message_queue_benchmark: benchmarks/message_queue_benchmark.cpp $(CORE_SOURCES)
	$(CXX) $(CPPFLAGS) $(CXXFLAGS) \
		benchmarks/message_queue_benchmark.cpp \
		$(CORE_SOURCES) \
		-o $@

debug: CXXFLAGS := -Wall -Wextra -Wpedantic -std=c++17 -O0 -g

debug: clean all

clean:
	rm -f \
		$(MAIN_TARGET) \
		$(BENCHMARK_TARGETS)