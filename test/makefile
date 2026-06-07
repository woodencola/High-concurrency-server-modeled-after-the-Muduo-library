.PHONY:ALL
ALL:buffer_test log_test Client_test Server_test Channel_test poller_test
buffer_test:buffertest.cc
	g++ -o $@ $^ -std=c++17 -lpthread
log_test:log_test.cc
	g++ -o $@ $^ -std=c++17 -lpthread
Client_test:Client_test.cc
	g++ -o $@ $^ -std=c++17 -lpthread
Server_test:Server_test.cc
	g++ -o $@ $^ -std=c++17 -lpthread
Channel_test:Channel_test.cc
	g++ -o $@ $^ -std=c++17 -lpthread
poller_test:poller_test.cc
	g++ -o $@ $^ -std=c++17 -lpthread
.PHONY:clean
clean:
	rm -rf buffer_test log_test Client_test Server_test Channel_test poller_test