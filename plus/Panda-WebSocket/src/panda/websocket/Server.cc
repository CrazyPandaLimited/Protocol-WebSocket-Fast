#include <panda/websocket/Server.h>
//#include <thread>
//#include <vector>
//#include <chrono>
//#include <string.h>
//#include <algorithm>
//#include <sys/types.h>
//#include <sys/socket.h>

namespace panda { namespace websocket {

using std::cout;
//using panda::event::buf_t;
using panda::event::TCP;
//using panda::event::Timer;
//using panda::event::Stream;
//using panda::event::StreamError;

//thread_local std::vector<Stream*> clients;
//thread_local int tnum;
//thread_local Loop* mainloop = NULL;
//thread_local TCP*  srv = NULL;

Server::Server () : loop(NULL) {

}

void Server::init (ServerConfig config) throw(ConfigError) {
	if (!config.loop) config.loop = Loop::default_loop();
	loop = config.loop;

	cout << "server[init]: loop is default = " << (loop == Loop::default_loop()) << "\n";
	cout << "server[init]: num locations = " << config.locations.size() << "\n";

	if (!config.locations.size()) throw ConfigError("no locations to listen supplied");
}

//void die (int en, const char* msg) {
//	cout << "error(" << msg << "): " << (en ? strerror(en) : "") << "\n";
//	throw "ebanarot";
//}

//void kick_client (Stream* handle) {
//	handle->write("you are kicked\n", 15);
//	handle->disconnect();
//	clients.erase(std::find(clients.begin(), clients.end(), handle));
//	//cout <<"Thread("<<tnum<<") client kicked, now i have " << clients.size() << " clients\n";
//}

//bool on_read (Stream* handle, const buf_t* buf, const StreamError& err) {
//	//cout <<"Thread("<<tnum<<") on_read\n";
//	if (strncmp(buf->base, "ready", 5)) {
//		kick_client(handle);
//		return false;
//	}
//	//cout <<"Thread("<<tnum<<") OK(ready)\n";
//	return false;
//}

//void on_connect (Stream* stream, const StreamError& err) {
//	//cout <<"Thread("<<tnum<<") somebody connected to " << (uint64_t)stream << "\n";
//	if (err) cout << err;
//
//	TCP* client = new TCP(mainloop);
//	stream->accept(client);
//
//	clients.push_back(client);
//	//cout <<"Thread("<<tnum<<") now i have " << clients.size() << " clients\n";
//	client->read_callback = on_read;
//}

//void on_timer (Timer* timer) {
//	//cout <<"Thread("<<tnum<<") timer\n";
//}

//void start_server (int num, std::string host, std::string port) {
//	tnum = num;
//	cout <<"Thread("<<tnum<<") started!\n";
//
//	mainloop = new Loop();
//
//	int srvsock = socket(AF_INET, SOCK_STREAM, 0);
//	cout <<"Thread("<<tnum<<") srvsock=" << srvsock << "\n";
//
//	int on = 1;
//	int err = setsockopt(srvsock, SOL_SOCKET, SO_REUSEPORT, &on, sizeof(on));
//	if (err) die(err, "setsockopt");
//
//	cout <<"Thread("<<tnum<<") listening to " << host << ":" << port << "\n";
//	try {
//		srv = new TCP(mainloop);
//		srv->open(srvsock);
//		srv->bind(host.c_str(), port.c_str());
//		srv->listen(1000, on_connect);
//	}
//	catch (StreamError& err) {
//		cout <<"Thread("<<tnum<<") err: " << err.what() << "\n";
//		throw err;
//	}
//
//	Timer* timer = new Timer(mainloop);
//	timer->timer_callback = on_timer;
//	timer->start(1000);
//
//	cout <<"Thread("<<tnum<<") running loop!\n";
//	mainloop->run();
//
//	cout <<"Thread("<<tnum<<") finished!\n";
//}

void Server::run () {
	cout << "RUN\n";

	//std::vector<std::thread> threads;

	//for (int i = 0; i < nthr; ++i) {
	//	threads.push_back(std::thread(start_server, i+1, "dev.crazypanda.ru", "4680"));
	//}

	//for (int i = 0; i < nthr; ++i) {
	//	threads[i].join();
	//	cout << "Thread " << i << " joined\n";
	//}

	cout << "Main thread finished!\n";
}

//void Server::run_threaded (int num) {
//	int srvsock = socket(AF_INET, SOCK_STREAM, 0);
//	cout <<"Thread("<<num<<") srvsock=" << srvsock << "\n";
//
//	int srvsock2 = socket(AF_INET, SOCK_STREAM, 0);
//	cout << "srvsock2=" << srvsock2 << "\n";
//
//	int on = 1;
//	int err;
//	if (err = setsockopt(srvsock, SOL_SOCKET, SO_REUSEPORT, &on, sizeof(on))) {
//		cout << "setsockopt ERROR " << err << "\n";
//		throw "SETSOCKOPT ERROR";
//	}
//
//	on = 1;
//	if (err = setsockopt(srvsock2, SOL_SOCKET, SO_REUSEPORT, &on, sizeof(on))) {
//		cout << "setsockopt ERROR " << err << "\n";
//		throw "SETSOCKOPT ERROR";
//	}
//
//	sockaddr_in srv_addr;
//	memset(&srv_addr, 0, sizeof(sockaddr_in));
//	srv_addr.sin_family = AF_INET;
//	srv_addr.sin_port = htons(4680);
//	srv_addr.sin_addr.s_addr = inet_addr("93.159.239.44");
//	//inet_aton("93.159.239.44", &srv_addr.sin_addr);
//
//
//
//	err = bind(srvsock, (struct sockaddr*)&srv_addr, sizeof(srv_addr));
//	cout << "BIND ERROR " << err << "\n";
//	if (err) throw "BIND ERROR";
//	err = bind(srvsock2, (struct sockaddr*)&srv_addr, sizeof(srv_addr));
//	cout << "BIND ERROR " << err << "\n";
//	if (err) throw "BIND ERROR";
//
//	err = listen(srvsock, 1000);
//	cout << "LISTEN ERROR " << err << "\n";
//	if (err) {
//		throw "LISTEN ERROR";
//	}
//
//	err = listen(srvsock2, 1000);
//	cout << "LISTEN2 ERROR " << err << " " << strerror(errno) << "\n";
//	if (err) {
//		throw "LISTEN2 ERROR";
//	}
//}

Server::~Server () {
}

}}
