.PHONY:ALL
ALL:buffer_test log_test Client_test Server_test Channel_test poller_test EventLoop_test Connection_test Accertor_test TcpServer
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
EventLoop_test:EventLoop_test.cc
	g++ -o $@ $^ -std=c++17 -lpthread -g
Connection_test:Connection_test.cc
	g++ -o $@ $^ -std=c++17 -lpthread -g
Accertor_test:Accertor_test.cc
	g++ -o $@ $^ -std=c++17 -lpthread -g
TcpServer:TcpServer.cc
	g++ -o $@ $^ -std=c++17 -lpthread -g
.PHONY:clean
clean:
	rm -rf buffer_test log_test Client_test Server_test Channel_test poller_test EventLoop_test Connection_test Accertor_test TcpServer