#include <stdio.h>
#include <string>

#include <unistd.h> 
#include <stdio.h> 
#include <sys/socket.h> 
#include <stdlib.h> 
#include <netinet/in.h> 
#include <string.h> 
#include <arpa/inet.h>

#include <future>
#include <deque>

static std::string ip;
static unsigned short port;

static int num_connections = 0;
static bool connection_status[2] = { false, false };
static std::string user_names[2] = { "NONE", "NONE" };
static std::deque<std::string> connection_msg_box[2] = { };
std::future<int> connection0;
std::future<int> connection1;


int handle_new_connection(int client_fd, int connection_pos){
	printf("Connection redirected.\n");
	num_connections++;	
	connection_status[connection_pos] = true;

	send(client_fd, "ENTN", 4, 0);
		
	char buffer[256];
	memset(&buffer, 0, sizeof(char) * 256);
	read(client_fd, buffer, 255);
		
	if(std::string(buffer) == "GETN"){	
		send(client_fd, user_names[0].c_str(), user_names[0].size(), 0);
		send(client_fd, user_names[1].c_str(), user_names[1].size(), 0);
		close(client_fd);	
		connection_status[connection_pos] = false;
		user_names[connection_pos] = "NONE";
		num_connections--;
		return 1;
	}
	
	user_names[connection_pos] = "EMPT";

	printf("User: '%s' connected!\n", buffer);
	user_names[connection_pos] = std::string(buffer);

	for(;;){
		memset(&buffer, 0, sizeof(char) * 256);
		int eof = read(client_fd, buffer, 255);
		if(eof == 0)
			break;
	
		if(std::string(buffer) == "\r\n")
			continue;
	
		if(std::string(buffer) == "CHCK"){
			std::string msg;
			if(connection_msg_box[connection_pos].size() <= 0)
				msg = "EMPT";
			else{
				msg = connection_msg_box[connection_pos].front();
				connection_msg_box[connection_pos].pop_front();
			}	
			send(client_fd, msg.c_str(), msg.size(), 0);
		}
		else{
			printf("Adding new message from connectin: '%i', msg: '%s'\n", connection_pos, buffer);	
			if(connection_pos == 0)
				connection_msg_box[1].push_back(std::string(buffer));
			else if (connection_pos == 1)
				connection_msg_box[0].push_back(std::string(buffer));

		}
	}


	close(client_fd);	
	connection_status[connection_pos] = false;
	user_names[connection_pos] = "NONE";
	num_connections--;
	return 1;
}


int main(int argc, char** argv){
	if(argc < 3){
		printf("Usage: %s [IP] [PORT]\n", argv[0]);
		return -1;	
	}
	printf("Creating server room at '%s' ip and '%s' port.\n", argv[1], argv[2]);

	ip = std::string(argv[1]);
	port = std::atoi(argv[2]);

	int server_fd = socket(AF_INET, SOCK_STREAM, 0);
	if(server_fd == 0){
		printf("Cannot create socket!\n");
		exit(EXIT_FAILURE);
	}


	int opt = 1;
	if(setsockopt(server_fd, SOL_SOCKET, SO_REUSEADDR | SO_REUSEPORT, &opt, sizeof(opt)) == -1){
		printf("Cannot setsockopt!\n");
		exit(EXIT_FAILURE);
	}

	struct sockaddr_in address;
	address.sin_family = AF_INET;
	address.sin_addr.s_addr = inet_addr(ip.c_str());
	address.sin_port = htons(port);


	if(bind(server_fd, (struct sockaddr *)&address, sizeof(address)) < 0){
		printf("Cannot bind socket!\n");
		exit(EXIT_FAILURE);
	}

	if(listen(server_fd, 5) < 0){
		printf("Cannot listen to socket!\n");
		exit(EXIT_FAILURE);
	}

	
	for(;;){
		int client_fd;
		struct sockaddr_in client_address;
		socklen_t client_len = sizeof(client_address);

		client_fd = accept(server_fd, (struct sockaddr *) &client_address, &client_len);
		if(client_fd < 0){
			printf("Cannot accept connection!\n");
			continue;
		}

		printf("Got connection form %s, at port %d\n",
				inet_ntoa(client_address.sin_addr), ntohs(client_address.sin_port));

		if(num_connections < 2){
			if(connection_status[0] == false)			
				connection0 = std::async(handle_new_connection, client_fd, 0);
			else if(connection_status[1] == false)			
				connection1 = std::async(handle_new_connection, client_fd, 1);
		}
		else{
			printf("Too many connections!\n");
			send(client_fd, "FULL", 4, 0);
			send(client_fd, user_names[0].c_str(), user_names[0].size(), 0);
			send(client_fd, user_names[1].c_str(), user_names[1].size(), 0);
			close(client_fd);
		}
	}

	close(server_fd);	


	return 0;


}



















