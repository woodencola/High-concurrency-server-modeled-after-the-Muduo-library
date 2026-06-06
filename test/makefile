.PHONY:ALL
ALL:buffer_test log_test
buffer_test:buffertest.cc
	g++ -o $@ $^ -std=c++17 -lpthread
log_test:log_test.cc
	g++ -o $@ $^ -std=c++17 -lpthread
.PHONY:clean
clean:
	rm -rf buffer_test